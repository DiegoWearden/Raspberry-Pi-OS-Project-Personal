#include "uart.h"
#include <stdarg.h> // For va_list, va_start, va_end

void itoa(int value, char* str, int base) {
    char *p = str;
    int negative = 0;

    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }

    // Convert to string
    do {
        int remainder = value % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
        value /= base;
    } while (value);

    if (negative) *p++ = '-';

    *p = '\0';

    // Reverse the string
    char *start = str;
    char *end = p - 1;
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    for (const char* p = format; *p != '\0'; p++) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 's': {
                    char* s = va_arg(args, char*);
                    uart_puts(s);
                    break;
                }
                case 'd': {
                    int i = va_arg(args, int);
                    char buf[32];
                    itoa(i, buf, 10);
                    uart_puts(buf);
                    break;
                }
                case 'x': {
                    int i = va_arg(args, int);
                    char buf[32];
                    itoa(i, buf, 16);
                    uart_puts(buf);
                    break;
                }
                case 'c': {
                    int c = va_arg(args, int);
                    uart_putc((char)c);
                    break;
                }
                default:
                    uart_putc('%');
                    uart_putc(*p);
                    break;
            }
        } else {
            uart_putc(*p);
        }
    }

    va_end(args);

    // Add carriage return at the end
    uart_putc('\r');
}
