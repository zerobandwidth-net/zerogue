/*
 * pack.c
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
#include "inventory.h"
#include "keys.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "ring.h"
#include "use.h"

char *curse_message = "You can't, it appears to be cursed.";

extern object level_objects;
extern unsigned short dungeon[DROWS][DCOLS];
extern struct id id_scrolls[];
extern fighter rogue;

object * check_duplicate(object *, object *);
char next_avail_ichar(void);
boolean mask_pack(object *, unsigned short);
int is_pack_letter(short *, unsigned short *);

object * add_to_pack(object *obj, object *pack, int condense)
{
	object *op ;

	if( condense )
	{
		if(( op = check_duplicate( obj, pack ) ))
		{
			/* (zerogue 0.4.0) Moved actual merge from check_duplicate() */
			op->quantity += obj->quantity ;
			free_object(obj) ;
			return(op) ;
		}
		else
		{
			obj->ichar = next_avail_ichar();
		}
	}
	if (pack->next_object == 0)
	{
		pack->next_object = obj;
	}
	else
	{
		op = pack->next_object;

		while (op->next_object)
		{
			op = op->next_object;
		}
		op->next_object = obj;
	}
	obj->next_object = 0;
	return(obj);
}

void take_from_pack(object *obj, object *pack)
{
	while (pack->next_object != obj)
	{
		pack = pack->next_object;
	}
	pack->next_object = pack->next_object->next_object;
}

object * pick_up(int row, int col, short *status)
{
	object *obj;

	obj = object_at(&level_objects, row, col);
	*status = 1;

	if ((obj->what_is == SCROLL) && (obj->which_kind == SCARE_MONSTER)
	    && obj->picked_up)
	{
		message("The scroll turns to dust as you pick it up.", 0);
		dungeon[row][col] &= (~OBJECT);
		vanish(obj, 0, &level_objects);
		*status = 0;
		if (id_scrolls[SCARE_MONSTER].id_status == UNIDENTIFIED)
		{
			id_scrolls[SCARE_MONSTER].id_status = IDENTIFIED;
		}
		return(0);
	}

	if (obj->what_is == GOLD)
	{
		rogue.gold += obj->quantity;
		dungeon[row][col] &= ~(OBJECT);
		take_from_pack(obj, &level_objects);
		print_stats(STAT_GOLD);
		return(obj);	/* obj will be free_object()ed in one_move_rogue() */
	}

	if (pack_count(obj) >= MAX_PACK_COUNT)
	{
		message("Pack too full.", 1);
		return(0);
	}

	dungeon[row][col] &= ~(OBJECT);
	take_from_pack(obj, &level_objects);
	obj = add_to_pack(obj, &rogue.pack, 1);
	obj->picked_up = 1;
	return(obj);
}

void drop(void)
{
	object *obj, *new;
	short ch;
	char desc[DCOLS];

	if (dungeon[rogue.row][rogue.col] & (OBJECT | STAIRS | TRAP))
	{
		message("There's already something there.", 0);
		return;
	}
	if (!rogue.pack.next_object)
	{
		message("You have nothing to drop.", 0);
		return;
	}
	if ((ch = pack_letter("Drop what?", ALL_OBJECTS)) == ROGUE_KEY_CANCEL)
	{
		return;
	}
	if (!(obj = get_letter_object(ch)))
	{
		message("No such item.", 0);
		return;
	}
	if (obj->in_use_flags & BEING_WIELDED)
	{
		if (obj->is_cursed)
		{
			message(curse_message, 0);
			return;
		}
		unwield(rogue.weapon);
	} else if (obj->in_use_flags & BEING_WORN) {
		if (obj->is_cursed)
		{
			message(curse_message, 0);
			return;
		}
		mv_aquatars();
		unwear(rogue.armor);
		print_stats(STAT_ARMOR);
	} else if (obj->in_use_flags & ON_EITHER_HAND) {
		if (obj->is_cursed)
		{
			message(curse_message, 0);
			return;
		}
		un_put_on(obj);
	}
	obj->row = rogue.row;
	obj->col = rogue.col;

	if ((obj->quantity > 1) && (obj->what_is != WEAPON))
	{
		obj->quantity--;
		new = alloc_object();
		*new = *obj;
		new->quantity = 1;
		obj = new;
	}
	else
	{
		obj->ichar = 'L';
		take_from_pack(obj, &rogue.pack);
	}
	place_at(obj, rogue.row, rogue.col);
	(void) strcpy(desc, "Dropped: ");
	get_desc( obj, desc+strlen(desc), 0 ) ;
	message(desc, 0);
	(void) reg_move();
}

object * check_duplicate(object *obj, object *pack)
{
	object *op ;

// (zerogue 0.4.0) Massive simplification here; see CHANGES.
	if( !(is_stackable(obj)) )
	{
		return(0) ;
	}

	op = pack->next_object ;

	while( op )
	{
		if( could_stack_items( op, obj ) )
			return(op) ;

		op = op->next_object ;
	}

	return(0) ;
}

char next_avail_ichar(void)
{
	object *obj;
	int i;
	boolean ichars[26];

	for (i = 0; i < 26; i++)
	{
		ichars[i] = 0;
	}
	obj = rogue.pack.next_object;
	while (obj)
	{
		ichars[(obj->ichar - 'a')] = 1;
		obj = obj->next_object;
	}
	for (i = 0; i < 26; i++)
	{
		if (!ichars[i])
		{
			return(i + 'a');
		}
	}
	return('?');
}

void wait_for_ack(void)
{
	while (rgetchar() != ' ')
		;
}

short pack_letter(char *prompt, unsigned short mask)
{
	short ch;
	unsigned short tmask = mask;

	if (!mask_pack(&rogue.pack, mask))
	{
		message("Nothing appropriate.", 0);
		return(ROGUE_KEY_CANCEL);
	}
	for (;;)
	{

		message(prompt, 0);

		for (;;)
		{
			ch = rgetchar();
			if (!is_pack_letter(&ch, &mask))
			{
				sound_bell();
			}
			else
			{
				break;
			}
		}

		if (ch == ROGUE_KEY_LIST)
		{
			do
			{
				check_message();
				ch = inv_sel(&rogue.pack, mask, " -- Press space or letter --", " !)]*=?abcdefghijklmnopqrstuvwxyz\033");
				mask = tmask;
			} while ((ch != ROGUE_KEY_CANCEL) && (ch != ' ') && !islower(ch) && is_pack_letter(&ch, &mask));
			if (islower(ch))
				break;
		}
		else
		{
			break;
		}
		mask = tmask;
	}
	check_message();
	return(ch);
}

void take_off(void)
{
	char desc[DCOLS];
	object *obj;

	if (rogue.armor)
	{
		if (rogue.armor->is_cursed)
		{
			message(curse_message, 0);
		}
		else
		{
			mv_aquatars();
			obj = rogue.armor;
			unwear(rogue.armor);
			(void) strcpy(desc, "You removed ");
			get_desc( obj, desc+12, 0 ) ;
			message(desc, 0);
			print_stats(STAT_ARMOR);
			(void) reg_move();
		}
	}
	else
	{
		message("You're not wearing any armor.", 0);
	}
}

void wear(void)
{
	short ch;
	object *obj;
	char desc[DCOLS];

	if (rogue.armor)
	{
		message("Your already wearing some.", 0);
		return;
	}
	ch = pack_letter("Wear what?", ARMOR);

	if (ch == ROGUE_KEY_CANCEL)
	{
		return;
	}
	if (!(obj = get_letter_object(ch)))
	{
		message("No such item.", 0);
		return;
	}
	if (obj->what_is != ARMOR)
	{
		message("You can't wear that!", 0);
		return;
	}
	obj->identified = 1;
	(void) strcpy(desc, "You put on ");
	get_desc( obj, desc + 11, 0 ) ;
	message(desc, 0);
	do_wear(obj);
	print_stats(STAT_ARMOR);
	(void) reg_move();
}

void unwear(object *obj)
{
	if (obj)
	{
		obj->in_use_flags &= (~BEING_WORN);
	}
	rogue.armor = (object *) 0;
}

void do_wear(object *obj)
{
	rogue.armor = obj;
	obj->in_use_flags |= BEING_WORN;
	obj->identified = 1;
}

void wield(void)
{
	short ch;
	object *obj;
	char desc[DCOLS];

	if (rogue.weapon && rogue.weapon->is_cursed)
	{
		message(curse_message, 0);
		return;
	}
	ch = pack_letter("Wield what?", WEAPON);

	if (ch == ROGUE_KEY_CANCEL)
	{
		return;
	}
	if (!(obj = get_letter_object(ch)))
	{
		message("No such item.", 0);
		return;
	}
	if (obj->what_is & (ARMOR | RING))
	{
		sprintf(desc, "You can't wield %s.",
			((obj->what_is == ARMOR) ? "armor" : "rings"));
		message(desc, 0);
		return;
	}
	if (obj->in_use_flags & BEING_WIELDED)
	{
		message("In use", 0);
	}
	else
	{
		unwield(rogue.weapon);
		if( obj->what_is == WEAPON )
			obj->identified = 1 ; // (zerogue 0.4.3)
		(void) strcpy(desc, "Wielding ");
		get_desc( obj, desc + 9, 0 ) ;
		message(desc, 0);
		do_wield(obj);
		(void) reg_move();
	}
}

void do_wield(object *obj)
{
	rogue.weapon = obj;
	obj->in_use_flags |= BEING_WIELDED;
}

void unwield(object *obj)
{
	if (obj)
	{
		obj->in_use_flags &= (~BEING_WIELDED);
	}
	rogue.weapon = (object *) 0;
}

void call_it(void)
{
	short ch;
	object *obj;
	struct id *id_table;
	char buf[MAX_TITLE_LENGTH+2];

	ch = pack_letter("Call what?", (SCROLL | POTION | WAND | RING));

	if (ch == ROGUE_KEY_CANCEL)
	{
		return;
	}
	if (!(obj = get_letter_object(ch)))
	{
		message("No such item.", 0);
		return;
	}
	if (!(obj->what_is & (SCROLL | POTION | WAND | RING)))
	{
		message("Surely you already know what that's called!", 0);
		return;
	}
	id_table = get_id_table(obj);

	if (get_input_line("Call it:", buf, id_table[obj->which_kind].title,
			   "", 1, 1))
	{
		id_table[obj->which_kind].id_status = CALLED;
		(void) strcpy(id_table[obj->which_kind].title, buf);
	}
}

short pack_count(object *new_obj)
{
	object *obj ;
	short count = 0 ;

// (zerogue 0.4.0) Massive simplification here; see CHANGES.
// (zerogue 0.4.1) Changed the way that objects are counted.
	obj = rogue.pack.next_object ;

	do
	{
		if( !new_obj )
			count++ ;
		else if( !(could_stack_items( obj, new_obj )) )
			count++ ;
	} while(( obj = obj->next_object )) ;

	return( count ) ;
}

boolean mask_pack(object *pack, unsigned short mask)
{
	while (pack->next_object)
	{
		pack = pack->next_object;
		if (pack->what_is & mask)
		{
			return(1);
		}
	}
	return(0);
}

int is_pack_letter(short *c, unsigned short *mask)
{
	if (((*c == '?') || (*c == '!') || (*c == ':') || (*c == '=')
	    || (*c == ')') || (*c == ']') || (*c == '/') || (*c == ',')))
	{
		switch(*c)
		{
		case '?':
			*mask = SCROLL;
			break;
		case '!':
			*mask = POTION;
			break;
		case ':':
			*mask = FOOD;
			break;
		case ')':
			*mask = WEAPON;
			break;
		case ']':
			*mask = ARMOR;
			break;
		case '/':
			*mask = WAND;
			break;
		case '=':
			*mask = RING;
			break;
		case ',':
			*mask = AMULET;
			break;
		}
		*c = ROGUE_KEY_LIST;
		return(1);
	}
	return(((*c >= 'a') && (*c <= 'z')) || (*c == ROGUE_KEY_CANCEL)
	       || (*c == ROGUE_KEY_LIST));
}

boolean has_amulet(void)
{
	return(mask_pack(&rogue.pack, AMULET));
}

void kick_into_pack(void)
{
    object *obj;
    char desc[DCOLS];
    short n, stat;

    if (!(dungeon[rogue.row][rogue.col] & OBJECT))
    {
        message("Nothing here.", 0);
    }
    else
    {
        if ((obj = pick_up(rogue.row, rogue.col, &stat)))
	{
            get_desc( obj, desc, 1 ) ;
            if (obj->what_is == GOLD)
	    {
                message(desc, 0);
                free_object(obj);
            }
	    else
	    {
                n = strlen(desc);
                desc[n] = '(';
                desc[n+1] = obj->ichar;
               desc[n+2] = ')';
                desc[n+3] = 0;
                message(desc, 0);
            }
        }
        if (obj || (!stat))
	{
            (void) reg_move();
        }
    }
}

object * select_from_pack( short *pch, char *prompt, unsigned short mask )
{
	if( ! is_pack_letter( pch, &mask ) ) *pch = 0 ;
	object *obj = 0 ;

	while( !obj )
	{
		if( !(*pch) ) *pch = pack_letter( prompt, mask ) ;
		if( *pch == ROGUE_KEY_CANCEL ) return 0 ;
		if(!( obj = get_letter_object(*pch) ))
		{
			message( "No such item. Try again.", 0 ) ;
			message( "", 0 ) ;
			check_message() ;
			*pch = 0 ;
		}
	}

	return obj ;
}

