CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -march=native

all: driver

driver: driver.cpp bplustree.cpp bplustree.hpp
	$(CXX) $(CXXFLAGS) driver.cpp bplustree.cpp -o driver

bench: bench.cpp bplustree.cpp bplustree.hpp
	$(CXX) $(CXXFLAGS) bench.cpp bplustree.cpp -o bench

clean:
	rm -f driver index_cpp.idx
