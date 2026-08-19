#include <dlfcn.h>
#include <stdio.h>

typedef int (*shared_function)(int);

int main(int argc, char **argv)
{
	void *handle;
	shared_function function;
	const char *error;

	if (argc != 2)
		return (2);
	handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (handle == NULL)
	{
		fprintf(stderr, "dlopen:%s\n", dlerror());
		return (3);
	}
	dlerror();
	*(void **)(&function) = dlsym(handle, "shared_exported_function");
	error = dlerror();
	if (error != NULL)
	{
		fprintf(stderr, "dlsym:%s\n", error);
		dlclose(handle);
		return (4);
	}
	printf("shared-result:%d\n", function(5));
	dlclose(handle);
	return (0);
}
