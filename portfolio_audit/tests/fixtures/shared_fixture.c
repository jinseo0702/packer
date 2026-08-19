int shared_exported_data = 31;
static int shared_local_data = 37;
extern int shared_weak_undefined_function(int value) __attribute__((weak));

__attribute__((visibility("hidden"))) int shared_hidden_function(int value)
{
	return (value + shared_local_data);
}

__attribute__((weak)) int shared_weak_function(int value)
{
	return (value + shared_exported_data);
}

int shared_exported_function(int value)
{
	if (shared_weak_undefined_function != 0)
		value += shared_weak_undefined_function(value);
	return (shared_hidden_function(value) + shared_weak_function(value));
}
