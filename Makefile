CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

HEADERS  = $(wildcard include/*.h)
SRC      = $(wildcard src/*.cpp)
LIB_SRC  = $(filter-out src/main.cpp, $(SRC))   # everything except the real main

TESTS     = $(wildcard tests/*_test.cpp)
TEST_BINS = $(TESTS:.cpp=.exe)

# "make" builds the real program
main.exe: $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

# "make test" builds and runs every tests/*_test.cpp (each has its own main)
test: $(TEST_BINS)
	$(foreach t,$(TEST_BINS),"$(t)" &&) echo All tests ran.

tests/%.exe: tests/%.cpp $(LIB_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $< $(LIB_SRC) -o $@

.PHONY: test
