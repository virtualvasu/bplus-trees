# B+ Tree Index Implementation

A high-performance B+ tree index implementation for databases with disk-based storage using memory-mapped I/O.

## Features

- **Optimized for Speed**: Tuned for maximum performance with native architecture optimizations
- **Memory-Mapped I/O**: Direct memory access to disk pages for minimal overhead
- **Persistent Storage**: All data persists to disk automatically
- **Large Dataset Support**: Handles datasets larger than RAM efficiently
- **Standard API**: Simple C-style API for easy integration
- **4KB Pages**: Industry-standard 4096-byte page size
- **Fixed-Size Records**: 100-byte data records with integer keys

## Technical Specifications

- **Page Size**: 4096 bytes
- **Data Size**: 100 bytes per record
- **Key Type**: 32-bit signed integer
- **Leaf Node Capacity**: 39 entries per leaf
- **Internal Node Fanout**: 510 children per internal node
- **Storage**: File-backed with memory-mapped I/O

## Requirements

### Ubuntu/Linux
```bash
sudo apt-get update
sudo apt-get install build-essential g++
```

### Required Tools
- GCC/G++ compiler (version 4.8 or later)
- Make utility
- POSIX-compliant system (Linux, Unix, macOS)

## Setup and Installation

### 1. Extract Files
```bash
cd /path/to/dbms
ls -la
# You should see: bptree.h, bptree.cpp, test_driver.cpp, Makefile, README.md
```

### 2. Compilation

#### Quick Build
```bash
make
```

#### Build with Verbose Output
```bash
make clean && make
```

#### Build Static Library Only
```bash
make library
```

### 3. Execution

#### Run Test Driver
```bash
./test_driver
```

#### Run with Performance Timing
```bash
make run
```

## API Documentation

### Initialization

```c
void initBPTree(const char* filename);
```
Initializes the B+ tree index. Creates a new file if it doesn't exist, or loads existing index from file.

**Parameters:**
- `filename`: Path to the index file

**Example:**
```c
initBPTree("myindex.dat");
```

---

### Insert/Update

```c
int writeData(int key, const unsigned char* data);
```
Inserts a new key-value pair or updates existing key with new data.

**Parameters:**
- `key`: Integer key for indexing
- `data`: Pointer to 100-byte data array

**Returns:**
- `1` if successful
- `0` if failed

**Example:**
```c
unsigned char data[100];
memset(data, 42, 100);
int result = writeData(12345, data);
```

---

### Read

```c
int readData(int key, unsigned char* data_out);
```
Searches for a key and retrieves its data.

**Parameters:**
- `key`: Integer key to search for
- `data_out`: Pre-allocated 100-byte buffer for output

**Returns:**
- `1` if key found (data copied to data_out)
- `0` if key not found

**Example:**
```c
unsigned char buffer[100];
if (readData(12345, buffer)) {
    printf("Key found!\n");
}
```

---

### Delete

```c
int deleteData(int key);
```
Deletes a key and its associated data from the index.

**Parameters:**
- `key`: Integer key to delete

**Returns:**
- `1` if successfully deleted
- `0` if key not found or deletion failed

**Example:**
```c
int result = deleteData(12345);
```

---

### Range Query

```c
unsigned char** readRangeData(int lowerKey, int upperKey, int* n);
```
Retrieves all key-value pairs in the specified range (inclusive).

**Parameters:**
- `lowerKey`: Lower bound of range (inclusive)
- `upperKey`: Upper bound of range (inclusive)
- `n`: Pointer to integer that will store the number of results

**Returns:**
- Array of pointers to 100-byte data arrays
- `NULL` if no results found

**Important:** Must free returned data using `freeRangeData()`

**Example:**
```c
int count;
unsigned char** results = readRangeData(100, 200, &count);
if (results) {
    printf("Found %d results\n", count);
    for (int i = 0; i < count; i++) {
        // Process results[i] (100 bytes)
    }
    freeRangeData(results, count);
}
```

---

### Free Range Data

```c
void freeRangeData(unsigned char** data, int n);
```
Frees memory allocated by `readRangeData()`.

**Parameters:**
- `data`: Array returned by `readRangeData()`
- `n`: Number of entries (returned by `readRangeData()`)

---

### Cleanup

```c
void closeBPTree(void);
```
Closes the B+ tree and flushes all changes to disk. Should be called before program exit.

**Example:**
```c
closeBPTree();
```

## Performance Optimizations

### Implemented Optimizations

1. **Memory-Mapped I/O**: Direct memory access eliminates buffer copying
2. **Optimal Node Fanout**: Calculated to maximize page utilization
3. **Binary Search**: Fast logarithmic search in nodes
4. **Linked Leaf Nodes**: Efficient range queries without tree traversal
5. **Amortized Growth**: File doubles in size to reduce reallocation overhead
6. **Compiler Optimizations**: `-O3 -march=native -ffast-math` flags
7. **Cache-Friendly Layout**: Structures packed to minimize cache misses
8. **Asynchronous Sync**: Non-blocking writes to disk

### Expected Performance

- **Insertions**: ~50,000-100,000 ops/second
- **Point Queries**: ~100,000-200,000 ops/second  
- **Range Queries**: Depends on result size, ~50,000 records/second
- **Sequential Access**: Faster than random due to cache locality

## Testing

### Run All Tests
```bash
make test
```

### Test Coverage
The test driver verifies:
1. Basic insert and read operations
2. Multiple inserts (100 keys)
3. Update existing keys
4. Delete operations
5. Range queries
6. Large dataset (100,000 keys)
7. Persistence (reload from disk)
8. Sequential vs random access patterns
9. Edge cases (negative keys, INT_MAX, etc.)

## Makefile Targets

```bash
make          # Build test driver (default)
make test     # Build and run tests
make run      # Build and run with timing
make clean    # Remove build artifacts
make library  # Build static library
make help     # Show help message
```

## File Structure

```
dbms/
├── bptree.h           # API header file
├── bptree.cpp         # B+ tree implementation
├── test_driver.cpp    # Test driver program
├── Makefile           # Build configuration
├── README.md          # This file
└── test_index.dat     # Created at runtime (index file)
```

## Usage Example

```c
#include "bptree.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Initialize
    initBPTree("myindex.dat");
    
    // Insert data
    unsigned char data[100];
    for (int i = 0; i < 1000; i++) {
        memset(data, i % 256, 100);
        writeData(i, data);
    }
    
    // Read data
    unsigned char buffer[100];
    if (readData(500, buffer)) {
        printf("Found key 500\n");
    }
    
    // Range query
    int count;
    unsigned char** results = readRangeData(100, 200, &count);
    printf("Found %d keys in range [100, 200]\n", count);
    freeRangeData(results, count);
    
    // Delete
    deleteData(500);
    
    // Cleanup
    closeBPTree();
    
    return 0;
}
```

## Troubleshooting

### Compilation Errors

**Error: `mmap` not found**
- Solution: Ensure you're compiling on a POSIX-compliant system (Linux/Unix/macOS)

**Error: `O_CREAT` undeclared**
- Solution: Check that `<fcntl.h>` is available on your system

### Runtime Errors

**Segmentation Fault**
- Ensure `initBPTree()` is called before any other API functions
- Ensure `data_out` buffer is allocated (100 bytes) before calling `readData()`
- Always call `freeRangeData()` after `readRangeData()`

**Permission Denied**
- Ensure write permissions in the directory
- Check disk space availability

## Design Decisions

### Why Memory-Mapped I/O?
- **Performance**: Eliminates system call overhead
- **Simplicity**: OS handles page management
- **Efficiency**: Lazy loading and automatic write-back

### Why 4KB Pages?
- Standard OS page size for alignment
- Efficient disk I/O operations
- Balances memory usage vs. I/O overhead

### Node Fanout Calculation
- **Leaf Nodes**: Maximizes entries while fitting in 4KB page
- **Internal Nodes**: High fanout (510) reduces tree height for faster searches

## Limitations

1. **No Concurrency**: Not thread-safe (single-threaded only)
2. **Simplified Deletion**: Doesn't fully rebalance tree on underflow
3. **Fixed Data Size**: 100 bytes per record (not variable)
4. **Integer Keys Only**: Keys must be 32-bit signed integers

## Future Enhancements

- Thread-safe operations with read-write locks
- Full tree rebalancing on deletions
- Variable-length data support
- Composite key support
- Bulk loading optimization

## License

This implementation is provided for educational purposes.

## Author

Created for DBMS course assignment - B+ Tree Index Implementation

## Testing on Ubuntu

```bash
# On standard Ubuntu desktop
sudo apt-get install build-essential
cd /path/to/dbms
make clean
make test
```

Expected output: All tests should pass with performance metrics displayed.

---

For questions or issues, refer to the source code comments or API documentation above.
