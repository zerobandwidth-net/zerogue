/*
 * appraise.c
 * New feature in zerogue 0.4.0
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
#include "keys.h"
#include "pack.h"
#include "object.h"
#include "random.h"
#include "message.h"
#include "inventory.h"
#include "level.h"
#include "move.h"
#include "use.h"

extern boolean wizard;
extern struct id id_potions[];
extern struct id id_scrolls[];
extern struct id id_wands[];
extern struct id id_rings[];
extern struct id id_weapons[];
extern struct id id_armors[];
extern fighter rogue;

extern short halluc ;

/**
 * (zerogue 0.4.0) New "appraise" command, allowing a player a random chance
 * of appraising an item's properties without a scroll of identify.
 */
void appraise( short ichar )
{
	short ch = ( ichar ? ichar : 0 ) ;
	char desc[DCOLS] ;
	object *obj ;

	obj = select_from_pack( &ch, "Appraise which item?", ALL_OBJECTS ) ;
	if( !obj ) return ;

	if( halluc )
	{
		message( "All this colorful music is messing with your head, man.", 0 ) ;
		return ;
	}

	/* This turns out to be -way- overpowered if more than about 5%. */
	if( rand_percent( PERFECT_APPRAISAL_CHANCE + (int)(get_rogue_level(1) / 10) ) )
	{
		identify_item(ch) ;
		add_exp(1,1) ;
		return ;
	}

	desc[0] = '(' ;
	desc[1] = ch ;
	desc[2] = ')' ;
	desc[3] = ' ' ;

	if( obj->identified )
		get_desc( obj, desc+4, 1 ) ;
	else
	{
		switch( obj->what_is )
		{
			case ARMOR:
				if( obj->is_cursed )
				{
					if( coin_toss() )
						strcpy( desc+4, "You feel that there is something wrong about this armor." ) ;
					else if( obj->d_enchant != 0 )
						strcpy( desc+4, "This suit of armor seems to be of fine quality." ) ;
					else
						strcpy( desc+4, "There seems to be nothing special about this armor." ) ;
				}
				else
				{
					if( obj->d_enchant < 0 )
						strcpy( desc+4, "This armor seems to be of poor quality." ) ;
					else if( obj->d_enchant > 0 )
						strcpy( desc+4, "This suit of armor seems to be of fine quality." ) ;
					else
						strcpy( desc+4, "There seems to be nothing special about this armor." ) ;
				}
			break ;
			case WEAPON:
				if( obj->is_cursed )
				{
					if( coin_toss() )
						strcpy( desc+4, "You feel that there is something wrong about this weapon." ) ;
					else if( obj->d_enchant != 0 || obj->hit_enchant != 0 )
						strcpy( desc+4, "You feel a sense of power emanating from this weapon." ) ;
					else
						strcpy( desc+4, "There seems to be nothing special about this weapon." ) ;
				}
				else
				{
					if( obj->d_enchant > 0 || obj->hit_enchant > 0 )
						strcpy( desc+4, "You feel a sense of power emanating from this weapon." ) ;
					else if( obj->d_enchant < 0 || obj->hit_enchant < 0 )
						strcpy( desc+4, "This weapon seems to be of shoddy craftsmanship." ) ;
					else
						strcpy( desc+4, "There seems to be nothing special about this weapon." ) ;
				}
			break ;
			case SCROLL:
				switch( obj->which_kind )
				{
					case PROTECT_ARMOR:
					case ENCH_WEAPON:
					case ENCH_ARMOR:
					case REMOVE_CURSE:
						strcpy( desc+4, "The scroll's rods are made of fine steel." ) ;
					break ;
					case HOLD_MONSTER:
					case CREATE_MONSTER:
					case AGGRAVATE_MONSTER:
						strcpy( desc+4, "You notice that the scroll rods end in monster heads." ) ;
					break ;
					case MAGIC_MAPPING:
						strcpy( desc+4, "This scroll seems larger than others you've seen." ) ;
					break ;
					default:
						strcpy( desc+4, "This object seems to be paper, with writing on it." ) ;
				}
			break ;
			case POTION:
				switch( obj->which_kind )
				{
					case INCREASE_STRENGTH:
					case RESTORE_STRENGTH:
					case RAISE_LEVEL:
					case RESTORE_LEVEL: // (zerogue 0.4.1)
						strcpy( desc+4, "This potion looks like a protein shake." ) ;
					break ;
					case HEALING:
					case EXTRA_HEALING:
						strcpy( desc+4, "This looks more like a salve than a potion." ) ;
					break ;
					case DETECT_MONSTER:
					case DETECT_OBJECTS:
					case SEE_INVISIBLE:
						strcpy( desc+4, "The cork on this bottle is an eye dropper." ) ;
					break ;
					case LEVITATION:
					case HASTE_SELF:
						strcpy( desc+4, "The liquid inside is syrupy, and fizzes when shaken." ) ;
					break ;
					case POISON:
					case BLINDNESS:
					case HALLUCINATION:
					case CONFUSION:
						strcpy( desc+4, "The vapors remind you of that night at the brothel..." ) ;
					break ;
					default:
						strcpy( desc+4, "It's definitely a potion." ) ;
				}
			break ;
			case WAND:
				switch( obj->which_kind )
				{
					case TELE_AWAY:
					case SLOW_MONSTER:
					case CONFUSE_MONSTER:
					case POLYMORPH:
					case PUT_TO_SLEEP:
					case CANCELLATION:
						strcpy( desc+4, "This wand crackles with...mischief?" ) ;
					break ;
					case HASTE_MONSTER:
						strcpy( desc+4, "Are those... claw marks?  Hmm..." ) ;
					break ;
					case INVISIBILITY:
						strcpy( desc+4, "Your vision blurs slightly." ) ;
					break ;
					case MAGIC_MISSILE:
						strcpy( desc+4, "This seems like a weapon." ) ;
					break ;
					default:
						strcpy( desc+4, "Aside from its fine workmanship, it's just a stick." ) ;
				}
			break ;
			case RING:
				if( obj->is_cursed && coin_toss() )
					strcpy( desc+4, "The ring seems too eager to be on your finger." ) ;
				else
				{
					switch( obj->which_kind )
					{
						case STEALTH:
						case R_SEE_INVISIBLE:
						case SEARCHING:
							strcpy( desc+4, "You notice a tiny eye carved into the ring." ) ;
						break ;
						case REGENERATION:
						case SLOW_DIGEST:
						case ADD_STRENGTH:
						case DEXTERITY:
							strcpy( desc+4, "A warrior figure is carved into the ring." ) ;
						break ;
						case SUSTAIN_STRENGTH:
							strcpy( desc+4, "A rattlesnake is carved into this ring." ) ;
						break ;
						case ADORNMENT:
							strcpy( desc+4, "This is the most beautiful ring you've seen in a while." ) ;
						break ;
						case MAINTAIN_ARMOR:
							strcpy( desc+4, "An aquator is carved into this ring." ) ;
						break ;
						case R_TELEPORT:
							strcpy( desc+4, "The gemstone's setting looks like an open portal." ) ;
						break ;
						default:
							strcpy( desc+4, "At least it looks like it would fit." ) ;
					}
				}
			break ;
			default:
				get_desc( obj, desc+4, 1 ) ;
		}
	}

	message( desc, 0 ) ;

	(void)reg_move() ;

	return ;
}
