# Makefile for B+ Tree Index Implementation
# DBMS Assignment - Semester 5

# Python interpreter
PYTHON = python3

# Main files
DRIVER = driver.py
BPTREE = bplustree.py

# Test output
TEST_OUTPUT = test_results.txt

# Default target
.DEFAULT_GOAL := help

# PHONY targets (not actual files)
.PHONY: help run test clean all check

##@ General

help: ## Display this help message
	@echo "B+ Tree Index Implementation - Makefile"
	@echo "========================================"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Available targets:"
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf ""} /^[a-zA-Z_-]+:.*?##/ { printf "  %-15s %s\n", $$1, $$2 } /^##@/ { printf "\n%s\n", substr($$0, 5) } ' $(MAKEFILE_LIST)
	@echo ""

##@ Running

run: check ## Run the driver program (default)
	@echo "Running B+ Tree driver program..."
	@echo "=================================="
	@$(PYTHON) $(DRIVER)

test: check ## Run tests and save output to file
	@echo "Running comprehensive tests..."
	@echo "Output will be saved to $(TEST_OUTPUT)"
	@$(PYTHON) $(DRIVER) | tee $(TEST_OUTPUT)
	@echo ""
	@echo "Test results saved to $(TEST_OUTPUT)"

##@ Development

check: ## Check if Python and required files exist
	@command -v $(PYTHON) >/dev/null 2>&1 || { echo "Error: Python 3 not found. Please install Python 3.6+"; exit 1; }
	@test -f $(BPTREE) || { echo "Error: $(BPTREE) not found"; exit 1; }
	@test -f $(DRIVER) || { echo "Error: $(DRIVER) not found"; exit 1; }
	@chmod +x $(DRIVER) 2>/dev/null || true
	@echo "Environment check passed ✓"

validate: check ## Validate Python syntax
	@echo "Validating Python syntax..."
	@$(PYTHON) -m py_compile $(BPTREE)
	@$(PYTHON) -m py_compile $(DRIVER)
	@echo "Syntax validation passed ✓"

##@ Cleanup

clean: ## Remove all generated files
	@echo "Cleaning up generated files..."
	@rm -f *.idx
	@rm -f *.pyc
	@rm -f __pycache__/*.pyc
	@rmdir __pycache__ 2>/dev/null || true
	@rm -f $(TEST_OUTPUT)
	@echo "Cleanup complete ✓"

clean-test: ## Remove only test index files
	@echo "Removing test index files..."
	@rm -f test_*.idx
	@rm -f benchmark.idx
	@echo "Test files removed ✓"

##@ Documentation

docs: ## Display API documentation path
	@echo "API Documentation available in:"
	@echo "  - README.md    : Project overview and usage"
	@echo "  - API_DOCS.md  : Detailed API reference (man-style)"

show-readme: ## Display README.md
	@cat README.md

show-api: ## Display API documentation
	@cat API_DOCS.md

##@ All-in-one

all: clean validate run ## Clean, validate, and run

# Installation check
install-check: ## Verify installation requirements
	@echo "Checking Python installation..."
	@$(PYTHON) --version
	@echo ""
	@echo "Checking Python modules..."
	@$(PYTHON) -c "import os, mmap, struct; print('Required modules available ✓')"
	@echo ""
	@echo "Installation check complete ✓"
