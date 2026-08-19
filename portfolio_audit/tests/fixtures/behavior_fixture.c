#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print_arguments(int argc, char **argv)
{
	int index;

	printf("argc=%d\n", argc);
	index = 0;
	while (index < argc)
	{
		printf("argv[%d]=%zu:", index, strlen(argv[index]));
		fwrite(argv[index], 1, strlen(argv[index]), stdout);
		putchar('\n');
		index++;
	}
	return (0);
}

static int copy_stdin(void)
{
	unsigned char buffer[64];
	size_t count;

	while ((count = fread(buffer, 1, sizeof(buffer), stdin)) != 0)
		fwrite(buffer, 1, count, stdout);
	return (ferror(stdin) || ferror(stdout));
}

int main(int argc, char **argv)
{
	const char *mode;
	const char *value;

	mode = argc > 1 ? argv[1] : "default";
	if (strcmp(mode, "argv") == 0)
		return (print_arguments(argc, argv));
	if (strcmp(mode, "stdout") == 0)
		return (printf("stdout:stable\n") < 0);
	if (strcmp(mode, "stderr") == 0)
		return (fprintf(stderr, "stderr:stable\n") < 0);
	if (strcmp(mode, "exit") == 0)
		return (argc > 2 ? atoi(argv[2]) & 0xff : 3);
	if (strcmp(mode, "stdin") == 0)
		return (copy_stdin());
	if (strcmp(mode, "env") == 0)
	{
		value = getenv("PACKER_TEST_ENV");
		printf("env=%s\n", value == NULL ? "<unset>" : value);
		return (0);
	}
	printf("behavior:default\n");
	return (0);
}
