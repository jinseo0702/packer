#include "../include/woody.h"

static void	cleanup_all(t_unit *unit,
	t_encryption **enc_array,
	t_meta **meta_array,
	uint8_t **payload)
{
	if (payload != NULL && *payload != NULL)
	{
		free(*payload);
		*payload = NULL;
	}
	if (meta_array != NULL && *meta_array != NULL)
	{
		free(*meta_array);
		*meta_array = NULL;
	}
	if (enc_array != NULL && *enc_array != NULL)
	{
		free(*enc_array);
		*enc_array = NULL;
	}
	if (unit != NULL)
		destroy_unit(unit);
}

int	main(int argc, char **argv)
{
	t_unit			unit;
	t_encryption	*enc_array;
	t_meta			*meta_array;
	uint8_t			*payload;
	size_t			payload_size;
	size_t			meta_count;
	size_t			stub_index;
	size_t			i;
	const char		*path;
	int				err;

	ft_memset(&unit, 0, sizeof(unit));
	enc_array = NULL;
	meta_array = NULL;
	payload = NULL;
	err = parse_args(argc, argv, &path, &unit.key);
	if (err != OK)
		return (NM_LOG(err), 1);
	err = create_unit_from_path(path, &unit);
	if (err != OK)
		return (NM_LOG(err), 1);
	err = detect_format(&unit);
	if (err != FORMAT_ELF)
		return (cleanup_all(&unit, &enc_array, &meta_array, &payload), NM_LOG(ERR_FORMAT), 1);
	err = parse_elf_header(&unit);
	if (err != OK)
		return (cleanup_all(&unit, &enc_array, &meta_array, &payload), NM_LOG(err), 1);
	err = collect_encryption_segments(&unit, &enc_array, &meta_array, &meta_count, &stub_index);
	if (err != OK)
		return (cleanup_all(&unit, &enc_array, &meta_array, &payload), NM_LOG(err), 1);
	i = 0;
	while (i < meta_count)
	{
		err = encrypt_segment(unit.base, unit.limit, &enc_array[i], unit.key);
		if (err != OK)
			return (cleanup_all(&unit, &enc_array, &meta_array, &payload), NM_LOG(err), 1);
		i++;
	}
	err = prepare_payload(&payload, &payload_size, &unit,
		&enc_array[stub_index], meta_array, meta_count, unit.key);
	if (err != OK)
		return (cleanup_all(&unit, &enc_array, &meta_array, &payload), NM_LOG(err), 1);
	err = write_woody_file(&unit, payload, payload_size, &enc_array[stub_index]);
	if (err != OK)
		return (cleanup_all(&unit, &enc_array, &meta_array, &payload), NM_LOG(err), 1);
	cleanup_all(&unit, &enc_array, &meta_array, &payload);
	return (0);
}
