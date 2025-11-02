#include "calc.h"
#include <ncurses.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

/*

	Immediate mode io:
		Write a result to the console as soon as it is possible.
		Ex:
			> 1 *
			syntax error

			> 1 * 1
			1

		No enter required

		Ops: hex() bin() basic arithmetic ...

*/

typedef struct
{
	entrypoint	*calc_func;
	time_t		lastModified;
	bool		isValid;
}	hotcode;

ENTRY_POINT(entry_point_stub) { (void)calc_data; return 1; };

internal time_t
LastModified(const char *filepath)
{
	struct stat	file;

	stat(filepath, &file);
	return (file.st_mtim.tv_sec);
}

internal hotcode
LoadCode(void)
{
	local_persist void *Code;
	hotcode Result;
	const char code_bin[] = "./bin/hotcode.so";

	if (!Code)
	{
		printf("[LOG]No code yet loaded\n");
		Code = dlopen(code_bin, RTLD_LAZY);
	}
	else
	{
		printf("[LOG]Code already loaded, reloading\n");
		dlclose(Code);
		Code = dlopen(code_bin, RTLD_LAZY);
	}

	if (Code)
	{
		Result.lastModified = LastModified(code_bin);
		Result.calc_func = (entrypoint *)dlsym(Code, "calc");

		Result.isValid = Result.calc_func;
	}

	if (!Result.isValid)
	{
		dprintf(2, "[ERROR]Couldn't load code: %s\n", dlerror());
		Result.calc_func = entry_point_stub;
	}

	return (Result);
}

int	main(int argc, char **argv)
{
	// Parse argv maybe make a -h command
	(void)argc;
	(void)argv;
	int	shouldClose = 0;
	hotcode	code;
	data		stuff;

	stuff.mem.memory_cap = Kilobytes(16);
	stuff.mem.memory_use = 0;
	stuff.mem.memory = (uint8 *) aligned_alloc(sysconf(_SC_PAGESIZE)), stuff.mem.memory_cap);

	initscr();
	raw();
	echo(); // TODO: Is this what i want?
	keypad(stdscr, TRUE);

	while (!shouldClose)
	{
		if (LastModified("./bin/hotcode.so") != code.lastModified)
			code = LoadCode();
		shouldClose = code.calc_func(&stuff);
	}

	endwin();
	return 0;
}
