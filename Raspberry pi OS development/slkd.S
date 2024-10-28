#include "uart.h"
#include "utils.h"
#include "printf.h"
#include "atomic.h"
#include "stdint.h"
#include "percpu.h"
#include "spinlock.h"
#include "entry.h"

Spinlock lock;

#define PACKED __attribute__((__packed__))

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
    // printf("index: %d\n", index);
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


void mmu_init() {
    uint64_t i, r;

    uint64_t base_address = (uint64_t)&ttlb_lvl1[0] | 0x3;
    ttbr = (uint64_t*)base_address;

    // Set up Level 0 table to point to Level 1 table
    // MMU_CFG.ttlb_lvl0[0] = (uint64_t)(level1_addr) | 0x3;
    for (i = 0x0; i < 0x3F000000; i += 2097152) {
        map_page(ttlb_lvl1, i, i, 0x711); // Normal memory
    }
    
    for (i = 0x3F000000; i < 0x3FE00000 + 2097152; i += 2097152) {
        map_page(ttlb_lvl1, i, i, 0x701); // Device memory
    }

    uint64_t mair = (0xFFULL << 0) | (0x04ULL << 8);
    asm volatile("msr mair_el1, %0" : : "r" (mair));

    // Configure TCR_EL1
    uint64_t tcr = (25ULL << 0)  | // T0SZ = 16 (48-bit address space)
                   (0ULL << 14)  | // TG0 = 4KB
                   (3ULL << 32)  | // Inner Shareable
                   (1ULL << 8)   | // IRGN0 = Normal, WB, RA, WA
                   (1ULL << 10);   // ORGN0 = Normal, WB, RA, WA
    asm volatile("msr tcr_el1, %0" : : "r" (tcr));

    // Set TTBR0_EL1 to point to Level 0 table
    // ttlb_base = (uint64_t *)(&MMU_CFG.ttlb_lvl0[0]);
    asm volatile("msr ttbr0_el1, %0" : : "r"(ttbr));

    // Enable MMU and caches in SCTLR_EL1
    asm volatile("msr sctlr_el1, xzr");
    asm volatile("dsb sy; isb; mrs %0, sctlr_el1" : "=r" (r));
    r |= 1; // Enable MMU
    r |= (1 << 2); // Enable data cache
    r |= (1 << 12); // Enable instruction cache
    asm volatile("msr sctlr_el1, %0; isb" : : "r"(r));

    // Allow MMU to settle
    asm volatile("nop; nop");
}

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

extern "C" void kernel_init() {
    unsigned int r;
    uart_init();

    uart_puts("UART initialized!!\n\r");
    init_printf(0x0, uart_putc_wrapper);

    mmu_init(); // Initialize MMU
    

    uint64_t sctlr_el1, ttbr0_el1, ttbr1_el1, tcr_el1, mair_el1, current_el;

    asm volatile("mrs %0, SCTLR_EL1" : "=r"(sctlr_el1));
    asm volatile("mrs %0, TTBR0_EL1" : "=r"(ttbr0_el1));
    asm volatile("mrs %0, TTBR1_EL1" : "=r"(ttbr1_el1));
    asm volatile("mrs %0, TCR_EL1" : "=r"(tcr_el1));
    asm volatile("mrs %0, MAIR_EL1" : "=r"(mair_el1));
    asm volatile("mrs %0, CurrentEL" : "=r"(current_el));

    printf("SCTLR_EL1: ");
    print_binary(sctlr_el1);
    uart_putc('\r');

    printf("TTBR0_EL1: ");
    print_binary(ttbr0_el1);
    uart_putc('\r');

    printf("TTBR1_EL1: ");
    print_binary(ttbr1_el1);
    uart_putc('\r');

    printf("TCR_EL1: ");
    print_binary(tcr_el1);
    uart_putc('\r');

    printf("MAIR_EL1: ");
    print_binary(mair_el1);
    uart_putc('\r');

    printf("CurrentEL: ");
    print_binary(current_el);
    uart_putc('\r');

    print_ttlb_lvl1_mappings();

    test_null_mapping(); // Test for null mapping

    printf("ALIGN CHECK\n");

    struct align_check1 ac1;
    memcpy(&ac1, buffer, 7);
    printf("UNPACKED: A: %X, B: %X, C: %X, D: %X\n", ac1.a, ac1.b, ac1.c, ac1.d);

    struct align_check2 ac2;
    memcpy(&ac2, buffer, 7);
    printf("PACKED: A: %X, B: %X, C: %X, D: %X\n", ac2.a, ac2.b, ac2.c, ac2.d);

    printf("Exception level %d\n", get_el());
    printf("Core ID: %d\n", getCoreID());

    // test_atomic_operations();
    test_stxr_ldxr_operations();
    // lock.unlock();
}
