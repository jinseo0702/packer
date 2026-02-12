#include "../include/woody.h"

static const char	*g_error_table[] = {
#define X(id, str) [id] = str,
	ERROR_LIST
#undef X
};

const char	*get_error_msg(t_error err)
{
	if (err < 0 || err >= ERROR_END)
		return ("Unknown error");
	if (g_error_table[err] == NULL)
		return ("Unknown error");
	return (g_error_table[err]);
}
