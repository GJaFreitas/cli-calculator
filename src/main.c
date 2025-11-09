#include "calc.h"
#include <ncurses.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

ENTRY_POINT(entry_point_stub) { (void)calc_data; (void)lexer; return 1; };

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
		Code = dlopen(code_bin, RTLD_LAZY);
	}
	else
	{
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
		Result.calc_func = entry_point_stub;
		dprintf(2, "[ERROR]Couldn't load code: %s\n", dlerror());
	}

	return (Result);
}

void	*
LocalAlloc(ProgramMemory *mem, uint64 size)
{
	void	*ptr = NULL;

	if (mem->memory_use + size < mem->memory_cap)
	{
		ptr = &mem->memory[mem->memory_use];
		mem->memory_use += size;
	}
	else
	{
		fprintf(stderr, "[LOG]Yikes, memory cap\n");
	}
	return (ptr);
}

int	main(int argc, char **argv)
{
	(void)argv;
	(void)argc;
	int	shouldClose = 0;
	hotcode	code;

	static Data		Data;
	static Lexer		lexer;

	local_persist ProgramMemory	perm_mem;
	local_persist ProgramMemory	transient_mem;

	lexer.mem_transient = &transient_mem;
	lexer.mem_permanent = &perm_mem;
	Data.mem_transient = &transient_mem;
	Data.mem_permanent = &perm_mem;

	perm_mem.memory_cap = sysconf(_SC_PAGESIZE);
	perm_mem.memory_use = 0;
	perm_mem.memory = (uint8 *) aligned_alloc(sysconf(_SC_PAGESIZE), perm_mem.memory_cap);

	transient_mem.memory_cap = sysconf(_SC_PAGESIZE) * 2;
	transient_mem.memory_use = 0;
	transient_mem.memory = (uint8 *) aligned_alloc(sysconf(_SC_PAGESIZE), transient_mem.memory_cap);

	Data.in_buffer = LocalAlloc(Data.mem_permanent, 512);
	Data.out_buffer = LocalAlloc(Data.mem_permanent, 512);
	Data.command_buffer = LocalAlloc(Data.mem_permanent, 512);
	lexer.input = Data.in_buffer;

	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);

	while (!shouldClose)
	{
		transient_mem.memory_use = 0;
		lexer.input = Data.in_buffer;
		lexer.mem_transient = &transient_mem;
		lexer.mem_permanent = &perm_mem;
		if (LastModified("./bin/hotcode.so") != code.lastModified)
			code = LoadCode();
		shouldClose = code.calc_func(&Data, &lexer);
		if (!shouldClose)
			memset(&lexer, 0, sizeof(lexer));
	}

	free(perm_mem.memory);
	free(transient_mem.memory);

	// endwin();
	return 0;
}
