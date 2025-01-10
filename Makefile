SHELL := /bin/bash

# 1) "all": build the kernel in src/ and all test images
all: build-tests

# 2) Identify tests by scanning tests/*.cpp
TESTS_DIR  ?= tests
TESTS_CPP  := $(wildcard $(TESTS_DIR)/*.cpp)
TEST_NAMES := $(notdir $(basename $(TESTS_CPP)))

# 3) "build-tests": Build all test images before running tests
build-tests:
	@for t in $(TEST_NAMES); do \
		echo "=========================================================="; \
		echo "Building test: $$t"; \
		$(MAKE) -C src ../build/$$t.img; \
	done

# 4) "test": Run QEMU for each test image, capturing output and comparing results
test: all
	@for t in $(TEST_NAMES); do \
		echo "=========================================================="; \
		echo "Running test: $$t"; \
		echo "Running QEMU, capturing output to '$(TESTS_DIR)/$$t.out'..."; \
		rm -f $(TESTS_DIR)/$$t.out; \
		qemu-system-aarch64 \
		    -M raspi3b \
		    -kernel build/$$t.img \
		    -smp 4 \
		    -serial file:$(TESTS_DIR)/$$t.out \
		    -nographic \
		    -no-reboot \
		    -no-shutdown || true; \
		\
		if [ -f "$(TESTS_DIR)/$$t.ok" ]; then \
			echo "Comparing output..."; \
			if diff -q $(TESTS_DIR)/$$t.out $(TESTS_DIR)/$$t.ok >/dev/null 2>&1; then \
				echo "$$t: pass"; \
			else \
				echo "$$t: fail"; \
				echo "Differences found, saving to $(TESTS_DIR)/$$t.diff"; \
				diff $(TESTS_DIR)/$$t.ok $(TESTS_DIR)/$$t.out > $(TESTS_DIR)/$$t.diff; \
			fi; \
		else \
			echo "Warning: No $(TESTS_DIR)/$$t.ok file found."; \
		fi; \
	done


# 5) "run": Run the normal kernel (already built by "all") in QEMU
run: all
	qemu-system-aarch64 \
	    -M raspi3b \
	    -kernel build/kernel8.img \
	    -smp 4 \
	    -serial stdio \
	    -nographic

# 6) Clean: Remove all build files and test outputs except source files
clean:
	@$(MAKE) -C src clean

.PHONY: all test clean run build-tests