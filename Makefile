# B+ Tree Index Makefile

CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall -Wextra -march=native -ffast-math
LDFLAGS = 

# Source files
SOURCES = bptree.cpp
HEADERS = bptree.h
TEST_SOURCES = test_driver.cpp

# Output files
TARGET = test_driver
LIBRARY = libbptree.a

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

.PHONY: all clean test run

# Default target
all: $(TARGET)

# Build test driver
$(TARGET): $(TEST_OBJECTS) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build successful! Run with: ./$(TARGET)"

# Build static library
$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "Library built: $(LIBRARY)"

# Compile source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	./$(TARGET)

# Run with timing
run: $(TARGET)
	@echo "Running B+ Tree test driver..."
	time ./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TARGET) $(LIBRARY)
	rm -f test_index.dat
	@echo "Cleaned build artifacts"

# Help target
help:
	@echo "B+ Tree Index Makefile"
	@echo "======================"
	@echo "Available targets:"
	@echo "  make          - Build test driver (default)"
	@echo "  make test     - Build and run tests"
	@echo "  make run      - Build and run with timing"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make library  - Build static library"
	@echo "  make help     - Show this help message"

library: $(LIBRARY)
