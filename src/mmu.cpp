#include "stdint.h"         // for uint64_t
#include "mmu.h"
#include "printf.h"

//------------------------------------------------------------------------------
//            ARCHITECTURE-SPECIFIC DEFINES
//------------------------------------------------------------------------------
#if __aarch64__ == 1
/* AARCH64 */
/* We have 2Mb blocks, so we need 2 of 512 entries  */
/* Covers 2GB which is enough for the 1GB + QA7     */
#define NUM_PAGE_TABLE_ENTRIES 512
/* Each Block is 2Mb in size */
#define LEVEL1_BLOCKSIZE (1 << 21)
/* LEVEL1 TABLE ALIGNMENT 4K */
#define TLB_ALIGNMENT 4096
/* LEVEL2 TABLE ALIGNMENT 4K */
#define TLB2_ALIGNMENT 4096
#else
/* AARCH32 */
/* We have 1MB blocks, with minimum 4096 entries    */
/* Covers 4GB which is more than 1GB + QA7 we need  */
#define NUM_PAGE_TABLE_ENTRIES 4096
/* Each Block is 1Mb in size */
#define LEVEL1_BLOCKSIZE (1 << 20)
/* LEVEL1 TABLE ALIGNMENT 16K */
#define TLB_ALIGNMENT 16384
/* LEVEL2 TABLE ALIGNMENT 1K */
#define TLB2_ALIGNMENT 1024
#endif

//------------------------------------------------------------------------------
//             GLOBAL PAGE-TABLE ARRAYS (exactly as in C code)
//------------------------------------------------------------------------------
/* First Level Page Table for 1:1 mapping */
static RegType_t __attribute__((aligned(TLB_ALIGNMENT))) page_table_map1to1[NUM_PAGE_TABLE_ENTRIES] = { 0 };

/* First Level Page Table for virtual mapping */
static RegType_t __attribute__((aligned(TLB_ALIGNMENT))) page_table_virtualmap[NUM_PAGE_TABLE_ENTRIES] = { 0 };

/* First Level Page Table for virtual mapping */
static RegType_t __attribute__((aligned(TLB2_ALIGNMENT))) Stage2virtual[512] = { 0 };

//------------------------------------------------------------------------------
//    NAMED ENUMS (instead of anonymous) SO WE CAN REFERENCE IN C++
//------------------------------------------------------------------------------
enum S2AP_Enum : uint8_t {
    STAGE2_S2AP_NOREAD_EL0 = 1,  // No read access for EL0
    STAGE2_S2AP_NO_WRITE    = 2, // No write access
};

enum SH_Enum : uint8_t {
    STAGE2_SH_OUTER_SHAREABLE = 2, // Outer shareable
    STAGE2_SH_INNER_SHAREABLE = 3, // Inner shareable
};

enum APTable_Enum : uint8_t {
    APTABLE_NOEFFECT          = 0, // No effect
    APTABLE_NO_EL0            = 1, // Access at EL0 not permitted
    APTABLE_NO_WRITE          = 2, // Write access not permitted
    APTABLE_NO_WRITE_EL0_READ = 3  // Write no, read no at EL0
};

// We’ll treat EntryType similarly. 1 => block, 3 => page.
enum EntryType_Enum : uint8_t {
    ENTRYTYPE_BLOCK = 1,
    ENTRYTYPE_PAGE  = 3
};

//------------------------------------------------------------------------------
//        THE UNION WITH BITFIELDS, KEEPING SAME LAYOUT & FIELD ORDER
//------------------------------------------------------------------------------
typedef union {
    struct {
        // [0-1]: 1 for block descriptor, 3 for a page descriptor
        uint64_t EntryType : 2;         
        // [2-5]
        uint64_t MemAttr   : 4;         
        // [6-7]
        S2AP_Enum S2AP     : 2;         
        // [8-9]
        SH_Enum SH         : 2;         
        // [10]
        uint64_t AF        : 1;         // Access flag
        // [11]
        uint64_t _reserved11 : 1;       
        // [12-47]: 36 bits of address
        uint64_t Address   : 36;        
        // [48-51]
        uint64_t _reserved48_51 : 4;    
        // [52]
        uint64_t Contiguous : 1;        
        // [53]
        uint64_t _reserved53 : 1;       
        // [54]
        uint64_t XN        : 1;         
        // [55-58]
        uint64_t _reserved55_58 : 4;    
        // [59]
        uint64_t PXNTable  : 1;         
        // [60]
        uint64_t XNTable   : 1;         
        // [61-62]
        APTable_Enum APTable : 2;       
        // [63]
        uint64_t NSTable   : 1;         
    };
    // Full 64-bit direct access
    uint64_t Raw64;
} VMSAv8_64_DESCRIPTOR;

// Verify it’s the same size as RegType_t (64 bits, presumably)

//------------------------------------------------------------------------------
//             STAGE-2 AND STAGE-3 DESCRIPTOR TABLES
//              EXACTLY LIKE ORIGINAL C CODE
//------------------------------------------------------------------------------
/* 1 to 1 mapping: 1024 entries x 2MB = 2GB coverage */
static VMSAv8_64_DESCRIPTOR __attribute__((aligned(TLB_ALIGNMENT)))
    Stage2map1to1[1024] = { 0 };

/* Basic single table of 512 descriptors for final stage3 (virtual) */
static VMSAv8_64_DESCRIPTOR __attribute__((aligned(TLB_ALIGNMENT)))
    Stage3virtual[512] = { 0 };

//------------------------------------------------------------------------------
//                 MMU_setup_pagetable (exact same logic)
//------------------------------------------------------------------------------

void MMU_setup_pagetable(void)
{
    uint32_t base;
    uint32_t msg[5] = { 0 };

    // Retrieve VC memory size
    if (mailbox_tag_message(msg, 5, MAILBOX_TAG_GET_VC_MEMORY, 8, 8, 0, 0))
    {
        // msg[3] has VC base addr; msg[4] = VC memory size
        // Convert VC4 memory base address to block count
        msg[3] /= LEVEL1_BLOCKSIZE;  
    }

    //--------------------------------------------------------------------------
    // 1) Initialize 1:1 mapping in Stage2map1to1
    //    EXACT same logic, but we must match field order in designators!
    //--------------------------------------------------------------------------
    // Field order in the union is: 
    // 1) EntryType, 2) MemAttr, 3) S2AP, 4) SH, 5) AF, 6) _reserved11,
    // 7) Address, 8) _reserved48_51, 9) Contiguous, 10) _reserved53,
    // 11) XN, 12) _reserved55_58, 13) PXNTable, 14) XNTable, 15) APTable, 16) NSTable

    // (a) Ram from 0x0 to start of VC4 RAM
    for (base = 0; base < msg[3]; base++)
    {
        Stage2map1to1[base] = (VMSAv8_64_DESCRIPTOR){
            .EntryType  = ENTRYTYPE_BLOCK,         // was 1
            .MemAttr    = MT_NORMAL,               // "normal"
            .S2AP       = (S2AP_Enum)0,            // not used in original? default 0
            .SH         = STAGE2_SH_INNER_SHAREABLE,
            .AF         = 1,
            ._reserved11 = 0,
            .Address    = static_cast<uint64_t>(base) << (21 - 12),
            ._reserved48_51 = 0,
            .Contiguous = 0,
            ._reserved53 = 0,
            .XN         = 0,
            ._reserved55_58 = 0,
            .PXNTable   = 0,
            .XNTable    = 0,
            .APTable    = (APTable_Enum)0,
            .NSTable    = 0
        };
        VMSAv8_64_DESCRIPTOR thing = (VMSAv8_64_DESCRIPTOR){
            .EntryType  = ENTRYTYPE_BLOCK,         // was 1
            .MemAttr    = MT_NORMAL,               // "normal"
            .S2AP       = (S2AP_Enum)0,            // not used in original? default 0
            .SH         = STAGE2_SH_INNER_SHAREABLE,
            .AF         = 1,
            ._reserved11 = 0,
            .Address    = static_cast<uint64_t>(base) << (21 - 12),
            ._reserved48_51 = 0,
            .Contiguous = 0,
            ._reserved53 = 0,
            .XN         = 0,
            ._reserved55_58 = 0,
            .PXNTable   = 0,
            .XNTable    = 0,
            .APTable    = (APTable_Enum)0,
            .NSTable    = 0
        };
        // printf("thing: 0x%X\n", thing);
    }

    // (b) VC ram up to 0x3F000000
    for (; base < 512 - 8; base++)
    {
        Stage2map1to1[base] = (VMSAv8_64_DESCRIPTOR){
            .EntryType  = ENTRYTYPE_BLOCK,
            .MemAttr    = MT_NORMAL_NC,  // was MT_NORMAL_NC
            .S2AP       = (S2AP_Enum)0,
            .SH         = (SH_Enum)0,   // not used in original, default
            .AF         = 1,
            ._reserved11 = 0,
            .Address    = static_cast<uint64_t>(base) << (21 - 12),
            ._reserved48_51 = 0,
            .Contiguous = 0,
            ._reserved53 = 0,
            .XN         = 0,
            ._reserved55_58 = 0,
            .PXNTable   = 0,
            .XNTable    = 0,
            .APTable    = (APTable_Enum)0,
            .NSTable    = 0
        };
        // printf("thing:2 0x%X\n", Stage2map1to1[base]);
    }

    //(c) 16 MB peripherals at 0x3F000000 - 0x40000000
    for (; base < 512; base++)
    {
        Stage2map1to1[base] = (VMSAv8_64_DESCRIPTOR){
            .EntryType  = ENTRYTYPE_BLOCK,
            .MemAttr    = MT_DEVICE_NGNRNE, // device
            .S2AP       = (S2AP_Enum)0,
            .SH         = (SH_Enum)0,      // not used
            .AF         = 1,
            ._reserved11 = 0,
            .Address    = static_cast<uint64_t>(base) << (21 - 12),
            ._reserved48_51 = 0,
            .Contiguous = 0,
            ._reserved53 = 0,
            .XN         = 0,
            ._reserved55_58 = 0,
            .PXNTable   = 0,
            .XNTable    = 0,
            .APTable    = (APTable_Enum)0,
            .NSTable    = 0
        };
                VMSAv8_64_DESCRIPTOR thing = (VMSAv8_64_DESCRIPTOR){
            .EntryType  = ENTRYTYPE_BLOCK,         // was 1
            .MemAttr    = MT_NORMAL,               // "normal"
            .S2AP       = (S2AP_Enum)0,            // not used in original? default 0
            .SH         = STAGE2_SH_INNER_SHAREABLE,
            .AF         = 1,
            ._reserved11 = 0,
            .Address    = static_cast<uint64_t>(base) << (21 - 12),
            ._reserved48_51 = 0,
            .Contiguous = 0,
            ._reserved53 = 0,
            .XN         = 0,
            ._reserved55_58 = 0,
            .PXNTable   = 0,
            .XNTable    = 0,
            .APTable    = (APTable_Enum)0,
            .NSTable    = 0
        };
        // printf("thing:3 0x%X\n", thing);
    }

    // (d) 2 MB for mailboxes at 0x40000000 (shared device, never execute)
    Stage2map1to1[512] = (VMSAv8_64_DESCRIPTOR){
        .EntryType  = ENTRYTYPE_BLOCK,
        .MemAttr    = MT_DEVICE_NGNRNE,
        .S2AP       = (S2AP_Enum)0,
        .SH         = (SH_Enum)0,
        .AF         = 1,
        ._reserved11 = 0,
        .Address    = static_cast<uint64_t>(512) << (21 - 12),
        ._reserved48_51 = 0,
        .Contiguous = 0,
        ._reserved53 = 0,
        .XN         = 0,
        ._reserved55_58 = 0,
        .PXNTable   = 0,
        .XNTable    = 0,
        .APTable    = (APTable_Enum)0,
        .NSTable    = 0
    };

    //--------------------------------------------------------------------------
    // 2) Level 1: two valid entries mapping each 1GB in stage2 => 2GB total
    //    EXACT same code except for casting to 64-bit
    //--------------------------------------------------------------------------
    page_table_map1to1[0] =
        0x8000000000000000ULL
        | static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&Stage2map1to1[0]))
        | 3;

    page_table_map1to1[1] =
        0x8000000000000000ULL
        | static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&Stage2map1to1[512]))
        | 3;

    //--------------------------------------------------------------------------
    // 3) Initialize virtual mapping for TTBR1
    //--------------------------------------------------------------------------
    // Stage2virtual[511] has 1 valid entry pointing to Stage3virtual[0]
    // (Originally was commented out or partially used)
    Stage2virtual[511] =
        0x8000000000000000ULL
        | static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&Stage3virtual[0]))
        | 3;

    // The top-level "page_table_virtualmap" has 1 valid entry pointing to Stage2virtual[0]
    page_table_virtualmap[511] =
        0x8000000000000000ULL
        | static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&Stage2virtual[0]))
        | 3;
}

//------------------------------------------------------------------------------
//                 MMU_enable (unchanged logic)
//------------------------------------------------------------------------------
void MMU_enable(void)
{
    // Each core calls this with the same tables
    enable_mmu_tables(&page_table_map1to1[0], &page_table_virtualmap[0]);
}

#if __aarch64__ == 1
// empty or additional code for AArch64...
#endif
