#!../../bin/linux-x86_64/dmm

dbLoadDatabase "../../dbd/dmm.dbd"
dmm_registerRecordDeviceDriver pdbbase

epicsEnvSet("STREAM_PROTOCOL_PATH", "${PWD}")
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "${PWD}")

vxi11Configure("DMM19", "192.168.1.40", 0, "5.0", "gpib0,19")
#vxi11Configure("DMM22", "192.168.1.40", 0, "5.0", "gpib0,22")

asynSetTraceMask("DMM19",-1,0x3f)
asynSetTraceIOMask("DMM19",-1,2)

#dbLoadRecords("hp3458a.db","P=DMM:19:,PORT=DMM19")
#dbLoadRecords("hp3458a.db","P=DMM:22:,PORT=DMM22")
dbLoadRecords("srq.db")

iocInit

