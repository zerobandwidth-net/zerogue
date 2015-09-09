/*
 * special_hit.c
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
#include <string.h>
#include "rogue.h"
#include "hit.h"
#include "inventory.h"
#include "level.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "score.h"
#include "special_hit.h"
#include "use.h"

short less_hp = 0;
char *flame_name = "flame";
boolean being_held;
short stolen_gold = 0 ; // (zerogue 0.4.0) Pot of leprechauns' stolen gold.

extern int cur_level;
extern short max_level, blind, levitate, ring_exp, expmod ; // (zerogue 0.4.1) added expmod
extern long level_points[];
extern boolean mon_disappeared;
extern boolean sustain_strength, maintain_armor;
extern char *you_can_move_again;
extern fighter rogue;
extern unsigned short dungeon[DROWS][DCOLS];
extern object level_monsters;
extern room rooms[];
extern object level_objects;

void freeze(object *);
void sting(object *);
void drain_life(void);
void drop_level(void);
void steal_item(object *);
void disappear(object *);
int try_to_cough(short, short, object *);
int gold_at(short, short);
void get_closer(short *, short *, short, short);

void special_hit(object *monster)
{
	if ((monster->m_flags & CONFUSED) && rand_percent(66))
	{
		return;
	}
	if (monster->m_flags & RUSTS)
	{
		rust(monster);
	}
	if ((monster->m_flags & HOLDS) && !levitate)
	{
		being_held = 1;
	}
	if (monster->m_flags & FREEZES)
	{
		freeze(monster);
	}
	if (monster->m_flags & STINGS)
	{
		sting(monster);
	}
	if (monster->m_flags & DRAINS_LIFE)
	{
		drain_life();
	}
	if (monster->m_flags & DROPS_LEVEL)
	{
		drop_level();
	}
	if (monster->m_flags & STEALS_GOLD)
	{
		steal_gold(monster);
	}
	else
	{	if (monster->m_flags & STEALS_ITEM)
		{
			steal_item(monster);
		}
	}
}

void rust(object *monster)
{
	if ((!rogue.armor) || (get_armor_class(rogue.armor) <= 1)
	    || (rogue.armor->which_kind == LEATHER))
	{
		return;
	}
	if ((rogue.armor->is_protected) || maintain_armor)
	{
		if (monster && (!(monster->m_flags & RUST_VANISHED)))
		{
			message("The rust vanishes instantly.", 0);
			monster->m_flags |= RUST_VANISHED;
		}
	}
	else
	{
		rogue.armor->d_enchant--;
		message("Your armor weakens.", 0);
		print_stats(STAT_ARMOR);
	}
}

void freeze(object *monster)
{
	short freeze_percent = 99;
	short i, n;

	if (rand_percent(12))
	{
		return;
	}
	freeze_percent -= (rogue.str_current+(rogue.str_current / 2));
	freeze_percent -= ( get_rogue_level(1) * 4 ) ; // (zerogue 0.4.1)
	freeze_percent -= (get_armor_class(rogue.armor) * 5);
	freeze_percent -= (rogue.hp_max / 3);

	if (freeze_percent > 10)
	{
		monster->m_flags |= FREEZING_ROGUE;
		message("You are frozen.", 1);

		n = get_rand(4, 8);
		for (i = 0; i < n; i++)
		{
			mv_mons();
		}
		if (rand_percent(freeze_percent))
		{
			for (i = 0; i < 50; i++)
			{
				mv_mons();
			}
			killed_by((object *)0, HYPOTHERMIA);
		}
		message(you_can_move_again, 1);
		monster->m_flags &= (~FREEZING_ROGUE);
	}
}

void steal_gold(object *monster)
{
	int amount ;

	if( (rogue.gold <= 0) || rand_percent(10) ) return ;

	amount = get_rand( (cur_level*10), (cur_level * 30) ) ;

	if( amount > rogue.gold )
		amount = rogue.gold ;

	rogue.gold -= amount ;
	stolen_gold += amount ; // (zerogue 0.4.0) Track stolen gold over time.
	message( "Your purse feels lighter.", 0 ) ;
	print_stats( STAT_GOLD ) ;
	if(monster) disappear(monster) ; // (zerogue 0.4.3) Allows use of function without a monster.
}

void steal_item(object *monster)
{
	object *obj;
	unsigned int i;
	int n;
	int t = 0;
	char desc[80];
	boolean has_something = 0;

	if( rand_percent(15) ) return ;

	obj = rogue.pack.next_object ;

	if( ! obj ) // Pack was already empty.
	{
		disappear(monster) ;
		return ;
	}

	while( obj ) // Search for an unequipped item to steal.
	{
		if (!(obj->in_use_flags & BEING_USED))
		{
			has_something = 1 ;
			break ;
		}

		obj = obj->next_object ;
	}

	if( !has_something ) // All objects are in use.
	{
		disappear(monster) ;
		return ;
	}

	n = get_rand(0, MAX_PACK_COUNT) ;
	obj = rogue.pack.next_object ;

	for (i = 0; i <= n; i++)
	{
		obj = obj->next_object;
		while ((!obj) || (obj->in_use_flags & BEING_USED))
		{
			if (!obj)
			{
				obj = rogue.pack.next_object;
			}
			else
			{
				obj = obj->next_object;
			}
		}
	}
	(void) strcpy(desc, "she stole ") ;
	if( obj->what_is != WEAPON )
	{
		t = obj->quantity ;
		obj->quantity = 1 ;
	}
	get_desc( obj, desc+10, 0 ) ;
	message(desc, 0) ;

	obj->quantity = ((obj->what_is != WEAPON) ? t : 1);

	vanish(obj, 0, &rogue.pack);

	disappear(monster);
}

void disappear(object *monster)
{
	int row, col;

	row = monster->row;
	col = monster->col;

	dungeon[row][col] &= ~MONSTER;
	if (rogue_can_see(row, col))
	{
		mvaddch(row, col, get_dungeon_char(row, col));
	}
	take_from_pack(monster, &level_monsters);
	free_object(monster);
	mon_disappeared = 1;
}

void cough_up(object *monster)
{
	object *obj;
	short row, col, i, n;

	if (cur_level < max_level)
	{
		return;
	}

	if( monster->m_flags & STEALS_GOLD )
	{
		obj = alloc_object() ;
		obj->what_is = GOLD ;
		/*
		 * (zerogue 0.4.0) A killed leprechaun drops all gold in the pot.  If
		 * there is no gold in the pot, one gold piece is dropped.
		 */
		if( stolen_gold > 0 )
		{
			obj->quantity = stolen_gold ;
			stolen_gold = 0 ;
		}
		else obj->quantity = 1 ;

		// The old algorithm was: get_rand((cur_level * 15), (cur_level * 30))
	}
	else
	{
		if (!rand_percent((int) monster->drop_percent))
		{
			return;
		}
		obj = gr_object();
	}
	row = monster->row;
	col = monster->col;

	for (n = 0; n <= 5; n++)
	{
		for (i = -n; i <= n; i++)
		{
			if (try_to_cough(row+n, col+i, obj))
			{
				return;
			}
			if (try_to_cough(row-n, col+i, obj))
			{
				return;
			}
		}
		for (i = -n; i <= n; i++)
		{
			if (try_to_cough(row+i, col-n, obj))
			{
				return;
			}
			if (try_to_cough(row+i, col+n, obj))
			{
				return;
			}
		}
	}
	free_object(obj);
}

int try_to_cough(short row, short col, object *obj)
{
	if ((row < MIN_ROW) || (row > (DROWS-2)) || (col < 0)
	    || (col>(DCOLS-1)))
	{
		return(0);
	}
	if ((!(dungeon[row][col] & (OBJECT | STAIRS | TRAP)))
	    && (dungeon[row][col] & (TUNNEL | FLOOR | DOOR)))
	{
		place_at(obj, row, col);
		if (((row != rogue.row) || (col != rogue.col))
		    && (!(dungeon[row][col] & MONSTER)))
		{
			mvaddch(row, col, get_dungeon_char(row, col));
		}
		return(1);
	}
	return(0);
}

int seek_gold(object *monster)
{
	short i, j, rn, s;

	if ((rn = get_room_number(monster->row, monster->col)) < 0)
	{
		return(0);
	}
	for (i = rooms[rn].top_row+1; i < rooms[rn].bottom_row; i++)
	{
		for (j = rooms[rn].left_col+1; j < rooms[rn].right_col; j++)
		{
			if ((gold_at(i, j)) && !(dungeon[i][j] & MONSTER))
			{
				monster->m_flags |= CAN_FLIT;
				s = mon_can_go(monster, i, j);
				monster->m_flags &= (~CAN_FLIT);
				if (s)
				{
					move_mon_to(monster, i, j);
					monster->m_flags |= ASLEEP;
					monster->m_flags &= (~(WAKENS | SEEKS_GOLD));
					return(1);
				}
				monster->m_flags &= (~SEEKS_GOLD);
				monster->m_flags |= CAN_FLIT;
				mv_monster(monster, i, j);
				monster->m_flags &= (~CAN_FLIT);
				monster->m_flags |= SEEKS_GOLD;
				return(1);
			}
		}
	}
	return(0);
}

int gold_at(short row, short col)
{
	if (dungeon[row][col] & OBJECT)
	{
		object *obj;

		if ((obj = object_at(&level_objects, row, col)) &&
				(obj->what_is == GOLD))
		{
			return(1);
		}
	}
	return(0);
}

void check_gold_seeker(object *monster)
{
	monster->m_flags &= (~SEEKS_GOLD);
}

int check_imitator(object *monster)
{
	char msg[80];

	if (monster->m_flags & IMITATES)
	{
		wake_up(monster);
		if (!blind)
		{
			mvaddch(monster->row, monster->col,
			        get_dungeon_char(monster->row, monster->col));
			check_message();
			sprintf(msg, "Wait, that's a %s!", mon_name(monster));
			message(msg, 1);
		}
		return(1);
	}
	return(0);
}

int imitating(short row, short col)
{
	if (dungeon[row][col] & MONSTER)
	{
//		object *object_at(), *monster;
		object *monster;

		if ((monster = object_at(&level_monsters, row, col)))
		{
			if (monster->m_flags & IMITATES)
			{
				return(1);
			}
		}
	}
	return(0);
}

void sting(object *monster)
{
	short sting_chance = 35;
	char msg[80];

	if ((rogue.str_current <= 3) || sustain_strength)
	{
		return;
	}
	sting_chance += (6 * (6 - get_armor_class(rogue.armor)));

	if( get_rogue_level(1) > 8 ) // (zerogue 0.4.1)
		sting_chance -= (6 * ( get_rogue_level(1) - 8) ) ;
	
	if (rand_percent(sting_chance))
	{
		sprintf(msg, "The %s's bite has weakened you.",
		mon_name(monster));
		message(msg, 0);
		rogue.str_current--;
		print_stats(STAT_STRENGTH);
	}
}

/*
 * (zerogue 0.4.1) Modified the drop_level() function such that it will apply a
 * "negative level" to the rogue, rather than actually modifying the rogue's
 * experience level.
 */
void drop_level(void)
{
	int hp_drop, hp_diff ;

	if( rand_percent(80) || (rogue.exp <= 5) ) return ;

	expmod -= 1 ; // Apply a negative level.

	hp_diff = rogue.hp_max - rogue.hp_current ; // Preserve the old difference, if any.

	hp_drop = hp_raise() ; // Calculate a drop in max HP.

	if( rogue.hp_max > hp_drop ) // If max HP can handle the drop, apply it.
		rogue.hp_max -= hp_drop ;
	else // Otherwise, set max HP to 1 instead of killing the rogue.
		rogue.hp_max = 1 ;

	rogue.hp_current = rogue.hp_max + hp_diff ; // Apply the old difference in HP.
	if( rogue.hp_current < 1 ) rogue.hp_current = 1 ; // If this would kill the rogue, set current HP to 1.

	print_stats( STAT_EXP ) ;

/* LinuxRogue 0.3.2 algorithm saved for reference
	int hp ;

	if( rand_percent(80) || (rogue.exp <= 5) ) return ;

	rogue.exp_points = level_points[rogue.exp-2] - get_rand(9, 29);
	rogue.exp -= 2;
	hp = hp_raise();
	if ((rogue.hp_current -= hp) <= 0)
	{
		rogue.hp_current = 1;
	}
	if ((rogue.hp_max -= hp) <= 0)
	{
		rogue.hp_max = 1;
	}
	add_exp(1, 0);
*/
}

void drain_life(void)
{
	short n;

	if (rand_percent(60) || (rogue.hp_max <= 30) || (rogue.hp_current < 10))
	{
		return;
	}
	n = get_rand(1, 3);		/* 1 Hp, 2 Str, 3 both */

	if ((n != 2) || (!sustain_strength))
	{
		message("You feel weaker.", 0);
	}
	if (n != 2)
	{
		rogue.hp_max--;
		rogue.hp_current--;
		less_hp++;
	}
	if (n != 1)
	{
		if ((rogue.str_current > 3) && (!sustain_strength))
		{
			rogue.str_current--;
			if (coin_toss())
			{
				rogue.str_max--;
			}
		}
	}
	print_stats((STAT_STRENGTH | STAT_HP));
}

int m_confuse(object *monster)
{
	char msg[80];

	if (!rogue_can_see(monster->row, monster->col))
	{
		return(0);
	}
	if (rand_percent(45))
	{
		monster->m_flags &= (~CONFUSES);	/* will not confuse the rogue */
		return(0);
	}
	if (rand_percent(55))
	{
		monster->m_flags &= (~CONFUSES);
		sprintf(msg, "The gaze of the %s has confused you.", mon_name(monster));
		message(msg, 1);
		confuse();
		return(1);
	}
	return(0);
}

int flame_broil(object *monster)
{
	short row, col;

	if ((!mon_sees(monster, rogue.row, rogue.col)) || coin_toss())
	{
		return(0);
	}
	row = rogue.row - monster->row;
	col = rogue.col - monster->col;
	if (row < 0)
	{
		row = -row;
	}
	if (col < 0)
	{
		col = -col;
	}
	if (((row != 0) && (col != 0) && (row != col))
	    || ((row > 7) || (col > 7)))
	{
		return(0);
	}
	if ((!blind) && (!rogue_is_around(monster->row, monster->col)))
	{
		row = monster->row;
		col = monster->col;
		get_closer(&row, &col, rogue.row, rogue.col);
		standout();
		do
		{
			mvaddch(row, col, '~');
			refresh();
			get_closer(&row, &col, rogue.row, rogue.col);
		} while ((row != rogue.row) || (col != rogue.col));
		standend();
		row = monster->row; col = monster->col;
		get_closer(&row, &col, rogue.row, rogue.col);
		do
		{
			mvaddch(row, col, get_dungeon_char(row, col));
			refresh();
			get_closer(&row, &col, rogue.row, rogue.col);
		} while ((row != rogue.row) || (col != rogue.col));
	}
	mon_hit(monster, flame_name, 1);
	return(1);
}

void get_closer(short *row, short *col, short trow, short tcol)
{
	if (*row < trow)
	{
		(*row)++;
	}
	else
	{
		if (*row > trow)
		{
			(*row)--;
		}
	}
	if (*col < tcol)
	{
		(*col)++;
	}
	else
	{
		if (*col > tcol)
		{
			(*col)--;
		}
	}
}
