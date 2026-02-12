#include "../include/woody.h"

int	detect_format(const t_unit *unit)
{
	if (!CHECK_RANGE(0, SELFMAG, unit->limit))
		return (ERR_FORMAT);
	if (ft_memcmp(unit->base, ELFMAG, SELFMAG) != 0)
		return (ERR_FORMAT);
	return (FORMAT_ELF);
}
