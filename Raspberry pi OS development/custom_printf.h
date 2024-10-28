#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h> // For va_list, va_start, va_end

// Function declarations
void printf(const char* format, ...);
void uart_puts(const char* str);

#endif // PRINTF_H
