CXX=g++
CXXFLAGS=-Wall -pedantic -g
LD=g++
LDFLAGS=-g

all: testseq testthr

testseq: mainseq.o solution.o input.o
	$(LD) $(LDFLAGS) -o $@ $^ -lpthread
	
testthr: mainthr.o solution.o input.o
	$(LD) $(LDFLAGS) -o $@ $^ -lpthread

clean:
	rm -f *.o testseq testthr
	
clear: clean
	rm -f core *~ *.bak

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
input.o: input.cpp common.h
mainseq.o: mainseq.cpp common.h
mainthr.o: mainthr.cpp common.h
solution.o: solution.cpp common.h
