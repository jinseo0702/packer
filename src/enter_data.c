#include "../include/woody.h"

static void	serialize_u64(uint8_t *dst, uint64_t val)
{
	ft_memcpy(dst, &val, sizeof(uint64_t));
}

static size_t	calc_payload_size(size_t sc_len, size_t meta_count)
{
	return (sc_len + 16 + 32 + (meta_count * 24));
}

static void	fill_marker(uint8_t *buf)
{
	static const char	marker[16] = "....WOODY....\n";

	ft_memcpy(buf, marker, 16);
}

static void	fill_metadata(uint8_t *buf, const t_unit *unit,
	const t_encryption *stub_enc, const t_meta *meta_array,
	size_t meta_count, uint64_t key)
{
	uint64_t	real_entry;
	size_t		off;
	size_t		i;
	uint64_t	new_entry;

	if (unit->elf_class == ELFCLASS32)
		real_entry = (uint64_t)unit->ElfN_Ehdr.Ehdr32->e_entry;
	else
		real_entry = unit->ElfN_Ehdr.Ehdr64->e_entry;
	off = 0;
	serialize_u64(buf + off, real_entry);
	off += 8;
	new_entry = stub_enc->p_vaddr + stub_enc->p_filesize;
	serialize_u64(buf + off, new_entry);
	off += 8;
	serialize_u64(buf + off, key);
	off += 8;
	serialize_u64(buf + off, (uint64_t)meta_count);
	off += 8;
	i = 0;
	while (i < meta_count)
	{
		serialize_u64(buf + off, meta_array[i].p_vaddr);
		off += 8;
		serialize_u64(buf + off, meta_array[i].p_memsz);
		off += 8;
		serialize_u64(buf + off, meta_array[i].p_reverse_flags);
		off += 8;
		i++;
	}
}

int	prepare_payload(uint8_t **payload, size_t *payload_size,
	const t_unit *unit, const t_encryption *stub_enc,
	const t_meta *meta_array, size_t meta_count, uint64_t key)
{
	const uint8_t	*sc_code;
	size_t			sc_len;
	size_t			total;
	size_t			off;
	int				err;

	if (payload == NULL || payload_size == NULL)
		return (ERR_USAGE);
	err = get_shellcode(unit->elf_class, &sc_code, &sc_len);
	if (err != OK)
		return (err);
	total = calc_payload_size(sc_len, meta_count);
	*payload = malloc(total);
	if (*payload == NULL)
		return (ERR_MALLOC);
	ft_memset(*payload, 0, total);
	off = 0;
	ft_memcpy(*payload + off, sc_code, sc_len);
	off += sc_len;
	fill_marker(*payload + off);
	off += 16;
	fill_metadata(*payload + off, unit, stub_enc,
		meta_array, meta_count, key);
	*payload_size = total;
	return (OK);
}
