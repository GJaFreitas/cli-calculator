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

ENTRY_POINT(entry_point_stub) { (void)calc_data; (void)Lexer; return 1; };

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
LocalAlloc(program_memory *mem, uint64 size)
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
	if (argc < 2)
	{
		printf("No argv\n");
		return 1;
	}
	(void)argv;
	int	shouldClose = 0;
	hotcode	code;

	static lexer		Lexer;
	static data		Data;

	local_persist program_memory	perm_mem;
	local_persist program_memory	transient_mem;

	Lexer.mem_transient = &transient_mem;
	Lexer.mem_permanent = &perm_mem;
	Data.mem_transient = &transient_mem;
	Data.mem_permanent = &perm_mem;

	perm_mem.memory_cap = sysconf(_SC_PAGESIZE) * 2;
	perm_mem.memory_use = 0;
	perm_mem.memory = (uint8 *) aligned_alloc(sysconf(_SC_PAGESIZE), perm_mem.memory_cap);

	transient_mem.memory_cap = sysconf(_SC_PAGESIZE);
	transient_mem.memory_use = 0;
	transient_mem.memory = (uint8 *) aligned_alloc(sysconf(_SC_PAGESIZE), transient_mem.memory_cap);

	Data.in_buffer = LocalAlloc(Data.mem_permanent, 512);
	Data.out_buffer = LocalAlloc(Data.mem_permanent, 512);
	Lexer.input = Data.in_buffer;


	// For unit tests
	strcpy(Lexer.input, argv[1]);

	// initscr();
	// raw();
	// noecho();
	// keypad(stdscr, TRUE);

	while (!shouldClose)
	{
		transient_mem.memory_use = 0;
		Lexer.input = Data.in_buffer;
		Lexer.mem_transient = &transient_mem;
		Lexer.mem_permanent = &perm_mem;
		if (LastModified("./bin/hotcode.so") != code.lastModified)
			code = LoadCode();
		shouldClose = code.calc_func(&Data, &Lexer);
		if (!shouldClose)
			memset(&Lexer, 0, sizeof(lexer));
	}

	free(perm_mem.memory);
	free(transient_mem.memory);

	// endwin();
	return 0;
}
