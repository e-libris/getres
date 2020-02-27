# Compiler variables
#CCFLAGS = -ansi -Wall -std=c++0x
#CCFLAGS = -ansi -Wall -std=c++17
CCFLAGS = -ansi -Wall -lstdc++fs -std=c++17 -lcurl

# Rule to link object code files to create executable file
get_resources: get_resources.o procreceipt.o
	g++ $(CCFLAGS) -o get_resources get_resources.o procreceipt.o

# Rules to compile source code files to object code
get_resources.o: get_resources.cc procreceipt.h
	g++ $(CCFLAGS) -c get_resources.cc

procreceipt.o:	procreceipt.cc procreceipt.h
	g++ $(CCFLAGS) -c procreceipt.cc

# Pseudo-target to remove object code and executable files
clean:
	-rm *.o get_resources
