/*******************************************************************************
 *  Filename: rpi-smartstart.cpp
 *  Converted to C++ from rpi-smartstart.c
 *  Logic is identical, with minor C++-compliant changes.
 *******************************************************************************/

#include "stdint.h"     // C++ standard for uint32_t, etc.
#include "rpi-SmartStart.h"  // This unit's header

/***************************************************************************}
{   PRIVATE INTERNAL RASPBERRY PI REGISTER STRUCTURE DEFINITIONS            }
****************************************************************************/

// Everything below remains structurally the same as the original C code.
// We only update includes, minor scoping changes, and ensure it compiles as C++.

//--------------------------------------------------------------------------
// RASPBERRY PI GPIO HARDWARE REGISTERS
//--------------------------------------------------------------------------

struct __attribute__((__packed__, aligned(4))) GPIORegisters {
    uint32_t GPFSEL[6];
    uint32_t reserved1;
    uint32_t GPSET[2];
    uint32_t reserved2;
    uint32_t GPCLR[2];
    uint32_t reserved3;
    const uint32_t GPLEV[2];
    uint32_t reserved4;
    uint32_t GPEDS[2];
    uint32_t reserved5;
    uint32_t GPREN[2];
    uint32_t reserved6;
    uint32_t GPFEN[2];
    uint32_t reserved7;
    uint32_t GPHEN[2];
    uint32_t reserved8;
    uint32_t GPLEN[2];
    uint32_t reserved9;
    uint32_t GPAREN[2];
    uint32_t reserved10;
    uint32_t GPAFEN[2];
    uint32_t reserved11;
    uint32_t GPPUD;
    uint32_t GPPUDCLK[2];
};

//--------------------------------------------------------------------------
// RASPBERRY PI SYSTEM TIMER HARDWARE REGISTERS
//--------------------------------------------------------------------------

struct __attribute__((__packed__, aligned(4))) SystemTimerRegisters {
    uint32_t ControlStatus;
    uint32_t TimerLo;
    uint32_t TimerHi;
    uint32_t Compare0;
    uint32_t Compare1;
    uint32_t Compare2;
    uint32_t Compare3;
};

//--------------------------------------------------------------------------
// TIMER CONTROL REGISTER
//--------------------------------------------------------------------------

typedef union {
    struct {
        unsigned unused : 1;
        unsigned Counter32Bit : 1;    // @1
        TIMER_PRESCALE Prescale : 2;  // @2-3
        unsigned unused1 : 1;
        unsigned TimerIrqEnable : 1;  // @5
        unsigned unused2 : 1;
        unsigned TimerEnable : 1;     // @7
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} time_ctrl_reg_t;

//--------------------------------------------------------------------------
// RASPBERRY PI ARM TIMER HARDWARE REGISTERS
//--------------------------------------------------------------------------

struct __attribute__((__packed__, aligned(4))) ArmTimerRegisters {
    uint32_t Load;
    const uint32_t Value;
    time_ctrl_reg_t Control;
    uint32_t Clear;
    const uint32_t RawIRQ;
    const uint32_t MaskedIRQ;
    uint32_t Reload;
};

//--------------------------------------------------------------------------
// IRQ BASIC PENDING REGISTER
//--------------------------------------------------------------------------

typedef union {
    struct __attribute__((__packed__, aligned(4))) {
        const unsigned Timer_IRQ_pending : 1;
        const unsigned Mailbox_IRQ_pending : 1;
        const unsigned Doorbell0_IRQ_pending : 1;
        const unsigned Doorbell1_IRQ_pending : 1;
        const unsigned GPU0_halted_IRQ_pending : 1;
        const unsigned GPU1_halted_IRQ_pending : 1;
        const unsigned Illegal_access_type1_pending : 1;
        const unsigned Illegal_access_type0_pending : 1;
        const unsigned Bits_set_in_pending_register_1 : 1;
        const unsigned Bits_set_in_pending_register_2 : 1;
        const unsigned GPU_IRQ_7_pending : 1;
        const unsigned GPU_IRQ_9_pending : 1;
        const unsigned GPU_IRQ_10_pending : 1;
        const unsigned GPU_IRQ_18_pending : 1;
        const unsigned GPU_IRQ_19_pending : 1;
        const unsigned GPU_IRQ_53_pending : 1;
        const unsigned GPU_IRQ_54_pending : 1;
        const unsigned GPU_IRQ_55_pending : 1;
        const unsigned GPU_IRQ_56_pending : 1;
        const unsigned GPU_IRQ_57_pending : 1;
        const unsigned GPU_IRQ_62_pending : 1;
        unsigned reserved : 10;
    };
    const uint32_t Raw32;
} irq_basic_pending_reg_t;

//--------------------------------------------------------------------------
// FIQ CONTROL REGISTER
//--------------------------------------------------------------------------

typedef union {
    struct __attribute__((__packed__, aligned(4))) {
        unsigned SelectFIQSource : 7;
        unsigned EnableFIQ : 1;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} fiq_control_reg_t;

//--------------------------------------------------------------------------
// ENABLE BASIC IRQ REGISTER
//--------------------------------------------------------------------------

typedef union {
    struct __attribute__((__packed__, aligned(4))) {
        unsigned Enable_Timer_IRQ : 1;
        unsigned Enable_Mailbox_IRQ : 1;
        unsigned Enable_Doorbell0_IRQ : 1;
        unsigned Enable_Doorbell1_IRQ : 1;
        unsigned Enable_GPU0_halted_IRQ : 1;
        unsigned Enable_GPU1_halted_IRQ : 1;
        unsigned Enable_Illegal_access_type1 : 1;
        unsigned Enable_Illegal_access_type0 : 1;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} irq_enable_basic_reg_t;

//--------------------------------------------------------------------------
// DISABLE BASIC IRQ REGISTER
//--------------------------------------------------------------------------

typedef union {
    struct __attribute__((__packed__, aligned(4))) {
        unsigned Disable_Timer_IRQ : 1;
        unsigned Disable_Mailbox_IRQ : 1;
        unsigned Disable_Doorbell0_IRQ : 1;
        unsigned Disable_Doorbell1_IRQ : 1;
        unsigned Disable_GPU0_halted_IRQ : 1;
        unsigned Disable_GPU1_halted_IRQ : 1;
        unsigned Disable_Illegal_access_type1 : 1;
        unsigned Disable_Illegal_access_type0 : 1;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} irq_disable_basic_reg_t;

//--------------------------------------------------------------------------
// RASPBERRY PI IRQ HARDWARE REGISTERS
//--------------------------------------------------------------------------

struct __attribute__((__packed__, aligned(4))) IrqControlRegisters {
    const irq_basic_pending_reg_t IRQBasicPending;
    uint32_t IRQPending1;
    uint32_t IRQPending2;
    fiq_control_reg_t FIQControl;
    uint32_t EnableIRQs1;
    uint32_t EnableIRQs2;
    irq_enable_basic_reg_t EnableBasicIRQs;
    uint32_t DisableIRQs1;
    uint32_t DisableIRQs2;
    irq_disable_basic_reg_t DisableBasicIRQs;
};

//--------------------------------------------------------------------------
// RASPBERRY PI MAILBOX REGISTERS
//--------------------------------------------------------------------------

struct __attribute__((__packed__, aligned(4))) MailBoxRegisters {
    const uint32_t Read0;
    uint32_t Unused[3];
    uint32_t Peek0;
    uint32_t Sender0;
    uint32_t Status0;
    uint32_t Config0;
    uint32_t Write1;
    uint32_t Unused2[3];
    uint32_t Peek1;
    uint32_t Sender1;
    uint32_t Status1;
    uint32_t Config1;
};

//--------------------------------------------------------------------------
// MINI UART REGISTERS & STRUCT
//--------------------------------------------------------------------------

typedef union {
    struct __attribute__((__packed__)) {
        unsigned DATA : 8;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} mu_io_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned RXDI : 1;
        unsigned TXEI : 1;
        unsigned LSI : 1;
        unsigned MSI : 1;
        unsigned reserved : 28;
    };
    uint32_t Raw32;
} mu_ie_reg_t;

typedef union {
    const struct __attribute__((__packed__)) {
        unsigned PENDING : 1;
        enum {
            MU_NO_INTERRUPTS = 0,
            MU_TXE_INTERRUPT = 1,
            MU_RXD_INTERRUPTS = 2,
        } SOURCE : 2;
        unsigned reserved_rd : 29;
    };
    struct __attribute__((__packed__)) {
        unsigned unused : 1;
        unsigned RXFIFO_CLEAR : 1;
        unsigned TXFIFO_CLEAR : 1;
        unsigned reserved_wr : 29;
    };
    uint32_t Raw32;
} mu_ii_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned DATA_LENGTH : 1;
        unsigned reserved : 5;
        unsigned BREAK : 1;
        unsigned DLAB : 1;
        unsigned reserved1 : 24;
    };
    uint32_t Raw32;
} mu_lcr_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned reserved : 1;
        unsigned RTS : 1;
        unsigned reserved1 : 30;
    };
    uint32_t Raw32;
} mu_mcr_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned RXFDA : 1;
        unsigned RXOE : 1;
        unsigned reserved : 3;
        unsigned TXFE : 1;
        unsigned TXIdle : 1;
        unsigned reserved1 : 25;
    };
    uint32_t Raw32;
} mu_lsr_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned reserved : 4;
        unsigned CTS : 1;
        unsigned reserved1 : 27;
    };
    uint32_t Raw32;
} mu_msr_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned USER_DATA : 8;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} mu_scratch_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned RXE : 1;
        unsigned TXE : 1;
        unsigned EnableRTS : 1;
        unsigned EnableCTS : 1;
        enum {
            FIFOhas3spaces = 0,
            FIFOhas2spaces = 1,
            FIFOhas1spaces = 2,
            FIFOhas4spaces = 3,
        } RTSflowLevel : 2;
        unsigned RTSassertLevel : 1;
        unsigned CTSassertLevel : 1;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} mu_cntl_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned RXFDA : 1;
        unsigned TXFE : 1;
        unsigned RXIdle : 1;
        unsigned TXIdle : 1;
        unsigned RXOE : 1;
        unsigned TXFF : 1;
        unsigned RTS : 1;
        unsigned CTS : 1;
        unsigned TXFCE : 1;
        unsigned TX_DONE : 1;
        unsigned reserved : 6;
        unsigned RXFIFOLEVEL : 4;
        unsigned reserved1 : 4;
        unsigned TXFIFOLEVEL : 4;
        unsigned reserved2 : 4;
    };
    uint32_t Raw32;
} mu_stat_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned DIVISOR : 16;
        unsigned reserved : 16;
    };
    uint32_t Raw32;
} mu_baudrate_reg_t;

struct __attribute__((__packed__, aligned(4))) MiniUARTRegisters {
    mu_io_reg_t IO;
    mu_ie_reg_t IER;
    mu_ii_reg_t IIR;
    mu_lcr_reg_t LCR;
    mu_mcr_reg_t MCR;
    const mu_lsr_reg_t LSR;
    const mu_msr_reg_t MSR;
    mu_scratch_reg_t SCRATCH;
    mu_cntl_reg_t CNTL;
    const mu_stat_reg_t STAT;
    mu_baudrate_reg_t BAUD;
};

//--------------------------------------------------------------------------
// PL011 UART REGISTERS & STRUCT
//--------------------------------------------------------------------------

typedef union {
    struct __attribute__((__packed__)) {
        unsigned DATA : 8;
        unsigned FE : 1;
        unsigned PE : 1;
        unsigned BE : 1;
        unsigned OE : 1;
        unsigned _reserved : 20;
    };
    uint32_t Raw32;
} pl011_data_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned CTS : 1;
        unsigned DSR : 1;
        unsigned DCD : 1;
        unsigned BUSY : 1;
        unsigned RXFE : 1;
        unsigned TXFF : 1;
        unsigned RXFF : 1;
        unsigned TXFE : 1;
        unsigned _reserved : 24;
    };
    uint32_t Raw32;
} pl011_fr_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned DIVISOR : 16;
        unsigned _reserved : 16;
    };
    uint32_t Raw32;
} pl011_ibrd_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned DIVISOR : 6;
        unsigned _reserved : 26;
    };
    uint32_t Raw32;
} pl011_fbrd_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned BRK : 1;
        unsigned PEN : 1;
        unsigned EPS : 1;
        unsigned STP2 : 1;
        unsigned FEN : 1;
        enum {
            PL011_DATA_5BITS = 0,
            PL011_DATA_6BITS = 1,
            PL011_DATA_7BITS = 2,
            PL011_DATA_8BITS = 3,
        } DATALEN : 2;
        unsigned SPS : 1;
        unsigned _reserved : 24;
    };
    uint32_t Raw32;
} pl011_lrch_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned UARTEN : 1;
        unsigned _unused : 6;
        unsigned LBE : 1;
        unsigned TXE : 1;
        unsigned RXE : 1;
        unsigned DTR_unused : 1;
        unsigned RTS : 1;
        unsigned OUT : 2;
        unsigned RTSEN : 1;
        unsigned CTSEN : 1;
        unsigned _reserved : 16;
    };
    uint32_t Raw32;
} pl011_cr_reg_t;

typedef union {
    struct __attribute__((__packed__)) {
        unsigned RIMIC : 1;
        unsigned CTSMIC : 1;
        unsigned DCDMIC : 1;
        unsigned DSRMIC : 1;
        unsigned RXIC : 1;
        unsigned TXIC : 1;
        unsigned RTIC : 1;
        unsigned FEIC : 1;
        unsigned PEIC : 1;
        unsigned BEIC : 1;
        unsigned OEIC : 1;
        unsigned _reserved : 21;
    };
    uint32_t Raw32;
} pl011_icr_reg_t;

struct __attribute__((__packed__, aligned(4))) PL011UARTRegisters {
    pl011_data_reg_t DR;
    uint32_t RSRECR;
    uint32_t _unused[4];
    pl011_fr_reg_t FR;
    uint32_t _unused1[2];
    pl011_ibrd_reg_t IBRD;
    pl011_fbrd_reg_t FBRD;
    pl011_lrch_reg_t LCRH;
    pl011_cr_reg_t CR;
    uint32_t IFLS;
    uint32_t IMSC;
    uint32_t RIS;
    uint32_t MIS;
    pl011_icr_reg_t ICR;
    uint32_t DMACR;
};

//--------------------------------------------------------------------------
// QA7 / MULTICORE LOCAL TIMER REGISTERS
//--------------------------------------------------------------------------

typedef union {
    struct {
        enum {
            LOCALTIMER_TO_CORE0_IRQ = 0,
            LOCALTIMER_TO_CORE1_IRQ = 1,
            LOCALTIMER_TO_CORE2_IRQ = 2,
            LOCALTIMER_TO_CORE3_IRQ = 3,
            LOCALTIMER_TO_CORE0_FIQ = 4,
            LOCALTIMER_TO_CORE1_FIQ = 5,
            LOCALTIMER_TO_CORE2_FIQ = 6,
            LOCALTIMER_TO_CORE3_FIQ = 7,
        } Routing : 3;
        unsigned unused : 29;
    };
    uint32_t Raw32;
} local_timer_int_route_reg_t;

typedef union {
    struct {
        unsigned ReloadValue : 28;
        unsigned TimerEnable : 1;
        unsigned IntEnable : 1;
        unsigned unused : 1;
        unsigned IntPending : 1;
    };
    uint32_t Raw32;
} local_timer_ctrl_status_reg_t;

typedef union {
    struct {
        unsigned unused : 30;
        unsigned Reload : 1;
        unsigned IntClear : 1;
    };
    uint32_t Raw32;
} local_timer_clr_reload_reg_t;

typedef union {
    struct {
        unsigned nCNTPSIRQ_IRQ : 1;
        unsigned nCNTPNSIRQ_IRQ : 1;
        unsigned nCNTHPIRQ_IRQ : 1;
        unsigned nCNTVIRQ_IRQ : 1;
        unsigned nCNTPSIRQ_FIQ : 1;
        unsigned nCNTPNSIRQ_FIQ : 1;
        unsigned nCNTHPIRQ_FIQ : 1;
        unsigned nCNTVIRQ_FIQ : 1;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} generic_timer_int_ctrl_reg_t;

typedef union {
    struct {
        unsigned Mailbox0_IRQ : 1;
        unsigned Mailbox1_IRQ : 1;
        unsigned Mailbox2_IRQ : 1;
        unsigned Mailbox3_IRQ : 1;
        unsigned Mailbox0_FIQ : 1;
        unsigned Mailbox1_FIQ : 1;
        unsigned Mailbox2_FIQ : 1;
        unsigned Mailbox3_FIQ : 1;
        unsigned reserved : 24;
    };
    uint32_t Raw32;
} mailbox_int_ctrl_reg_t;

typedef union {
    struct {
        unsigned CNTPSIRQ : 1;
        unsigned CNTPNSIRQ : 1;
        unsigned CNTHPIRQ : 1;
        unsigned CNTVIRQ : 1;
        unsigned Mailbox0_Int : 1;
        unsigned Mailbox1_Int : 1;
        unsigned Mailbox2_Int : 1;
        unsigned Mailbox3_Int : 1;
        unsigned GPU_Int : 1;
        unsigned PMU_Int : 1;
        unsigned AXI_Int : 1;
        unsigned Timer_Int : 1;
        unsigned GPIO_Int : 16;
        unsigned reserved : 4;
    };
    uint32_t Raw32;
} core_int_source_reg_t;

struct __attribute__((__packed__, aligned(4))) QA7Registers {
    local_timer_int_route_reg_t TimerRouting;
    uint32_t GPIORouting;
    uint32_t AXIOutstandingCounters;
    uint32_t AXIOutstandingIrq;
    local_timer_ctrl_status_reg_t TimerControlStatus;
    local_timer_clr_reload_reg_t TimerClearReload;
    uint32_t unused;
    generic_timer_int_ctrl_reg_t CoreTimerIntControl[4];
    mailbox_int_ctrl_reg_t CoreMailboxIntControl[4];
    core_int_source_reg_t CoreIRQSource[4];
    core_int_source_reg_t CoreFIQSource[4];
};

//--------------------------------------------------------------------------
// RASPBERRY PI MODEL NUMBER
//--------------------------------------------------------------------------

typedef union {
    struct {
        unsigned revision : 4;
        unsigned model : 8;
        unsigned processor : 4;
        unsigned manufacturer : 4;
        unsigned memory_size : 3;
        unsigned new_revision : 1;
        unsigned _reserved : 8;
    };
    uint32_t Raw32;
} model_number_t;

/***************************************************************************}
{  PRIVATE INTERNAL RASPBERRY PI REGISTER STRUCTURE CHECKS & GLOBALS        }
****************************************************************************/

static_assert(sizeof(CODE_TYPE) == 0x04, "Structure CODE_TYPE should be 0x04 bytes in size");
static_assert(sizeof(CPU_ID) == 0x04,   "Structure CPU_ID should be 0x04 bytes in size");
static_assert(sizeof(model_number_t) == 0x04, "Structure model_number_t should be 0x04 bytes in size");
static_assert(sizeof(GPIORegisters) == 0xA0, "Structure GPIORegisters should be 0xA0 bytes in size");
static_assert(sizeof(SystemTimerRegisters) == 0x1C, "SystemTimerRegisters should be 0x1C bytes in size");
static_assert(sizeof(ArmTimerRegisters) == 0x1C, "ArmTimerRegisters should be 0x1C bytes in size");
static_assert(sizeof(IrqControlRegisters) == 0x28, "IrqControlRegisters should be 0x28 bytes in size");
static_assert(sizeof(MailBoxRegisters) == 0x40, "MailBoxRegisters should be 0x40 bytes in size");
static_assert(sizeof(MiniUARTRegisters) == 0x2C, "MiniUARTRegisters should be 0x2C bytes in size");
static_assert(sizeof(PL011UARTRegisters) == 0x4C, "PL011UARTRegisters should be 0x4C bytes in size");
static_assert(sizeof(QA7Registers) == 0x5C, "QA7Registers should be 0x5C bytes in size");

uint32_t RPi_IO_Base_Addr       = 0x3F000000;
uint32_t RPi_ARM_TO_GPU_Alias;
uint32_t RPi_BootAddr;
uint32_t RPi_CoresReady;
uint32_t RPi_CPUBootMode;
CPU_ID   RPi_CpuId;
CODE_TYPE RPi_CompileMode;
uint32_t RPi_CPUCurrentMode;
SMARTSTART_VER RPi_SmartStartVer;
void* RPi_coreCB_PTR[4];

/***************************************************************************}
{   PRIVATE POINTERS TO RASPBERRY PI REGISTER BANK STRUCTURES               }
****************************************************************************/
#define GPIO       ((volatile GPIORegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x200000))
#define SYSTEMTIMER ((volatile SystemTimerRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x3000))
#define IRQ        ((volatile IrqControlRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB200))
#define ARMTIMER   ((volatile ArmTimerRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB400))
#define MAILBOX    ((volatile MailBoxRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0xB880))
#define MINIUART   ((volatile MiniUARTRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x00215040))
#define PL011UART  ((volatile PL011UARTRegisters*)(uintptr_t)(RPi_IO_Base_Addr + 0x00201000))
#define QA7        ((volatile QA7Registers*)(uintptr_t)(0x40000024))

/***************************************************************************}
{   ARM CPU ID STRINGS                                                      }
****************************************************************************/
static const char* ARM6_STR = "arm1176jzf-s";
static const char* ARM7_STR = "cortex-a7";
static const char* ARM8_STR = "cortex-a53";
static const char* ARMX_STR = "unknown ARM cpu";

/***************************************************************************}
{   PUBLIC C INTERFACE ROUTINES (same code, now C++-compatible)            }
****************************************************************************/

/*==========================================================================}
{   PUBLIC CPU ID ROUTINES PROVIDED BY RPi-SmartStart API                  }
{==========================================================================*/

extern "C" const char* RPi_CpuIdString(void)
{
    switch (RPi_CpuId.PartNumber) {
    case 0xb76:  // ARM 6 CPU
        return ARM6_STR;
    case 0xc07:  // ARM 7 CPU
        return ARM7_STR;
    case 0xd03:  // ARM 8 CPU
        return ARM8_STR;
    default:
        return ARMX_STR;
    }
}

/*==========================================================================}
{   PUBLIC GPIO ROUTINES PROVIDED BY RPi-SmartStart API                    }
{==========================================================================*/

#define MAX_GPIO_NUM 54

extern "C" bool gpio_setup(unsigned int gpio, GPIOMODE mode)
{
    if ((gpio < MAX_GPIO_NUM) && (mode <= 7)) {
        uint32_t bit = ((gpio % 10) * 3);
        uint32_t mem = GPIO->GPFSEL[gpio / 10];
        mem &= ~(7 << bit);
        mem |= (mode << bit);
        GPIO->GPFSEL[gpio / 10] = mem;
        return true;
    }
    return false;
}

extern "C" bool gpio_output(unsigned int gpio, bool on)
{
    if (gpio < MAX_GPIO_NUM) {
        volatile uint32_t* p;
        uint32_t bit = 1 << (gpio % 32);
        p = (on) ? &GPIO->GPSET[0] : &GPIO->GPCLR[0];
        p[gpio / 32] = bit;
        return true;
    }
    return false;
}

extern "C" bool gpio_input(unsigned int gpio)
{
    if (gpio < MAX_GPIO_NUM) {
        uint32_t bit = 1 << (gpio % 32);
        uint32_t mem = GPIO->GPLEV[gpio / 32];
        if (mem & bit) return true;
    }
    return false;
}

extern "C" bool gpio_checkEvent(unsigned int gpio)
{
    if (gpio < MAX_GPIO_NUM) {
        uint32_t bit = 1 << (gpio % 32);
        uint32_t mem = GPIO->GPEDS[gpio / 32];
        if (mem & bit) return true;
    }
    return false;
}

extern "C" bool gpio_clearEvent(unsigned int gpio)
{
    if (gpio < MAX_GPIO_NUM) {
        uint32_t bit = 1 << (gpio % 32);
        GPIO->GPEDS[gpio / 32] = bit;
        return true;
    }
    return false;
}

extern "C" bool gpio_edgeDetect(unsigned int gpio, bool lifting, bool Async)
{
    if (gpio < MAX_GPIO_NUM) {
        volatile uint32_t* p;
        uint32_t bit = 1 << (gpio % 32);
        if (lifting) {
            p = (Async) ? &GPIO->GPAREN[0] : &GPIO->GPREN[0];
        } else {
            p = (Async) ? &GPIO->GPAFEN[0] : &GPIO->GPFEN[0];
        }
        p[gpio / 32] = bit;
        return true;
    }
    return false;
}

extern "C" bool gpio_fixResistor(unsigned int gpio, GPIO_FIX_RESISTOR resistor)
{
    if ((gpio < MAX_GPIO_NUM) && (resistor <= 2)) {
        uint32_t regnum = gpio / 32;
        uint32_t bit = 1 << (gpio % 32);
        GPIO->GPPUD = resistor;
        timer_wait(2);
        GPIO->GPPUDCLK[regnum] = bit;
        timer_wait(2);
        GPIO->GPPUD = 0;
        GPIO->GPPUDCLK[regnum] = 0;
        return true;
    }
    return false;
}

/*==========================================================================}
{   PUBLIC TIMER ROUTINES PROVIDED BY RPi-SmartStart API                   }
{==========================================================================*/

extern "C" uint64_t timer_getTickCount64(void)
{
    uint32_t hiCount;
    uint32_t loCount;
    do {
        hiCount = SYSTEMTIMER->TimerHi;
        loCount = SYSTEMTIMER->TimerLo;
    } while (hiCount != SYSTEMTIMER->TimerHi);
    return (((uint64_t)hiCount << 32) | loCount);
}

extern "C" void timer_wait(uint64_t usec)
{
    usec += timer_getTickCount64();
    while (timer_getTickCount64() < usec) { /* busy-wait */ }
}

extern "C" uint64_t tick_difference(uint64_t us1, uint64_t us2)
{
    if (us1 > us2) {
        uint64_t td = UINT64_MAX - us1 + 1;
        return us2 + td;
    }
    return (us2 - us1);
}

/*==========================================================================}
{   PUBLIC PI MAILBOX ROUTINES PROVIDED BY RPi-SmartStart API              }
{==========================================================================*/

#define MAIL_EMPTY 0x40000000
#define MAIL_FULL  0x80000000

extern "C" bool mailbox_write(MAILBOX_CHANNEL channel, uint32_t message)
{
    if ((channel >= 0) && (channel <= MB_CHANNEL_GPU)) {
        message &= ~(0xF);
        message |= channel;
        while ((MAILBOX->Status1 & MAIL_FULL) != 0);
        MAILBOX->Write1 = message;
        return true;
    }
    return false;
}

extern "C" uint32_t mailbox_read(MAILBOX_CHANNEL channel)
{
    if ((channel >= 0) && (channel <= MB_CHANNEL_GPU)) {
        uint32_t value;
        do {
            while ((MAILBOX->Status0 & MAIL_EMPTY) != 0);
            value = MAILBOX->Read0;
        } while ((value & 0xF) != channel);
        value &= ~(0xF);
        return value;
    }
    return 0xFEEDDEAD;
}

extern "C" bool mailbox_tag_message(uint32_t* response_buf,
                                    uint8_t data_count,
                                    ...)
{
    // Exactly the same logic, just using C++ varargs
    uint32_t __attribute__((aligned(16))) message[data_count + 3];
    uint32_t addr = (uint32_t)(uintptr_t)&message[0];
    va_list list;
    va_start(list, data_count);
    message[0] = (data_count + 3) * 4;
    message[data_count + 2] = 0;
    message[1] = 0;
    for (int i = 0; i < data_count; i++) {
        message[2 + i] = va_arg(list, uint32_t);
    }
    va_end(list);

#if __aarch64__ == 1
    __asm volatile("dc civac, %0" : : "r"(addr) : "memory");
#else
    __asm volatile("mcr p15, 0, %0, c7, c14, 1" : : "r"(addr));
#endif

    mailbox_write(MB_CHANNEL_TAGS, ARMaddrToGPUaddr(addr));
    mailbox_read(MB_CHANNEL_TAGS);

#if __aarch64__ == 1
    __asm volatile("dc civac, %0" : : "r"(addr) : "memory");
#else
    __asm volatile("mcr p15, 0, %0, c7, c14, 1" : : "r"(addr));
#endif

    if (message[1] == 0x80000000) {
        if (response_buf) {
            for (int i = 0; i < data_count; i++) {
                response_buf[i] = message[2 + i];
            }
        }
        return true;
    }
    return false;
}

/*==========================================================================}
{   PUBLIC PI TIMER INTERRUPT ROUTINES PROVIDED BY RPi-SmartStart API       }
{==========================================================================*/

extern "C" void ClearTimerIrq(void)
{
    ARMTIMER->Clear = 0x1;
}

extern "C" bool TimerSetup(uint32_t period_in_us)
{
    uint32_t Buffer[5] = {0};
    bool resValue = false;
    ARMTIMER->Control.TimerEnable = 0;
    if (mailbox_tag_message(&Buffer[0], 5,
                            MAILBOX_TAG_GET_CLOCK_RATE, 8, 8, 4, 0))
    {
        volatile uint32_t divisor = Buffer[4] / 250;
        divisor /= 10000;
        divisor *= period_in_us;
        divisor /= 100;
        if (divisor != 0) {
            ARMTIMER->Load = divisor;
            ARMTIMER->Control.Counter32Bit = 1;
            ARMTIMER->Control.Prescale = Clkdiv1;
            resValue = true;
        }
        ARMTIMER->Control.TimerEnable = 1;
    }
    return resValue;
}

extern "C" bool TimerIrqSetup(uint32_t period_in_us)
{
    if (TimerSetup(period_in_us)) {
        IRQ->EnableBasicIRQs.Enable_Timer_IRQ = 1;
        ARMTIMER->Control.TimerIrqEnable = 1;
        return true;
    }
    return false;
}

/*==========================================================================}
{   PUBLIC PI MULTICORE LOCAL TIMER ROUTINES                                }
{==========================================================================*/

extern "C" void ClearLocalTimerIrq(void)
{
    QA7->TimerClearReload.IntClear = 1;
    QA7->TimerClearReload.Reload = 1;
}

extern "C" bool LocalTimerSetup(uint32_t period_in_us)
{
    if (RPi_CpuId.PartNumber != 0xB76) {
        volatile uint32_t divisor = 384 * period_in_us;
        divisor /= 10;
        QA7->TimerControlStatus.ReloadValue = divisor;
        QA7->TimerControlStatus.TimerEnable = 1;
        QA7->TimerClearReload.Reload = 1;
        return true;
    }
    return false;
}

extern "C" bool LocalTimerIrqSetup(uint32_t period_in_us, uint8_t coreNum)
{
    if (LocalTimerSetup(period_in_us)) {
        QA7->TimerRouting.Routing = (local_timer_int_route_reg_t::LOCALTIMER_TO_CORE0_IRQ + coreNum);
        QA7->TimerControlStatus.IntEnable = 1;
        QA7->TimerClearReload.IntClear = 1;
        QA7->TimerClearReload.Reload = 1;
        QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_IRQ = 1;
        QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_FIQ = 0;
        return true;
    }
    return false;
}

extern "C" bool LocalTimerFiqSetup(uint32_t period_in_us, uint8_t coreNum)
{
    if (LocalTimerSetup(period_in_us)) {
        QA7->TimerRouting.Routing = (local_timer_int_route_reg_t::LOCALTIMER_TO_CORE0_FIQ + coreNum);
        QA7->TimerControlStatus.IntEnable = 1;
        QA7->TimerClearReload.IntClear = 1;
        QA7->TimerClearReload.Reload = 1;
        QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_FIQ = 1;
        QA7->CoreTimerIntControl[coreNum].nCNTPNSIRQ_IRQ = 0;
        return true;
    }
    return false;
}

/*==========================================================================}
{   MINIUART ROUTINES                                                       }
{==========================================================================*/

#define AUX_ENABLES (RPi_IO_Base_Addr+0x00215004)

extern "C" char miniuart_getc(void)
{
    while (MINIUART->LSR.RXFDA == 0) { /* busy-wait */ }
    return (char)(MINIUART->IO.DATA);
}

extern "C" void miniuart_putc(char c)
{
    while (MINIUART->LSR.TXFE == 0) { /* busy-wait */ }
    MINIUART->IO.DATA = (unsigned)(c);
}

extern "C" void miniuart_puts(char* str)
{
    if (str) {
        while (*str) {
            miniuart_putc(*str++);
        }
    }
}

/*==========================================================================}
{   PL011 UART ROUTINES                                                     }
{==========================================================================*/

extern "C" bool pl011_uart_init(unsigned int baudrate)
{
    PL011UART->CR.Raw32 = 0;
    gpio_setup(14, GPIO_ALTFUNC0);
    gpio_setup(15, GPIO_ALTFUNC0);
    if (mailbox_tag_message(nullptr, 5,
                            MAILBOX_TAG_SET_CLOCK_RATE, 8, 8, CLK_UART_ID, 4000000))
    {
        uint32_t divisor, fracpart;
        divisor = 4000000 / (baudrate * 16);
        PL011UART->IBRD.DIVISOR = divisor;
        fracpart = 4000000 - (divisor * baudrate * 16);
        fracpart *= 4;
        fracpart += baudrate / 2;
        fracpart /= baudrate;
        PL011UART->FBRD.DIVISOR = fracpart;
        PL011UART->LCRH.DATALEN = pl011_lrch_reg_t::PL011_DATA_8BITS;
        PL011UART->LCRH.PEN = 0;
        PL011UART->LCRH.STP2 = 0;
        PL011UART->LCRH.FEN = 0;
        PL011UART->CR.UARTEN = 1;
        PL011UART->CR.RXE = 1;
        PL011UART->CR.TXE = 1;
        return true;
    }
    return false;
}

extern "C" char pl011_uart_getc(void)
{
    while (PL011UART->FR.RXFE != 0) { /* busy-wait */ }
    return (char)(PL011UART->DR.DATA);
}

extern "C" void pl011_uart_putc(char c)
{
    while (PL011UART->FR.TXFF != 0) { /* busy-wait */ }
    PL011UART->DR.DATA = (unsigned)(c);
}

extern "C" void pl011_uart_puts(char* str)
{
    if (str) {
        while (*str) {
            pl011_uart_putc(*str++);
        }
    }
}

/*==========================================================================}
{   PUBLIC PI ACTIVITY LED ROUTINES                                        }
{==========================================================================*/
static bool ModelCheckHasRun = false;
static bool UseExpanderGPIO = false;
static uint_fast8_t ActivityGPIOPort = 47;

extern "C" bool set_Activity_LED(bool on)
{
    if (!ModelCheckHasRun) {
        uint32_t model[4];
        ModelCheckHasRun = true;
        if (mailbox_tag_message(&model[0], 4,
            MAILBOX_TAG_GET_BOARD_REVISION, 4, 4, 0))
        {
            model[0] &= 0x00FFFFFF;
            if ((model[3] >= 0x0002) && (model[3] <= 0x000f)) {
                ActivityGPIOPort = 16;
                UseExpanderGPIO = false;
            }
            else if (model[3] < 0xa02082) {
                ActivityGPIOPort = 47;
                UseExpanderGPIO = false;
            }
            else if ((model[3] == 0xa02082) ||
                     (model[3] == 0xa020a0) ||
                     (model[3] == 0xa22082) ||
                     (model[3] == 0xa32082))
            {
                ActivityGPIOPort = 130;
                UseExpanderGPIO = true;
            }
            else {
                ActivityGPIOPort = 29;
                UseExpanderGPIO = false;
            }
        }
        else return false;
    }
    if (UseExpanderGPIO) {
        return mailbox_tag_message(nullptr, 5,
            MAILBOX_TAG_SET_GPIO_STATE, 8, 8, ActivityGPIOPort, (uint32_t)on);
    }
    else {
        gpio_output(ActivityGPIOPort, on);
    }
    return true;
}

/*==========================================================================}
{   PUBLIC ARM CPU SPEED SET ROUTINES                                      }
{==========================================================================*/

extern "C" bool ARM_setmaxspeed(int (*prn_handler)(const char *fmt, ...))
{
    uint32_t Buffer[5] = {0};
    if (mailbox_tag_message(&Buffer[0], 5, MAILBOX_TAG_GET_MAX_CLOCK_RATE,
                            8, 8, CLK_ARM_ID, 0))
    {
        if (mailbox_tag_message(&Buffer[0], 5, MAILBOX_TAG_SET_CLOCK_RATE,
                                8, 8, CLK_ARM_ID, Buffer[4]))
        {
            if (prn_handler) {
                prn_handler("CPU frequency set to %u Hz\n", Buffer[4]);
            }
            return true;
        }
    }
    return false;
}

