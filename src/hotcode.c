#include "calc.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
GetNumber(const char *stream_ptr, uint32 *index, program_memory *scratch)
{
	uint32	size = 0;
	uint32	rounded_size;

	for (; IsNum(stream_ptr[*index + size]);) size++;
	rounded_size = NextMult8(size);
	char	*token_content = LocalAlloc(scratch, rounded_size + 1);
	for (uint32 i = 0; i < size; i++) token_content[i] = stream_ptr[*index + i];
	token_content[size] = 0;
	*index += size;
	Assert(size != 0);
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
_NextToken(lexer *Lexer, program_memory *scratch, uint32 *token_size)
{
	token	*Token;
	uint32	idx = Lexer->index;

	Token = LocalAlloc(scratch, sizeof(token));

	while (IsSpace(Lexer->input[idx])) (idx)++;
	if (Lexer->input[idx] == 0) return NULL;

	char	cur = Lexer->input[idx];
	if (IsNum(cur))
	{
		Token->token_string = GetNumber(Lexer->input, &idx, scratch);
		// TODO: Of course this is wrong and has to be handled better
		Token->integer_value = atoi(Token->token_string);
		// ----------------------------------------------------------
		switch (Lexer->input[idx])
		{
			case 'b': Token->type = T_INTEGER_BIN; break;
			case 'h': Token->type = T_INTEGER_HEX; break;

			default: Token->type = T_INTEGER; break;
		}
	}
	else if (IsOperator(cur))
	{
		if (cur == '*' && Lexer->input[idx + 1] == '*') {
			cur = '$'; // Using this for exponentiation
			idx++;
		}
		Token->type = GetOperatorType(cur);
		Token->token_string = LocalAlloc(scratch, 1 + 1);
		Token->token_string[0] = cur;
		Token->token_string[1] = 0;
		(idx)++;
	}
	else if (IsChar(cur))
	{
	}

	Token->id = idx;
	*token_size = idx - Lexer->index;
	Assert(idx != Lexer->index);

	return (Token);
}

internal token *
PeekNextToken(lexer *Lexer, program_memory *mem)
{
	token_info	stack_entry;
	uint32	token_size;
	token	*tok;

	tok = _NextToken(Lexer, mem, &token_size);
	if (tok == NULL) return NULL;
	Lexer->index += token_size;
	stack_entry.tok = tok;
	stack_entry.size = token_size;
	Lexer->peek.token_stack[Lexer->peek.stack_idx] = stack_entry;
	Lexer->peek.stack_idx++;
	return (tok);
}

internal token *
ConsumePeekedToken(lexer *Lexer)
{
	token	*tok;
	// NOTE: Of course the stack idx is pointing to the next unused stack entry
	int32	index = Lexer->peek.stack_idx - 1;
	
	tok = Lexer->peek.token_stack[index].tok;
	Lexer->peek.stack_idx--;
	return (tok);
}

internal token *
ConsumeNextToken(lexer *Lexer, program_memory *mem)
{
	token	*tok;
	uint32	token_size;

	tok = _NextToken(Lexer, mem, &token_size);
	Lexer->index += token_size;
	return (tok);
}

inline internal AST *
ASTAttachToken(token *Token, program_memory *mem_arena)
{
	AST	*node;

	node = LocalAlloc(mem_arena, sizeof(AST));
	node->Token = Token;
	return (node);
}

global_var uint8	precedence_table[] = {'+', '-', '*', '/', '$'};

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

internal AST * ParseExpression(lexer *Lexer, uint32 min_prec);

internal AST *
ParseIncreasingPrecedence(lexer *Lexer, AST *left, uint32 min_prec)
{
	token	*next = PeekNextToken(Lexer, Lexer->mem_transient);

	if (next == NULL)
	{
		ADD_FLAG(Lexer->flags, LEXER_FINISHED);
		return (left);
	}
	uint32	next_prec = GetPrecedence(next);

	if (next_prec < min_prec)
	{
		// TODO: ?????????????????
		Lexer->index -= Lexer->peek.token_stack[Lexer->peek.stack_idx - 1].size;
		Lexer->peek.stack_idx--;
		return left;
	} else {
		AST *right = ParseExpression(Lexer, next_prec);
		return MakeBinaryTree(
			left,
			ASTAttachToken(ConsumePeekedToken(Lexer), Lexer->mem_transient),
			right
		);
	}
}

internal AST *
ParseExpression(lexer *Lexer, uint32 min_prec)
{
	AST	*node;
	AST	*left;

	if (Lexer->input[Lexer->index] == 0)
		return NULL;
	left = ASTAttachToken(ConsumeNextToken(Lexer, Lexer->mem_transient), Lexer->mem_transient);

	while (true)
	{
		node = ParseIncreasingPrecedence(Lexer, left, min_prec);
		if (node && (node->Token->id == left->Token->id)) break ;
		if (Lexer->flags & LEXER_FINISHED) break ;
		left = node;
	}
	return (node);
}

inline internal solution
BinaryOperation(char op, token *l, token *r)
{
	solution sol;
	switch (l->type)
	{
		default:
		switch (op)
		{
			case '<':
				sol.int_solution = (l->integer_value < r->integer_value);
				break ;
			case '>':
				sol.int_solution = (l->integer_value > r->integer_value);
				break ;
		} break;
		case T_FLOAT:
		// NOTE: Solution has to be a float because if there is any float value
		// at all it 'poisons' the whole operation with its floatiness
		switch (op)
		{
			case '<':
				sol.float_32_solution = (l->float_32_value < r->float_32_value);
				break ;
			case '>':
				sol.float_32_solution = (l->float_32_value > r->float_32_value);
				break ;
		} break;
	}
	return (sol);
}

inline internal solution
PlusMinus(char op, token *l, token *r)
{
	solution sol;
	switch (l->type)
	{
		default:
		switch (op)
		{
			case '+':
				sol.int_solution = (l->integer_value + r->integer_value);
				break ;
			case '-':
				sol.int_solution = (l->integer_value - r->integer_value);
				break ;
		} break;
		case T_FLOAT:
		switch (op)
		{
			case '+':
				sol.float_32_solution = (l->float_32_value + r->float_32_value);
				break ;
			case '-':
				sol.float_32_solution = (l->float_32_value - r->float_32_value);
				break ;
		} break;
	}
	return (sol);
}

inline internal solution
MulDiv(char op, token *l, token *r)
{
	solution sol;
	switch (l->type)
	{
		// NOTE: Since division doesnt guarantee an int return division also poisons all
		// results going forward into being floats
		default: sol.int_solution = (l->integer_value * r->integer_value); break ;

		case T_FLOAT:
		switch (op)
		{
			case '*':
				sol.float_32_solution = (l->float_32_value * r->float_32_value);
				break ;
			case '/':
				sol.float_32_solution = (l->float_32_value / r->float_32_value);
				break ;
		} break;
	}
	return (sol);
}

// TODO: This function is giving the wrong output
inline internal solution
Exponential(token *l, token *r)
{
	solution sol;
	switch (l->type)
	{
		default: 
			if (r->integer_value == 0)
			{
				sol.int_solution = 0;
				return (sol);
			}
			sol.int_solution = l->integer_value;
			for (int64 i = 1; i < r->integer_value; i++)
				sol.int_solution *= sol.int_solution;
			break ;
		case T_FLOAT:
			if (r->float_32_value == 0)
			{
				sol.float_32_solution = 0;
				return (sol);
			}
			sol.float_32_solution = l->float_32_value;
			// NOTE: I am absolutely not allowing exponentiation of values like 1.5
			int	helper = (int) r->float_32_value;
			for (int i = 1; i < helper; i++)
				sol.float_32_solution *= sol.float_32_solution;
			break ;
	}
	return (sol);
}

internal token
DoOperation(token *op, token *l, token *r)
{
	token		result;
	solution	sol;

	if (l->type == T_FLOAT || r->type == T_FLOAT || op->token_string[0] == '/')
	{
		if (l->type != T_FLOAT) l->float_32_value = (float32)l->integer_value;
		if (r->type != T_FLOAT) r->float_32_value = (float32)r->integer_value;
		l->type = T_FLOAT;
		r->type = T_FLOAT;
	}

	switch (op->type)
	{
		case T_OP_BIN: sol = BinaryOperation(op->token_string[0], l, r); break ;
		case T_OP_PLUSMINUS: sol = PlusMinus(op->token_string[0], l, r); break ;
		case T_OP_MULTDIVIDE: sol = MulDiv(op->token_string[0], l, r); break ;
		case T_OP_EXPONENTIAL: sol = Exponential(l, r); break ;
		default: sol = (solution){.int_solution = 0};
	}
	result.type = l->type;
	if (result.type == T_FLOAT) result.float_32_value = sol.float_32_solution;
	else result.integer_value = sol.int_solution;

	return (result);
}

inline internal AST *
SolverPartialTreeNoOp(AST *root, token *sol, program_memory *mem)
{
	switch (root->Token->type)
	{
		case T_FLOAT: sol->float_32_value = root->Token->float_32_value;
			break ;
		default : sol->integer_value = root->Token->integer_value;
	}
	sol->type = root->Token->type;
	return (ASTAttachToken(sol, mem));
}

inline internal AST *
SolverPartialTreeWithOp(AST *root, token *sol, program_memory *mem)
{
	if (root->left)
	{
		switch (root->left->Token->type)
		{
			case T_FLOAT: sol->float_32_value = root->left->Token->float_32_value;
				break ;
			default : sol->integer_value = root->left->Token->integer_value;
		}
		sol->type = root->left->Token->type;
		return (ASTAttachToken(sol, mem));
	} else {
		switch (root->right->Token->type)
		{
			case T_FLOAT: sol->float_32_value = root->right->Token->float_32_value;
				break ;
			default : sol->integer_value = root->right->Token->integer_value;
		}
		sol->type = root->right->Token->type;
		return (ASTAttachToken(sol, mem));
	}
}

// Neither child is an operation
inline internal bool
SolverBaseCase(AST *node)
{
	return (!OpToken(node->left->Token) && !OpToken(node->right->Token));
}

internal AST *
Solver(AST *root, program_memory *mem)
{
	token	*sol = LocalAlloc(mem, sizeof(token));

	if (root->Token->type > T_EXPRESSION) {
		return SolverPartialTreeNoOp(root, sol, mem);
	}

	// 1 + _
	if (root->Token->type < T_EXPRESSION && (!root->left || !root->right)) {
		return SolverPartialTreeWithOp(root, sol, mem);
	}

	if (SolverBaseCase(root))
	{
		*sol = DoOperation(
			root->Token,
			root->left->Token,
			root->right->Token
		);
		return (ASTAttachToken(sol, mem));
	}

	// HACK: Clear memory leak here but its okay because arena allocator resets
	if (root->left->Token->type < T_EXPRESSION)
		root->left = Solver(root->left, mem);
	if (root->right->Token->type < T_EXPRESSION)
		root->right = Solver(root->right, mem);
	*sol = DoOperation(root->Token, root->left->Token, root->right->Token);
	return (ASTAttachToken(sol, mem));
}

internal char *
ParseFull(lexer *Lexer)
{
	solution	Solution;
	token		sol_token;
	char	*output = 0;

	Lexer->tree = ParseExpression(Lexer, 0);
	sol_token = *Solver(Lexer->tree, Lexer->mem_transient)->Token;
	Solution.int_solution = sol_token.integer_value;
	Solution.float_32_solution = sol_token.float_32_value;
	if (sol_token.type == T_FLOAT)
		printf("%.2f\n", Solution.float_32_solution);
	else
		printf("%ld\n", Solution.int_solution);
	return (output);
}

internal int32
ProccessCommand(data *calc_data, char *str)
{
	REMOVE_FLAG(calc_data->flags, ENTER_HIT);
	if (!strncmp(str, "quit", 4))
		calc_data->shouldClose = 1;
	return (calc_data->shouldClose);

	// calc_data->row += 2;

	// while (calc_data->in_index)
	// 	calc_data->in_buffer[--calc_data->in_index] = 0;
}

ENTRY_POINT(calc)
{

	// fgets(Lexer->input, 512, stdin);
	// Lexer->input[strlen(Lexer->input) - 1] = 0;

	if (ProccessCommand(calc_data, Lexer->input))
		return (calc_data->shouldClose);
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

	calc_data->out_buffer = ParseFull(Lexer);
	calc_data->shouldClose = 1;
	// print_tree(Lexer->tree);

	// if (calc_data->flags & ENTER_HIT) ProccessCommand(calc_data);

	// refresh();
	// move(calc_data->row, calc_data->in_index);
	return (calc_data->shouldClose);
}
