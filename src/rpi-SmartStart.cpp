#include "stdint.h"     // C++ standard for uint32_t, etc.
#include "rpi-SmartStart.h"  // This unit's header

#define MB_CHANNEL_TAGS 8

// Mailbox register structure
struct MailBoxRegisters {
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

volatile MailBoxRegisters* const MAILBOX = (volatile MailBoxRegisters*)(0x3F00B880);

#define MAIL_EMPTY 0x40000000
#define MAIL_FULL  0x80000000

bool mailbox_write(uint8_t channel, uint32_t message) {
    if ((channel <= MB_CHANNEL_TAGS)) {
        message &= ~0xF;
        message |= channel;
        while ((MAILBOX->Status1 & MAIL_FULL) != 0);
        MAILBOX->Write1 = message;
        return true;
    }
    return false;
}

uint32_t mailbox_read(uint8_t channel) {
    uint32_t value;
    do {
        while ((MAILBOX->Status0 & MAIL_EMPTY) != 0);
        value = MAILBOX->Read0;
    } while ((value & 0xF) != channel);
    return value & ~0xF;
}

extern "C" bool mailbox_tag_message(uint32_t* response_buf, uint8_t data_count, ...) {
    uint32_t message[data_count + 3] __attribute__((aligned(16)));
    uint32_t addr = (uint32_t)(uintptr_t)message;
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
