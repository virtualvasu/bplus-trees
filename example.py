#!/usr/bin/env python3
"""
Simple Example - B+ Tree Index Usage

This is a minimal example showing how to use the B+ tree index.
Perfect for quick testing and learning the API.
"""

from bplustree import BPlusTree
import os

def main():
    # Clean up any existing index
    if os.path.exists("example.idx"):
        os.remove("example.idx")
    
    print("="*60)
    print(" B+ Tree Index - Simple Example")
    print("="*60)
    
    # Create/Open index
    print("\n1. Creating new B+ tree index...")
    tree = BPlusTree("example.idx")
    print("   ✓ Index created: example.idx")
    
    # Insert some data
    print("\n2. Inserting records...")
    records = [
        (1, b"Alice - Computer Science - GPA: 3.8"),
        (2, b"Bob - Electrical Engineering - GPA: 3.6"),
        (3, b"Carol - Mechanical Engineering - GPA: 3.9"),
        (4, b"David - Mathematics - GPA: 3.7"),
        (5, b"Eve - Physics - GPA: 3.5"),
    ]
    
    for key, data in records:
        success = tree.writeData(key, data)
        status = "✓" if success else "✗"
        print(f"   {status} Inserted key={key}")
    
    # Read individual records
    print("\n3. Reading individual records...")
    for key in [1, 3, 5]:
        data = tree.readData(key)
        if data:
            print(f"   Key {key}: {data[:50].decode('utf-8', errors='ignore')}...")
        else:
            print(f"   Key {key}: NOT FOUND")
    
    # Update a record
    print("\n4. Updating a record...")
    updated_data = b"Bob - EE - GPA: 3.8 (Updated!)"
    tree.writeData(2, updated_data)
    data = tree.readData(2)
    print(f"   Updated key=2: {data[:40].decode('utf-8', errors='ignore')}...")
    
    # Range query
    print("\n5. Range query [2, 4]...")
    data_list, count = tree.readRangeData(2, 4)
    print(f"   Found {count} records:")
    for i, data in enumerate(data_list, 1):
        print(f"     {i}. {data[:45].decode('utf-8', errors='ignore')}...")
    
    # Delete a record
    print("\n6. Deleting a record...")
    success = tree.deleteData(3)
    print(f"   {'✓' if success else '✗'} Deleted key=3")
    
    # Verify deletion
    data = tree.readData(3)
    print(f"   Verify: key=3 exists = {data is not None} (should be False)")
    
    # Close the index
    print("\n7. Closing index...")
    tree.close()
    print("   ✓ Index closed")
    
    # Test persistence
    print("\n8. Testing persistence (reopening index)...")
    tree2 = BPlusTree("example.idx")
    
    remaining_keys = [1, 2, 4, 5]  # Key 3 was deleted
    verified = 0
    for key in remaining_keys:
        if tree2.readData(key) is not None:
            verified += 1
    
    print(f"   Verified {verified}/{len(remaining_keys)} records persisted")
    tree2.close()
    
    # Cleanup
    print("\n9. Cleaning up...")
    os.remove("example.idx")
    print("   ✓ Removed example.idx")
    
    print("\n" + "="*60)
    print(" Example completed successfully!")
    print("="*60)

if __name__ == "__main__":
    main()
