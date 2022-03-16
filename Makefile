all: liveness.so available.so reaching.so inputs

CXX = clang
CXXFLAGS = $(shell llvm-config --cxxflags) -fcolor-diagnostics -g -O0 -fPIC
OPT = opt
TEST = extra_test

available-support.o: available-support.cpp available-support.h

dataflow.o: dataflow.cpp dataflow.h
liveness.o: liveness.cpp 
available.o: available.cpp
reaching.o: reaching.cpp

%.so: %.o dataflow.o available-support.o
	$(CXX) -dylib -shared $^ -o $@

# TESTING
inputs : $(patsubst %.c,%.bc,$(wildcard $(TEST)/*.c)) 
 
%.tmp: %.c
	$(CXX) -O0 -emit-llvm -c $^ -o $@ 

%.bc: %.tmp
	$(OPT) -mem2reg $^ -o $@

# CLEAN
clean:
	rm -f *.o *~ *.so out $(TEST)/*.bc                 
