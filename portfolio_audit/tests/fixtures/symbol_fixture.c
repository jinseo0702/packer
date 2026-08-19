#include <stdio.h>

int global_data_symbol = 42;
int global_bss_symbol;
const int global_rodata_symbol = 17;
static int local_data_symbol = 9;
static int local_bss_symbol;

int scope_global_value = 11;
static int scope_local_value = 13;

__attribute__((weak)) int weak_defined_data = 23;
extern int weak_undefined_function(void) __attribute__((weak));

__asm__(".globl absolute_symbol\n"
        ".set absolute_symbol, 0x1234\n");

static int local_text_symbol(int value)
{
	return (value + local_data_symbol + local_bss_symbol + scope_local_value);
}

__attribute__((weak)) int weak_defined_function(int value)
{
	return (value + weak_defined_data);
}

int global_text_symbol(int value)
{
	if (weak_undefined_function != NULL)
		value += weak_undefined_function();
	return (local_text_symbol(value) + weak_defined_function(value));
}

int main(int argc, char **argv)
{
	int value;

	value = global_text_symbol(argc) + global_data_symbol
		+ global_bss_symbol + global_rodata_symbol + scope_global_value;
	printf("symbol-fixture:%d:%s\n", value, argv[0]);
	return (value == -1);
}
