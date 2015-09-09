/*
 * throw.c
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
#include "keys.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"
#include "room.h"
#include "special_hit.h"
#include "throw.h"
#include "use.h"
#include "zap.h"

extern char *curse_message;
extern char hit_message[];
extern fighter rogue;
extern unsigned short dungeon[DROWS][DCOLS];
extern object level_monsters;
extern struct id id_potions[] ; // (zerogue 0.4.0) Needed to identify potions that hit monsters.

object * get_thrown_at_monster(object *, short, short *, short *);
int throw_at_monster(object *, object *);
void flop_weapon(object *, short, short);
void identify_thrown_object( object * ) ; // (zerogue 0.4.0) Identify objects that inflict status on monsters.

void throw(void)
{
	short wch;
	boolean first_miss = 1;
	object *weapon;
	short dir, row, col;
	object *monster;

	while (!is_direction(dir = rgetchar()))
	{
		sound_bell();
		if (first_miss)
		{
			message("Direction? ", 0);
			first_miss = 0;
		}
	}
	check_message();
	if (dir == ROGUE_KEY_CANCEL)
	{
		return;
	}

	if( ( wch = pack_letter( "Throw what?", (WEAPON|POTION) ) ) == ROGUE_KEY_CANCEL )
		return ;

	check_message() ;

	if (!(weapon = get_letter_object(wch)))
	{
		message("No such item.", 0);
		return;
	}
	if ((weapon->in_use_flags & BEING_USED) && weapon->is_cursed)
	{
		message(curse_message, 0);
		return;
	}
	row = rogue.row; col = rogue.col;

	if ((weapon->in_use_flags & BEING_WIELDED) && (weapon->quantity <= 1))
	{
		unwield(rogue.weapon);
	} else if (weapon->in_use_flags & BEING_WORN) {
		mv_aquatars();
		unwear(rogue.armor);
		print_stats(STAT_ARMOR);
	} else if (weapon->in_use_flags & ON_EITHER_HAND) {
		un_put_on(weapon);
	}
	monster = get_thrown_at_monster(weapon, dir, &row, &col);
	mvaddch(rogue.row, rogue.col, rogue.fchar);
	refresh();

	if (rogue_can_see(row, col) && ((row != rogue.row) || (col != rogue.col))){
		mvaddch(row, col, get_dungeon_char(row, col));
	}
	if (monster)
	{
		wake_up(monster);
		check_gold_seeker(monster);

		if (!throw_at_monster(monster, weapon))
		{
			flop_weapon(weapon, row, col);
		}
	}
	else
	{
		flop_weapon(weapon, row, col);
	}
	vanish(weapon, 1, &rogue.pack);
}

int throw_at_monster(object *monster, object *weapon)
{
	short damage, hit_chance;
	short t;

	hit_chance = get_hit_chance(weapon);
	damage = get_weapon_damage(weapon);
	if ((weapon->which_kind == ARROW)
	    && (rogue.weapon && (rogue.weapon->which_kind == BOW)))
	{
		damage += get_weapon_damage(rogue.weapon);
		damage = ((damage * 2) / 3);
		hit_chance += (hit_chance / 3);
	}
	else
	{
		if ((weapon->in_use_flags & BEING_WIELDED)
		    && ((weapon->which_kind == DAGGER)
		    || (weapon->which_kind == SHURIKEN)
			|| (weapon->which_kind == DART)))
		{
			damage = ((damage * 3) / 2);
			hit_chance += (hit_chance / 3);
		}
	}
	t = weapon->quantity;
	weapon->quantity = 1;
	weapon->quantity = t;

	if (!rand_percent(hit_chance))
	{
		sprintf( hit_message, "The %smisses.", name_of(weapon) ) ;
		return(0);
	}
	char* mn = mon_name(monster) ;
	if ((weapon->what_is == WAND) && rand_percent(75))
	{
		zap_monster(monster, weapon->which_kind);
	}
	else if( weapon->what_is == POTION )
	{
		if( rand_percent( 25 + get_rogue_level(1) ) ) // (zerogue 0.4.1)
		{
			switch( weapon->which_kind )
			{
				case INCREASE_STRENGTH:
					if( monster->m_hit_chance < 96 )
						monster->m_hit_chance += 5 ;
					sprintf( hit_message,
					         "The %s glows red for a moment.",
					         mn
					       ) ;
					identify_thrown_object( weapon ) ;
				break ;
				case EXTRA_HEALING:
					monster->hp_to_kill += get_rand(1,12) ;
				case HEALING:
					monster->hp_to_kill += get_rand(1,6) ;
					sprintf( hit_message,
					         "The %s seems healthier.",
					         mn
					       ) ;
					identify_thrown_object( weapon ) ;
				break ;
				case BLINDNESS:
					monster->m_hit_chance = monster->first_level ;
					sprintf( hit_message,
					         "The %s begins flailing about blindly.",
					         mn
					       ) ;
					identify_thrown_object( weapon ) ;
				break ;
				case HALLUCINATION:
				case CONFUSION:
					monster->m_flags |= CONFUSED ;
					monster->moves_confused = get_rand(1,6) + get_rogue_level(1) ;
					sprintf( hit_message,
					         "The %s seems disoriented.",
					         mn
					       ) ;
					identify_thrown_object( weapon ) ;
				break ;
				case POISON:
					monster->damage = "1d2" ;
					sprintf( hit_message,
					         "The %s appears visibly weaker.",
					         mn
					       ) ;
					identify_thrown_object( weapon ) ;
				break ;
				default:
					sprintf( hit_message,
					         "The flask shatters on the %s's body, but nothing happens.",
					         mn
				           ) ;
			}
		}
		else
		{
			sprintf( hit_message,
			         "The flask bounces off the %s and shatters on the floor.", mn
			       ) ;
		}
	}
	else
	{
		sprintf( hit_message,
		         "The %shits the %s. (%i)  ",
		         name_of(weapon),
		         mn,
		         damage
		       ) ;
		if( ! mon_damage( monster, damage ) )
			sprintf( hit_message + strlen(hit_message), "Defeated the %s.", mn ) ;
	}
	return(1);
}

object * get_thrown_at_monster(object *obj, short dir, short *row,
                                      short *col)
{
	short orow, ocol;
	short i, ch;

	orow = *row; ocol = *col;

	ch = get_mask_char(obj->what_is);

	for (i = 0; i < 24; i++)
	{
		get_dir_rc(dir, row, col, 0);
		if ((dungeon[*row][*col] == NOTHING)
		    || ((dungeon[*row][*col] & (HORWALL | VERTWALL | HIDDEN))
		    && (!(dungeon[*row][*col] & TRAP))))
		{
			*row = orow;
			*col = ocol;
			return(0);
		}
		if ((i != 0) && rogue_can_see(orow, ocol))
		{
			mvaddch(orow, ocol, get_dungeon_char(orow, ocol));
		}
		if (rogue_can_see(*row, *col))
		{
			if (!(dungeon[*row][*col] & MONSTER))
			{
				mvaddch(*row, *col, ch);
			}
			refresh();
		}
		orow = *row; ocol = *col;
		if (dungeon[*row][*col] & MONSTER)
		{
			if (!imitating(*row, *col))
			{
				return(object_at(&level_monsters, *row, *col));
			}
		}
		if (dungeon[*row][*col] & TUNNEL)
		{
			i += 2;
		}
	}
	return(0);
}

void flop_weapon(object *weapon, short row, short col)
{
	object *new_weapon, *monster;
	short i = 0;
	char msg[80];
	boolean found = 0;
	short mch, dch;
	unsigned short mon;

	while ((i < 9) && dungeon[row][col] & ~(FLOOR | TUNNEL | DOOR | MONSTER))
	{
		rand_around(i++, &row, &col);
		if ((row > (DROWS-2)) || (row < MIN_ROW)
		    || (col > (DCOLS-1)) || (col < 0) || (!dungeon[row][col])
			|| (dungeon[row][col] & ~(FLOOR | TUNNEL | DOOR | MONSTER)))
		{
			continue;
		}
		found = 1;
		break;
	}

	if (found || (i == 0))
	{
		new_weapon = alloc_object();
		*new_weapon = *weapon;
		new_weapon->in_use_flags = NOT_USED;
		new_weapon->quantity = 1;
		new_weapon->ichar = 'L';
		place_at(new_weapon, row, col);
		if (rogue_can_see(row, col)
		    && ((row != rogue.row) || (col != rogue.col)))
		{
			mon = dungeon[row][col] & MONSTER;
			dungeon[row][col] &= (~MONSTER);
			dch = get_dungeon_char(row, col);
			if (mon)
			{
				mch = mvinch(row, col);
				if ((monster = object_at(&level_monsters, row, col)))
				{
					monster->trail_char = dch;
				}
				if ((mch < 'A') || (mch > 'Z'))
				{
					mvaddch(row, col, dch);
				}
			}
			else
			{
				mvaddch(row, col, dch);
			}
			dungeon[row][col] |= mon;
		}
	}
	else
	{
		short t;

		t = weapon->quantity;
		weapon->quantity = 1;
		sprintf(msg, "The %svanishes as it hits the ground.",
		        name_of(weapon));
		weapon->quantity = t;
		message(msg, 0);
	}
}

void rand_around(short i, short *r, short *c)
{
	static char pos[] = "\010\007\001\003\004\005\002\006\0";
	static short row, col;
	short j;

	if (i == 0)
	{
		short x, y, o, t;

		row = *r;
		col = *c;

		o = get_rand(1, 8);

		for (j = 0; j < 5; j++)
		{
			x = get_rand(0, 8);
			y = (x + o) % 9;
			t = pos[x];
			pos[x] = pos[y];
			pos[y] = t;
		}
	}
	switch((short)pos[i])
	{
	case 0:
		*r = row + 1;
		*c = col + 1;
		break;
	case 1:
		*r = row + 1;
		*c = col - 1;
		break;
	case 2:
		*r = row - 1;
		*c = col + 1;
		break;
	case 3:
		*r = row - 1;
		*c = col - 1;
		break;
	case 4:
		*r = row;
		*c = col + 1;
		break;
	case 5:
		*r = row + 1;
		*c = col;
		break;
	case 6:
		*r = row;
		*c = col;
		break;
	case 7:
		*r = row - 1;
		*c = col;
		break;
	case 8:
		*r = row;
		*c = col - 1;
		break;
	}
}

/*
 * (zerogue 0.4.0) An object that hits a monster and inflicts a status effect
 * should be identified.
 */
void identify_thrown_object( object *obj )
{
    switch( obj->what_is )
	{
		case POTION:
			if( id_potions[obj->which_kind].id_status != CALLED )
				id_potions[obj->which_kind].id_status = IDENTIFIED ;
			break ;
		default: ; // Do nothing.
	}

	return ;
}
