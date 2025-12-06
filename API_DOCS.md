# B+ Tree Index API Documentation

## NAME

**bplustree** - Disk-based B+ tree index for database systems

## SYNOPSIS

```python
from bplustree import BPlusTree

tree = BPlusTree(filename="index.idx")
tree.writeData(key, data)
tree.readData(key)
tree.readRangeData(lowerKey, upperKey)
tree.deleteData(key)
tree.close()
```

## DESCRIPTION

The **bplustree** module provides a complete implementation of a disk-based B+ tree index structure commonly used in database management systems. The implementation uses memory-mapped I/O for efficient disk access and supports datasets larger than available RAM.

### Key Features

- **Persistent Storage**: All data is stored in a single index file with 4096-byte pages
- **Efficient I/O**: Memory-mapped file access for optimal performance
- **Standard B+ Tree Operations**: Insert, delete, search, and range queries
- **Automatic Rebalancing**: Maintains B+ tree invariants through splitting and merging
- **Type Safety**: Integer keys with fixed 100-byte data tuples

## CLASSES

### BPlusTree

Main class for B+ tree index operations.

#### Constructor

```python
BPlusTree(filename: str = "bptree.idx")
```

Creates a new B+ tree index or opens an existing one.

**Parameters:**
- `filename` (str): Path to the index file (default: "bptree.idx")

**Behavior:**
- If file exists and contains data: Opens existing index
- If file doesn't exist or is empty: Creates new index with empty root

**Example:**
```python
# Create new index
tree = BPlusTree("students.idx")

# Open existing index
tree = BPlusTree("existing.idx")
```

---

## METHODS

### writeData

```python
writeData(key: int, data: bytes) -> bool
```

Insert a new key-data pair or update an existing key.

**Parameters:**
- `key` (int): Integer key for indexing (must be >= 0)
- `data` (bytes): Data bytes to store (any length)

**Returns:**
- `bool`: True if operation successful, False otherwise

**Behavior:**
- Data is automatically padded to 100 bytes if shorter
- Data is automatically truncated to 100 bytes if longer
- If key already exists, updates the data (no duplicate keys)
- Automatically splits nodes when they become full
- Changes are immediately written to disk

**Performance:**
- Average case: O(log n) where n is number of records
- Worst case: O(log n) with node splits

**Example:**
```python
# Insert new record
tree.writeData(101, b"Student: Alice, Grade: A")

# Update existing record
tree.writeData(101, b"Student: Alice, Grade: A+")

# Insert with exactly 100 bytes
data = b"X" * 100
tree.writeData(102, data)

# Insert with padding (< 100 bytes)
tree.writeData(103, b"Short data")  # Padded with null bytes

# Insert with truncation (> 100 bytes)
tree.writeData(104, b"Y" * 200)  # Truncated to 100 bytes
```

---

### readData

```python
readData(key: int) -> Optional[bytes]
```

Search for a key and retrieve its associated data.

**Parameters:**
- `key` (int): Integer key to search for

**Returns:**
- `bytes`: 100-byte data if key exists
- `None`: If key does not exist in index

**Performance:**
- Average case: O(log n)
- Worst case: O(log n)

**Example:**
```python
# Search for existing key
data = tree.readData(101)
if data:
    print(f"Found: {data.decode('utf-8', errors='ignore')}")
else:
    print("Key not found")

# Search for non-existent key
data = tree.readData(999)
assert data is None
```

---

### readRangeData

```python
readRangeData(lowerKey: int, upperKey: int) -> Tuple[Optional[List[bytes]], int]
```

Retrieve all data for keys within a specified range (inclusive).

**Parameters:**
- `lowerKey` (int): Lower bound of range (inclusive)
- `upperKey` (int): Upper bound of range (inclusive)

**Returns:**
- `tuple`: (data_list, count) where:
  - `data_list` (List[bytes]): List of 100-byte data tuples, or None if no records
  - `count` (int): Number of records found (0 if none)

**Behavior:**
- Includes both lowerKey and upperKey if they exist
- Returns records in ascending key order
- Efficient sequential scan through leaf nodes
- Returns (None, 0) if no keys in range

**Performance:**
- O(log n + k) where k is the number of records in range
- Utilizes leaf node linking for efficient scanning

**Example:**
```python
# Range query with results
data_list, count = tree.readRangeData(100, 200)
print(f"Found {count} records")
for i, data in enumerate(data_list):
    print(f"Record {i}: {data}")

# Empty range
data_list, count = tree.readRangeData(5000, 6000)
assert count == 0
assert data_list is None

# Single element range
data_list, count = tree.readRangeData(101, 101)
# Returns 1 record if key 101 exists

# Get first 100 records
data_list, count = tree.readRangeData(0, 99)
```

---

### deleteData

```python
deleteData(key: int) -> bool
```

Delete a key and its associated data from the index.

**Parameters:**
- `key` (int): Integer key to delete

**Returns:**
- `bool`: True if deletion successful, False if key not found

**Behavior:**
- Removes key-data pair from index
- Automatically merges or redistributes nodes to maintain B+ tree properties
- Changes are immediately written to disk
- Returns False if key doesn't exist (no error thrown)

**Performance:**
- Average case: O(log n)
- Worst case: O(log n) with node merging

**Example:**
```python
# Delete existing key
if tree.deleteData(101):
    print("Deletion successful")
else:
    print("Key not found")

# Verify deletion
data = tree.readData(101)
assert data is None

# Delete non-existent key
result = tree.deleteData(999)
assert result == False
```

---

### close

```python
close() -> None
```

Close the index file and release resources.

**Parameters:**
- None

**Returns:**
- None

**Behavior:**
- Flushes any pending writes to disk
- Closes memory-mapped file
- Closes underlying file handle
- Should always be called when done with the index

**Best Practice:**
Use with context manager pattern or ensure close() is called:

**Example:**
```python
# Manual close
tree = BPlusTree("index.idx")
try:
    tree.writeData(1, b"data")
finally:
    tree.close()

# Or ensure cleanup
tree = BPlusTree("index.idx")
tree.writeData(1, b"data")
tree.close()
```

---

## INTERNAL CLASSES

### BPlusTreeNode

Represents a node in the B+ tree (internal use only).

**Attributes:**
- `is_leaf` (bool): True if leaf node, False if internal
- `page_num` (int): Page number in index file
- `keys` (List[int]): List of keys in node
- `children` (List): Child page numbers (internal) or data bytes (leaf)
- `parent_page` (int): Parent page number (-1 if root)
- `next_leaf` (int): Next leaf page number (-1 if last)

**Methods:**
- `serialize()`: Convert node to 4096-byte representation
- `deserialize(data, page_num)`: Create node from byte data

---

## CONSTANTS

```python
PAGE_SIZE = 4096           # Size of each page in bytes
KEY_SIZE = 4               # Size of integer key in bytes
DATA_SIZE = 100            # Size of data tuple in bytes
HEADER_SIZE = 16           # Size of page header in bytes
INTERNAL_ORDER = ~509      # Max keys in internal node
LEAF_ORDER = ~39           # Max keys in leaf node
```

---

## FILE FORMAT

### Metadata Page (Page 0)

```
Offset | Size | Description
-------|------|-------------
0      | 4    | Root page number
4      | 4    | Next available page number
8-4095 | -    | Reserved
```

### Node Pages (Page 1+)

**Header (16 bytes):**
```
Offset | Size | Description
-------|------|-------------
0      | 1    | Node type (1=leaf, 0=internal)
1      | 4    | Number of keys
5      | 4    | Parent page number
9      | 4    | Next leaf page (leaf nodes only)
13     | 3    | Reserved
```

**Leaf Node Data:**
```
Offset | Size | Description
-------|------|-------------
16     | -    | Key-data pairs: (key[4], data[100]) repeated
```

**Internal Node Data:**
```
Offset | Size | Description
-------|------|-------------
16     | -    | Keys (4 bytes each)
-      | -    | Child pointers (4 bytes each)
```

---

## EXAMPLES

### Example 1: Basic CRUD Operations

```python
from bplustree import BPlusTree

# Create index
tree = BPlusTree("student.idx")

# Create: Insert student records
tree.writeData(101, b"Alice, CS, GPA: 3.8")
tree.writeData(102, b"Bob, EE, GPA: 3.6")
tree.writeData(103, b"Carol, ME, GPA: 3.9")

# Read: Retrieve a student
data = tree.readData(102)
print(f"Student 102: {data.decode('utf-8', errors='ignore')}")

# Update: Modify a record
tree.writeData(102, b"Bob, EE, GPA: 3.7")

# Delete: Remove a record
tree.deleteData(103)

# Cleanup
tree.close()
```

### Example 2: Range Queries

```python
from bplustree import BPlusTree

tree = BPlusTree("products.idx")

# Insert product records
for i in range(1, 101):
    data = f"Product {i}, Price: ${i*10}".encode('utf-8')
    tree.writeData(i, data)

# Query products 20-30
products, count = tree.readRangeData(20, 30)
print(f"Found {count} products:")
for product in products:
    print(f"  {product.decode('utf-8', errors='ignore')}")

tree.close()
```

### Example 3: Large Dataset

```python
from bplustree import BPlusTree
import time

tree = BPlusTree("large.idx")

# Insert 100,000 records
start = time.time()
for i in range(100000):
    data = f"Record {i:06d}".encode('utf-8')
    tree.writeData(i, data)
print(f"Inserted 100k records in {time.time()-start:.2f}s")

# Random access
import random
start = time.time()
for _ in range(1000):
    key = random.randint(0, 99999)
    data = tree.readData(key)
print(f"1000 random reads in {time.time()-start:.2f}s")

tree.close()
```

### Example 4: Error Handling

```python
from bplustree import BPlusTree

tree = BPlusTree("safe.idx")

# Safe insertion
try:
    success = tree.writeData(1, b"data")
    if not success:
        print("Insert failed")
except Exception as e:
    print(f"Error: {e}")

# Safe deletion
if tree.deleteData(999):
    print("Deleted")
else:
    print("Key not found (expected)")

# Safe search
data = tree.readData(999)
if data is None:
    print("Key doesn't exist (expected)")

tree.close()
```

---

## PERFORMANCE CONSIDERATIONS

### Best Practices

1. **Batch Operations**: Group insertions when possible
2. **Sequential Keys**: Provide better performance than random keys
3. **Range Queries**: More efficient than multiple point queries
4. **File Persistence**: Data is always written to disk immediately
5. **Close Files**: Always call close() to ensure data integrity

### Typical Performance (SSD)

| Operation | Records | Time/Op |
|-----------|---------|---------|
| Insert | 10,000 | 0.5 ms |
| Search | 10,000 | 0.2 ms |
| Delete | 10,000 | 0.6 ms |
| Range (100) | 10,000 | 2 ms |

---

## LIMITATIONS

- **Key Type**: Only integer keys supported (32-bit signed)
- **Data Size**: Fixed at 100 bytes per tuple
- **Page Size**: Fixed at 4096 bytes
- **File Size**: Limited by filesystem (practically unlimited)
- **Concurrency**: No built-in locking (single-threaded use)

---

## ERROR HANDLING

All public methods return success/failure indicators:
- `writeData`: Returns False on failure
- `deleteData`: Returns False if key not found
- `readData`: Returns None if key not found
- `readRangeData`: Returns (None, 0) if no records

Internal errors are caught and logged but don't crash the program.

---

## SEE ALSO

- **B+ Tree Theory**: Database Systems textbooks
- **Python mmap**: Standard library documentation
- **driver.py**: Comprehensive testing examples

---

## NOTES

This implementation is optimized for educational purposes and demonstrates core B+ tree concepts. Production databases would include additional features like:
- Concurrent access control (locking)
- Transaction support (ACID properties)
- Crash recovery (write-ahead logging)
- Buffer pool management
- Variable-length records
- Multiple indexes

---

## VERSION

Version 1.0 - December 2025

## AUTHOR

DBMS Assignment - Semester 5
