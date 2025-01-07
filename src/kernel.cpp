#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "atomic.h"
#include "stdint.h"
#include "percpu.h"
#include "spinlock.h"
#include "entry.h"
#include "mmu.h"

Spinlock lock;

#define PACKED __attribute__((__packed__))

#define PAGE_SIZE 0x1000  // 4KB pages
#define DEVICE_FLAGS 0x704
#define NORMAL_FLAGS 0x705  // Normal memory, Write-Back Cacheable

struct Stack {
    static constexpr int BYTES = 4096;
    uint64_t bytes[BYTES] __attribute__ ((aligned(16)));
};

PerCPU<Stack> stacks;

uint64_t *ttbr;
uint64_t __attribute__((aligned(4096))) ttlb_lvl1[512];

static bool smpInitDone = false;

extern "C" uint64_t pickKernelStack(void) {
    return (uint64_t) &stacks.forCPU(smpInitDone ? getCoreID() : 0).bytes[Stack::BYTES];
}

void uart_putc_wrapper(void* p, char c) {
    (void)p; // Unused parameter
    if (c == '\n') {
        uart_putc('\r');
    }
    uart_putc(c);
}

Atomic<int> atomicCounter(0);

void test_stxr_ldxr_operations() {
    int success;
    int val = 0;
    int result;

    printf("Testing LDXR and STXR operations\n");

    // Load the value from the address into result, setting the exclusive monitor
    asm volatile("ldxr %w0, [%1]" : "=&r" (result) : "r" (&val));

    // Attempt to store a new value, if the exclusive monitor is set (i.e., no other cores modified the address)
    int new_val = 10;
    asm volatile("stxr %w0, %w2, [%1]" : "=&r" (success) : "r" (&val), "r" (new_val) : "memory");

    if (success == 0) {
        printf("STXR successful, value updated to: %d\n", val);
    } else {
        printf("STXR failed, value not updated\n");
    }

    // Checking the final value
    printf("Final value after STXR: %d\n", val);
}

void map_page(uint64_t *ttlb_lvl1, uint64_t va, uint64_t pa, uint64_t flags) {
    uint64_t index = (va >> 21) & 0x1FF;
    ttlb_lvl1[index] = (pa & 0xFFFFFFFFFFFFF000) | flags;
}

void unmap_page(uint64_t *ttlb_lvl1, uint64_t va) {
    // Calculate the index in the translation table
    uint64_t index = (va >> 21) & 0x01FF;
    
    // Clear the entry in the translation table
    ttlb_lvl1[index] = 0;

    // Invalidate the TLB entry for the virtual address
    asm volatile("dsb ishst; tlbi vaae1is, %0; dsb ish; isb" :: "r" (va >> 12));
}

void print_translation_table(uint64_t *ttlb_lvl1) {
    for (int i = 0; i < 512; i++) {
        if (ttlb_lvl1[i] != 0) {
            uint64_t pa = ttlb_lvl1[i] & 0xFFFFFFFFFFFFF000;
            uint64_t attrs = ttlb_lvl1[i] & 0xFFF;
            printf("TTLB Level 1 [%d]: PA = 0x%X, Attributes = 0x%X\n", i, pa, attrs);
        }
    }
}

void clear_caches() {
    // Invalidate entire data cache
    asm volatile ("dc ivac, x0" : : "r" (0));
    // Invalidate entire instruction cache
    asm volatile ("ic iallu");
    // Ensure completion of cache maintenance
    asm volatile ("dsb sy");
    // Synchronize instructions
    asm volatile ("isb");
}


// void mmu_init() {
//     uint64_t i, r;

//     // Clear the translation table
//     for (i = 0; i < 512; i++) {
//         ttlb_lvl1[i] = 0;
//     }

//     // Map normal memory regions
//     for (i = 0x0; i < 0x3FFFFFFF- PAGE_SIZE; i += PAGE_SIZE) {
//         map_page(ttlb_lvl1, i, i, NORMAL_FLAGS); // Normal memory
//     }

//     // Map device memory regions
//     for (i = 0x3F000000; i < 0x40000000; i += PAGE_SIZE) {
//         map_page(ttlb_lvl1, i, i, DEVICE_FLAGS); // Device memory
//     }

//     // Set Memory Attribute Indirection Register (MAIR_EL1)
//     uint64_t mair = (0xFF << 0) | (0x04 << 8);
//     asm volatile("msr mair_el1, %0" : : "r" (mair));
//     asm volatile("isb");

//     // Set Translation Control Register (TCR_EL1)
//     uint64_t tcr = (32ULL << 0)  | // T0SZ = 25 (32-bit address space)
//                    (1ULL << 10)  | // ORGN0 = Normal, WB, RA, WA
//                    (1ULL << 8)   | // IRGN0 = Normal, WB, RA, WA
//                    (3ULL << 12)  | // Inner Shareable
//                    (1ULL << 30);   // TG0 = 4KB (page granule)
//     asm volatile("msr tcr_el1, %0" : : "r" (tcr));

//     // Set Translation Table Base Register 0 (TTBR0_EL1)
//     asm volatile("msr ttbr0_el1, %0" : : "r" (ttlb_lvl1));

//     // Enable the MMU and caches in System Control Register (SCTLR_EL1)
//     asm volatile("mrs %0, sctlr_el1" : "=r" (r));
//     r |= 1;         // Enable MMU
//     r |= (1 << 2);  // Enable data cache
//     r |= (1 << 12); // Enable instruction cache
//     asm volatile("msr sctlr_el1, %0; isb" : : "r"(r));

//     // Print the translation table for debugging
//     print_translation_table(ttlb_lvl1);
// }

void print_binary(uint64_t value) {
    for (int i = 63; i >= 0; i--) {
        uart_putc((value & (1ULL << i)) ? '1' : '0');
        if (i % 4 == 0) { // Optional: Add space every 4 bits for readability
            uart_putc(' ');
        }
    }
    uart_putc('\n');
}

struct align_check1 {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint32_t d;
};

struct align_check2 {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint32_t d;
} PACKED;

uint8_t buffer[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70};

void *memcpy(void *dest, const void *src, uint64_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    for (uint64_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

void print_ttlb_lvl1_mappings() {
    printf("TTLB Level 1 Mappings:\n");
    for (int i = 0; i < 512; i++) {
        uint64_t entry = ttlb_lvl1[i];
        uint64_t address = (entry >> 21) << 21; // Extract the physical address
        printf("Entry %d: Address = 0x%X\n", i, address);
    }
}

void test_null_mapping() {
    // Check if the 0x0 address is not mapped
    uint64_t null_address = 0x0;
    uint64_t index = (null_address >> 21) & 0x01FF;
    uint64_t entry = ttlb_lvl1[index];

    if (entry != 0) {
        printf("Error: 0x0 address is mapped in the TTLB Level 1.\n");
    } else {
        printf("Success: 0x0 address is not mapped in the TTLB Level 1.\n");
    }
}

void test_atomic_operations_iso() {
    uint32_t *addr = (uint32_t *)0x8AF84;
    uint32_t result;

    // Load exclusive
    asm volatile("ldaxr %w0, [%1]" : "=&r"(result) : "r"(addr));

    // Store exclusive
    uint32_t new_val = 0xA;
    uint32_t success;
    asm volatile("stlxr %w0, %w1, [%2]" : "=&r"(success) : "r"(new_val), "r"(addr));

    printf("Value at 0x8AF84: 0x%X, stlxr result: %d\n", result, success);
}


void test_atomic_operations(void) {
    // Define variables
    int val = 0;
    int result;
    int new_val = 10;

    printf("Initial value of val: %d\n", val);

    // Perform atomic exchange
    result = __atomic_exchange_n(&val, new_val, __ATOMIC_SEQ_CST);

    printf("Value after atomic exchange: %d, old value: %d\n", val, result);

    // Check if the atomic exchange was successful
    if (val == new_val) {
        printf("Atomic exchange successful, value updated to: %d\n", val);
    } else {
        printf("Atomic exchange failed, value not updated. Current val: %d\n", val);
    }

    printf("Final value of val: %d\n", val);
}

void write_nops(uint64_t target_address, int count) {
    if (target_address % 4 != 0) {
        printf("Error: target_address (0x%X) is not aligned to 4 bytes.\n", target_address);
        return;
    }

    if (count <= 0) {
        printf("Error: Invalid count value (%d).\n", count);
        return;
    }

    asm volatile(
        // Initialize loop counter and base address
        "mov x0, %[address]        \n" // x0 = target_address (64-bit register)
        "mov x1, %[count]          \n" // x1 = count (loop counter, 64-bit register)

        // Load NOP instruction into w2 (32-bit)
        "movz w2, #0x201F          \n" // Load high 16 bits (0xD503)
        "movk w2, #0xD503, lsl #16 \n" // Load low 16 bits (0x201F) into w2

        // Loop start
        "1:                        \n"
        "ldxr w2, [x0]          \n" // Store NOP and increment address (x0 += 4)
        "subs x1, x1, #1           \n" // Decrement loop counter (x1 -= 1)
        "bne 1b                    \n" // Branch to loop start if x1 != 0

        // Memory barrier to ensure writes are visible
        "dsb ish                   \n"
        "isb                       \n"

        :                          // No output operands
        : [address] "r"(target_address), [count] "r"(count) // Input operands
        : "x0", "x1", "w2", "memory" // Clobbers: registers and memory
    );
}






void ensure_address_mapped(uint64_t *ttlb_lvl1, uint64_t va) {
    uint64_t pa = va & 0xFFFFFFFFFFFFF000; // Align physical address to 4KB page
    uint64_t flags = NORMAL_FLAGS;         // Normal memory attributes
    map_page(ttlb_lvl1, va, pa, flags);

    // Invalidate the TLB to reflect new mappings
    asm volatile("dsb ishst; tlbi vae1is, %0; dsb ish; isb" : : "r"(va >> 12));
}

void print_memory_value(uint64_t address) {
    uint32_t *memory_location = (uint32_t *)address; // Cast to a pointer to 32-bit data
    printf("Value at 0x%X: 0x%X\n", address, *memory_location);
}


Spinlock spin;
extern "C" void kernel_init() {

    uart_init();

    // uart_puts("UART initialized!!\n\r");
    init_printf(0x0, uart_putc_wrapper);

    // mmu_init(); // Initialize MMU
    MMU_setup_pagetable();
    MMU_enable();


    // uint64_t sctlr_el1, ttbr0_el1, ttbr1_el1, tcr_el1, mair_el1, current_el;

    // asm volatile("mrs %0, SCTLR_EL1" : "=r"(sctlr_el1));
    // asm volatile("mrs %0, TTBR0_EL1" : "=r"(ttbr0_el1));
    // asm volatile("mrs %0, TTBR1_EL1" : "=r"(ttbr1_el1));
    // asm volatile("mrs %0, TCR_EL1" : "=r"(tcr_el1));
    // asm volatile("mrs %0, MAIR_EL1" : "=r"(mair_el1));
    // asm volatile("mrs %0, CurrentEL" : "=r"(current_el));

    // printf("SCTLR_EL1: 0x%X\n", sctlr_el1);
    // print_binary(sctlr_el1);
    // printf("\r");

    // printf("TTBR0_EL1: 0x%X\n", ttbr0_el1);
    // print_binary(ttbr0_el1);
    // printf("\r");

    // printf("TTBR1_EL1: 0x%X\n", ttbr0_el1);
    // print_binary(ttbr1_el1);
    // printf("\r");

    // printf("TCR_EL1: 0x%X\n", tcr_el1);
    // print_binary(tcr_el1);
    // printf("\r");

    // printf("MAIR_EL1: 0x%X\n", mair_el1);
    // print_binary(mair_el1);
    // printf("\r");

    // printf("CurrentEL: 0x%X\n", current_el >> 2);
    // print_binary(current_el);
    // printf("\r");

    // printf("ALIGN CHECK\n");

    // struct align_check1 ac1;
    // memcpy(&ac1, buffer, 7);
    // printf("UNPACKED: A: %X, B: %X, C: %X, D: %X\n", ac1.a, ac1.b, ac1.c, ac1.d);

    // struct align_check2 ac2;
    // memcpy(&ac2, buffer, 7);
    // printf("PACKED: A: %X, B: %X, C: %X, D: %X\n", ac2.a, ac2.b, ac2.c, ac2.d);

    // printf("Exception level %d\n", get_el());

    // write_nops(0x8AF84, 5);
    // print_memory_value(0x8AF84);
    


    // Test atomic operations
    // test_atomic_operations_iso();
    spin.lock();
    printf("Core ID: %d\n", getCoreID());
    // test_atomic_operations();
    spin.unlock();
}