#ifndef WOODY_H
# define WOODY_H

# include <elf.h>
# include <errno.h>
# include <fcntl.h>
# include <stddef.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <sys/mman.h>
# include <stdio.h>
# include <unistd.h>
# include "../libft/libft.h"
# include "../ft_printf/libftprintf.h"

# define ERROR_LIST \
	X(OK, "Success") \
	X(ERR_MALLOC, "Memory allocation failed") \
	X(ERR_OPEN, "File open failed") \
	X(ERR_USAGE, "Usage: ./woody_woodpacker <elf_file> [hex_key]") \
	X(ERR_KEY_FORMAT, "Invalid key format") \
	X(ERR_KEY_GEN, "Failed to generate random key") \
	X(ERR_READ, "Read operation failed") \
	X(ERR_FORMAT, "Unknown file format") \
	X(ERR_LSEEK, "Lseek failed") \
	X(ERR_MMAP, "Mmap failed") \
	X(ERR_EMPTY_FILE, "File is empty") \
	X(FORMAT_ELF, "ELF format detected") \
	X(ERR_INVALID_CLASS, "Invalid ELF class") \
	X(ERR_INVALID_ENDIAN, "Only little-endian supported") \
	X(ERR_INVALID_TYPE, "Invalid ELF type (must be ET_EXEC or ET_DYN)") \
	X(ERR_INVALID_MACHINE, "Invalid machine type") \
	X(ERR_INVALID_PHDR, "Invalid program header table") \
	X(ERR_NO_LOAD_SEGMENT, "No PT_LOAD segment found") \
	X(ERR_NO_STUB_SPACE, "No space for shellcode injection") \
	X(ERR_RANGE, "Range check failed") \
	X(ERR_WRITE, "Write operation failed") \
	X(ERR_UNLINK, "Failed to remove output file")

typedef enum e_error {
# define X(id, str) id,
	ERROR_LIST
# undef X
	ERROR_END
}	t_error;

typedef struct s_unit{
	unsigned char		*base;
	uint64_t			limit;
	uint64_t			key;
	union {
		Elf32_Ehdr		*Ehdr32;
		Elf64_Ehdr		*Ehdr64;
	}					ElfN_Ehdr;
	union {
		Elf32_Phdr		*Phdr32;
		Elf64_Phdr		*Phdr64;
	}					ElfN_Phdr;
	int					fd;
	uint8_t				elf_class;
}	t_unit;

typedef struct s_encryption{
	uint64_t	p_offset;
	uint64_t	p_filesize;
	uint64_t	p_vaddr;
	size_t		phdr_index;
	uint8_t		in_stub;
}	t_encryption;

typedef struct s_meta{
	uint64_t	p_vaddr;
	uint64_t	p_memsz;
	uint64_t	p_reverse_flags;
}	t_meta;

static inline void	*MOVE_ADDRESS(void *base, uint64_t offset)
{
	unsigned char	*p;

	if (base == NULL)
		return (NULL);
	p = (unsigned char *)base;
	return ((void *)(p + offset));
}

static inline const void	*MOVE_ADDRESS_CONST(const void *base,
		uint64_t offset)
{
	const unsigned char	*p;

	if (base == NULL)
		return (NULL);
	p = (const unsigned char *)base;
	return ((const void *)(p + offset));
}

static inline uint8_t	CHECK_RANGE(uint64_t offset, uint64_t size,
		uint64_t limit)
{
	if (offset > limit)
		return (0);
	return (size <= (limit - offset));
}

const char	*get_error_msg(t_error err);

# ifdef DEBUG

#  define NM_LOG(err) real_print_error(err, __FILE__, __LINE__)
# else
#  define NM_LOG(err) real_print_error(err, NULL, 0)
# endif

static inline void	real_print_error(t_error err,
		const char *file, int line)
{
	ft_fprintf(2, "woody_packer: [%s] -> %s\n",
		get_error_msg(err), strerror(errno));
	if (file != NULL)
		ft_fprintf(2, "file: %s line: %d\n", file, line);
}

int		parse_args(int argc, char **argv,
			const char **path, uint64_t *key);
int		create_unit_from_path(const char *path, t_unit *unit);
void	destroy_unit(t_unit *unit);
int		detect_format(const t_unit *unit);
int		parse_elf_header(t_unit *unit);
int		collect_encryption_segments(const t_unit *unit,
			t_encryption **enc_array, t_meta **meta_array,
			size_t *count, size_t *stub_index);
int		encrypt_segment(unsigned char *base, uint64_t limit,
			const t_encryption *enc, uint64_t key);
int		encrypt_segment_32(unsigned char *base, uint64_t limit,
			const t_encryption *enc, uint64_t key);
int		get_shellcode(uint8_t elf_class,
			const uint8_t **code, size_t *len);
uint64_t	get_shellcode_size(uint8_t elf_class);
int		prepare_payload(uint8_t **payload, size_t *payload_size,
			const t_unit *unit, const t_encryption *stub_enc,
			const t_meta *meta_array, size_t meta_count,
			uint64_t key);
int		write_woody_file(const t_unit *unit,
			const uint8_t *payload, size_t payload_size,
			const t_encryption *stub_enc);

#endif
