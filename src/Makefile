#==============================================================================
# Cross compiler and tools
#==============================================================================
CROSS_COMPILE = aarch64-linux-gnu-
AS      = $(CROSS_COMPILE)as
CC      = $(CROSS_COMPILE)gcc
CXX     = $(CROSS_COMPILE)g++
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

#==============================================================================
# Directories
#==============================================================================
BUILD_DIR   = ../build
SRC_DIR     = .
INCLUDE_DIR = ../include
TEST_DIR    = ../tests

#==============================================================================
# Source files (Separate normal kernel sources vs. test sources)
#==============================================================================
ASM_SRC       = $(wildcard $(SRC_DIR)/*.S)
C_SRC         = $(wildcard $(SRC_DIR)/*.c)
KERNEL_CPP_SRC= $(wildcard $(SRC_DIR)/*.cpp)   # kernel-related .cpp in src/
TEST_CPP_SRC  = $(wildcard $(TEST_DIR)/*.cpp)  # test .cpp in tests/

#==============================================================================
# Object files
#==============================================================================
ASM_OBJ       = $(ASM_SRC:$(SRC_DIR)/%.S=$(BUILD_DIR)/%.o)
C_OBJ         = $(C_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
KERNEL_CPP_OBJ= $(KERNEL_CPP_SRC:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJ      = $(TEST_CPP_SRC:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/%.o)

#==============================================================================
# Output
#==============================================================================
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_IMG = $(BUILD_DIR)/kernel8.img

#==============================================================================
# Flags
#==============================================================================
CFLAGS  = -Wall -Wextra -nostdlib -ffreestanding -I$(INCLUDE_DIR) -g \
          -mcpu=cortex-a53 -march=armv8-a+crc -mstrict-align \
          -mno-outline-atomics -fpermissive \
          -fno-exceptions -fno-rtti

# Linker script is one level above (../linker.ld)
LDFLAGS = -T ../linker.ld -Wl,-Map=$(BUILD_DIR)/kernel.map

#==============================================================================
# Default build: normal kernel (no test objects)
#==============================================================================
all: $(KERNEL_IMG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

#------------------------------------------------------------------------------
# 1) Preprocess & assemble .S files
#------------------------------------------------------------------------------
$(ASM_OBJ): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(CC) -E -x assembler-with-cpp -I$(INCLUDE_DIR) $< -o $(BUILD_DIR)/$(<F).i
	$(AS) $(BUILD_DIR)/$(<F).i -o $@
	rm $(BUILD_DIR)/$(<F).i

#------------------------------------------------------------------------------
# 2) Compile .c sources
#------------------------------------------------------------------------------
$(C_OBJ): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

#------------------------------------------------------------------------------
# 3) Compile kernel .cpp sources
#------------------------------------------------------------------------------
$(KERNEL_CPP_OBJ): $(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

#------------------------------------------------------------------------------
# 4) Link normal kernel
#------------------------------------------------------------------------------
$(KERNEL_ELF): $(ASM_OBJ) $(C_OBJ) $(KERNEL_CPP_OBJ)
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(ASM_OBJ) $(C_OBJ) $(KERNEL_CPP_OBJ)

$(KERNEL_IMG): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_IMG)

#------------------------------------------------------------------------------
# 5) Pattern rule to build "tX.img" (one test)
#    Example usage: make t1.img
#------------------------------------------------------------------------------
# This rule says: to build build/t1.img,
# we need (ASM_OBJ + C_OBJ + KERNEL_CPP_OBJ) + build/t1.o.
# Then we link them into build/t1.elf, then objcopy -> build/t1.img.
#------------------------------------------------------------------------------
# Builds e.g. build/t0.img
$(BUILD_DIR)/%.img: $(BUILD_DIR)/%.o $(ASM_OBJ) $(C_OBJ) $(KERNEL_CPP_OBJ)
	@echo "Linking test image for $*"
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$*.elf \
	$(ASM_OBJ) $(C_OBJ) $(KERNEL_CPP_OBJ) $(BUILD_DIR)/$*.o
	$(OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $(BUILD_DIR)/$*.img

#------------------------------------------------------------------------------
# 6) Compile the test .cpp => build/tX.o
#------------------------------------------------------------------------------
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

#------------------------------------------------------------------------------
# 7) Clean
#------------------------------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)
	find $(TEST_DIR) -type f \( ! -name '*.cpp' -a ! -name '*.ok' \) -exec rm -f {} +

#------------------------------------------------------------------------------
# 8) QEMU run/debug
#------------------------------------------------------------------------------
run: all
	qemu-system-aarch64 -M raspi3b -kernel ../build/t0.img -smp 4 -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3b -kernel $(KERNEL_IMG) -smp 4 -serial stdio \
	                    -S -gdb tcp::1234

#------------------------------------------------------------------------------
# 9) "tests" target: build and run every test .o
#    Loops over each test object, links into a custom image, runs QEMU
#------------------------------------------------------------------------------
tests: all $(TEST_OBJ)
	@for testobj in $(TEST_OBJ); do \
		testname=$${testobj##*/}; \
		testbase=$${testname%.o}; \
		echo "---------------------------------------"; \
		echo "Linking and running test: $$testbase"; \
		$(LD) $(LDFLAGS) -o $(BUILD_DIR)/$$testbase.elf \
		    $(ASM_OBJ) $(C_OBJ) $(KERNEL_CPP_OBJ) $$testobj; \
		$(OBJCOPY) -O binary $(BUILD_DIR)/$$testbase.elf $(BUILD_DIR)/$$testbase.img; \
		echo "Launching QEMU for $$testbase.img ..."; \
		qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/$$testbase.img \
		    -smp 4 -serial stdio -nographic; \
	done

.PHONY: all clean run debug tests