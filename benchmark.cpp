#include "bptree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE 100
#define NUM_TESTS 10000

void generate_data(unsigned char* data, int key) {
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = (key + i) % 256;
    }
}

int verify_data(unsigned char* data, int key) {
    for (int i = 0; i < DATA_SIZE; i++) {
        if (data[i] != (unsigned char)((key + i) % 256)) {
            return 0;
        }
    }
    return 1;
}

void print_metrics(const char* operation, int count, double seconds) {
    printf("  %-25s: %8d ops in %8.3f sec = %10.0f ops/sec\n",
           operation, count, seconds, count / seconds);
}

int main() {
    printf("\n");
    printf("========================================================================\n");
    printf("  B+ TREE INDEX - COMPREHENSIVE PERFORMANCE METRICS\n");
    printf("========================================================================\n\n");
    
    // Initialize
    printf("[1] INITIALIZATION\n");
    clock_t start = clock();
    initBPTree("benchmark.dat");
    double init_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("  Tree initialized in %.3f seconds\n\n", init_time);
    
    unsigned char data[DATA_SIZE];
    unsigned char read_buf[DATA_SIZE];
    
    // Test 1: Sequential Inserts
    printf("[2] SEQUENTIAL INSERT PERFORMANCE\n");
    start = clock();
    int insert_count = 0;
    for (int i = 0; i < NUM_TESTS; i++) {
        generate_data(data, i);
        if (writeData(i, data)) {
            insert_count++;
        }
    }
    double insert_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    print_metrics("Sequential inserts", insert_count, insert_time);
    printf("\n");
    
    // Test 2: Sequential Reads
    printf("[3] SEQUENTIAL READ PERFORMANCE\n");
    start = clock();
    int read_count = 0;
    for (int i = 0; i < NUM_TESTS; i++) {
        if (readData(i, read_buf)) {
            if (verify_data(read_buf, i)) {
                read_count++;
            }
        }
    }
    double seq_read_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    print_metrics("Sequential reads", read_count, seq_read_time);
    printf("  Data verification: %s\n\n", 
           (read_count == insert_count) ? "PASS" : "FAIL");
    
    // Test 3: Random Reads
    printf("[4] RANDOM READ PERFORMANCE\n");
    srand(42);
    start = clock();
    int random_read_count = 0;
    for (int i = 0; i < NUM_TESTS; i++) {
        int key = rand() % NUM_TESTS;
        if (readData(key, read_buf)) {
            if (verify_data(read_buf, key)) {
                random_read_count++;
            }
        }
    }
    double rand_read_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    print_metrics("Random reads", random_read_count, rand_read_time);
    printf("\n");
    
    // Test 4: Update Operations
    printf("[5] UPDATE PERFORMANCE\n");
    start = clock();
    int update_count = 0;
    for (int i = 0; i < 1000; i++) {
        memset(data, i % 256, DATA_SIZE);
        if (writeData(i, data)) {
            update_count++;
        }
    }
    double update_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    print_metrics("Updates", update_count, update_time);
    printf("\n");
    
    // Test 5: Range Queries
    printf("[6] RANGE QUERY PERFORMANCE\n");
    
    // Small range
    start = clock();
    int n1;
    unsigned char** results1 = readRangeData(1000, 1100, &n1);
    double range1_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("  Range [1000-1100]        : %4d results in %.3f sec\n", n1, range1_time);
    if (results1) freeRangeData(results1, n1);
    
    // Medium range
    start = clock();
    int n2;
    unsigned char** results2 = readRangeData(2000, 3000, &n2);
    double range2_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("  Range [2000-3000]        : %4d results in %.3f sec\n", n2, range2_time);
    if (results2) freeRangeData(results2, n2);
    
    // Large range
    start = clock();
    int n3;
    unsigned char** results3 = readRangeData(0, 5000, &n3);
    double range3_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("  Range [0-5000]           : %4d results in %.3f sec\n", n3, range3_time);
    if (results3) freeRangeData(results3, n3);
    printf("\n");
    
    // Test 6: Delete Operations
    printf("[7] DELETE PERFORMANCE\n");
    start = clock();
    int delete_count = 0;
    for (int i = 5000; i < 6000; i++) {
        if (deleteData(i)) {
            delete_count++;
        }
    }
    double delete_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    print_metrics("Deletes", delete_count, delete_time);
    
    // Verify deletions
    int verify_delete = 0;
    for (int i = 5000; i < 6000; i++) {
        if (!readData(i, read_buf)) {
            verify_delete++;
        }
    }
    printf("  Deletion verification: %s (%d/%d)\n\n", 
           (verify_delete == delete_count) ? "PASS" : "FAIL",
           verify_delete, delete_count);
    
    // Test 7: Mixed Workload
    printf("[8] MIXED WORKLOAD PERFORMANCE\n");
    start = clock();
    int mixed_ops = 0;
    for (int i = 0; i < 1000; i++) {
        // Insert
        generate_data(data, 20000 + i);
        if (writeData(20000 + i, data)) mixed_ops++;
        
        // Read
        if (readData(i, read_buf)) mixed_ops++;
        
        // Update
        if (writeData(i, data)) mixed_ops++;
        
        // Range query (every 100 ops)
        if (i % 100 == 0) {
            int n;
            unsigned char** r = readRangeData(i, i + 50, &n);
            if (r) {
                mixed_ops += n;
                freeRangeData(r, n);
            }
        }
    }
    double mixed_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    print_metrics("Mixed operations", mixed_ops, mixed_time);
    printf("\n");
    
    // Test 8: Persistence Test
    printf("[9] PERSISTENCE TEST\n");
    closeBPTree();
    printf("  Database closed\n");
    
    start = clock();
    initBPTree("benchmark.dat");
    double reopen_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("  Database reopened in %.3f seconds\n", reopen_time);
    
    // Verify data persisted
    int persist_verify = 0;
    for (int i = 0; i < 100; i++) {
        if (readData(i, read_buf)) {
            persist_verify++;
        }
    }
    printf("  Persistence verification: %s (%d/100 keys found)\n\n",
           (persist_verify == 100) ? "PASS" : "FAIL", persist_verify);
    
    // Overall Summary
    printf("========================================================================\n");
    printf("  OVERALL PERFORMANCE SUMMARY\n");
    printf("========================================================================\n\n");
    
    double total_time = init_time + insert_time + seq_read_time + rand_read_time +
                       update_time + range1_time + range2_time + range3_time +
                       delete_time + mixed_time + reopen_time;
    
    printf("  Total Operations         : %d\n", 
           insert_count + read_count + random_read_count + update_count + 
           delete_count + mixed_ops);
    printf("  Total Time               : %.3f seconds\n", total_time);
    printf("  Average Throughput       : %.0f ops/sec\n\n",
           (insert_count + read_count + random_read_count + update_count + 
            delete_count + mixed_ops) / total_time);
    
    printf("  BREAKDOWN BY OPERATION:\n");
    printf("  ----------------------\n");
    printf("  Initialization           : %.3f sec\n", init_time);
    printf("  Insert operations        : %.3f sec (%.0f ops/sec)\n", 
           insert_time, insert_count / insert_time);
    printf("  Sequential reads         : %.3f sec (%.0f ops/sec)\n",
           seq_read_time, read_count / seq_read_time);
    printf("  Random reads             : %.3f sec (%.0f ops/sec)\n",
           rand_read_time, random_read_count / rand_read_time);
    printf("  Updates                  : %.3f sec (%.0f ops/sec)\n",
           update_time, update_count / update_time);
    printf("  Range queries            : %.3f sec\n",
           range1_time + range2_time + range3_time);
    printf("  Delete operations        : %.3f sec (%.0f ops/sec)\n",
           delete_time, delete_count / delete_time);
    printf("  Mixed workload           : %.3f sec (%.0f ops/sec)\n",
           mixed_time, mixed_ops / mixed_time);
    printf("  Reopen database          : %.3f sec\n\n", reopen_time);
    
    closeBPTree();
    
    printf("========================================================================\n");
    printf("  TEST COMPLETED SUCCESSFULLY\n");
    printf("========================================================================\n\n");
    
    return 0;
}
