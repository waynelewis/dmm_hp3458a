#!../../bin/linux-x86_64/dmm

dbLoadDatabase "../../dbd/dmm.dbd"
dmm_registerRecordDeviceDriver pdbbase

epicsEnvSet("STREAM_PROTOCOL_PATH", "${PWD}/../../protocol")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "${PWD}/../../db")

vxi11Configure("DMM", "192.168.1.40", 0, "5.0", "gpib0,22")

#asynSetTraceMask("DMM",-1,0x3f)
#asynSetTraceIOMask("DMM",-1,2)

dbLoadRecords("hp3458a.db","P=DMM:,PORT=DMM")

iocInit

