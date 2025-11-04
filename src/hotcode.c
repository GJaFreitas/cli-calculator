#include "calc.h"
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

internal AST * ParseExpression(lexer *Lexer, uint32 min_prec);

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

inline internal char *
GetNumber(const char *stream_ptr, uint32 *index, program_memory *scratch, uint32 *token_size)
{
	uint32	size = 0;
	uint32	rounded_size;

	for (; IsNum(stream_ptr[*index + size]);) size++;
	rounded_size = NextMult8(size);
	char	*token_content = LocalAlloc(scratch, rounded_size + 1);
	for (uint32 i = 0; i < size; i++) token_content[i] = stream_ptr[*index + i];
	token_content[size] = 0;
	*index += size;
	*token_size = rounded_size;
	return (token_content);
}

inline internal enum token_type
GetOperatorType(char cur)
{
	switch (cur) {
		case '+':
		case '-': return T_OP_PLUSMINUS; break;
		case '*':
		case '/': return T_OP_MULTDIVIDE; break;
		case '<':
		case '>': return T_OP_BIN; break;
		case '$': return T_OP_EXPONENTIAL; break;
	}
	return (T_UNKNONW);
}

internal token *
GetNextToken(lexer *Lexer, program_memory *scratch)
{
	token	*Token;

	Token = LocalAlloc(scratch, sizeof(token));

	while (IsSpace(Lexer->input[Lexer->index])) (Lexer->index)++;
	if (Lexer->input[Lexer->index] == 0) return NULL;

	char	cur = Lexer->input[Lexer->index];
	if (IsNum(cur))
	{
		Token->token_string = GetNumber(Lexer->input, &Lexer->index, scratch, &Token->token_size);
		switch (Lexer->input[Lexer->index])
		{
			case 'b': Token->type = T_INTEGER_BIN; break;
			case 'h': Token->type = T_INTEGER_HEX; break;

			default: Token->type = T_INTEGER; break;
		}
	}
	else if (IsOperator(cur))
	{
		if (cur == '*' && Lexer->input[Lexer->index + 1] == '*') {
			cur = '$'; // Using this for exponentiation
			Lexer->index++;
		}
		Token->type = GetOperatorType(cur);
		Token->token_string = LocalAlloc(scratch, 1 + 1);
		Token->token_string[0] = cur;
		Token->token_string[1] = 0;
	}
	else if (IsChar(cur))
	{
	}

	return (Token);
}

inline internal AST *
ASTAttachToken(token *Token, program_memory *scratch)
{
	AST	*node;

	node = LocalAlloc(scratch, sizeof(AST));
	node->Token = Token;
	return (node);
}

global_var uint8	precedence_table[] = {'+', '-', '*', '$'};

inline internal uint32
GetPrecedence(token *tok)
{
	char	operator = tok->token_string[0];
	int i = 0;

	if (!IsOperator(operator)) return 0;
	for (; precedence_table[i] != operator; i++);
	return (i+1);
}

inline internal AST *
MakeBinaryTree(AST *left, AST *op, AST *right)
{
	op->left = left;
	op->right = right;
	return (op);
}

internal AST *
ParseIncreasingPrecedence(lexer *Lexer, AST *left, uint32 min_prec)
{
	token	*next = GetNextToken(Lexer, Lexer->mem_transient);

	uint32	next_prec = GetPrecedence(next);

	if (next_prec <= min_prec)
	{
		return left;
	} else {
		AST *right = ParseExpression(Lexer, next_prec);
		return MakeBinaryTree(left, ASTAttachToken(next, Lexer->mem_transient), right);
	}
}

internal bool
TokenEqual(token *tok1, token *tok2)
{
	int	i = 0;

	while (tok1->token_string[i] == tok2->token_string[i] && tok1->token_string[i] != 0)
		i++;
	return (tok2->token_string[i] == 0);
}

internal AST *
ParseExpression(lexer *Lexer, uint32 min_prec)
{
	AST	*node;
	AST	*left;

	if (Lexer->input[Lexer->index] == 0)
		return NULL;
	left = ASTAttachToken(GetNextToken(Lexer, Lexer->mem_transient), Lexer->mem_transient);

	while (true)
	{
		node = ParseIncreasingPrecedence(Lexer, left, min_prec);
		if (node && TokenEqual(node->Token, left->Token)) break ;
		left = node;
	}
	return (node);
}

internal void
ParseFull(lexer *Lexer, program_memory *scratch)
{
	Lexer->tree = ParseExpression(Lexer, 0);
}

internal void
ProccessCommand(data *calc_data, char *str)
{
	REMOVE_FLAG(calc_data->flags, ENTER_HIT);
	if (!strncmp(str, "quit", 4))
		calc_data->shouldClose = 1;

	// calc_data->row += 2;

	// while (calc_data->in_index)
	// 	calc_data->in_buffer[--calc_data->in_index] = 0;
}

// internal void
// print_tree(AST *root)
// {
// 	AST	*left, *rigth, *op;
// 	token	*tok_l, *tok_r;
//
// 	op = root;
// 	for (;;)
// 	{
// 		left = op->left;
// 		rigth = op->right;
// 		tok_l = left->Token;
// 		tok_r = rigth->Token;
//
// 		printf("%s %s %s", tok_l->token_string, op->Token->token_string, tok_r->token_string);
//
// 		if (tok_l->type < T_EXPRESSION)
// 			op = left;
// 		else
// 			op = rigth;
// 	}
// 	printf("\n");
// }

ENTRY_POINT(calc)
{
	char	ch;

	// fgets(Lexer->input, 512, stdin);
	strcpy(Lexer->input, "1 + 1");

	ProccessCommand(calc_data, Lexer->input);
	// Assert(calc_data->in_index < 512);
	//
	// if (ch == '\n')
	// 	ADD_FLAG(calc_data->flags, ENTER_HIT);
	// else if (ch == '')
	// {
	// 	if (calc_data->in_index > 0)
	// 		calc_data->in_index--;
	// 	calc_data->in_buffer[calc_data->in_index] = ' ';
	// }
	// else
	// 	calc_data->in_buffer[calc_data->in_index++] = ch;

	// mvprintw(calc_data->row, 0, "%s", calc_data->in_buffer);

	ParseFull(Lexer, Lexer->mem_transient);

	// if (calc_data->flags & ENTER_HIT) ProccessCommand(calc_data);

	// refresh();
	// move(calc_data->row, calc_data->in_index);
	return (calc_data->shouldClose);
}
