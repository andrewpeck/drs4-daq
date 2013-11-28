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

CPP_OBJ       = DRS.o pugixml.o
OBJECTS       = musbstd.o strlcpy.o


all: drsdaq

drsdaq: $(OBJECTS) DRS.o drsdaq.o pugixml.o
	$(CXX) $(CFLAGS) $(OBJECTS) DRS.o drsdaq.o -o drsdaq $(LIBS) #$(WXLIBS)

drsdaq.o: src/drsdaq.cpp include/DRS.h include/pugixml.h
	$(CXX) $(CFLAGS) -c $<

$(CPP_OBJ): %.o: src/%.cpp include/%.h include/pugixml.h
	$(CXX) $(CFLAGS) $(WXFLAGS) -c $<

$(OBJECTS): %.o: src/%.c include/DRS.h include/pugixml.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o drsdaq
