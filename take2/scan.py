import logging
import time
from vxi11 import Instrument as _Instrument

import epicscorelibs.path.cothread
from cothread import Sleep
from cothread.catools import caput as _caput

_log = logging.getLogger(__name__)

def getargs():
    from argparse import ArgumentParser
    P = ArgumentParser()
    P.add_argument('--ipaddr')
    P.add_argument('--prefix')
    P.add_argument('addrs', nargs='+')
    return P.parse_args()

class Instrument(_Instrument):
    def __init__(self, ip, addr):
        self.__P = ip, addr
        _Instrument.__init__(self, ip, addr)
    def write(self, value, *args, **kws):
        _log.debug('%s Send %s', self.__P, repr(value))
        _Instrument.write(self, value, *args, **kws)
    def read(self, *args, **kws):
        R = _Instrument.read(self, *args, **kws)
        _log.debug('%s Recv %s', self.__P, repr(R))
        return R

def caput(*args, **kws):
    _log.debug('caput %s %s', args, kws)
    _caput(*args, **kws)

class Session(object):
    def __init__(self, args):
        self.args = args
        self.Is = []

    def setup(self):
        self.Is = [Instrument(self.args.ipaddr, 'gpib0,%s'%addr) for addr in self.args.addrs]
        for I in self.Is:
            I.timeout = 1

            # enable use of EoI and ask for ID string
            name = I.ask('END;ID?')
            assert name==u'HP3458A', name

            # TARM HOLD
            #  disable trigger arming
            # TRIG AUTO;NRDGS 1,AUTO
            #  not using start trigger and sample trigger
            # INBUF ON
            #  Don't hold bus lock while executing
            # MEM FIFO
            #  store sample data internally until read
            I.write('EXTOUT APER,POS;TARM HOLD;TRIG EXT;NRDGS 1,AUTO;INBUF ON;MEM FIFO')

    def close(self):
        Is, self.Is = self.Is, []
        for I in Is:
            I.write('TARM AUTO;TRIG AUTO;NRDGS 1,AUTO;INBUF OFF;MEM OFF')

    def __enter__(self):
        return self

    def __exit__(self, A,B,C):
        self.close()

    def loop(self):
        # arm all for single acquisition
        for I in self.Is:
            I.write('TARM SGL')

        Vs = [I.read() for I in self.Is]

        caput(self.args.prefix+":I", Vs)
#        for addr, V in zip(self.args.addrs, Vs):
#            caput('%s:%s:I'%(self.args.prefix, addr), float(V))

def main(args):
    with Session(args) as S:
        while True:
            try:
                S.setup()
                T = time.time()+1.0
                while True:
                    now = time.time()
                    caput(args.prefix+':LTIME', T-now)
                    if now>=T:
                        _log.warn('Loop too long %f >= %f', now, T)
                        T = now+1.0
                    else: # now < T
                        _log.debug('Sleep %f', T-now)
                        Sleep(T-now)
                        T += 1.0
                    S.loop()
            except KeyboardInterrupt:
                _log.debug('Done')
                break
            except: # errors, including timeout
                _log.exception('Error')
                Sleep(10) # hold-off before reconnect

if __name__=='__main__':
    args = getargs()
    logging.basicConfig(level=logging.DEBUG)
    main(args)
