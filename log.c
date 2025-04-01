#include "log.h"

static void uart_send_char(char c)
{
}
void log_init(void)
{
}

static void printf_decimal(uint8_t data)
{
	uint8_t one, ten, hundreds = 0;

	if (data <= 9) {
		one = data + 0x30;
		uart_send_char(one);
	} else if (data < 100) {
		one = (data % 10) + 0x30;
		ten = (data / 10) + 0x30;
		uart_send_char(ten);
		uart_send_char(one);
	} else {
		hundreds = (data / 100) + 0x30;
		ten = (data % 100);
		one = (ten / 10) + 0x30;
		ten = (ten % 10) + 0x30;
		uart_send_char(hundreds);
		uart_send_char(ten);
		uart_send_char(one);
	}
}

static void printf_hex(uint8_t data)
{
	uint8_t one, ten = 0;

	one = (data >> 4);
	ten = (data & 0x0F);
	if (one <= 0x09) {
		one += 0x30;
	} else {
		one += 0x37;
	}
	if (ten <= 0x09) {
		ten += 0x30;
	} else {
		ten += 0x37;
	}
	uart_send_char(one);
	uart_send_char(ten);
}

/// @brief Log 打印，基本格式"[函数名] 参数名 %x\n",参数"
/// @param format
/// @param data
void log(const char *format, uint8_t data)
{
	char *p_format = format;

	while ((*(p_format) != '\n')) {
		if (*(p_format) == '%') {
			if (*(p_format + 1) == 'd') {
				printf_decimal(data);
			}
			if (*(p_format + 1) == 'x') {
				printf_hex(data);
			}
			if (*(p_format + 1) == 's') {
				uart_send_char(data);
			}
			p_format++;
		} else {
			uart_send_char(*(p_format));
		}
		p_format++;
	}
	uart_send_char('\r');
	uart_send_char('\n');
}

void uart_printf_array(const uint8_t *arr, uint8_t len)
{
	uint8_t i = 0;
	uint8_t *p_arr = arr;
	for (; i < len; i++) {
		printf_hex(*(p_arr + i));
		uart_send_char(' ');
	}
	uart_send_char('\r');
	uart_send_char('\n');
}

void os_log_test(void)
{
	log("Hellow work\n", 0);
	uint8_t data = 100;
	log("test %d\n", data);
	data = 255;
	log("test %x\n", data);
}