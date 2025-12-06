"""
B+ Tree Index Implementation for Database Systems

This module implements a disk-based B+ tree index with the following specifications:
- Page size: 4096 bytes
- Key type: Integer (4 bytes)
- Data size: 100 bytes per tuple
- Uses memory-mapped I/O for efficient disk access
- Supports insertion, deletion, point queries, and range queries
"""

import os
import mmap
import struct
from typing import Optional, List, Tuple
import bisect

# Constants
PAGE_SIZE = 4096
KEY_SIZE = 4  # Integer key
DATA_SIZE = 100  # Fixed size data tuple
HEADER_SIZE = 16  # Page header: is_leaf(1) + num_keys(4) + parent_page(4) + next_leaf(4) + reserved(3)

# Calculate order of B+ tree based on page size
# For internal nodes: each entry is (key, child_pointer) = 4 + 4 = 8 bytes
# For leaf nodes: each entry is (key, data) = 4 + 100 = 104 bytes
# Internal node: HEADER_SIZE + (order * 8) + 4 (extra child pointer) <= PAGE_SIZE
# Leaf node: HEADER_SIZE + (order * 104) <= PAGE_SIZE

INTERNAL_ORDER = (PAGE_SIZE - HEADER_SIZE - 4) // 8  # ~509
LEAF_ORDER = (PAGE_SIZE - HEADER_SIZE) // (KEY_SIZE + DATA_SIZE)  # ~39


class BPlusTreeNode:
    """Represents a node in the B+ tree (either internal or leaf)"""
    
    def __init__(self, is_leaf: bool = True, page_num: int = 0):
        self.is_leaf = is_leaf
        self.page_num = page_num
        self.keys = []
        self.children = []  # For internal nodes: page numbers; For leaf nodes: data bytes
        self.parent_page = -1  # -1 means no parent (root)
        self.next_leaf = -1  # For leaf nodes: points to next leaf (-1 if last)
    
    def serialize(self) -> bytes:
        """Serialize node to 4096 bytes for disk storage"""
        buffer = bytearray(PAGE_SIZE)
        
        # Header
        buffer[0] = 1 if self.is_leaf else 0
        struct.pack_into('<I', buffer, 1, len(self.keys))
        struct.pack_into('<i', buffer, 5, self.parent_page)
        struct.pack_into('<i', buffer, 9, self.next_leaf)
        
        offset = HEADER_SIZE
        
        if self.is_leaf:
            # Leaf node: store (key, data) pairs
            for i in range(len(self.keys)):
                struct.pack_into('<i', buffer, offset, self.keys[i])
                offset += KEY_SIZE
                buffer[offset:offset + DATA_SIZE] = self.children[i][:DATA_SIZE]
                offset += DATA_SIZE
        else:
            # Internal node: store keys and child page numbers
            # Bulk pack keys (faster than loop)
            if len(self.keys) > 0:
                fmt = '<' + 'i' * len(self.keys)
                struct.pack_into(fmt, buffer, offset, *self.keys)
                offset += KEY_SIZE * len(self.keys)
            
            # Bulk pack child pointers
            if len(self.children) > 0:
                fmt = '<' + 'i' * len(self.children)
                struct.pack_into(fmt, buffer, offset, *self.children)
        
        return bytes(buffer)
    
    @staticmethod
    def deserialize(data: bytes, page_num: int) -> 'BPlusTreeNode':
        """Deserialize 4096 bytes from disk to create a node"""
        is_leaf = data[0] == 1
        num_keys = struct.unpack_from('<I', data, 1)[0]
        parent_page = struct.unpack_from('<i', data, 5)[0]
        next_leaf = struct.unpack_from('<i', data, 9)[0]
        
        node = BPlusTreeNode(is_leaf, page_num)
        node.parent_page = parent_page
        node.next_leaf = next_leaf
        
        offset = HEADER_SIZE
        
        if is_leaf:
            # Pre-allocate lists for faster appending
            node.keys = [0] * num_keys
            node.children = [None] * num_keys
            
            # Read (key, data) pairs - optimized
            for i in range(num_keys):
                node.keys[i] = struct.unpack_from('<i', data, offset)[0]
                offset += KEY_SIZE
                node.children[i] = data[offset:offset + DATA_SIZE]
                offset += DATA_SIZE
        else:
            # Pre-allocate lists
            node.keys = [0] * num_keys
            node.children = [0] * (num_keys + 1)
            
            # Bulk unpack keys (much faster than loop)
            if num_keys > 0:
                fmt = '<' + 'i' * num_keys
                node.keys = list(struct.unpack_from(fmt, data, offset))
                offset += KEY_SIZE * num_keys
            
            # Bulk unpack child pointers
            if num_keys >= 0:
                num_children = num_keys + 1
                fmt = '<' + 'i' * num_children
                node.children = list(struct.unpack_from(fmt, data, offset))
        
        return node


class BPlusTree:
    """B+ Tree implementation with disk persistence"""
    
    def __init__(self, filename: str = "bptree.idx"):
        self.filename = filename
        self.file = None
        self.mmap = None
        self.root_page = 0
        self.next_page = 1
        
        # Open or create the index file
        if os.path.exists(filename) and os.path.getsize(filename) > 0:
            self._open_existing()
        else:
            self._create_new()
    
    def _create_new(self):
        """Create a new B+ tree index file"""
        # Create file with initial metadata page
        with open(self.filename, 'wb') as f:
            # Metadata page (page 0)
            metadata = bytearray(PAGE_SIZE)
            struct.pack_into('<I', metadata, 0, 1)  # root_page = 1
            struct.pack_into('<I', metadata, 4, 2)  # next_page = 2 (page 1 is root)
            f.write(metadata)
            
            # Root page (page 1) - initially an empty leaf
            root = BPlusTreeNode(is_leaf=True, page_num=1)
            f.write(root.serialize())
        
        self.root_page = 1
        self.next_page = 2
        
        # Open with memory mapping
        self.file = open(self.filename, 'r+b')
        self.mmap = mmap.mmap(self.file.fileno(), 0)
    
    def _open_existing(self):
        """Open existing B+ tree index file"""
        self.file = open(self.filename, 'r+b')
        self.mmap = mmap.mmap(self.file.fileno(), 0)
        
        # Read metadata
        self.root_page = struct.unpack_from('<I', self.mmap, 0)[0]
        self.next_page = struct.unpack_from('<I', self.mmap, 4)[0]
    
    def _write_metadata(self):
        """Write metadata to page 0"""
        struct.pack_into('<I', self.mmap, 0, self.root_page)
        struct.pack_into('<I', self.mmap, 4, self.next_page)
        # Don't flush on every metadata write - let OS handle it
    
    def _read_page(self, page_num: int) -> BPlusTreeNode:
        """Read a page from disk"""
        offset = page_num * PAGE_SIZE
        needed_size = (page_num + 1) * PAGE_SIZE
        
        # Extend mmap if needed using resize (much faster)
        if offset >= len(self.mmap):
            self.mmap.resize(needed_size)
        
        data = self.mmap[offset:offset + PAGE_SIZE]
        return BPlusTreeNode.deserialize(data, page_num)
    
    def _write_page(self, node: BPlusTreeNode):
        """Write a page to disk"""
        offset = node.page_num * PAGE_SIZE
        needed_size = (node.page_num + 1) * PAGE_SIZE
        
        # Extend file if needed using resize (much faster)
        if offset >= len(self.mmap):
            self.mmap.resize(needed_size)
        
        serialized = node.serialize()
        self.mmap[offset:offset + PAGE_SIZE] = serialized
        # Don't flush on every write - let OS batch writes
    
    def _allocate_page(self) -> int:
        """Allocate a new page and return its page number"""
        page_num = self.next_page
        self.next_page += 1
        self._write_metadata()
        return page_num
    
    def _find_leaf(self, key: int) -> BPlusTreeNode:
        """Find the leaf node that should contain the key"""
        node = self._read_page(self.root_page)
        
        while not node.is_leaf:
            # Binary search for better performance on large nodes
            keys = node.keys
            left, right = 0, len(keys)
            while left < right:
                mid = (left + right) // 2
                if key < keys[mid]:
                    right = mid
                else:
                    left = mid + 1
            
            child_page = node.children[left]
            node = self._read_page(child_page)
        
        return node
    
    def _insert_in_leaf(self, node: BPlusTreeNode, key: int, data: bytes) -> Optional[Tuple[int, BPlusTreeNode]]:
        """Insert key-data pair in leaf node. Returns (split_key, new_node) if split occurs"""
        # Use binary search to find insertion position (faster)
        i = bisect.bisect_left(node.keys, key)
        
        # Check if key already exists
        if i < len(node.keys) and node.keys[i] == key:
            # Update existing key
            node.children[i] = data[:DATA_SIZE]
            self._write_page(node)
            return None
        
        # Insert new key-data pair
        node.keys.insert(i, key)
        node.children.insert(i, data[:DATA_SIZE])
        
        # Check if split is needed
        if len(node.keys) <= LEAF_ORDER:
            self._write_page(node)
            return None
        
        # Split the leaf node
        mid = len(node.keys) // 2
        new_node = BPlusTreeNode(is_leaf=True, page_num=self._allocate_page())
        
        # Move half of keys to new node
        new_node.keys = node.keys[mid:]
        new_node.children = node.children[mid:]
        node.keys = node.keys[:mid]
        node.children = node.children[:mid]
        
        # Update leaf pointers
        new_node.next_leaf = node.next_leaf
        node.next_leaf = new_node.page_num
        new_node.parent_page = node.parent_page
        
        # Write both nodes
        self._write_page(node)
        self._write_page(new_node)
        
        return (new_node.keys[0], new_node)
    
    def _insert_in_parent(self, left_node: BPlusTreeNode, key: int, right_node: BPlusTreeNode):
        """Insert key in parent after a split"""
        if left_node.parent_page == -1:
            # Create new root
            new_root = BPlusTreeNode(is_leaf=False, page_num=self._allocate_page())
            new_root.keys = [key]
            new_root.children = [left_node.page_num, right_node.page_num]
            
            left_node.parent_page = new_root.page_num
            right_node.parent_page = new_root.page_num
            
            self.root_page = new_root.page_num
            self._write_metadata()
            self._write_page(new_root)
            self._write_page(left_node)
            self._write_page(right_node)
            return
        
        # Insert in existing parent
        parent = self._read_page(left_node.parent_page)
        
        # Use binary search to find insertion position
        i = bisect.bisect_left(parent.keys, key)
        
        parent.keys.insert(i, key)
        parent.children.insert(i + 1, right_node.page_num)
        right_node.parent_page = parent.page_num
        
        # Check if parent needs to split
        if len(parent.keys) <= INTERNAL_ORDER:
            self._write_page(parent)
            self._write_page(right_node)
            return
        
        # Split internal node
        mid = len(parent.keys) // 2
        split_key = parent.keys[mid]
        
        new_parent = BPlusTreeNode(is_leaf=False, page_num=self._allocate_page())
        new_parent.keys = parent.keys[mid + 1:]
        new_parent.children = parent.children[mid + 1:]
        parent.keys = parent.keys[:mid]
        parent.children = parent.children[:mid + 1]
        
        # Update children's parent pointers
        for child_page in new_parent.children:
            child = self._read_page(child_page)
            child.parent_page = new_parent.page_num
            self._write_page(child)
        
        new_parent.parent_page = parent.parent_page
        
        self._write_page(parent)
        self._write_page(new_parent)
        self._write_page(right_node)
        
        # Recursively insert in parent
        self._insert_in_parent(parent, split_key, new_parent)
    
    def writeData(self, key: int, data: bytes) -> bool:
        """Insert key-data pair into the B+ tree"""
        try:
            # Ensure data is exactly 100 bytes
            if len(data) < DATA_SIZE:
                data = data + b'\x00' * (DATA_SIZE - len(data))
            elif len(data) > DATA_SIZE:
                data = data[:DATA_SIZE]
            
            leaf = self._find_leaf(key)
            result = self._insert_in_leaf(leaf, key, data)
            
            if result is not None:
                split_key, new_node = result
                self._insert_in_parent(leaf, split_key, new_node)
            
            return True
        except Exception as e:
            print(f"Error in writeData: {e}")
            return False
    
    def readData(self, key: int) -> Optional[bytes]:
        """Search for a key and return its data"""
        try:
            leaf = self._find_leaf(key)
            
            # Binary search in leaf
            for i in range(len(leaf.keys)):
                if leaf.keys[i] == key:
                    return leaf.children[i]
            
            return None
        except Exception as e:
            print(f"Error in readData: {e}")
            return None
    
    def readRangeData(self, lower_key: int, upper_key: int) -> Tuple[Optional[List[bytes]], int]:
        """Search for all keys in range [lower_key, upper_key] and return their data"""
        try:
            result = []
            
            # Find the leaf containing lower_key
            leaf = self._find_leaf(lower_key)
            
            # Traverse leaf nodes
            while leaf is not None:
                for i in range(len(leaf.keys)):
                    if lower_key <= leaf.keys[i] <= upper_key:
                        result.append(leaf.children[i])
                    elif leaf.keys[i] > upper_key:
                        return (result, len(result)) if result else (None, 0)
                
                # Move to next leaf
                if leaf.next_leaf == -1:
                    break
                leaf = self._read_page(leaf.next_leaf)
            
            return (result, len(result)) if result else (None, 0)
        except Exception as e:
            print(f"Error in readRangeData: {e}")
            return (None, 0)
    
    def _find_key_in_parent(self, parent: BPlusTreeNode, child_page: int) -> int:
        """Find the index of child_page in parent's children"""
        for i in range(len(parent.children)):
            if parent.children[i] == child_page:
                return i
        return -1
    
    def deleteData(self, key: int) -> bool:
        """Delete a key from the B+ tree"""
        try:
            leaf = self._find_leaf(key)
            
            # Find the key in leaf
            key_index = -1
            for i in range(len(leaf.keys)):
                if leaf.keys[i] == key:
                    key_index = i
                    break
            
            if key_index == -1:
                return False  # Key not found
            
            # Remove the key
            del leaf.keys[key_index]
            del leaf.children[key_index]
            
            # If leaf is root or has enough keys, just write it
            if leaf.page_num == self.root_page:
                self._write_page(leaf)
                return True
            
            min_keys = (LEAF_ORDER + 1) // 2
            
            if len(leaf.keys) >= min_keys:
                self._write_page(leaf)
                return True
            
            # Need to handle underflow
            self._handle_underflow(leaf)
            return True
            
        except Exception as e:
            print(f"Error in deleteData: {e}")
            return False
    
    def _handle_underflow(self, node: BPlusTreeNode):
        """Handle underflow in a node (merge or redistribute)"""
        if node.parent_page == -1:
            # Root node
            if len(node.keys) == 0 and not node.is_leaf and len(node.children) == 1:
                # Make the only child the new root
                self.root_page = node.children[0]
                self._write_metadata()
                child = self._read_page(self.root_page)
                child.parent_page = -1
                self._write_page(child)
            else:
                self._write_page(node)
            return
        
        parent = self._read_page(node.parent_page)
        node_index = self._find_key_in_parent(parent, node.page_num)
        
        # Try to borrow from left sibling
        if node_index > 0:
            left_sibling = self._read_page(parent.children[node_index - 1])
            min_keys = (LEAF_ORDER + 1) // 2 if node.is_leaf else (INTERNAL_ORDER + 1) // 2
            
            if len(left_sibling.keys) > min_keys:
                # Borrow from left
                if node.is_leaf:
                    node.keys.insert(0, left_sibling.keys[-1])
                    node.children.insert(0, left_sibling.children[-1])
                    left_sibling.keys.pop()
                    left_sibling.children.pop()
                    parent.keys[node_index - 1] = node.keys[0]
                else:
                    node.keys.insert(0, parent.keys[node_index - 1])
                    parent.keys[node_index - 1] = left_sibling.keys[-1]
                    node.children.insert(0, left_sibling.children[-1])
                    left_sibling.keys.pop()
                    left_sibling.children.pop()
                    
                    # Update child's parent pointer
                    child = self._read_page(node.children[0])
                    child.parent_page = node.page_num
                    self._write_page(child)
                
                self._write_page(node)
                self._write_page(left_sibling)
                self._write_page(parent)
                return
        
        # Try to borrow from right sibling
        if node_index < len(parent.children) - 1:
            right_sibling = self._read_page(parent.children[node_index + 1])
            min_keys = (LEAF_ORDER + 1) // 2 if node.is_leaf else (INTERNAL_ORDER + 1) // 2
            
            if len(right_sibling.keys) > min_keys:
                # Borrow from right
                if node.is_leaf:
                    node.keys.append(right_sibling.keys[0])
                    node.children.append(right_sibling.children[0])
                    right_sibling.keys.pop(0)
                    right_sibling.children.pop(0)
                    parent.keys[node_index] = right_sibling.keys[0]
                else:
                    node.keys.append(parent.keys[node_index])
                    parent.keys[node_index] = right_sibling.keys[0]
                    node.children.append(right_sibling.children[0])
                    right_sibling.keys.pop(0)
                    right_sibling.children.pop(0)
                    
                    # Update child's parent pointer
                    child = self._read_page(node.children[-1])
                    child.parent_page = node.page_num
                    self._write_page(child)
                
                self._write_page(node)
                self._write_page(right_sibling)
                self._write_page(parent)
                return
        
        # Merge with sibling
        if node_index > 0:
            # Merge with left sibling
            left_sibling = self._read_page(parent.children[node_index - 1])
            
            if node.is_leaf:
                left_sibling.keys.extend(node.keys)
                left_sibling.children.extend(node.children)
                left_sibling.next_leaf = node.next_leaf
            else:
                left_sibling.keys.append(parent.keys[node_index - 1])
                left_sibling.keys.extend(node.keys)
                left_sibling.children.extend(node.children)
                
                # Update children's parent pointers
                for child_page in node.children:
                    child = self._read_page(child_page)
                    child.parent_page = left_sibling.page_num
                    self._write_page(child)
            
            parent.keys.pop(node_index - 1)
            parent.children.pop(node_index)
            
            self._write_page(left_sibling)
            self._write_page(parent)
            
            # Check if parent underflows
            min_keys = (INTERNAL_ORDER + 1) // 2
            if parent.page_num != self.root_page and len(parent.keys) < min_keys:
                self._handle_underflow(parent)
            elif parent.page_num == self.root_page:
                if len(parent.keys) == 0 and len(parent.children) == 1:
                    self.root_page = parent.children[0]
                    self._write_metadata()
                    child = self._read_page(self.root_page)
                    child.parent_page = -1
                    self._write_page(child)
                else:
                    self._write_page(parent)
        else:
            # Merge with right sibling
            right_sibling = self._read_page(parent.children[node_index + 1])
            
            if node.is_leaf:
                node.keys.extend(right_sibling.keys)
                node.children.extend(right_sibling.children)
                node.next_leaf = right_sibling.next_leaf
            else:
                node.keys.append(parent.keys[node_index])
                node.keys.extend(right_sibling.keys)
                node.children.extend(right_sibling.children)
                
                # Update children's parent pointers
                for child_page in right_sibling.children:
                    child = self._read_page(child_page)
                    child.parent_page = node.page_num
                    self._write_page(child)
            
            parent.keys.pop(node_index)
            parent.children.pop(node_index + 1)
            
            self._write_page(node)
            self._write_page(parent)
            
            # Check if parent underflows
            min_keys = (INTERNAL_ORDER + 1) // 2
            if parent.page_num != self.root_page and len(parent.keys) < min_keys:
                self._handle_underflow(parent)
            elif parent.page_num == self.root_page:
                if len(parent.keys) == 0 and len(parent.children) == 1:
                    self.root_page = parent.children[0]
                    self._write_metadata()
                    child = self._read_page(self.root_page)
                    child.parent_page = -1
                    self._write_page(child)
                else:
                    self._write_page(parent)
    
    def close(self):
        """Close the index file"""
        try:
            if self.mmap is not None:
                self.mmap.flush()  # Flush all writes on close
                self.mmap.close()
                self.mmap = None
        except:
            pass
        
        try:
            if self.file is not None:
                self.file.close()
                self.file = None
        except:
            pass
    
    def __del__(self):
        """Destructor to ensure file is closed"""
        try:
            self.close()
        except:
            pass
