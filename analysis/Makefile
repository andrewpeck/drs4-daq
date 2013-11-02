ROOTLIBS      	= $(shell root-config --libs)
CFLAGS		= -O3

all:		ASCII2ROOT CreatePlots ProcessTOF

ASCII2ROOT: 	ASCII2ROOT.o
		g++ -o ASCII2ROOT ASCII2ROOT.o $(ROOTLIBS) $(CFLAGS)

CreatePlots: 	CreatePlots.o
		g++ -o CreatePlots CreatePlots.o $(ROOTLIBS) $(CFLAGS)

ProcessTOF: 	ProcessTOF.o
		g++ -o ProcessTOF ProcessTOF.o $(ROOTLIBS) $(CFLAGS)

.cc.o: 		$<.cc
		g++ -I${ROOTSYS}/include -c $< $(CFLAGS)

clean:
		@rm -f *.o ASCII2ROOT CreatePlots ProcessTOF
