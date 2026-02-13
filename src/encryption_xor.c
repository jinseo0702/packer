#include "../include/woody.h"

int	encrypt_segment(unsigned char *base, uint64_t limit,
	const t_encryption *enc, uint64_t key)
{
	unsigned char	*ptr;
	uint8_t			*key_bytes;
	uint64_t		i;

	if (enc->p_filesize == 0)
		return (OK);
	if (!CHECK_RANGE(enc->p_offset, enc->p_filesize, limit))
		return (ERR_RANGE);
	ptr = base + enc->p_offset;
	i = 0;
	while (i + 8 <= enc->p_filesize)
	{
		*(uint64_t *)(ptr + i) ^= key;
		i += 8;
	}
	key_bytes = (uint8_t *)&key;
	while (i < enc->p_filesize)
	{
		ptr[i] ^= *(unsigned char *)(key_bytes);
		i++;
	}
	return (OK);
}

int	encrypt_segment_32(unsigned char *base, uint64_t limit,
	const t_encryption *enc, uint64_t key)
{
	unsigned char	*ptr;
	uint8_t			*key_bytes;
	uint32_t		i;

	if (enc->p_filesize == 0)
		return (OK);
	if (!CHECK_RANGE(enc->p_offset, enc->p_filesize, limit))
		return (ERR_RANGE);
	ptr = base + enc->p_offset;
	i = 0;
	while (i + 4 <= enc->p_filesize)
	{
		*(uint32_t *)(ptr + i) ^= key;
		i += 4;
	}
	key_bytes = (uint8_t *)&key;
	while (i < enc->p_filesize)
	{
		ptr[i] ^= *(unsigned char *)(key_bytes);
		i++;
	}
	return (OK);
}