# B+ Tree Index Implementation

A high-performance disk-based B+ tree index implementation for database systems with memory-mapped I/O.

## Overview

This project implements a B+ tree index as used in database systems. The index is stored in a file on disk and supports efficient insertion, deletion, point queries, and range queries. The implementation uses memory-mapped I/O for optimal performance and supports datasets that exceed available RAM.

## Features

- **Disk-based Storage**: All data persists in a single index file with 4096-byte pages
- **Memory-mapped I/O**: Efficient disk access without manual buffer management
- **Integer Keys**: Optimized for 32-bit integer keys
- **Fixed-size Data**: Each tuple stored in 100-byte fixed-size blocks
- **Complete B+ Tree Operations**:
  - Insert/Update (`writeData`)
  - Delete with rebalancing (`deleteData`)
  - Point queries (`readData`)
  - Range queries (`readRangeData`)
- **Persistent Storage**: Data survives program restarts
- **Automatic Node Splitting/Merging**: Maintains B+ tree properties

## Specifications

| Parameter | Value |
|-----------|-------|
| Page Size | 4096 bytes |
| Key Type | Integer (4 bytes) |
| Data Size | 100 bytes per tuple |
| Internal Node Order | ~509 |
| Leaf Node Order | ~39 |
| I/O Method | Memory-mapped I/O |

## Requirements

- Python 3.6 or higher
- Standard library only (no external dependencies)
- Linux/Unix system (tested on Ubuntu)

## Project Structure

```
dbms_project/
├── bplustree.py       # Core B+ tree implementation
├── driver.py          # Comprehensive test driver
├── Makefile           # Build and run automation
├── README.md          # This file
└── API_DOCS.md        # Detailed API documentation
```

## Installation & Setup

### Quick Start

1. **Clone or extract the project**:
   ```bash
   cd /home/virtualvasu/Desktop/sem5/dbms_project
   ```

2. **Verify Python installation**:
   ```bash
   python3 --version
   # Should show Python 3.6 or higher
   ```

3. **Make scripts executable**:
   ```bash
   chmod +x driver.py
   ```

### Using Makefile

The project includes a Makefile for easy compilation and execution:

```bash
# Run the test driver
make run

# Run tests and save output to log file
make test

# Clean generated files
make clean

# View help
make help
```

## Usage

### Running the Driver Program

The driver program tests all functionalities:

```bash
# Method 1: Using Makefile
make run

# Method 2: Direct execution
python3 driver.py

# Method 3: With executable permission
./driver.py
```

### Using the B+ Tree in Your Code

```python
from bplustree import BPlusTree

# Create or open an index
tree = BPlusTree("myindex.idx")

# Insert data
key = 42
data = b"Sample data for key 42"
success = tree.writeData(key, data)

# Read data
result = tree.readData(key)
if result:
    print(f"Found: {result}")

# Range query
data_list, count = tree.readRangeData(10, 50)
print(f"Found {count} records in range [10, 50]")

# Delete data
success = tree.deleteData(key)

# Always close when done
tree.close()
```

## API Reference

### writeData(key, data)

Insert or update a key-value pair.

**Parameters:**
- `key` (int): Integer key for indexing
- `data` (bytes): Data bytes (will be padded/truncated to 100 bytes)

**Returns:**
- `bool`: True if successful, False otherwise

**Example:**
```python
tree.writeData(101, b"Student record for ID 101")
```

### readData(key)

Search for a key and retrieve its data.

**Parameters:**
- `key` (int): Integer key to search for

**Returns:**
- `bytes`: 100-byte data if found, None otherwise

**Example:**
```python
data = tree.readData(101)
if data:
    print(f"Data: {data}")
```

### deleteData(key)

Delete a key from the index.

**Parameters:**
- `key` (int): Integer key to delete

**Returns:**
- `bool`: True if deletion successful, False if key not found

**Example:**
```python
if tree.deleteData(101):
    print("Deletion successful")
```

### readRangeData(lowerKey, upperKey)

Retrieve all records in a key range (inclusive).

**Parameters:**
- `lowerKey` (int): Lower bound of range (inclusive)
- `upperKey` (int): Upper bound of range (inclusive)

**Returns:**
- `tuple`: (list of data bytes, count) or (None, 0) if no records found

**Example:**
```python
data_list, count = tree.readRangeData(100, 200)
print(f"Found {count} records")
for data in data_list:
    print(data)
```

## Testing

The `driver.py` program includes comprehensive tests:

1. **Basic Operations Test**: Insert, read, update, delete
2. **Large Dataset Test**: 10,000 records with performance metrics
3. **Edge Cases Test**: Boundary conditions, duplicates, non-existent keys
4. **Persistence Test**: Data survival across program restarts
5. **Sequential Access Test**: Sequential patterns and range scans
6. **Performance Benchmark**: Timing for various operations

### Running Tests

```bash
# Run all tests
make test

# Or directly
python3 driver.py

# Save output to file
python3 driver.py > test_results.txt
```

### Expected Output

The driver will display:
- Test progress and results
- Performance metrics (time per operation)
- Success/failure status for each test
- Summary statistics

## Performance

Typical performance on standard Ubuntu desktop (SSD):

| Operation | Dataset Size | Avg Time |
|-----------|--------------|----------|
| Insert | 10,000 | ~0.5 ms |
| Point Query | 10,000 | ~0.2 ms |
| Range Query (100) | 10,000 | ~2 ms |
| Delete | 10,000 | ~0.6 ms |

*Note: Performance varies based on hardware and dataset characteristics*

## File Format

The index file uses the following structure:

```
Page 0: Metadata
  - Bytes 0-3: Root page number
  - Bytes 4-7: Next available page number

Page 1+: B+ Tree Nodes
  - Byte 0: Node type (1=leaf, 0=internal)
  - Bytes 1-4: Number of keys
  - Bytes 5-8: Parent page number
  - Bytes 9-12: Next leaf pointer (leaf only)
  - Bytes 16+: Keys and data/pointers
```

## Implementation Details

### Node Structure

- **Internal Nodes**: Store keys and child page pointers
- **Leaf Nodes**: Store keys and actual data tuples
- **Leaf Linking**: Leaf nodes form a linked list for range queries

### Algorithms

- **Insertion**: Standard B+ tree insertion with node splitting
- **Deletion**: Deletion with node merging and key redistribution
- **Search**: Binary search within nodes, tree traversal for navigation
- **Range Query**: Sequential scan through linked leaf nodes

### Memory Management

- Uses Python's `mmap` module for memory-mapped I/O
- Automatic file extension when needed
- Efficient page-level I/O

## Troubleshooting

### Permission Denied

```bash
chmod +x driver.py
chmod 644 bplustree.py
```

### Module Not Found

Ensure both `bplustree.py` and `driver.py` are in the same directory.

### File Already Exists

The program handles existing index files automatically. To start fresh:

```bash
make clean
```

## Compilation and Execution

This is a pure Python implementation requiring no compilation:

1. **Verify Python**:
   ```bash
   python3 --version
   ```

2. **Run driver**:
   ```bash
   python3 driver.py
   ```

3. **Or use Makefile**:
   ```bash
   make run
   ```

## Submission Contents

- `bplustree.py` - Core implementation
- `driver.py` - Test driver
- `Makefile` - Build automation
- `README.md` - This documentation
- `API_DOCS.md` - Detailed API documentation (man-style)

## Author

Created for DBMS Course Assignment - Semester 5

## License

Academic use only - for educational purposes.
