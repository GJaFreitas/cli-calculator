#pragma once

#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ncurses.h>

#define Kilobytes(x) (x * 1024)
#define Megabytes(x) (Kilobytes(x) * 1024)
#define Gigabytes(x) (Megabytes(x) * 1024)

#define global_var static
#define internal static
#define local_persist static

#define real32 float
#define real64 double

#define uint8	uint8_t
#define uint32	uint32_t
#define uint64	uint64_t
#define int8	int8_t
#define int32	int32_t
#define int64	int64_t

// NOTE: If in debug mode test that thing, if not dont
#ifdef DEBUG
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 1;}
#else
#define Assert(Expression)
#endif

typedef struct
{
	uint8	*memory;
	uint64	memory_use;
	uint64	memory_cap;
}	program_memory;

enum state
{
};

typedef struct
{
	program_memory	mem;
	uint32	row, col;
	char	last_char;
	char	*in_buffer;
	char	*out_buffer;

	enum state	state;

}	data;

#define ENTRY_POINT(name) int name(data *calc_data)
typedef ENTRY_POINT(entrypoint);
