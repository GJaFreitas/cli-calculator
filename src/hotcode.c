#include "calc.h"
#include <ncurses.h>

ENTRY_POINT(calc)
{
	ch = getch();
	if (ch == 'q')
		exit(1);
	mvaddch(col, row, ch);
	row++;
	refresh();
	return (0);
}
