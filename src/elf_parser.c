#include "../include/woody.h"

static int	validate_class_and_endian(t_unit *unit)
{
	unsigned char	ei_class;
	unsigned char	ei_data;

	ei_class = unit->base[EI_CLASS];
	if (ei_class != ELFCLASS32 && ei_class != ELFCLASS64)
		return (ERR_INVALID_CLASS);
	unit->elf_class = ei_class;
	ei_data = unit->base[EI_DATA];
	if (ei_data != ELFDATA2LSB)
		return (ERR_INVALID_ENDIAN);
	return (OK);
}

static int	validate_type_and_machine(const t_unit *unit)
{
	uint16_t	e_type;
	uint16_t	e_machine;

	if (unit->elf_class == ELFCLASS32)
	{
		e_type = unit->ElfN_Ehdr.Ehdr32->e_type;
		e_machine = unit->ElfN_Ehdr.Ehdr32->e_machine;
	}
	else
	{
		e_type = unit->ElfN_Ehdr.Ehdr64->e_type;
		e_machine = unit->ElfN_Ehdr.Ehdr64->e_machine;
	}
	if (e_type != ET_EXEC && e_type != ET_DYN)
		return (ERR_INVALID_TYPE);
	if (unit->elf_class == ELFCLASS32 && e_machine != EM_386)
		return (ERR_INVALID_MACHINE);
	if (unit->elf_class == ELFCLASS64 && e_machine != EM_X86_64)
		return (ERR_INVALID_MACHINE);
	return (OK);
}

static int	validate_phdr_table_32(t_unit *unit)
{
	Elf32_Ehdr	*ehdr;
	uint64_t	phentsize;
	uint64_t	total;

	ehdr = unit->ElfN_Ehdr.Ehdr32;
	if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
		return (ERR_INVALID_PHDR);
	if (!(ehdr->e_phentsize == sizeof(Elf32_Phdr)))
		return (ERR_INVALID_PHDR);
	phentsize = ehdr->e_phentsize;
	if (phentsize == 0)
		phentsize = sizeof(Elf32_Phdr);
	total = (uint64_t)ehdr->e_phnum * phentsize;
	if (!CHECK_RANGE((uint64_t)ehdr->e_phoff, total, unit->limit))
		return (ERR_INVALID_PHDR);
	unit->ElfN_Phdr.Phdr32 = (Elf32_Phdr *)MOVE_ADDRESS(
			unit->base, (uint64_t)ehdr->e_phoff);
	return (OK);
}

static int	validate_phdr_table_64(t_unit *unit)
{
	Elf64_Ehdr	*ehdr;
	uint64_t	phentsize;
	uint64_t	total;

	ehdr = unit->ElfN_Ehdr.Ehdr64;
	if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
		return (ERR_INVALID_PHDR);
	if (!(ehdr->e_phentsize == sizeof(Elf64_Phdr)))
		return (ERR_INVALID_PHDR);
	phentsize = ehdr->e_phentsize;
	if (phentsize == 0)
		phentsize = sizeof(Elf64_Phdr);
	total = (uint64_t)ehdr->e_phnum * phentsize;
	if (!CHECK_RANGE(ehdr->e_phoff, total, unit->limit))
		return (ERR_INVALID_PHDR);
	unit->ElfN_Phdr.Phdr64 = (Elf64_Phdr *)MOVE_ADDRESS(
			unit->base, ehdr->e_phoff);
	return (OK);
}

/* A shared object can have both a nonzero entry and PT_INTERP (e.g. libc).
 * Require the executable's DF_1_PIE marker, including for static PIE. */
static int	has_pie_flag(const t_unit *unit, uint64_t offset, uint64_t size)
{
	Elf32_Dyn	dyn32;
	Elf64_Dyn	dyn64;
	uint64_t	step;
	int64_t		tag;
	uint64_t	value;
	int			pie;

	step = sizeof(dyn64);
	if (unit->elf_class == ELFCLASS32)
		step = sizeof(dyn32);
	if (!CHECK_RANGE(offset, size, unit->limit) || size % step != 0)
		return (ERR_INVALID_PHDR);
	pie = 0;
	while (size >= step)
	{
		if (unit->elf_class == ELFCLASS32)
		{
			ft_memcpy(&dyn32, unit->base + offset, sizeof(dyn32));
			tag = dyn32.d_tag;
			value = dyn32.d_un.d_val;
		}
		else
		{
			ft_memcpy(&dyn64, unit->base + offset, sizeof(dyn64));
			tag = dyn64.d_tag;
			value = dyn64.d_un.d_val;
		}
		if (tag == DT_NULL)
			return (pie ? OK : ERR_NOT_PIE);
		if (tag == DT_FLAGS_1 && (value & DF_1_PIE))
			pie = 1;
		offset += step;
		size -= step;
	}
	return (ERR_INVALID_PHDR);
}

static int	validate_executable(const t_unit *unit)
{
	uint16_t	i;
	Elf32_Phdr	*ph32;
	Elf64_Phdr	*ph64;

	if (unit->elf_class == ELFCLASS32)
	{
		if (unit->ElfN_Ehdr.Ehdr32->e_type == ET_EXEC)
			return (OK);
		for (i = 0; i < unit->ElfN_Ehdr.Ehdr32->e_phnum; i++)
		{
			ph32 = &unit->ElfN_Phdr.Phdr32[i];
			if (ph32->p_type == PT_DYNAMIC)
				return (has_pie_flag(unit, ph32->p_offset, ph32->p_filesz));
		}
	}
	else
	{
		if (unit->ElfN_Ehdr.Ehdr64->e_type == ET_EXEC)
			return (OK);
		for (i = 0; i < unit->ElfN_Ehdr.Ehdr64->e_phnum; i++)
		{
			ph64 = &unit->ElfN_Phdr.Phdr64[i];
			if (ph64->p_type == PT_DYNAMIC)
				return (has_pie_flag(unit, ph64->p_offset, ph64->p_filesz));
		}
	}
	return (ERR_NOT_PIE);
}

int	parse_elf_header(t_unit *unit)
{
	int	err;

	if (!CHECK_RANGE(0, sizeof(Elf64_Ehdr), unit->limit))
		return (ERR_INVALID_PHDR);
	err = validate_class_and_endian(unit);
	if (err != OK)
		return (err);
	if (unit->elf_class == ELFCLASS32)
		unit->ElfN_Ehdr.Ehdr32 = (Elf32_Ehdr *)unit->base;
	else
		unit->ElfN_Ehdr.Ehdr64 = (Elf64_Ehdr *)unit->base;
	err = validate_type_and_machine(unit);
	if (err != OK)
		return (err);
	if (unit->elf_class == ELFCLASS32)
		err = validate_phdr_table_32(unit);
	else
		err = validate_phdr_table_64(unit);
	if (err != OK)
		return (err);
	return (validate_executable(unit));
}
