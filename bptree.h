#ifndef BPTREE_H
#define BPTREE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the B+ tree index with a file
 * @param filename: path to the index file
 */
void initBPTree(const char* filename);

/**
 * Close and cleanup the B+ tree
 */
void closeBPTree(void);

/**
 * Write data to the B+ tree index
 * @param key: integer key for indexing
 * @param data: 100-byte data array
 * @return 1 if successful, 0 otherwise
 */
int writeData(int key, const unsigned char* data);

/**
 * Delete data from the B+ tree index
 * @param key: integer key to delete
 * @return 1 if successful, 0 otherwise
 */
int deleteData(int key);

/**
 * Read data from the B+ tree index
 * @param key: integer key to search
 * @param data_out: output buffer for 100-byte data (must be pre-allocated)
 * @return 1 if found, 0 otherwise
 */
int readData(int key, unsigned char* data_out);

/**
 * Read range of data from the B+ tree index
 * @param lowerKey: lower bound of range (inclusive)
 * @param upperKey: upper bound of range (inclusive)
 * @param n: pointer to store the number of results
 * @return array of 100-byte data arrays, NULL if no results
 * Note: Caller must free the returned array using freeRangeData()
 */
unsigned char** readRangeData(int lowerKey, int upperKey, int* n);

/**
 * Free memory allocated by readRangeData
 * @param data: array returned by readRangeData
 * @param n: number of entries in the array
 */
void freeRangeData(unsigned char** data, int n);

#ifdef __cplusplus
}
#endif

#endif // BPTREE_H
