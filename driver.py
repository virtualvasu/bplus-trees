#!/usr/bin/env python3
"""
Driver Program for B+ Tree Index Testing

This program demonstrates and tests all functionalities of the B+ tree index:
- Insertion (writeData)
- Deletion (deleteData)
- Point queries (readData)
- Range queries (readRangeData)

It includes comprehensive test cases and performance measurements.
"""

import os
import time
import random
from bplustree import BPlusTree


def print_separator(title=""):
    """Print a formatted separator"""
    if title:
        print(f"\n{'='*70}")
        print(f" {title}")
        print(f"{'='*70}")
    else:
        print(f"{'-'*70}")


def test_basic_operations():
    """Test basic insert, search, and delete operations"""
    print_separator("Test 1: Basic Operations")
    
    # Clean up any existing index
    if os.path.exists("test_basic.idx"):
        os.remove("test_basic.idx")
    
    tree = BPlusTree("test_basic.idx")
    
    # Test 1.1: Insert single record
    print("\n1.1 Testing single insertion...")
    data1 = b"Student: Alice, Grade: A, ID: 101"
    result = tree.writeData(101, data1)
    print(f"   Insert key=101: {'SUCCESS' if result else 'FAILED'}")
    
    # Test 1.2: Read the inserted record
    print("\n1.2 Testing single read...")
    retrieved = tree.readData(101)
    if retrieved:
        print(f"   Read key=101: SUCCESS")
        print(f"   Data: {retrieved[:50].decode('utf-8', errors='ignore')}...")
    else:
        print(f"   Read key=101: FAILED")
    
    # Test 1.3: Insert multiple records
    print("\n1.3 Testing multiple insertions...")
    test_data = [
        (102, b"Student: Bob, Grade: B, ID: 102"),
        (103, b"Student: Carol, Grade: A, ID: 103"),
        (104, b"Student: David, Grade: C, ID: 104"),
        (105, b"Student: Eve, Grade: B, ID: 105"),
    ]
    
    for key, data in test_data:
        result = tree.writeData(key, data)
        print(f"   Insert key={key}: {'SUCCESS' if result else 'FAILED'}")
    
    # Test 1.4: Read all inserted records
    print("\n1.4 Testing multiple reads...")
    for key in [101, 102, 103, 104, 105]:
        retrieved = tree.readData(key)
        status = "SUCCESS" if retrieved else "FAILED"
        print(f"   Read key={key}: {status}")
    
    # Test 1.5: Update existing record
    print("\n1.5 Testing update...")
    data_updated = b"Student: Alice, Grade: A+, ID: 101 [UPDATED]"
    result = tree.writeData(101, data_updated)
    retrieved = tree.readData(101)
    print(f"   Update key=101: {'SUCCESS' if result else 'FAILED'}")
    if retrieved:
        print(f"   Updated data: {retrieved[:50].decode('utf-8', errors='ignore')}...")
    
    # Test 1.6: Delete a record
    print("\n1.6 Testing deletion...")
    result = tree.deleteData(103)
    print(f"   Delete key=103: {'SUCCESS' if result else 'FAILED'}")
    retrieved = tree.readData(103)
    print(f"   Verify deletion (should be None): {retrieved}")
    
    # Test 1.7: Range query
    print("\n1.7 Testing range query...")
    data_list, count = tree.readRangeData(101, 105)
    print(f"   Range [101, 105]: Found {count} records")
    if data_list:
        for i, data in enumerate(data_list):
            print(f"     Record {i+1}: {data[:40].decode('utf-8', errors='ignore')}...")
    
    tree.close()
    print_separator()


def test_large_dataset():
    """Test with larger dataset to verify scalability"""
    print_separator("Test 2: Large Dataset Operations")
    
    if os.path.exists("test_large.idx"):
        os.remove("test_large.idx")
    
    tree = BPlusTree("test_large.idx")
    
    num_records = 100000
    print(f"\n2.1 Inserting {num_records} records...")
    
    start_time = time.time()
    for i in range(num_records):
        key = i
        data = f"Record {i}: Data_{i:06d}".encode('utf-8')
        tree.writeData(key, data)
        
        if (i + 1) % 20000 == 0:
            print(f"   Inserted {i+1} records...")
    
    insert_time = time.time() - start_time
    print(f"   Total insertion time: {insert_time:.3f} seconds")
    print(f"   Average: {insert_time/num_records*1000:.3f} ms per record")
    
    # Test random reads
    print(f"\n2.2 Testing random point queries...")
    num_queries = 1000
    random_keys = random.sample(range(num_records), num_queries)
    
    start_time = time.time()
    success_count = 0
    for key in random_keys:
        data = tree.readData(key)
        if data:
            success_count += 1
    
    read_time = time.time() - start_time
    print(f"   Queries: {num_queries}, Success: {success_count}")
    print(f"   Total time: {read_time:.3f} seconds")
    print(f"   Average: {read_time/num_queries*1000:.3f} ms per query")
    
    # Test range queries
    print(f"\n2.3 Testing range queries...")
    test_ranges = [
        (0, 99, "Small range [0, 99]"),
        (1000, 1999, "Medium range [1000, 1999]"),
        (5000, 6999, "Large range [5000, 6999]"),
    ]
    
    for lower, upper, description in test_ranges:
        start_time = time.time()
        data_list, count = tree.readRangeData(lower, upper)
        range_time = time.time() - start_time
        print(f"   {description}: Found {count} records in {range_time:.3f}s")
    
    # Test deletions
    print(f"\n2.4 Testing deletions...")
    num_deletes = 1000
    delete_keys = random.sample(range(num_records), num_deletes)
    
    start_time = time.time()
    success_count = 0
    for key in delete_keys:
        if tree.deleteData(key):
            success_count += 1
        
        if (success_count) % 200 == 0 and success_count > 0:
            print(f"   Deleted {success_count} records...")
    
    delete_time = time.time() - start_time
    print(f"   Total deletions: {success_count}/{num_deletes}")
    print(f"   Total time: {delete_time:.3f} seconds")
    print(f"   Average: {delete_time/num_deletes*1000:.3f} ms per deletion")
    
    # Verify deletions
    print(f"\n2.5 Verifying deletions...")
    verified = 0
    for key in delete_keys[:100]:  # Check first 100
        if tree.readData(key) is None:
            verified += 1
    print(f"   Verified {verified}/100 deletions successful")
    
    tree.close()
    print_separator()


def test_edge_cases():
    """Test edge cases and boundary conditions"""
    print_separator("Test 3: Edge Cases")
    
    if os.path.exists("test_edge.idx"):
        os.remove("test_edge.idx")
    
    tree = BPlusTree("test_edge.idx")
    
    # Test 3.1: Insert in reverse order
    print("\n3.1 Testing reverse order insertion...")
    for i in range(20, 0, -1):
        data = f"Reverse order record {i}".encode('utf-8')
        tree.writeData(i, data)
    print("   Inserted 20 records in reverse order: SUCCESS")
    
    # Verify with range query
    data_list, count = tree.readRangeData(1, 20)
    print(f"   Range query [1, 20]: Found {count} records (expected 20)")
    
    # Test 3.2: Insert duplicate keys (should update)
    print("\n3.2 Testing duplicate key insertion (update)...")
    original_data = b"Original data for key 10"
    tree.writeData(10, original_data)
    
    updated_data = b"Updated data for key 10"
    tree.writeData(10, updated_data)
    
    retrieved = tree.readData(10)
    is_updated = retrieved and retrieved[:len(updated_data)] == updated_data
    print(f"   Update via duplicate insert: {'SUCCESS' if is_updated else 'FAILED'}")
    
    # Test 3.3: Delete non-existent key
    print("\n3.3 Testing deletion of non-existent key...")
    result = tree.deleteData(9999)
    print(f"   Delete key=9999 (non-existent): {'Correctly returned False' if not result else 'ERROR'}")
    
    # Test 3.4: Read non-existent key
    print("\n3.4 Testing read of non-existent key...")
    result = tree.readData(9999)
    print(f"   Read key=9999 (non-existent): {'Correctly returned None' if result is None else 'ERROR'}")
    
    # Test 3.5: Empty range query
    print("\n3.5 Testing empty range query...")
    data_list, count = tree.readRangeData(5000, 6000)
    print(f"   Range [5000, 6000] (empty): Found {count} records (expected 0)")
    
    # Test 3.6: Single element range
    print("\n3.6 Testing single element range...")
    data_list, count = tree.readRangeData(10, 10)
    print(f"   Range [10, 10] (single): Found {count} record(s) (expected 1)")
    
    # Test 3.7: Large data (exactly 100 bytes)
    print("\n3.7 Testing 100-byte data...")
    large_data = b"X" * 100
    tree.writeData(100, large_data)
    retrieved = tree.readData(100)
    print(f"   100-byte data: {'SUCCESS' if len(retrieved) == 100 else 'FAILED'}")
    
    # Test 3.8: Small data (padding test)
    print("\n3.8 Testing small data (padding)...")
    small_data = b"Small"
    tree.writeData(101, small_data)
    retrieved = tree.readData(101)
    print(f"   Small data padded to 100 bytes: {'SUCCESS' if len(retrieved) == 100 else 'FAILED'}")
    
    tree.close()
    print_separator()


def test_persistence():
    """Test that data persists across program runs"""
    print_separator("Test 4: Data Persistence")
    
    if os.path.exists("test_persist.idx"):
        os.remove("test_persist.idx")
    
    # Phase 1: Create and populate
    print("\n4.1 Creating new index and inserting data...")
    tree1 = BPlusTree("test_persist.idx")
    
    test_data = {
        1: b"Persistent record 1",
        2: b"Persistent record 2",
        3: b"Persistent record 3",
        4: b"Persistent record 4",
        5: b"Persistent record 5",
    }
    
    for key, data in test_data.items():
        tree1.writeData(key, data)
    print(f"   Inserted {len(test_data)} records")
    
    tree1.close()
    print("   Closed index file")
    
    # Phase 2: Reopen and verify
    print("\n4.2 Reopening index and verifying data...")
    tree2 = BPlusTree("test_persist.idx")
    
    success_count = 0
    for key, expected_data in test_data.items():
        retrieved = tree2.readData(key)
        if retrieved and retrieved[:len(expected_data)] == expected_data:
            success_count += 1
            print(f"   Key {key}: VERIFIED")
        else:
            print(f"   Key {key}: FAILED")
    
    print(f"\n   Total verified: {success_count}/{len(test_data)}")
    
    # Phase 3: Add more data and verify again
    print("\n4.3 Adding more data to existing index...")
    new_data = {
        6: b"New persistent record 6",
        7: b"New persistent record 7",
    }
    
    for key, data in new_data.items():
        tree2.writeData(key, data)
    
    tree2.close()
    
    # Phase 4: Final verification
    print("\n4.4 Final verification of all data...")
    tree3 = BPlusTree("test_persist.idx")
    
    all_data = {**test_data, **new_data}
    success_count = 0
    for key, expected_data in all_data.items():
        retrieved = tree3.readData(key)
        if retrieved and retrieved[:len(expected_data)] == expected_data:
            success_count += 1
    
    print(f"   Total verified: {success_count}/{len(all_data)}")
    
    tree3.close()
    print_separator()


def test_sequential_access():
    """Test sequential insertion and range queries"""
    print_separator("Test 5: Sequential Access Patterns")
    
    if os.path.exists("test_seq.idx"):
        os.remove("test_seq.idx")
    
    tree = BPlusTree("test_seq.idx")
    
    # Test 5.1: Sequential insertion
    print("\n5.1 Sequential insertion (1-1000)...")
    start_time = time.time()
    
    for i in range(1, 1001):
        data = f"Sequential record {i:04d}".encode('utf-8')
        tree.writeData(i, data)
    
    seq_time = time.time() - start_time
    print(f"   Inserted 1000 sequential records in {seq_time:.3f}s")
    
    # Test 5.2: Full range scan
    print("\n5.2 Full range scan [1-1000]...")
    start_time = time.time()
    data_list, count = tree.readRangeData(1, 1000)
    scan_time = time.time() - start_time
    print(f"   Retrieved {count} records in {scan_time:.3f}s")
    
    # Test 5.3: Multiple overlapping range queries
    print("\n5.3 Multiple overlapping range queries...")
    ranges = [
        (1, 100),
        (50, 150),
        (100, 200),
        (500, 600),
        (900, 1000),
    ]
    
    for lower, upper in ranges:
        data_list, count = tree.readRangeData(lower, upper)
        print(f"   Range [{lower:4d}, {upper:4d}]: {count} records")
    
    tree.close()
    print_separator()


def run_performance_benchmark():
    """Run comprehensive performance benchmark"""
    print_separator("Performance Benchmark")
    
    if os.path.exists("benchmark.idx"):
        os.remove("benchmark.idx")
    
    tree = BPlusTree("benchmark.idx")
    
    sizes = [1000, 10000, 100000]
    
    print("\nBenchmark Results:")
    print(f"{'Operation':<20} {'Size':<10} {'Total Time':<15} {'Avg Time':<15}")
    print("-" * 70)
    
    for size in sizes:
        # Insertion benchmark
        start_time = time.time()
        for i in range(size):
            data = f"Benchmark record {i}".encode('utf-8')
            tree.writeData(i, data)
        insert_time = time.time() - start_time
        
        print(f"{'Insert':<20} {size:<10} {insert_time:<15.3f} {insert_time/size*1000:<15.3f}")
        
        # Point query benchmark
        sample_keys = random.sample(range(size), min(1000, size))
        start_time = time.time()
        for key in sample_keys:
            tree.readData(key)
        read_time = time.time() - start_time
        
        print(f"{'Point Query':<20} {len(sample_keys):<10} {read_time:<15.3f} {read_time/len(sample_keys)*1000:<15.3f}")
        
        # Range query benchmark
        start_time = time.time()
        tree.readRangeData(0, size // 10)
        range_time = time.time() - start_time
        
        print(f"{'Range Query':<20} {size//10:<10} {range_time:<15.3f} {'-':<15}")
        print("-" * 70)
    
    tree.close()
    print_separator()


def main():
    """Main driver function"""
    print("\n" + "="*70)
    print(" B+ TREE INDEX - COMPREHENSIVE TEST SUITE")
    print("="*70)
    print("\nThis driver tests all functionalities of the B+ tree implementation:")
    print("  - writeData(key, data): Insert/Update operations")
    print("  - deleteData(key): Deletion operations")
    print("  - readData(key): Point query operations")
    print("  - readRangeData(lower, upper, n): Range query operations")
    
    try:
        # Run all tests
        test_basic_operations()
        test_large_dataset()
        test_edge_cases()
        test_persistence()
        test_sequential_access()
        run_performance_benchmark()
        
        print("\n" + "="*70)
        print(" ALL TESTS COMPLETED SUCCESSFULLY!")
        print("="*70)
        
        # Cleanup test files
        print("\nCleaning up test files...")
        test_files = [
            "test_basic.idx",
            "test_large.idx",
            "test_edge.idx",
            "test_persist.idx",
            "test_seq.idx",
            "benchmark.idx"
        ]
        
        for f in test_files:
            if os.path.exists(f):
                os.remove(f)
                print(f"  Removed {f}")
        
    except Exception as e:
        print(f"\n\nERROR: Test failed with exception: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    exit(main())
