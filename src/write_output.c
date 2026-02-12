#include "../include/woody.h"

static int	write_all(int fd, const void *buf, size_t len)
{
	size_t	written;
	ssize_t	ret;

	written = 0;
	while (written < len)
	{
		ret = write(fd, (const uint8_t *)buf + written, len - written);
		if (ret <= 0)
			return (ERR_WRITE);
		written += (size_t)ret;
	}
	return (OK);
}

static int	patch_entry_32(int fd, uint64_t entry)
{
	uint32_t	v;
	off_t		off;

	if (entry > UINT32_MAX)
		return (ERR_RANGE);
	v = (uint32_t)entry;
	off = lseek(fd, offsetof(Elf32_Ehdr, e_entry), SEEK_SET);
	if (off < 0)
		return (ERR_LSEEK);
	return (write_all(fd, &v, sizeof(v)));
}

static int	patch_entry_64(int fd, uint64_t entry)
{
	off_t	off;

	off = lseek(fd, offsetof(Elf64_Ehdr, e_entry), SEEK_SET);
	if (off < 0)
		return (ERR_LSEEK);
	return (write_all(fd, &entry, sizeof(entry)));
}

static int	patch_phdr_32(int fd, const t_unit *unit,
	const t_encryption *stub_enc, size_t payload_size)
{
	Elf32_Ehdr	*ehdr;
	uint64_t	entsize;
	uint64_t	offset;
	Elf32_Phdr	*src;
	Elf32_Phdr	phdr;
	off_t		seek;

	ehdr = unit->ElfN_Ehdr.Ehdr32;
	entsize = ehdr->e_phentsize;
	if (entsize == 0)
		entsize = sizeof(Elf32_Phdr);
	offset = ehdr->e_phoff + ((uint64_t)stub_enc->phdr_index * entsize);
	if (!CHECK_RANGE(offset, sizeof(Elf32_Phdr), unit->limit))
		return (ERR_RANGE);
	src = (Elf32_Phdr *)MOVE_ADDRESS_CONST(unit->base, offset);
	phdr = *src;
	if ((uint64_t)phdr.p_filesz + payload_size > UINT32_MAX)
		return (ERR_RANGE);
	if ((uint64_t)phdr.p_memsz + payload_size > UINT32_MAX)
		return (ERR_RANGE);
	phdr.p_filesz = (uint32_t)((uint64_t)phdr.p_filesz + payload_size);
	phdr.p_memsz = (uint32_t)((uint64_t)phdr.p_memsz + payload_size);
	seek = lseek(fd, (off_t)offset, SEEK_SET);
	if (seek < 0)
		return (ERR_LSEEK);
	return (write_all(fd, &phdr, sizeof(phdr)));
}

static int	patch_phdr_64(int fd, const t_unit *unit,
	const t_encryption *stub_enc, size_t payload_size)
{
	Elf64_Ehdr	*ehdr;
	uint64_t	entsize;
	uint64_t	offset;
	Elf64_Phdr	*src;
	Elf64_Phdr	phdr;
	off_t		seek;

	ehdr = unit->ElfN_Ehdr.Ehdr64;
	entsize = ehdr->e_phentsize;
	if (entsize == 0)
		entsize = sizeof(Elf64_Phdr);
	offset = ehdr->e_phoff + ((uint64_t)stub_enc->phdr_index * entsize);
	if (!CHECK_RANGE(offset, sizeof(Elf64_Phdr), unit->limit))
		return (ERR_RANGE);
	src = (Elf64_Phdr *)MOVE_ADDRESS_CONST(unit->base, offset);
	phdr = *src;
	phdr.p_filesz += payload_size;
	phdr.p_memsz += payload_size;
	seek = lseek(fd, (off_t)offset, SEEK_SET);
	if (seek < 0)
		return (ERR_LSEEK);
	return (write_all(fd, &phdr, sizeof(phdr)));
}

int	write_woody_file(const t_unit *unit,
	const uint8_t *payload,
	size_t payload_size,
	const t_encryption *stub_enc)
{
	int		fd;
	uint64_t	new_entry;
	off_t		seek;
	int		err;

	if (unit == NULL || payload == NULL || stub_enc == NULL)
		return (ERR_USAGE);
	fd = open("woody", O_CREAT | O_WRONLY | O_TRUNC, 0755);
	if (fd < 0)
		return (ERR_OPEN);
	err = write_all(fd, unit->base, (size_t)unit->limit);
	if (err != OK)
		return (close(fd), err);
	new_entry = stub_enc->p_vaddr + stub_enc->p_filesize;
	if (unit->elf_class == ELFCLASS32)
		err = patch_entry_32(fd, new_entry);
	else
		err = patch_entry_64(fd, new_entry);
	if (err != OK)
		return (close(fd), err);
	if (unit->elf_class == ELFCLASS32)
		err = patch_phdr_32(fd, unit, stub_enc, payload_size);
	else
		err = patch_phdr_64(fd, unit, stub_enc, payload_size);
	if (err != OK)
		return (close(fd), err);
	seek = lseek(fd, (off_t)(stub_enc->p_offset + stub_enc->p_filesize), SEEK_SET);
	if (seek < 0)
		return (close(fd), ERR_LSEEK);
	err = write_all(fd, payload, payload_size);
	if (err != OK)
		return (close(fd), err);
	if (close(fd) < 0)
		return (ERR_WRITE);
	return (OK);
}
