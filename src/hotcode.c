#include "calc.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

inline internal char *
GetNumber(const char *stream_ptr, uint32 *index, ProgramMemory *scratch)
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

inline internal enum TokenType
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

internal Token *
_NextToken(Lexer *lexer, ProgramMemory *scratch, uint32 *token_size)
{
	Token	*token;
	uint32	idx = lexer->index;

	token = LocalAlloc(scratch, sizeof(Token));

	while (IsSpace(lexer->input[idx])) (idx)++;
	if (lexer->input[idx] == 0) return NULL;

	char	cur = lexer->input[idx];
	if (IsNum(cur))
	{
		token->token_string = GetNumber(lexer->input, &idx, scratch);
		// TODO: Of course this is wrong and has to be handled better
		token->integer_value = atoi(token->token_string);
		// ----------------------------------------------------------
		switch (lexer->input[idx])
		{
			case 'b': token->type = T_INTEGER_BIN; break;
			case 'h': token->type = T_INTEGER_HEX; break;

			default: token->type = T_INTEGER; break;
		}
	}
	else if (IsOperator(cur))
	{
		if (cur == '*' && lexer->input[idx + 1] == '*') {
			cur = '$'; // Using this for exponentiation
			idx++;
		}
		if (IsOperator(lexer->input[idx + 1]))
		{
			ADD_FLAG(lexer->flags, LEXER_SYNTAX_ERROR);
			return (NULL);
		}
		token->type = GetOperatorType(cur);
		token->token_string = LocalAlloc(scratch, 1 + 1);
		token->token_string[0] = cur;
		token->token_string[1] = 0;
		(idx)++;
	}
	else if (IsChar(cur))
	{
		ADD_FLAG(lexer->flags, LEXER_SYNTAX_ERROR);
		return (NULL);
	}
	else
	{
		return (NULL);
	}

	token->id = idx;
	*token_size = idx - lexer->index;
	Assert(idx != lexer->index);

	return (token);
}

internal Token *
PeekNextToken(Lexer *lexer, ProgramMemory *mem)
{
	TokenInfo	stack_entry;
	uint32	token_size;
	Token	*tok;

	tok = _NextToken(lexer, mem, &token_size);
	if (tok == NULL) return NULL;
	lexer->index += token_size;
	stack_entry.tok = tok;
	stack_entry.size = token_size;
	lexer->peek.token_stack[lexer->peek.stack_idx] = stack_entry;
	lexer->peek.stack_idx++;
	return (tok);
}

internal Token *
ConsumePeekedToken(Lexer *lexer)
{
	Token	*tok;
	// NOTE: Of course the stack idx is pointing to the next unused stack entry
	int32	index = lexer->peek.stack_idx - 1;
	
	tok = lexer->peek.token_stack[index].tok;
	lexer->peek.stack_idx--;
	return (tok);
}

internal Token *
ConsumeNextToken(Lexer *lexer, ProgramMemory *mem)
{
	Token	*tok;
	uint32	token_size;

	tok = _NextToken(lexer, mem, &token_size);
	lexer->index += token_size;
	return (tok);
}

inline internal AST *
ASTAttachToken(Token *token, ProgramMemory *mem_arena)
{
	AST	*node;

	node = LocalAlloc(mem_arena, sizeof(AST));
	node->token = token;
	return (node);
}

global_var uint8	precedence_table[] = {'+', '-', '*', '/', '$'};

inline internal uint32
GetPrecedence(Token *tok)
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

internal AST * ParseExpression(Lexer *lexer, uint32 min_prec);

internal AST *
ParseIncreasingPrecedence(Lexer *lexer, AST *left, uint32 min_prec)
{
	Token	*next = PeekNextToken(lexer, lexer->mem_transient);

	if (next == NULL || (lexer->flags & LEXER_SYNTAX_ERROR))
	{
		ADD_FLAG(lexer->flags, LEXER_FINISHED);
		return (left);
	}
	uint32	next_prec = GetPrecedence(next);

	if (next_prec < min_prec)
	{
		lexer->index -= lexer->peek.token_stack[lexer->peek.stack_idx - 1].size;
		lexer->peek.stack_idx--;
		return left;
	} else {
		AST *right = ParseExpression(lexer, next_prec);
		return MakeBinaryTree(
			left,
			ASTAttachToken(ConsumePeekedToken(lexer), lexer->mem_transient),
			right
		);
	}
}

internal AST *
ParseExpression(Lexer *lexer, uint32 min_prec)
{
	AST	*node;
	AST	*left;

	if (lexer->flags & LEXER_FINISHED)
		return NULL;
	left = ASTAttachToken(ConsumeNextToken(lexer, lexer->mem_transient), lexer->mem_transient);

	while (true)
	{
		node = ParseIncreasingPrecedence(lexer, left, min_prec);
		if (lexer->flags & LEXER_FINISHED) break ;
		if (node && (node->token->id == left->token->id)) break ;
		left = node;
	}
	return (node);
}

inline internal Solution
BinaryOperation(char op, Token *l, Token *r)
{
	Solution sol;
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

inline internal Solution
PlusMinus(char op, Token *l, Token *r)
{
	Solution sol;
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

inline internal Solution
MulDiv(char op, Token *l, Token *r)
{
	Solution sol;
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
inline internal Solution
Exponential(Token *l, Token *r)
{
	Solution sol;
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

internal Token
DoOperation(Token *op, Token *l, Token *r)
{
	Token		result;
	Solution	sol;

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
		default: sol = (Solution){.int_solution = 0};
	}
	result.type = l->type;
	if (result.type == T_FLOAT) result.float_32_value = sol.float_32_solution;
	else result.integer_value = sol.int_solution;

	return (result);
}

inline internal AST *
SolverPartialTreeNoOp(AST *root, Token *sol, ProgramMemory *mem)
{
	switch (root->token->type)
	{
		case T_FLOAT: sol->float_32_value = root->token->float_32_value;
			break ;
		default : sol->integer_value = root->token->integer_value;
	}
	sol->type = root->token->type;
	return (ASTAttachToken(sol, mem));
}

internal AST * Solver(AST *root, ProgramMemory *mem);

inline internal AST *
SolverPartialTreeWithOp(AST *root, Token *sol, ProgramMemory *mem)
{
	if (root->left)
	{
		if (root->left->token->type < T_EXPRESSION)
			return (Solver(root->left, mem));
		switch (root->left->token->type)
		{
			case T_FLOAT: sol->float_32_value = root->left->token->float_32_value;
				break ;
			default : sol->integer_value = root->left->token->integer_value;
		}
		sol->type = root->left->token->type;
		return (ASTAttachToken(sol, mem));
	} else {
		if (root->right->token->type < T_EXPRESSION)
			return (Solver(root->right, mem));
		switch (root->right->token->type)
		{
			case T_FLOAT: sol->float_32_value = root->right->token->float_32_value;
				break ;
			default : sol->integer_value = root->right->token->integer_value;
		}
		sol->type = root->right->token->type;
		return (ASTAttachToken(sol, mem));
	}
}

// Neither child is an operation
inline internal bool
SolverBaseCase(AST *node)
{
	return (!OpToken(node->left->token) && !OpToken(node->right->token));
}

internal AST *
Solver(AST *root, ProgramMemory *mem)
{
	Token	*sol = LocalAlloc(mem, sizeof(Token));

	if (root->token->type > T_EXPRESSION) {
		return SolverPartialTreeNoOp(root, sol, mem);
	}

	// 1 + _
	if (root->token->type < T_EXPRESSION && (!root->left->token || !root->right->token)) {
		return SolverPartialTreeWithOp(root, sol, mem);
	}

	if (SolverBaseCase(root))
	{
		*sol = DoOperation(
			root->token,
			root->left->token,
			root->right->token
		);
		return (ASTAttachToken(sol, mem));
	}

	// HACK: Clear memory leak here but its okay because arena allocator resets
	if (root->left->token->type < T_EXPRESSION)
		root->left = Solver(root->left, mem);
	if (root->right->token->type < T_EXPRESSION)
		root->right = Solver(root->right, mem);
	*sol = DoOperation(root->token, root->left->token, root->right->token);
	return (ASTAttachToken(sol, mem));
}

internal char *
ParseFull(Lexer *lexer)
{
	Solution	Solution;
	Token		sol_token;
	char	*output = 0;

	lexer->tree = ParseExpression(lexer, 0);
	// TODO: Make this write to output buffer
	if (!(lexer->flags & LEXER_SYNTAX_ERROR))
	{
		sol_token = *Solver(lexer->tree, lexer->mem_transient)->token;
		Solution.int_solution = sol_token.integer_value;
		Solution.float_32_solution = sol_token.float_32_value;
		if (sol_token.type == T_FLOAT)
			mvwprintw(stdscr, 2, 0, "%.2f\n", Solution.float_32_solution);
		else
			mvwprintw(stdscr, 2, 0, "%ld\n", Solution.int_solution);
	} else {
		mvwprintw(stdscr, 2, 0, "Syntax error\n");
	}
	return (output);
}

internal void
ProccessInput(Data *data)
{
	switch (data->mode)
	{
		case COMMAND:
			// TODO: If more commands are added this will have to be its own parser thing
			if (!strncmp(data->command_buffer, "quit", data->command_index))
				data->shouldClose = 1;
			data->command_index = 0;
		break;

		case INSERT:
			for (uint32 i = 0; i < data->in_index; i++)
				data->in_buffer[i] = 0;
			data->in_index = 0;
		break;
		
		default: fprintf(stderr, "Something has clearly gone wrong\n"); exit(1);
	}
}


// A cool idea here would be that while this function is called the tree builds itself,
// waiting when necessary like every time ProccessChar() is called _NextToken() is called
// and if returns a token then the tree changes and Solver() gets called again
//
// Im not going to do that


// 
internal inline void
ProccessChar(Data *calc)
{
	char	ch;

	ch = getch();

	switch (ch)
	{
		case '\n':
			ProccessInput(calc);
		break;

		case '':
			calc->shouldClose = 1;
		break;

		// ctrl+c goes to normal mode
		case '':
			calc->mode = NORMAL;
		break;

		// command mode like in vim
		case ':':
			if (calc->mode == NORMAL)
				calc->mode = COMMAND;
		break;

		case '':
			if (calc->in_index > 0)
			{
				calc->in_index--;
				calc->in_buffer[calc->in_index] = ' ';
			}
		break ;

		default:
			if (calc->flags & TOGGLE_COMMAND)
				calc->command_buffer[calc->command_index++] = ch;
			else
				calc->in_buffer[calc->in_index++] = ch;
		break ;
	}
}

// TODO: Have an indicator of the mode we are currently in
ENTRY_POINT(calc)
{
	ProccessChar(data);
	Assert(data->in_index < 512);

	if (data->in_index && data->mode == INSERT)
		data->out_buffer = ParseFull(lexer);

	refresh();
	mvwprintw(stdscr, 0, 0, "%s", data->in_buffer);
	move(data->row, data->in_index);
	return (data->shouldClose);
}
