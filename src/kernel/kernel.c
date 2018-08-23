#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;

    uart_init();
    uart_puts("Hello, kernel World!\r\n");

    while (1) {
		unsigned char entered_char = uart_getc();
        
		if (entered_char == '\r'){
			uart_putc('\n');
		}
		uart_putc(entered_char);
    }
}