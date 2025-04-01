#ifndef _LOG_H_
#define _LOG_H_

#include "os_type.h"

//API
void log(const char *format, uint8_t data);
void FPC_log(const char *format, uint8_t data);
void log_init(void);
void uart_printf_array(const uint8_t *arr, uint8_t len);

///test code
void os_log_test(void);
#endif