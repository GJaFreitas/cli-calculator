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
#define float32 float
#define float64 double

#define uint8	uint8_t
#define uint16	uint16_t
#define uint32	uint32_t
#define uint64	uint64_t
#define int8	int8_t
#define int32	int32_t
#define int64	int64_t

// NOTE: If in debug mode test that thing, if not dont
#ifdef DEBUG
#define Assert(Expression) if (!(Expression)) {printf("%s\n", (char*)0);}
#else
#define Assert(Expression)
#endif

typedef struct
{
	uint8	*memory;
	uint64	memory_use;
	uint64	memory_cap;
}	ProgramMemory;

enum TokenType
{
	T_OP_BIN,
	T_OP_PLUSMINUS,
	T_OP_MULTDIVIDE,
	T_OP_EXPONENTIAL,

	T_EXPRESSION,
	T_INTEGER,
	T_INTEGER_HEX,
	T_INTEGER_BIN,
	T_FLOAT,
	T_UNKNONW
};

typedef struct
{
	// NULL terminated strings
	enum TokenType	type;
	union {

		struct { uint32 size; uint8 *data; } str_value;
		int64	integer_value;
		float32	float_32_value;

	};
	char	*token_string;
	uint32	id;
}	Token;

static inline bool	OpToken(Token *token)
{
	return (token->type < T_EXPRESSION);
}

// NOTE: In reality i should just use floats everywhere
// to make this easier on myself with pretty much 0
// repercussions but since this is basically my 'intro to
// compiler design' ill keep the int and float distinction
typedef union
{
	int64	int_solution;
	real32	float_32_solution;
}	Solution;

typedef struct AST
{
	Token	*token;
	struct AST	*left;
	struct AST	*right;
}	AST;


typedef struct
{
	uint32	size;
	Token	*tok;
}	TokenInfo;

typedef struct TokenStack
{
	uint32		stack_idx;
	TokenInfo	token_stack[32];
}	TokenStack;

#define LEXER_FINISHED		(1 << 31)
#define LEXER_SYNTAX_ERROR	(1 << 30)

typedef struct Lexer
{
	int32	flags;
	uint32	index;
	ProgramMemory	*mem_permanent;
	ProgramMemory	*mem_transient;
	AST	*tree;
	char	*input;
	TokenStack	peek;
}	Lexer;

#define ADD_FLAG(x, y)		((x) |= (y))
#define REMOVE_FLAG(x, y)	((x) &= ~(y))

// User hit enter
#define ENTER_HIT	(1 << 31)

typedef struct PreviousInput 
{
	char	*command;
	char	*result;
	struct PreviousInput	*next;
	struct PreviousInput	*prev;
}	PreviousInput;

typedef struct
{
	ProgramMemory	*mem_permanent;
	ProgramMemory	*mem_transient;

	PreviousInput	*cmds_list;
	PreviousInput	*current;
	uint32	row, col;
	uint32	shouldClose;
	char	*in_buffer;
	uint16	in_index;
	char	*out_buffer;
	uint16	out_index;

	uint32		flags;

}	Data;

inline internal bool	IsNum(uint8 c)
{
	return (c >= '0' && c <= '9');
}

inline internal bool	IsSpace(uint8 c)
{
	return (c == ' ');
}

inline internal bool	IsChar(uint8 c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

inline internal bool	IsOperator(uint8 c)
{
	return (c == '+' || c == '*' || c == '-' || c == '<' || c == '>' || c == '/' || c == '$');
}

inline internal uint32	NextMult8(uint32 num)
{
	return ((num + 7) & ~7);
}

void	*LocalAlloc(ProgramMemory *mem, uint64 size);

#define ENTRY_POINT(name) int name(Data *calc_data, Lexer *lexer)
typedef ENTRY_POINT(entrypoint);
