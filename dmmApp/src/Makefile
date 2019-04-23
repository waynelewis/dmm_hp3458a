TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = dmm
# dmm.dbd will be created and installed
DBD += dmm.dbd

# dmm.dbd will be made up from these files:
dmm_DBD += base.dbd
dmm_DBD += asyn.dbd
dmm_DBD += drvVxi11.dbd
dmm_DBD += stream.dbd

# Include dbd files from all support applications:
#dmm_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
dmm_LIBS += stream
dmm_LIBS += asyn

# dmm_registerRecordDeviceDriver.cpp derives from dmm.dbd
dmm_SRCS += dmm_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
dmm_SRCS_DEFAULT += dmmMain.cpp
dmm_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#dmm_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
dmm_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

