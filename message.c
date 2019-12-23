/*
 * message.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "rogue.h"
#include "init.h"
#include "keys.h"
#include "level.h"
#include "machdep.h"
#include "message.h"
#include "object.h"
#include "pack.h"

char msg_line[DCOLS] = "";
short msg_col = 0;
boolean msg_cleared = 1;
char hunger_str[8] = "";

extern boolean cant_int, did_int, interrupted, save_is_interactive;
extern short add_strength, expmod ; // (zerogue 0.4.1) added expmod
extern int cur_level;
extern fighter rogue;

void save_screen(void);
void pad(char *, short);

void message(char *msg, boolean intrpt)
{
	if (!save_is_interactive)
	{
		return;
	}
	if (intrpt)
	{
		interrupted = 1;
		md_slurp();
	}
	cant_int = 1;

	if (!msg_cleared)
	{
		mvaddstr(MIN_ROW - 1, msg_col, MORE);
		refresh();
		wait_for_ack();
		check_message();
	}
	(void) strcpy(msg_line, msg);
	mvaddstr(MIN_ROW - 1, 0, msg);
	addch(' ');
	refresh();
	msg_cleared = 0;
	msg_col = strlen(msg);

	cant_int = 0;
	if (did_int)
	{
		did_int = 0;
		onintr(0);
	}
}

void remessage(void)
{
	if (msg_line[0])
	{
		message(msg_line, 0);
	}
}

void check_message(void)
{
	if (msg_cleared)
	{
		return;
	}
	move(MIN_ROW - 1, 0);
	clrtoeol();
	refresh();
	msg_cleared = 1;
}

short get_input_line(char *prompt, char *buf, char *insert, char *if_cancelled,
                     boolean add_blank, boolean do_echo)
{
	short ch;
	short i = 0, n;

	message(prompt, 0);
	n = strlen(prompt);

	if (insert[0])
	{
		mvaddstr(0, n + 1, insert);
		(void) strcpy(buf, insert);
		i = strlen(insert);
		move(0, (n + i + 1));
		refresh();
	}

	while (((ch = rgetchar()) != '\r') && (ch != '\n') && (ch != ROGUE_KEY_CANCEL))
	{
		if ((ch >= ' ') && (ch <= '~') && (i < MAX_TITLE_LENGTH-2))
		{
			if ((ch != ' ') || (i > 0))
			{
				buf[i++] = ch;
				if (do_echo)
				{
					addch(ch);
				}
			}
		}
		if ((ch == '\b') && (i > 0))
		{
			if (do_echo)
			{
				mvaddch(0, i + n, ' ');
				move(MIN_ROW-1, i+n);
			}
			i--;
		}
		refresh();
	}
	check_message();
	if (add_blank)
	{
		buf[i++] = ' ';
	}
	else
	{
		while ((i > 0) && (buf[i-1] == ' '))
		{
			i--;
		}
	}

	buf[i] = 0;

	if ((ch == ROGUE_KEY_CANCEL) || (i == 0) || ((i == 1) && add_blank))
	{
		if (if_cancelled)
		{
			message(if_cancelled, 0);
		}
		return(0);
	}
	return(i);
}

int rgetchar(void)
{
	int ch;

	for(;;)
	{
		ch = getchar();

		switch(ch)
		{
		case ROGUE_KEY_REFRESH:
			wrefresh(curscr);
			break;
		case ROGUE_KEY_SAVE_SCREEN:
			save_screen();
			break;
		default:
			return(ch);
		}
	}
}
/*
original:
Level: 99 Gold: 999999 HP: 999(999) Str: 99(99) Arm: 99 Exp: 21/10000000 Hungry
0....:....1....:....2....:....3....:....4....:....5....:....6....:....7....:....
zerogue 0.4.6 (#2):
LV [99](99999999)  $999999  HP 999/999  STR 99/99  ATK 9d99 DEF 99+  SUST 999%
0....:....1....:....2....:....3....:....4....:....5....:....6....:....7....:....
*/
void print_stats(int stat_mask)
{
	char buf[16];
	boolean label;
	int row = DROWS - 1;

	label = (stat_mask & STAT_LABEL) ? 1 : 0;

	if( stat_mask & STAT_LEVEL || stat_mask & STAT_EXP )
	{
		if( label ) mvaddstr( row, 0, "LV " ) ;
		if( expmod != 0 )
			sprintf( buf, "[%d](%ld)", get_rogue_level(1), rogue.exp_points) ;
		else
			sprintf(buf, "%d (%ld)", rogue.exp, rogue.exp_points) ;
		mvaddstr( row, 3, buf ) ;
		pad( buf, 16 ) ;
	}
	if( stat_mask & STAT_GOLD )
	{
		if( rogue.gold > MAX_GOLD )
			rogue.gold = MAX_GOLD ;
		sprintf( buf, "$%ld", rogue.gold ) ;
		mvaddstr( row, 19, buf ) ;
		pad( buf, 7 ) ;
	}
	if( stat_mask & STAT_HP )
	{
		if( label ) mvaddstr( row, 28, "HP " ) ;
		if( rogue.hp_max > MAX_HP )
		{ // Failsafe to keep HP under the absolute maximum.
			rogue.hp_current -= ( rogue.hp_max - MAX_HP ) ;
			rogue.hp_max = MAX_HP ;
		}
		sprintf( buf, "%d/%d", rogue.hp_current, rogue.hp_max ) ;
		mvaddstr( row, 31, buf ) ;
		pad( buf, 9 ) ;
	}
	if( stat_mask & STAT_STRENGTH )
	{
		if( label ) mvaddstr( row, 40, "STR " ) ;
		if( rogue.str_max > MAX_STRENGTH )
		{ // Failsafe to keep strength under the absolute maximum.
			rogue.str_current -= ( rogue.str_max - MAX_STRENGTH ) ;
			rogue.str_max = MAX_STRENGTH ;
		}
		sprintf( buf, "%d/%d", ( rogue.str_current + add_strength ), rogue.str_max ) ;
		mvaddstr( row, 44, buf ) ;
		pad( buf, 7 ) ;
	}
	if( stat_mask & STAT_GEAR )
	{
		if( label )
		{
			mvaddstr( row, 51, "ATK " ) ;
			mvaddstr( row, 60, "DEF " ) ;
		}

		if( rogue.weapon )
			mvaddstr( row, 55, rogue.weapon->damage ) ;
		else
			mvaddstr( row, 55, "---  " ) ;

		if( rogue.armor )
		{
			if( rogue.armor->d_enchant > MAX_ARMOR )
				rogue.armor->d_enchant = MAX_ARMOR ;
			sprintf( buf, "%d%s%s", get_armor_class(rogue.armor),
					( rogue.armor->is_protected ? "+" : "" ),
					( rogue.armor->is_cursed ? "-" : "" )
				);
			mvaddstr( row, 64, buf ) ;
			pad( buf, 5 ) ;
		}
		else mvaddstr( row, 64, "---  " ) ;
	}
	if( stat_mask & STAT_HUNGER )
	{
		if( label ) mvaddstr( row, 69, "SUST " ) ;
		// calculate hunger as a percentage
		// reduced from ( 100 * rogue.moves_left ) / ( 4 * HUNGRY ) ;
		unsigned short hunger_pct = ( 25 * rogue.moves_left ) / HUNGRY ;
		sprintf( buf, "%d%%", hunger_pct ) ;
		mvaddstr( row, 74, buf ) ;
		clrtoeol() ;
	}

	refresh() ;
}

void pad(char *s, short n)
{
	short i;

	for (i = strlen(s); i < n; i++)
	{
		addch(' ');
	}
}

void save_screen(void)
{
	FILE *fp;
	short i, j;
	char buf[DCOLS+2];
	boolean found_non_blank;


	if ((fp = fopen("rogue.screen", "w")) != NULL)
	{
		for (i = 0; i < DROWS; i++)
		{
			found_non_blank = 0;
			for (j = (DCOLS - 1); j >= 0; j--)
			{
				buf[j] = mvinch(i, j);
				if (!found_non_blank)
				{
					if ((buf[j] != ' ') || (j == 0))
					{
						buf[j + ((j == 0) ? 0 : 1)] = 0;
						found_non_blank = 1;
					}
				}
			}
			fputs(buf, fp);
			putc('\n', fp);
		}
		fclose(fp);
	}
	else
	{
		sound_bell();
	}
}

void sound_bell(void)
{
	putchar(7);
	fflush(stdout);
}

boolean is_digit(short ch)
{
	return((ch >= '0') && (ch <= '9'));
}

int r_index(char *str, int ch, boolean last)
{
	int i = 0;

	if (last)
	{
		for (i = strlen(str) - 1; i >= 0; i--)
		{
			if (str[i] == ch)
			{
				return(i);
			}
		}
	}
	else
	{
		for (i = 0; str[i]; i++)
		{
			if (str[i] == ch)
			{
				return(i);
			}
		}
	}
	return(-1);
}

boolean is_valid_char( short ch, char *commands )
{
	short i, cl ;

	cl = strlen(commands) ;

	for( i = 0 ; i < cl ; i++ )
		if( commands[i] == ch ) return 1 ;

	return 0 ;
}
