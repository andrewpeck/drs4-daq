########################################################
#
#  Makefile for drsosc, drscl and drs_exam 
#  executables under linux
#
#  Requires wxWidgets 2.8.9 or newer
#
########################################################

# check for OS
DOS           = OS_LINUX

CFLAGS        = -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -Iinclude -D$(DOS) -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX
LIBS          = -lpthread -lutil -lusb-1.0

#CPP_OBJ       = DRS.o ConfigDialog.o DOFrame.o DOScreen.o DRSOsc.o MeasureDialog.o Measurement.o Osci.o InfoDialog.o DisplayDialog.o AboutDialog.o EPThread.o TriggerDialog.o rb.o
CPP_OBJ       = DRS.o
OBJECTS       = musbstd.o strlcpy.o


all: drsdaq

drsdaq: $(OBJECTS) DRS.o drsdaq.o
	$(CXX) $(CFLAGS) $(OBJECTS) DRS.o drsdaq.o -o drsdaq $(LIBS) $(WXLIBS)

drsdaq.o: drsdaq.cpp DRS.h
	$(CXX) $(CFLAGS) -c $<

$(CPP_OBJ): %.o: %.cpp %.h DRS.h
	$(CXX) $(CFLAGS) $(WXFLAGS) -c $<

$(OBJECTS): %.o: %.c DRS.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o drsdaq
