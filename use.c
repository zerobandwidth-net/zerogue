/*
 * use.c
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
#include "inventory.h"
#include "keys.h"
#include "level.h"
#include "machdep.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"
#include "room.h"
#include "use.h"

short halluc = 0;
short blind = 0;
short confused = 0;
short levitate = 0;
short haste_self = 0;
short expmod = 0 ; // (zerogue 0.4.1) modifier to the rogue's experience level
boolean see_invisible = 0;
short extra_hp = 0;
boolean detect_monster = 0;
char *strange_feeling = "You have a strange feeling for a moment, then it passes.";

extern int bear_trap;
extern char hunger_str[];
extern int cur_room;
extern long level_points[];
extern boolean being_held;
extern char *fruit, *you_can_move_again;
extern boolean sustain_strength;
extern fighter rogue;
extern object level_monsters;
extern object level_objects;
extern struct id id_scrolls[];
extern struct id id_potions[];
extern unsigned short dungeon[DROWS][DCOLS];
extern room rooms[];
// extern char *m_names[] ; // DEBUG

void potion_heal(int);
void go_blind(void);
void hold_monster(void);
char * get_ench_color(void);
void uncurse_all(void);

void quaff(void)
{
	short ch;
	object *obj;

	ch = pack_letter("Quaff what?", POTION);

	if( ch == ROGUE_KEY_CANCEL ) return ;

	if( !(obj = get_letter_object(ch)) )
	{
		message( "No such item.", 0 ) ;
		return ;
	}
	if( obj->what_is != POTION )
	{
		message( "You can't drink that!", 0 ) ;
		return ;
	}

	apply_potion( obj->which_kind ) ; // (zerogue 0.4.3)

	print_stats( ( STAT_STRENGTH | STAT_HP | STAT_EXP ) ) ;

	if( id_potions[obj->which_kind].id_status != CALLED )
		id_potions[obj->which_kind].id_status = IDENTIFIED ;

	vanish( obj, 1, &rogue.pack ) ;
}

/*
 * (zerogue 0.4.3) Applies the effect of a potion to the rogue.
 */
void apply_potion( unsigned short pkind )
{
	char buf[80] ;

	switch( pkind )
	{
		case INCREASE_STRENGTH:
			message( "You feel stronger now, what bulging muscles!", 0 ) ;
			rogue.str_current++ ;
			rogue.str_max++ ; // (zerogue 0.4.1)
			break ;
		case RESTORE_STRENGTH:
			rogue.str_current = rogue.str_max;
			message( "This tastes great, you feel warm all over.", 0 ) ;
			break ;
		case HEALING:
			message( "You begin to feel better.", 0 ) ;
			potion_heal(0) ;
			break ;
		case EXTRA_HEALING:
			message( "You begin to feel much better.", 0 ) ;
			potion_heal(1) ;
			break ;
		case POISON:
			if( !sustain_strength )
			{
				rogue.str_current -= get_rand( 1, 3 ) ;
				if( rogue.str_current < 1 )
					rogue.str_current = 1 ;
			}
			message( "You feel very sick now.", 0 ) ;
			if( halluc ) unhallucinate() ;
			break ;
		case RAISE_LEVEL:
			rogue.exp_points = level_points[rogue.exp - 1] ;
			add_exp( 1, 1 ) ;
			break ;
		case BLINDNESS:
			go_blind() ;
			break ;
		case HALLUCINATION:
			message( "Oh wow, everything seems so cosmic.", 0 ) ;
			halluc += get_rand( 500, 800 ) ;
			break ;
		case DETECT_MONSTER:
			show_monsters() ;
			if( !(level_monsters.next_monster) )
				message( strange_feeling, 0 ) ;
			break;
		case DETECT_OBJECTS:
			if( level_objects.next_object )
				if( !blind ) show_objects() ;
			else
				message( strange_feeling, 0 ) ;
			break;
		case CONFUSION:
			message( ( halluc ? "What a trippy feeling." :
			                    "You feel confused." ), 0 ) ;
			confuse() ;
			break;
		case LEVITATION:
			message( "You start to float in the air.", 0 ) ;
			levitate += get_rand( 15, 30 ) ;
			being_held = 0 ;
		   	bear_trap = 0 ;
			break ;
		case HASTE_SELF:
			message( "You feel yourself moving much faster.", 0 ) ;
			haste_self += get_rand( 11, 21 ) ;
			if( !(haste_self % 2) ) haste_self++ ;
			break ;
		case SEE_INVISIBLE:
			sprintf( buf, "Hmm ... this potion tastes like %sjuice.", fruit ) ;
			message( buf, 0 ) ;
			if( blind ) unblind() ;
			see_invisible = 1 ;
			relight() ;
			break ;
		case RESTORE_LEVEL:
			if( expmod < 0 )
			{
				// Restore lost hit points.
				short i ;
				for( i = 0 ; i > expmod ; i-- )
					rogue.hp_max += hp_raise() ;
				expmod = 0 ;  // Clear negative levels.
				message( "You feel your lost vitality returning.", 0 ) ;
			}
			else
				message( "Your thirst is thoroughly quenched.", 0 ) ;
			break ;
		default:
			message( "Nothing happens.", 0 ) ;
	}

	return ;
}

void read_scroll(void)
{
	short ch;
	object *obj;
	char msg[DCOLS];

	if(blind)
	{
		message("You can't see to read the scroll.",0);
		return;
	}

	ch = pack_letter("Read what?", SCROLL);

	if (ch == ROGUE_KEY_CANCEL)
	{
		return;
	}
	if (!(obj = get_letter_object(ch)))
	{
		message("No such item.", 0);
		return;
	}
	if (obj->what_is != SCROLL)
	{
		message("You can't read that!", 0);
		return;
	}
	switch(obj->which_kind)
	{
		case SCARE_MONSTER:
			message("You hear a maniacal laughter in the distance.", 0);
			break;
		case HOLD_MONSTER:
			hold_monster();
			break;
		case ENCH_WEAPON:
			if (rogue.weapon)
			{
				if (rogue.weapon->what_is == WEAPON)
				{
					sprintf(msg, "Your %sglow%s %sfor a moment.",
					name_of(rogue.weapon),
					((rogue.weapon->quantity <= 1) ? "s" : ""),
					get_ench_color());
					message(msg, 0);
					if (coin_toss())
					{
						rogue.weapon->hit_enchant++;
					}
					else
					{
						rogue.weapon->d_enchant++;
					}
				}
				rogue.weapon->is_cursed = 0;
			}
			else
			{
				message("Your hands tingle.", 0);
			}
			break;
		case ENCH_ARMOR:
			if (rogue.armor)
			{
				sprintf(msg, "Your armor glows %sfor a moment.",
				get_ench_color());
				message(msg, 0);
				rogue.armor->d_enchant++;
				rogue.armor->is_cursed = 0;
				print_stats(STAT_ARMOR);
			}
			else
			{
				message("Your skin crawls.", 0);
			}
			break;
		case IDENTIFY:
			message("This is a scroll of identify.", 0);
			obj->identified = 1;
			id_scrolls[obj->which_kind].id_status = IDENTIFIED;
			identify_item(0) ;
			break;
		case TELEPORT:
			tele();
			/* (zerogue 0.4.0) The first time a rogue uses this scroll,
			 * he is briefly confused.
			 */
			if( ! obj->identified && id_scrolls[obj->which_kind].id_status != IDENTIFIED )
				confused += get_rand(0,5) ;
			break;
		case SLEEP:
			sleepify(obj) ;
			break ;
		case PROTECT_ARMOR:
			if (rogue.armor)
			{
				message("Your armor is covered by a shimmering gold shield.", 0);
				rogue.armor->is_protected = 1;
				rogue.armor->is_cursed = 0;
			}
			else
			{
				message("Your acne seems to have disappeared.", 0);
			}
			break;
		case REMOVE_CURSE:
				message((!halluc) ?
					"You feel as though someone is watching over you." :
					"You feel in touch with the universal oneness.", 0);
			uncurse_all();
			break;
		case CREATE_MONSTER:
			/* (zerogue 0.4.0) A summoned monster is briefly confused. */
			create_monster(1) ;
			break;
		case AGGRAVATE_MONSTER:
			aggravate();
			break;
		case MAGIC_MAPPING:
			message("This scroll seems to have a map on it.", 0);
			draw_magic_map();
			break;
	}
	if (id_scrolls[obj->which_kind].id_status != CALLED)
	{
		id_scrolls[obj->which_kind].id_status = IDENTIFIED;
	}
	vanish(obj, (obj->which_kind != SLEEP), &rogue.pack);
}

/* vanish() does NOT handle a quiver of weapons with more than one
   arrow (or whatever) in the quiver.  It will only decrement the count.
*/

void vanish(object *obj, short rm, object *pack)
{
	if (obj->quantity > 1)
	{
		obj->quantity--;
	}
	else
	{
		if (obj->in_use_flags & BEING_WIELDED)
		{
			unwield(obj);
		} else if (obj->in_use_flags & BEING_WORN) {
			unwear(obj);
		} else if (obj->in_use_flags & ON_EITHER_HAND) {
			un_put_on(obj);
		}
		take_from_pack(obj, pack);
		free_object(obj);
	}
	if (rm)
	{
		(void) reg_move();
	}
}

void potion_heal(int extra)
{
	int ratio;
	short add = 0 ;

	if( rogue.hp_current < rogue.hp_max )
	{
		int i ;

		for( i = 0 ; i < get_rogue_level(1) ; i++ ) // (zerogue 0.4.1)
			add += get_rand(1,6) ;

		add += get_rogue_level(1) ; // (zerogue 0.4.1)

		if( ( rogue.hp_current + add > rogue.hp_max ) && ! extra )
			rogue.hp_current = rogue.hp_max ;
		else
			rogue.hp_current += add ;
	}

	if( blind ) unblind() ;
	
	if( confused && extra )
		unconfuse() ;
	else if (confused)
		confused = (confused / 2) + 1 ;

	if( halluc && extra )
		unhallucinate() ;
	else if( halluc )
		halluc = (halluc / 2) + 1 ;
}

void identify_item( short ichar )
{
	short ch = 0 ;
	object *obj;
	struct id *id_table;
	char desc[DCOLS];

	if( ichar ) ch = ichar ;

    while( !ch ) /* Until valid input is given...*/
    {
		ch = pack_letter( "What would you like to identify?", ALL_OBJECTS ) ;
 
		if( ch == ROGUE_KEY_CANCEL )
		{
			return ;
		}
		if( !(obj = get_letter_object(ch)) )
		{
			message( "No such item. Try again.", 0 ) ;
			message( "", 0 ) ;
			check_message() ;
			ch = 0 ;
		}
	}

	obj->identified = 1;
	if (obj->what_is & (SCROLL | POTION | WEAPON | ARMOR | WAND | RING))
	{
		id_table = get_id_table(obj);
		id_table[obj->which_kind].id_status = IDENTIFIED;
	}
	get_desc( obj, desc, 1 ) ;
	message(desc, 0);
}

void eat(void)
{
	short ch;
	short moves;
	object *obj;
	char buf[70];

	ch = pack_letter("Eat what?", FOOD);

	if (ch == ROGUE_KEY_CANCEL)
	{
		return;
	}
	if (!(obj = get_letter_object(ch)))
	{
		message("No such item.", 0);
		return;
	}
	if (obj->what_is != FOOD)
	{
		message("You can't eat that!", 0);
		return;
	}
	if ((obj->which_kind == FRUIT) || rand_percent(60))
	{
		moves = get_rand(900, 1100);
		if (obj->which_kind == RATION)
		{
			message("Yum, that tasted good.", 0);
		}
		else
		{
			sprintf(buf, "My, that was a yummy %s\b.", fruit);
			message(buf, 0);
		}
	}
	else
	{
		moves = get_rand(700, 900);
		message("Yuk, that food tasted awful.", 0);
		add_exp(2, 1);
	}
	rogue.moves_left /= 3;
	rogue.moves_left += moves;
	hunger_str[0] = 0;
	print_stats(STAT_HUNGER);

	vanish(obj, 1, &rogue.pack);
}

void hold_monster(void)
{
	short i, j;
	short mcount = 0;
	object *monster;
	short row, col;

	for (i = -2; i <= 2; i++)
	{
		for (j = -2; j <= 2; j++)
		{
			row = rogue.row + i;
			col = rogue.col + j;
			if ((row < MIN_ROW) || (row > (DROWS-2)) || (col < 0) ||
				 (col > (DCOLS-1)))
			{
				continue;
			}
			if (dungeon[row][col] & MONSTER)
			{
				monster = object_at(&level_monsters, row, col);
				monster->m_flags |= ASLEEP;
				monster->m_flags &= (~WAKENS);
				mcount++;
			}
		}
	}
	if (mcount == 0)
	{
		message("You feel a strange sense of loss.", 0);
	} else if (mcount == 1) {
		message("The monster freezes.", 0);
	} else {
		message("The monsters around you freeze.", 0);
	}
}

void tele(void)
{
	mvaddch(rogue.row, rogue.col, get_dungeon_char(rogue.row, rogue.col));

	if (cur_room >= 0)
	{
		darken_room(cur_room);
	}
	put_player(get_room_number(rogue.row, rogue.col));
	being_held = 0;
	bear_trap = 0;
}

void hallucinate(void)
{
	object *obj, *monster;
	char ch;

	if (blind) return;

	obj = level_objects.next_object;

	while (obj)
	{
		ch = mvinch(obj->row, obj->col);
		if (((ch < 'A') || (ch > 'Z')) && ((obj->row != rogue.row)
		    || (obj->col != rogue.col)))
		{
			if ((ch != ' ') && (ch != '.') && (ch != '#') && (ch != '+'))
			{
				addch(gr_obj_char());
			}
		}
		obj = obj->next_object;
	}
	monster = level_monsters.next_monster;

	while (monster)
	{
		ch = mvinch(monster->row, monster->col);
		if ((ch >= 'A') && (ch <= 'Z'))
		{
			addch(get_rand('A', 'Z'));
		}
		monster = monster->next_monster;
	}
}

void unhallucinate(void)
{
	halluc = 0;
	relight();
	message("Everything looks so boring now.", 1);
}

void unblind(void)
{
	blind = 0;
	message("The veil of darkness lifts.", 1);
	relight();
	if (halluc)
	{
		hallucinate();
	}
	if (detect_monster)
	{
		show_monsters();
	}
}

void relight(void)
{
	if (cur_room == PASSAGE)
	{
		light_passage(rogue.row, rogue.col);
	}
	else
	{
		light_up_room(cur_room);
	}
	mvaddch(rogue.row, rogue.col, rogue.fchar);
}

void take_a_nap(void)
{
	short i;

	i = get_rand(2, 5);
	md_sleep(1);

	while (i--)
	{
		mv_mons();
	}
	md_sleep(1);
	message(you_can_move_again, 0);
}

void go_blind(void)
{
	int i, j;

	if (!blind)
	{
		message("A cloak of darkness falls around you.", 0);
	}
	blind += get_rand(500, 800);

	if (detect_monster)
	{
		object *monster;

		monster = level_monsters.next_monster;

		while (monster)
		{
			mvaddch(monster->row, monster->col, monster->trail_char);
			monster = monster->next_monster;
		}
	}
	if (cur_room >= 0)
	{
		for (i = rooms[cur_room].top_row + 1;
			 i < rooms[cur_room].bottom_row; i++)
		{
			for (j = rooms[cur_room].left_col + 1;
				 j < rooms[cur_room].right_col; j++)
			{
				mvaddch(i, j, ' ');
			}
		}
	}
	mvaddch(rogue.row, rogue.col, rogue.fchar);
}

char * get_ench_color(void)
{
	if (halluc)
	{
		return(id_potions[get_rand(0, POTIONS-1)].title);
	}
	return("blue ");
}

void confuse(void)
{
	confused += get_rand(12, 22);
}

void unconfuse(void)
{
	char msg[80];

	confused = 0;
	sprintf(msg, "You feel less %s now.", (halluc ? "trippy" : "confused"));
	message(msg, 1);
}

void uncurse_all(void)
{
	object *obj;

	obj = rogue.pack.next_object;

	while (obj)
	{
		obj->is_cursed = 0;
		obj = obj->next_object;
	}
}

void sleepify( object *scroll )
{
	/* (zerogue 0.4.0) Added mechanic where a scroll of sleep can be used to
	 * put nearby monsters to sleep.  Modelled after hold_monster().
	 */
	short mcount = 0 ;
	short row, col ;

	if( scroll->identified || id_scrolls[scroll->which_kind].id_status == IDENTIFIED )
	{
		for( row = (rogue.row - 2) ; row <= (rogue.row + 2) ; row++ )
		{
			for( col = (rogue.col - 2) ; col <= (rogue.col + 2) ; col++ )
			{
				if( (row<MIN_ROW) || (row>(DROWS-2)) || (col<0) || (col>(DCOLS-1)) )
					continue ;

				if( dungeon[row][col] & MONSTER )
				{
					object *monster = object_at( &level_monsters, row, col ) ;
					monster->m_flags |= ( ASLEEP | NAPPING ) ;
					monster->nap_length += get_rand( 3, ( get_rogue_level(1) + 2 ) ) ; // (zerogue 0.4.1)
					++mcount ;
				}
			}
		}
	}

	if( mcount == 0 )
	{
		message( "You fall asleep.", 0 ) ;
		take_a_nap() ;
	}
	else if( mcount == 1 )
		message( "The monster falls asleep!", 0 ) ;
	else
		message( "The monsters around you fall asleep!", 0 ) ;

	return ;
}
