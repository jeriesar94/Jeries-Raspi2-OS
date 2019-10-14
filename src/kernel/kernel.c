#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <kernel/memory.h>
//#include <common/stdio.h>
#include <common/stdlib.h>

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;

	mem_initalization(atags);
    uart_init();
    uart_puts("Hello, kernel World!\r\n");

    while (1) {
		unsigned char entered_char = uart_getc();
        
		if (entered_char == '\r'){
			uart_putc('\n');
		}
		else if (entered_char == '\b'){ /*New Update to make printing on tty smoother (backspace will clear the character that will be written over directly)*/
			uart_putc('\b');
			uart_putc(' ');
		}
		uart_putc(entered_char);
    }
}