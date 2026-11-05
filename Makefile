CXX      = g++
# -iquote (not -I): include/process.h would otherwise shadow the Windows system <process.h>
CXXFLAGS = -std=c++17 -Wall -Wextra -iquote include

HEADERS  = $(wildcard include/*.h)
SRC      = $(wildcard src/*.cpp)

# "make" builds the program
main.exe: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@
