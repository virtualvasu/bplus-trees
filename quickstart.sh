#!/bin/bash
# Quick Start Script for B+ Tree Index
# Run this to verify everything is working

echo "╔════════════════════════════════════════════════════════════╗"
echo "║       B+ Tree Index - Quick Start Verification            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check Python
echo "1. Checking Python installation..."
if command -v python3 &> /dev/null; then
    python3 --version
    echo "   ✓ Python 3 found"
else
    echo "   ✗ Python 3 not found. Please install Python 3.6+"
    exit 1
fi
echo ""

# Validate syntax
echo "2. Validating Python syntax..."
python3 -m py_compile bplustree.py driver.py example.py
if [ $? -eq 0 ]; then
    echo "   ✓ All Python files are valid"
else
    echo "   ✗ Syntax errors found"
    exit 1
fi
echo ""

# Run simple example
echo "3. Running simple example..."
python3 example.py > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✓ Example ran successfully"
else
    echo "   ✗ Example failed"
    exit 1
fi
echo ""

# Quick API test
echo "4. Running quick API test..."
python3 << 'EOF' > /dev/null 2>&1
from bplustree import BPlusTree
import os
if os.path.exists('quicktest.idx'):
    os.remove('quicktest.idx')
tree = BPlusTree('quicktest.idx')
assert tree.writeData(1, b'test') == True
assert tree.readData(1) is not None
assert tree.deleteData(1) == True
assert tree.readData(1) is None
data_list, count = tree.readRangeData(1, 10)
assert count == 0
tree.close()
os.remove('quicktest.idx')
EOF

if [ $? -eq 0 ]; then
    echo "   ✓ All API functions work correctly"
else
    echo "   ✗ API test failed"
    exit 1
fi
echo ""

# Check documentation
echo "5. Checking documentation files..."
files=("README.md" "API_DOCS.md" "Makefile" "requirements.txt")
all_exist=true
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "   ✓ $file"
    else
        echo "   ✗ $file missing"
        all_exist=false
    fi
done
echo ""

if [ "$all_exist" = true ]; then
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║            ALL CHECKS PASSED - READY TO USE! ✓            ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Next steps:"
    echo "  • Run comprehensive tests:  make test"
    echo "  • Run driver program:       make run"
    echo "  • Run simple example:       python3 example.py"
    echo "  • View help:                make help"
    echo "  • Read documentation:       cat README.md"
    echo ""
else
    echo "Some files are missing. Please check the installation."
    exit 1
fi
