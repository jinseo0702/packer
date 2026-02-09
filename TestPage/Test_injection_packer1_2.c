#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static inline void *move_address(void *base, uint64_t offset)
{
	unsigned char *p;

	if (base == NULL)
		return (NULL);
	p = (unsigned char *)base;
	return ((void *)(p + offset));
}

static inline uint8_t check_range(uint64_t offset, uint64_t size, uint64_t limit)
{
	if (offset > limit)
		return (0);
	return (size <= (limit - offset));
}

static int is_hex_char(char c)
{
	if ('0' <= c && c <= '9')
		return (1);
	if ('a' <= c && c <= 'f')
		return (1);
	if ('A' <= c && c <= 'F')
		return (1);
	return (0);
}

static uint8_t hex_nibble(char c)
{
	if ('0' <= c && c <= '9')
		return ((uint8_t)(c - '0'));
	if ('a' <= c && c <= 'f')
		return ((uint8_t)(c - 'a' + 10));
	return ((uint8_t)(c - 'A' + 10));
}

static int parse_hex_key(const char *hex, uint8_t **out_key, size_t *out_key_len)
{
	size_t	hex_len;
	size_t	i;
	size_t	j;
	uint8_t	*key;

	if (hex == NULL || out_key == NULL || out_key_len == NULL)
		return (-1);
	if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
		hex += 2;
	hex_len = strlen(hex);
	if (hex_len == 0)
		return (-1);
	for (i = 0; i < hex_len; i++)
	{
		if (!is_hex_char(hex[i]))
			return (-1);
	}
	*out_key_len = (hex_len + 1) / 2;
	key = (uint8_t *)malloc(*out_key_len);
	if (key == NULL)
		return (-1);
	i = 0;
	j = 0;
	if ((hex_len & 1) != 0)
	{
		key[j++] = hex_nibble(hex[i++]);
	}
	while (i < hex_len)
	{
		key[j] = (uint8_t)((hex_nibble(hex[i]) << 4) | hex_nibble(hex[i + 1]));
		i += 2;
		j++;
	}
	*out_key = key;
	return (0);
}

typedef struct s_range
{
	uint64_t	start;
	uint64_t	end;
}	t_range;

static void sort_ranges(t_range *ranges, size_t n)
{
	size_t	i;
	size_t	j;

	i = 0;
	while (i < n)
	{
		j = i + 1;
		while (j < n)
		{
			if (ranges[j].start < ranges[i].start)
			{
				t_range tmp = ranges[i];
				ranges[i] = ranges[j];
				ranges[j] = tmp;
			}
			j++;
		}
		i++;
	}
}

static void xor_encrypt_range(uint8_t *buf, uint64_t start, uint64_t end,
	const uint8_t *key, size_t key_len)
{
	uint64_t	i;

	if (buf == NULL || key == NULL || key_len == 0)
		return ;
	i = start;
	while (i < end)
	{
		buf[i] ^= key[i % key_len];
		i++;
	}
}

static void xor_encrypt_excluding_ranges(uint8_t *buf, uint64_t buf_size,
	t_range *skip, size_t skip_count, const uint8_t *key, size_t key_len)
{
	uint64_t	pos;
	size_t		i;

	if (skip_count > 1)
		sort_ranges(skip, skip_count);
	pos = 0;
	i = 0;
	while (i < skip_count && pos < buf_size)
	{
		if (skip[i].start > pos)
			xor_encrypt_range(buf, pos, skip[i].start, key, key_len);
		if (skip[i].end > pos)
			pos = skip[i].end;
		i++;
	}
	if (pos < buf_size)
		xor_encrypt_range(buf, pos, buf_size, key, key_len);
}

static int write_all(int fd, const void *buf, size_t size)
{
	const uint8_t	*p;
	size_t			written;

	p = (const uint8_t *)buf;
	written = 0;
	while (written < size)
	{
		ssize_t n = write(fd, p + written, size - written);
		if (n <= 0)
			return (-1);
		written += (size_t)n;
	}
	return (0);
}

static int validate_elf64_x86_64(const Elf64_Ehdr *ehdr, size_t file_size)
{
	if (ehdr == NULL)
		return (-1);
	if (file_size < sizeof(Elf64_Ehdr))
		return (-1);
	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
		return (-1);
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
		return (-1);
	if (ehdr->e_machine != EM_X86_64)
		return (-1);
	return (0);
}

static int should_skip_section_type(uint32_t sh_type)
{
	/*
	 * Keep metadata sections readable so tools (objdump/readelf) can parse
	 * names, symbols, notes, and versioning without errors.
	 */
	if (sh_type == SHT_STRTAB)
		return (1);
	if (sh_type == SHT_SYMTAB || sh_type == SHT_DYNSYM)
		return (1);
	if (sh_type == SHT_NOTE)
		return (1);
	if (sh_type == SHT_REL || sh_type == SHT_RELA)
		return (1);
#ifdef SHT_GNU_verneed
	if (sh_type == SHT_GNU_verneed)
		return (1);
#endif
#ifdef SHT_GNU_versym
	if (sh_type == SHT_GNU_versym)
		return (1);
#endif
	return (0);
}

int main(int argc, char *argv[])
{
	int			in_fd;
	int			out_fd;
	struct stat	st;
	off_t		file_size_off;
	size_t		file_size;
	void		*in_map;
	uint8_t		*out_buf;
	uint8_t		*key;
	size_t		key_len;
	Elf64_Ehdr	*ehdr;
	uint64_t	phdr_off;
	uint64_t	phdr_size;
	uint64_t	shdr_off;
	uint64_t	shdr_size;
	t_range		*skip_ranges;
	size_t		skip_count;
	size_t		skip_cap;
	Elf64_Shdr	*shdrs;

	if (argc != 3)
		return (1);
	if (parse_hex_key(argv[1], &key, &key_len) != 0)
		return (1);
	in_fd = open(argv[2], O_RDONLY);
	if (in_fd < 0)
	{
		free(key);
		return (1);
	}
	if (fstat(in_fd, &st) != 0)
	{
		free(key);
		close(in_fd);
		return (1);
	}
	file_size_off = lseek(in_fd, 0, SEEK_END);
	if (file_size_off <= 0)
	{
		free(key);
		close(in_fd);
		return (1);
	}
	file_size = (size_t)file_size_off;
	in_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
	if (in_map == MAP_FAILED)
	{
		free(key);
		close(in_fd);
		return (1);
	}
	ehdr = (Elf64_Ehdr *)in_map;
	if (validate_elf64_x86_64(ehdr, file_size) != 0)
	{
		free(key);
		munmap(in_map, file_size);
		close(in_fd);
		return (1);
	}
	phdr_off = ehdr->e_phoff;
	phdr_size = (uint64_t)ehdr->e_phentsize * (uint64_t)ehdr->e_phnum;
	if (!check_range(phdr_off, phdr_size, file_size))
	{
		free(key);
		munmap(in_map, file_size);
		close(in_fd);
		return (1);
	}
	out_buf = (uint8_t *)malloc(file_size);
	if (out_buf == NULL)
	{
		free(key);
		munmap(in_map, file_size);
		close(in_fd);
		return (1);
	}
	memcpy(out_buf, in_map, file_size);

	/*
	 * Skip encryption ranges:
	 * - ELF header: [0, sizeof(Elf64_Ehdr))
	 * - Program header table (PHDR): [e_phoff, e_phoff + e_phentsize*e_phnum)
	 * - Section header table (SHDR): [e_shoff, e_shoff + e_shentsize*e_shnum)
	 * - Section header string table (shstrtab) contents: [sh_offset, sh_offset+sh_size)
	 *
	 * Keeping SHDR intact makes tools like objdump/readelf able to parse the file format.
	 */
	skip_cap = (size_t)ehdr->e_shnum + 8;
	skip_ranges = (t_range *)malloc(sizeof(t_range) * skip_cap);
	if (skip_ranges == NULL)
	{
		free(out_buf);
		free(key);
		munmap(in_map, file_size);
		close(in_fd);
		return (1);
	}
	skip_count = 0;
	skip_ranges[skip_count++] = (t_range){0, sizeof(Elf64_Ehdr)};
	skip_ranges[skip_count++] = (t_range){phdr_off, phdr_off + phdr_size};

	shdr_off = ehdr->e_shoff;
	shdr_size = (uint64_t)ehdr->e_shentsize * (uint64_t)ehdr->e_shnum;
	if (check_range(shdr_off, shdr_size, file_size))
	{
		skip_ranges[skip_count++] = (t_range){shdr_off, shdr_off + shdr_size};
		shdrs = (Elf64_Shdr *)move_address(out_buf, shdr_off);
		if (ehdr->e_shstrndx != SHN_UNDEF && ehdr->e_shstrndx < ehdr->e_shnum)
		{
			Elf64_Shdr *shstr = &shdrs[ehdr->e_shstrndx];
			if (check_range(shstr->sh_offset, shstr->sh_size, file_size))
				skip_ranges[skip_count++] = (t_range){shstr->sh_offset,
					shstr->sh_offset + shstr->sh_size};
		}
		/*
		 * Also skip common metadata section contents (string tables, notes,
		 * symbol/versioning tables) so objdump -D doesn't error out.
		 */
		for (uint16_t i = 0; i < ehdr->e_shnum && skip_count < skip_cap; i++)
		{
			Elf64_Shdr *s = &shdrs[i];
			if (!should_skip_section_type(s->sh_type))
				continue;
			if (s->sh_size == 0)
				continue;
			if (!check_range(s->sh_offset, s->sh_size, file_size))
				continue;
			skip_ranges[skip_count++] = (t_range){s->sh_offset, s->sh_offset + s->sh_size};
		}
	}
	xor_encrypt_excluding_ranges(out_buf, (uint64_t)file_size,
		skip_ranges, skip_count, key, key_len);
	free(skip_ranges);

	out_fd = open("woody", O_WRONLY | O_CREAT | O_TRUNC, (mode_t)(st.st_mode & 0777));
	if (out_fd < 0)
	{
		free(out_buf);
		free(key);
		munmap(in_map, file_size);
		close(in_fd);
		return (1);
	}
	if (write_all(out_fd, out_buf, file_size) != 0)
	{
		close(out_fd);
		free(out_buf);
		free(key);
		munmap(in_map, file_size);
		close(in_fd);
		return (1);
	}
	close(out_fd);
	free(out_buf);
	free(key);
	munmap(in_map, file_size);
	close(in_fd);
	return (0);
}
