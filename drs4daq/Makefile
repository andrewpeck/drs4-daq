########################################################
#  Makefile for drs_exam executables under linux
########################################################
#Detect OS type
OSTYPE = $(shell uname)
OBJDIR = $(shell pwd)/obj
DRVDIR = $(shell pwd)/drv
CURRENTDIR = $(shell pwd)

ifeq ($(OSTYPE),Darwin)
	CFLAGS        = -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -I$(DRVDIR) -I/opt/local/include -DOS_LINUX -DHAVE_LIBUSB 
else
	CFLAGS        = -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -I$(DRVDIR) -DOS_LINUX -DHAVE_LIBUSB 
#using DRS_MUTEX will require wxWidgets
#CFLAGS        = -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -I$(DRVDIR) -DOS_LINUX -DHAVE_LIBUSB -DUSE_DRS_MUTEX
endif

ifeq ($(OSTYPE),Darwin) 
	LIBS          = -lpthread -lutil -L/opt/local/lib -lusb
else 
	LIBS          = -lpthread -lutil -lusb
endif

DRS4OBJ = $(OBJDIR)/DRS.o
OBJECTS = $(OBJDIR)/musbstd.o $(OBJDIR)/mxml.o $(OBJDIR)/strlcpy.o
MAINOBJ	= $(OBJDIR)/drscl.o $(OBJDIR)/main.o
ROOTOBJ = $(OBJDIR)/TEvent.o $(OBJDIR)/TEventDict.o
ROOTSHAREDOBJ = $(OBJDIR)/TEvent.so

CFLAGS += -I$(shell /usr/bin/root-config --incdir) 
CFLAGS += $(shell /usr/bin/root-config --cflags)
LIBS += $(shell /usr/bin/root-config --libs)

target:= main
all: $(target)

$(target): $(OBJECTS) $(DRS4OBJ) $(OBJDIR)/main.o $(ROOTOBJ) $(ROOTSHAREDOBJ) 
	$(CXX) $(CFLAGS) $(OBJECTS) $(OBJDIR)/DRS.o $(OBJDIR)/main.o $(ROOTOBJ) -o $(target) $(LIBS) 

drscl: $(OBJECTS) $(DRS4OBJ) $(OBJDIR)/drscl.o
	$(CXX) $(CFLAGS) $(OBJECTS) $(OBJDIR)/DRS.o $(OBJDIR)/drscl.o -o drscl $(LIBS) $(WXLIBS)

$(MAINOBJ): $(OBJDIR)/%.o: %.cpp %.h $(DRVDIR)/mxml.h $(DRVDIR)/DRS.h libs/TEvent.h $(DRVDIR)/progressbar.h 
	$(CXX) $(CFLAGS) -c $< -o $@

#main.o: main.cpp $(DRVDIR)/mxml.h $(DRVDIR)/DRS.h
#	$(CXX) $(CFLAGS) -c $< -o $(OBJDIR)/main.o

$(DRS4OBJ): $(OBJDIR)/%.o: $(DRVDIR)/%.cpp $(DRVDIR)/%.h $(DRVDIR)/mxml.h $(DRVDIR)/DRS.h
	$(CXX) $(CFLAGS)  -c $< -o $@

$(OBJECTS): $(OBJDIR)/%.o: $(DRVDIR)/%.c $(DRVDIR)/mxml.h $(DRVDIR)/DRS.h 
	$(CXX) $(CFLAGS) -c $< -o $@

$(ROOTOBJ): $(OBJDIR)/%.o: libs/%.cpp
	cd libs; rootcint -f TEventDict.cpp -c -p -O2 -Wall -fPIC `root-config --cflags` TEvent.h
	$(CXX) $(CFLAGS) -c $< -o $@

$(ROOTSHAREDOBJ): $(ROOTOBJ)
	cd obj; $(CXX) -shared -o TEvent.so $(CFLAGS) $(LIBS)  TEventDict.o TEvent.o

clean:
	rm -f *.o $(target)
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.so

