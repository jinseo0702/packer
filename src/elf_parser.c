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
		return (validate_phdr_table_32(unit));
	return (validate_phdr_table_64(unit));
}
