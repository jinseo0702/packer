#include "../include/woody.h"

static int	hex_char_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	return (-1);
}

static int	parse_hex_key(const char *str, uint64_t *key)
{
	const char	*ptr;
	uint64_t	val;
	int			digit;
	int			i;

	ptr = str;
	if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
		ptr += 2;
	if (ft_strlen(ptr) != 16)
		return (ERR_KEY_FORMAT);
	val = 0;
	i = 0;
	while (i < 16)
	{
		digit = hex_char_to_val(ptr[i]);
		if (digit < 0)
			return (ERR_KEY_FORMAT);
		val = (val << 4) | (uint64_t)digit;
		i++;
	}
	*key = val;
	return (OK);
}

static int	generate_random_key(uint64_t *key)
{
	int		fd;
	ssize_t	ret;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return (ERR_KEY_GEN);
	ret = read(fd, key, sizeof(*key));
	close(fd);
	if (ret != (ssize_t)sizeof(*key))
		return (ERR_KEY_GEN);
	return (OK);
}

int	parse_args(int argc, char **argv, const char **path, uint64_t *key)
{
	int	err;

	if (argc < 2 || argc > 3)
		return (ERR_USAGE);
	*path = argv[1];
	if (argc == 3)
	{
		err = parse_hex_key(argv[2], key);
		if (err != OK)
			return (err);
	}
	else
	{
		err = generate_random_key(key);
		if (err != OK)
			return (err);
	}
	printf("KEY: 0x%016lx\n", *key);
	return (OK);
}
