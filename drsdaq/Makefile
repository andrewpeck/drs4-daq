########################################################
#
#  Makefile for drsdaq for linux
#
########################################################

# check for OS
DOS           = OS_LINUX

CFLAGS        = -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -Iinclude -D$(DOS) -DHAVE_USB -DHAVE_LIBUSB10 -DUSE_DRS_MUTEX
LIBS          = -lpthread -lutil -lusb-1.0

CPP_OBJ       = DRS.o
OBJECTS       = musbstd.o strlcpy.o


all: drsdaq

drsdaq: $(OBJECTS) DRS.o drsdaq.o
	$(CXX) $(CFLAGS) $(OBJECTS) DRS.o drsdaq.o -o drsdaq $(LIBS) #$(WXLIBS)

drsdaq.o: src/drsdaq.cpp include/DRS.h
	$(CXX) $(CFLAGS) -c $<

$(CPP_OBJ): %.o: src/%.cpp include/%.h
	$(CXX) $(CFLAGS) $(WXFLAGS) -c $<

$(OBJECTS): %.o: src/%.c include/DRS.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o drsdaq
