/*
 * trap.c
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
#include "rogue.h"
#include "hit.h"
#include "keys.h"
#include "message.h"
#include "move.h"
#include "random.h"
#include "room.h"
#include "score.h"
#include "special_hit.h"
#include "trap.h"
#include "use.h"
#include "level.h" // (zerogue 0.4.0) Added to support disarm_trap()

trap traps[MAX_TRAPS];
boolean trap_door = 0;
int bear_trap = 0;

char *trap_strings[TRAPS * 2] = {
	"trapdoor",
	"You fell down a trap.",
	
	"bear trap",
	"You are caught in a bear trap.",
	
	"teleportation trap",
	"Teleport.",
	
	"poison dart trap",
	"A small dart just hit you in the shoulder.",
	
	"sleeping gas trap",
	"A strange white mist envelops you and you fall asleep.",
	
	"rust trap",
	"A gush of water hits you on the head."
};

extern int cur_level;
extern short party_room;
extern char *new_level_message;
extern boolean interrupted;
extern short ring_exp;
extern boolean sustain_strength;
extern short blind, halluc, confused ;
extern unsigned short dungeon[DROWS][DCOLS];
extern fighter rogue;
extern room rooms[];

int trap_at(int row, int col)
{
	short i;

	for (i = 0; ((i < MAX_TRAPS) && (traps[i].trap_type != NO_TRAP)); i++)
	{
		if ((traps[i].trap_row == row) && (traps[i].trap_col == col))
		{
			return(traps[i].trap_type);
		}
	}
	return(NO_TRAP);
}

void trap_player( short row, short col )
{
	short t = trap_at( row, col ) ;

	if( t == NO_TRAP )   return ;

	dungeon[row][col] &= (~HIDDEN);
	if( rand_percent(get_rogue_level(1)) )
	{
		message( "The trap failed.", 1 ) ;
		return;
	}
	switch(t)
	{
	case TRAP_DOOR:
		trap_door = 1;
		new_level_message = trap_strings[(t*2)+1];
		break;
	case BEAR_TRAP:
		message(trap_strings[(t*2)+1], 1);
		bear_trap = get_rand(4, 7);
		break;
	case TELE_TRAP:
		mvaddch(rogue.row, rogue.col, '^');
		tele();
		confused += get_rand(0,5) ;
		break;
	case DART_TRAP:
		message(trap_strings[(t*2)+1], 1);
		rogue.hp_current -= get_damage("1d6", 1);
		if (rogue.hp_current <= 0)
		{
			rogue.hp_current = 0;
		}
		if ((!sustain_strength) && rand_percent(40) &&
			(rogue.str_current >= 3))
		{
			rogue.str_current--;
		}
		print_stats(STAT_HP | STAT_STRENGTH);
		if (rogue.hp_current <= 0)
		{
			killed_by((object *) 0, POISON_DART);
		}
		break;
	case SLEEPING_GAS_TRAP:
		message(trap_strings[(t*2)+1], 1);
		take_a_nap();
		break;
	case RUST_TRAP:
		message(trap_strings[(t*2)+1], 1);
		rust((object *) 0);
		break;
	default:
		message( "The trap failed.", 1 ) ;
	}
}

void add_traps(void)
{
	int i, n;
	int tries = 0;
	int row, col;

	if (cur_level <= 2)
	{
		n = 0;
	} else if (cur_level <= 7)
	{
		n = get_rand(0, 2);
	} else if (cur_level <= 11)
	{
		n = get_rand(1, 2);
	} else if (cur_level <= 16)
	{
		n = get_rand(2, 3);
	} else if (cur_level <= 21)
	{
		n = get_rand(2, 4);
	} else if (cur_level <= (AMULET_LEVEL + 2))
	{
		n = get_rand(3, 5);
	} else
	{
		n = get_rand(5, MAX_TRAPS);
	}
	for (i = 0; i < n; i++)
	{
		traps[i].trap_type = get_rand(0, (TRAPS - 1));

		if ((i == 0) && (party_room != NO_ROOM))
		{
			do
			{
				row = get_rand((rooms[party_room].top_row + 1),
						(rooms[party_room].bottom_row - 1));
				col = get_rand((rooms[party_room].left_col + 1),
						(rooms[party_room].right_col - 1));
				tries++;
			} while (((dungeon[row][col] & (OBJECT|STAIRS|TRAP|TUNNEL)) ||
			         (dungeon[row][col] == NOTHING)) && (tries < 15));
			if (tries >= 15)
			{
				gr_row_col(&row, &col, (FLOOR | MONSTER));
			}
		}
		else
		{
			gr_row_col(&row, &col, (FLOOR | MONSTER));
		}
		traps[i].trap_row = row;
		traps[i].trap_col = col;
		dungeon[row][col] |= (TRAP | HIDDEN);
	}
}

void id_trap(void)
{
	short dir, row, col;
	short t;
	char mbuf[DCOLS] ;

	message("Direction? ", 0);

	while (!is_direction(dir = rgetchar()))
	{
		sound_bell();
	}
	check_message();

	if (dir == ROGUE_KEY_CANCEL)
	{
		return;
	}
	row = rogue.row;
	col = rogue.col;

	get_dir_rc(dir, &row, &col, 0);

	if ((dungeon[row][col] & TRAP) && (!(dungeon[row][col] & HIDDEN)))
	{
		t = trap_at(row, col);

		/*
		 * (zerogue 0.4.0) If a trap is disarmed, its type is set to NO_TRAP,
		 * which is defined as -1.  This causes problems when used as an array
		 * index...
		 */
		if( t == NO_TRAP )
			message( "That trap is broken.", 0 ) ;
		else
		{
			sprintf( mbuf, "This is a %s.", trap_strings[t*2] ) ;
			message( mbuf, 0 ) ;
		}
	}
	else
	{
		message("No trap there.", 0);
	}
}

/*
 * (zerogue 0.4.0) Returns the index of the trap at a given row/column.
 */
short get_trap_at( short row, short col )
{
	short i ;

	for( i = 0 ; i < MAX_TRAPS ; i++ )
	{
		if( traps[i].trap_row == row && traps[i].trap_col == col )
			return(i) ;
	}
	return(NO_TRAP) ; // Abuse of constant to mean "no trap at that location"
}

/*
 * (zerogue 0.4.0) Attempt to disarm a revealed trap.  Basically the same as
 * id_trap().
 */
void disarm_trap( short count )
{
	short i, dir, row, col ;

	if( blind )
		message( "You can't see well enough to disarm a trap.", 0 ) ;
	else if( halluc )
		message( "Dude, what trap?  You trippin', man.", 0 ) ;
	else if( confused )
		message( "What's that, you say?  You have an arm for craps?", 0 ) ;
	else
	{
		message( "Direction? ", 0 ) ;

		while( !is_direction( dir = rgetchar() ) )
			sound_bell() ;
		check_message() ;

		if( dir == ROGUE_KEY_CANCEL ) return ;

		row = rogue.row ;
		col = rogue.col ;

		get_dir_rc( dir, &row, &col, 0 ) ;

		if( ( dungeon[row][col] & TRAP ) && ( !( dungeon[row][col] & HIDDEN ) ) )
		{
			short t = get_trap_at(row,col) ;

			if( t == NO_TRAP )
			{
				message( "You don't see a trap there.", 0 ) ;
				return ;
			}
			else if( traps[t].trap_type == NO_TRAP )
			{
				message( "This trap is already broken.", 0 ) ;
				return ;
			}

			for( i = 0 ; i < count ; i++ )
			{
				if( rand_percent( BASE_DISARM_CHANCE + get_rogue_level(1) ) ) // (zerogue 0.4.1)
				{
					char dmsg[DCOLS] ;
					(void)sprintf( dmsg, "You disarmed the %s.", trap_strings[traps[t].trap_type*2] ) ;
					message( dmsg, 1 ) ;
					add_exp( 5, 1 ) ;
					traps[t].trap_type = NO_TRAP ;
					return ;
				}

				if( interrupted ) return ;
			}

			// If the for() loop completes without being broken, then display
			// a failure message.
			message( "You failed to disarm the trap.", 0 ) ;
		}
		else
		{
			message( "You don't see a trap there.", 0 ) ;
		}
	}

	return ;
}

void show_traps(void)
{
	short i, j;

	for (i = 0; i < DROWS; i++)
	{
		for (j = 0; j < DCOLS; j++)
		{
			if (dungeon[i][j] & TRAP)
			{
				mvaddch(i, j, '^');
			}
		}
	}
}

void search(short n, boolean is_auto)
{
	short s, i, j, row, col, t;
	short shown = 0, found = 0;
	static boolean reg_search;
	char mbuf[DCOLS] ;

	for (i = -1; i <= 1; i++)
	{
		for (j = -1; j <= 1; j++)
		{
			row = rogue.row + i;
			col = rogue.col + j;
			if ((row < MIN_ROW) || (row >= (DROWS - 1)) ||
			    (col < 0) || (col >= DCOLS))
			{
				continue;
			}
			if (dungeon[row][col] & HIDDEN)
			{
				found++;
			}
		}
	}

	for (s = 0; s < n; s++)
	{
		for (i = -1; i <= 1; i++)
		{
			for (j = -1; j <= 1; j++)
			{
				row = rogue.row + i;
				col = rogue.col + j ;
				if ((row < MIN_ROW) || (row >= (DROWS - 1)) ||
				    (col < 0) || (col >= DCOLS))
				{
					continue;
				}
				if (dungeon[row][col] & HIDDEN)
				{
					if( rand_percent( 17 + get_rogue_level(1) ) ) // (zerogue 0.4.1)
					{
						dungeon[row][col] &= (~HIDDEN);
						if ((!blind) && ((row != rogue.row) ||
						    (col != rogue.col)))
						{
							mvaddch(row, col, get_dungeon_char(row, col));
						}
						shown++;
						if (dungeon[row][col] & TRAP)
						{
							t = trap_at( row, col ) ;

							if( t == NO_TRAP )
								message( "You found a broken trap.", 1 ) ;
							else
							{
								sprintf( mbuf, "You found a %s.", trap_strings[t*2] ) ;
								message( mbuf, 1 ) ;
							}
						}
					}
				}
				if (((shown == found) && (found > 0)) || interrupted)
				{
					return;
				}
			}
		}
		if ((!is_auto) && (reg_search = !reg_search))
		{
			(void) reg_move();
		}
	}
}
