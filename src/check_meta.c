#include "../include/woody.h"

static int	convert_pflags_to_prot(uint32_t p_flags)
{
	int	prot;

	prot = 0;
	if (p_flags & PF_R)
		prot |= PROT_READ;
	if (p_flags & PF_W)
		prot |= PROT_WRITE;
	if (p_flags & PF_X)
		prot |= PROT_EXEC;
	return (prot);
}

static size_t	count_valid_loads_32(const t_unit *unit)
{
	Elf32_Phdr	*phdr;
	uint16_t	phnum;
	size_t		n;
	uint16_t	i;

	phnum = unit->ElfN_Ehdr.Ehdr32->e_phnum;
	n = 0;
	i = 0;
	while (i < phnum)
	{
		phdr = &unit->ElfN_Phdr.Phdr32[i];
		if (phdr->p_type == PT_LOAD && phdr->p_offset != 0
			&& phdr->p_filesz > 0
			&& CHECK_RANGE(phdr->p_offset, phdr->p_filesz, unit->limit))
			n++;
		i++;
	}
	return (n);
}

static size_t	count_valid_loads_64(const t_unit *unit)
{
	Elf64_Phdr	*phdr;
	uint16_t	phnum;
	size_t		n;
	uint16_t	i;

	phnum = unit->ElfN_Ehdr.Ehdr64->e_phnum;
	n = 0;
	i = 0;
	while (i < phnum)
	{
		phdr = &unit->ElfN_Phdr.Phdr64[i];
		if (phdr->p_type == PT_LOAD && phdr->p_offset != 0
			&& phdr->p_filesz > 0
			&& CHECK_RANGE(phdr->p_offset, phdr->p_filesz, unit->limit))
			n++;
		i++;
	}
	return (n);
}

static int	fill_arrays_32(const t_unit *unit,
	t_encryption *enc, t_meta *meta, size_t *idx)
{
	Elf32_Phdr	*phdr;
	uint16_t	phnum;
	uint16_t	i;

	phnum = unit->ElfN_Ehdr.Ehdr32->e_phnum;
	*idx = 0;
	i = 0;
	while (i < phnum)
	{
		phdr = &unit->ElfN_Phdr.Phdr32[i];
		if (phdr->p_type == PT_LOAD && phdr->p_offset != 0
			&& phdr->p_filesz > 0
			&& CHECK_RANGE(phdr->p_offset, phdr->p_filesz, unit->limit)
			&& (phdr->p_flags & (PF_R | PF_X)) == (PF_R | PF_X))
		{
			enc[*idx].p_offset = (uint64_t)phdr->p_offset;
			enc[*idx].p_filesize = (uint64_t)phdr->p_filesz;
			enc[*idx].p_vaddr = (uint64_t)phdr->p_vaddr;
			enc[*idx].phdr_index = (size_t)i;
			enc[*idx].in_stub = 0;
			meta[*idx].p_vaddr = (uint64_t)phdr->p_vaddr;
			meta[*idx].p_memsz = (uint64_t)phdr->p_memsz;
			meta[*idx].p_reverse_flags
				= (uint64_t)convert_pflags_to_prot(phdr->p_flags);
			(*idx)++;
		}
		i++;
	}
	return (OK);
}

static int	fill_arrays_64(const t_unit *unit,
	t_encryption *enc, t_meta *meta, size_t *idx)
{
	Elf64_Phdr	*phdr;
	uint16_t	phnum;
	uint16_t	i;

	phnum = unit->ElfN_Ehdr.Ehdr64->e_phnum;
	*idx = 0;
	i = 0;
	while (i < phnum)
	{
		phdr = &unit->ElfN_Phdr.Phdr64[i];
		if (phdr->p_type == PT_LOAD && phdr->p_offset != 0
			&& phdr->p_filesz > 0
			&& CHECK_RANGE(phdr->p_offset, phdr->p_filesz, unit->limit)
			&& (phdr->p_flags & (PF_R | PF_X)) == (PF_R | PF_X))
		{
			enc[*idx].p_offset = phdr->p_offset;
			enc[*idx].p_filesize = phdr->p_filesz;
			enc[*idx].p_vaddr = phdr->p_vaddr;
			enc[*idx].phdr_index = (size_t)i;
			enc[*idx].in_stub = 0;
			meta[*idx].p_vaddr = phdr->p_vaddr;
			meta[*idx].p_memsz = phdr->p_memsz;
			meta[*idx].p_reverse_flags
				= (uint64_t)convert_pflags_to_prot(phdr->p_flags);
			(*idx)++;
		}
		i++;
	}
	return (OK);
}

static uint64_t	calc_required_size(uint8_t elf_class, size_t n)
{
	uint64_t	sc_len;

	if (elf_class == ELFCLASS32)
		sc_len = 352;
	else
		sc_len = 496;
	return (sc_len + 16 + 32 + (uint64_t)(n * 24));
}

static int	find_stub_segment(const t_unit *unit,
	t_encryption *enc, size_t count, size_t *stub_index)
{
	uint64_t	required;
	uint64_t	avail;
	uint64_t	p_memsz;
	uint64_t	p_filesz;
	size_t		i;
	uint32_t	flags;

	required = calc_required_size(unit->elf_class, count);
	i = 0;
	while (i < count)
	{
		if (unit->elf_class == ELFCLASS32)
		{
			p_memsz = (uint64_t)unit->ElfN_Phdr.Phdr32[enc[i].phdr_index].p_memsz;
			p_filesz = (uint64_t)unit->ElfN_Phdr.Phdr32[enc[i].phdr_index].p_filesz;
			flags = unit->ElfN_Phdr.Phdr32[enc[i].phdr_index].p_flags;
		}
		else
		{
			p_memsz = unit->ElfN_Phdr.Phdr64[enc[i].phdr_index].p_memsz;
			p_filesz = unit->ElfN_Phdr.Phdr64[enc[i].phdr_index].p_filesz;
			flags = unit->ElfN_Phdr.Phdr64[enc[i].phdr_index].p_flags;
		}
		avail = ((p_memsz + 4095) & ~((uint64_t)0xfff)) - p_filesz;
		if ((flags & (PF_R | PF_X)) == (PF_R | PF_X) && avail >= required)
		{
			enc[i].in_stub = 1;
			*stub_index = i;
			return (OK);
		}
		i++;
	}
	return (ERR_NO_STUB_SPACE);
}

int	collect_encryption_segments(const t_unit *unit,
	t_encryption **enc_array, t_meta **meta_array,
	size_t *count, size_t *stub_index)
{
	size_t	n;
	int		err;

	if (unit->elf_class == ELFCLASS32)
		n = count_valid_loads_32(unit);
	else
		n = count_valid_loads_64(unit);
	if (n == 0)
		return (ERR_NO_LOAD_SEGMENT);
	*enc_array = malloc(sizeof(t_encryption) * n);
	if (*enc_array == NULL)
		return (ERR_MALLOC);
	*meta_array = malloc(sizeof(t_meta) * n);
	if (*meta_array == NULL)
	{
		free(*enc_array);
		*enc_array = NULL;
		return (ERR_MALLOC);
	}
	if (unit->elf_class == ELFCLASS32)
		fill_arrays_32(unit, *enc_array, *meta_array, count);
	else
		fill_arrays_64(unit, *enc_array, *meta_array, count);
	err = find_stub_segment(unit, *enc_array, *count, stub_index);
	if (err != OK)
	{
		free(*enc_array);
		*enc_array = NULL;
		free(*meta_array);
		*meta_array = NULL;
		return (err);
	}
	return (OK);
}
