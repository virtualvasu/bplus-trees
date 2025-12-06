/*
 * Test Driver for B+ Tree Index
 * Tests all API functions with various scenarios
 */

#include "bptree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define TEST_DATA_SIZE 100
#define LARGE_TEST_SIZE 100000

// Helper function to generate random data
void generate_random_data(unsigned char* data, int key) {
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        data[i] = (key + i) % 256;
    }
}

// Helper function to verify data
int verify_data(unsigned char* data, int key) {
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        if (data[i] != (unsigned char)((key + i) % 256)) {
            return 0;
        }
    }
    return 1;
}

// Test 1: Basic Insert and Read
void test_basic_insert_read() {
    printf("\n=== Test 1: Basic Insert and Read ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    
    // Insert single key
    generate_random_data(data, 42);
    assert(writeData(42, data) == 1);
    printf("✓ Inserted key 42\n");
    
    // Read it back
    assert(readData(42, read_data) == 1);
    assert(verify_data(read_data, 42));
    printf("✓ Read key 42 successfully\n");
    
    // Try to read non-existent key
    assert(readData(999, read_data) == 0);
    printf("✓ Non-existent key returns 0\n");
}

// Test 2: Multiple Inserts
void test_multiple_inserts() {
    printf("\n=== Test 2: Multiple Inserts ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    
    // Insert 100 keys
    for (int i = 0; i < 100; i++) {
        generate_random_data(data, i);
        assert(writeData(i, data) == 1);
    }
    printf("✓ Inserted 100 keys\n");
    
    // Verify all keys
    for (int i = 0; i < 100; i++) {
        assert(readData(i, read_data) == 1);
        assert(verify_data(read_data, i));
    }
    printf("✓ All 100 keys verified\n");
}

// Test 3: Update Existing Key
void test_update() {
    printf("\n=== Test 3: Update Existing Key ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    
    // Insert key
    generate_random_data(data, 100);
    assert(writeData(100, data) == 1);
    printf("✓ Inserted key 100\n");
    
    // Update with new data
    memset(data, 99, TEST_DATA_SIZE);
    assert(writeData(100, data) == 1);
    printf("✓ Updated key 100\n");
    
    // Verify update
    assert(readData(100, read_data) == 1);
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        assert(read_data[i] == 99);
    }
    printf("✓ Update verified\n");
}

// Test 4: Delete Operation
void test_delete() {
    printf("\n=== Test 4: Delete Operation ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    
    // Insert keys 200-210
    for (int i = 200; i <= 210; i++) {
        generate_random_data(data, i);
        assert(writeData(i, data) == 1);
    }
    printf("✓ Inserted keys 200-210\n");
    
    // Delete key 205
    assert(deleteData(205) == 1);
    printf("✓ Deleted key 205\n");
    
    // Verify deletion
    assert(readData(205, read_data) == 0);
    printf("✓ Deleted key not found\n");
    
    // Verify other keys still exist
    assert(readData(204, read_data) == 1);
    assert(readData(206, read_data) == 1);
    printf("✓ Adjacent keys still exist\n");
    
    // Try to delete non-existent key
    assert(deleteData(999) == 0);
    printf("✓ Deleting non-existent key returns 0\n");
}

// Test 5: Range Query
void test_range_query() {
    printf("\n=== Test 5: Range Query ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    
    // Insert keys 300-320
    for (int i = 300; i <= 320; i++) {
        generate_random_data(data, i);
        writeData(i, data);
    }
    printf("✓ Inserted keys 300-320\n");
    
    // Range query [305, 315]
    int n;
    unsigned char** results = readRangeData(305, 315, &n);
    assert(results != NULL);
    assert(n == 11); // 305 to 315 inclusive
    printf("✓ Range query [305, 315] returned %d results\n", n);
    
    // Verify results
    for (int i = 0; i < n; i++) {
        assert(verify_data(results[i], 305 + i));
    }
    printf("✓ All range query results verified\n");
    
    freeRangeData(results, n);
    printf("✓ Freed range data\n");
    
    // Range query with no results
    results = readRangeData(400, 500, &n);
    assert(results == NULL);
    assert(n == 0);
    printf("✓ Empty range query returns NULL\n");
}

// Test 6: Large Dataset
void test_large_dataset() {
    printf("\n=== Test 6: Large Dataset ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    clock_t start, end;
    
    printf("Inserting %d keys...\n", LARGE_TEST_SIZE);
    start = clock();
    for (int i = 0; i < LARGE_TEST_SIZE; i++) {
        generate_random_data(data, i);
        assert(writeData(i, data) == 1);
        
        if ((i + 1) % 10000 == 0) {
            printf("  Inserted %d keys\n", i + 1);
        }
    }
    end = clock();
    double insert_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("✓ Inserted %d keys in %.2f seconds (%.0f ops/sec)\n", 
           LARGE_TEST_SIZE, insert_time, LARGE_TEST_SIZE / insert_time);
    
    // Random reads
    printf("Performing %d random reads...\n", LARGE_TEST_SIZE / 10);
    unsigned char read_data[TEST_DATA_SIZE];
    start = clock();
    int verify_count = LARGE_TEST_SIZE / 10;
    for (int i = 0; i < verify_count; i++) {
        int key = rand() % LARGE_TEST_SIZE;
        assert(readData(key, read_data) == 1);
        assert(verify_data(read_data, key));
    }
    end = clock();
    double read_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("✓ Performed %d random reads in %.2f seconds (%.0f ops/sec)\n", 
           verify_count, read_time, verify_count / read_time);
    
    // Range query on large dataset
    printf("Performing range query [10000, 20000]...\n");
    start = clock();
    int n;
    unsigned char** results = readRangeData(10000, 20000, &n);
    end = clock();
    double range_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    assert(n == 10001);
    printf("✓ Range query returned %d results in %.3f seconds\n", n, range_time);
    
    // Verify sample of range results
    for (int i = 0; i < 100; i++) {
        int idx = rand() % n;
        assert(results[idx] != NULL);
    }
    printf("✓ Verified sample of range results\n");
    
    freeRangeData(results, n);
}

// Test 7: Persistence (reload from disk)
void test_persistence() {
    printf("\n=== Test 7: Persistence Test ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    
    // Close and reopen
    closeBPTree();
    printf("✓ Closed B+ tree\n");
    
    initBPTree("test_index.dat");
    printf("✓ Reopened B+ tree from disk\n");
    
    // Verify some keys still exist
    assert(readData(42, read_data) == 1);
    assert(verify_data(read_data, 42));
    printf("✓ Key 42 persisted\n");
    
    assert(readData(5000, read_data) == 1);
    assert(verify_data(read_data, 5000));
    printf("✓ Key 5000 persisted\n");
    
    assert(readData(99999, read_data) == 1);
    assert(verify_data(read_data, 99999));
    printf("✓ Key 99999 persisted\n");
}

// Test 8: Sequential vs Random Access
void test_access_patterns() {
    printf("\n=== Test 8: Access Patterns ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    clock_t start, end;
    
    // Sequential reads
    printf("Performing 10000 sequential reads...\n");
    start = clock();
    for (int i = 0; i < 10000; i++) {
        assert(readData(i, read_data) == 1);
    }
    end = clock();
    double seq_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("✓ Sequential reads: %.3f seconds (%.0f ops/sec)\n", 
           seq_time, 10000 / seq_time);
    
    // Random reads
    printf("Performing 10000 random reads...\n");
    start = clock();
    for (int i = 0; i < 10000; i++) {
        int key = rand() % LARGE_TEST_SIZE;
        assert(readData(key, read_data) == 1);
    }
    end = clock();
    double rand_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("✓ Random reads: %.3f seconds (%.0f ops/sec)\n", 
           rand_time, 10000 / rand_time);
}

// Test 9: Edge Cases
void test_edge_cases() {
    printf("\n=== Test 9: Edge Cases ===\n");
    
    unsigned char data[TEST_DATA_SIZE];
    unsigned char read_data[TEST_DATA_SIZE];
    
    // Insert key 0
    generate_random_data(data, 0);
    assert(writeData(0, data) == 1);
    assert(readData(0, read_data) == 1);
    printf("✓ Key 0 works\n");
    
    // Insert negative key
    generate_random_data(data, -100);
    assert(writeData(-100, data) == 1);
    assert(readData(-100, read_data) == 1);
    printf("✓ Negative key works\n");
    
    // Insert INT_MAX
    generate_random_data(data, 2147483647);
    assert(writeData(2147483647, data) == 1);
    assert(readData(2147483647, read_data) == 1);
    printf("✓ INT_MAX key works\n");
    
    // Range query with reversed bounds
    int n;
    unsigned char** results = readRangeData(100, 50, &n);
    printf("✓ Reversed range query handled (n=%d)\n", n);
    if (results) freeRangeData(results, n);
}

int main() {
    printf("========================================\n");
    printf("  B+ Tree Index Test Driver\n");
    printf("========================================\n");
    
    // Initialize B+ tree
    initBPTree("test_index.dat");
    printf("✓ Initialized B+ tree with file: test_index.dat\n");
    
    srand(time(NULL));
    
    // Run all tests
    test_basic_insert_read();
    test_multiple_inserts();
    test_update();
    test_delete();
    test_range_query();
    test_large_dataset();
    test_persistence();
    test_access_patterns();
    test_edge_cases();
    
    // Cleanup
    closeBPTree();
    
    printf("\n========================================\n");
    printf("  All Tests Passed! ✓\n");
    printf("========================================\n");
    
    return 0;
}
