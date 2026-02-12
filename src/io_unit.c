#include "../include/woody.h"

int	create_unit_from_path(const char *path, t_unit *unit)
{
	off_t	file_size;

	unit->fd = open(path, O_RDONLY);
	if (unit->fd < 0)
		return (ERR_OPEN);
	file_size = lseek(unit->fd, 0, SEEK_END);
	if (file_size < 0)
	{
		close(unit->fd);
		unit->fd = -1;
		return (ERR_LSEEK);
	}
	if (file_size == 0)
	{
		close(unit->fd);
		unit->fd = -1;
		return (ERR_EMPTY_FILE);
	}
	unit->limit = (uint64_t)file_size;
	unit->base = mmap(NULL, (size_t)unit->limit,
			PROT_READ | PROT_WRITE, MAP_PRIVATE, unit->fd, 0);
	close(unit->fd);
	unit->fd = -1;
	if (unit->base == MAP_FAILED)
	{
		unit->base = NULL;
		return (ERR_MMAP);
	}
	return (OK);
}

void	destroy_unit(t_unit *unit)
{
	if (unit == NULL)
		return ;
	if (unit->base != NULL && unit->base != MAP_FAILED)
	{
		munmap(unit->base, (size_t)unit->limit);
		unit->base = NULL;
	}
	if (unit->fd >= 0)
	{
		close(unit->fd);
		unit->fd = -1;
	}
}
