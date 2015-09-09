/*
 * floorobject.c
 * Added in zerogue 0.4.2
 */

/* *** INCLUDES ************************************************************ */

#include <ncurses.h>
#include "rogue.h"
#include "floorobject.h"
#include "inventory.h"
#include "keys.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "score.h"
#include "use.h"

/* *** EXTERNAL REFERENCES ************************************************* */

extern fighter rogue ; // object.c
extern int cur_level, max_level ; // level.c
extern unsigned short dungeon[DROWS][DCOLS] ; // object.c
extern struct id id_potions[POTIONS] ; // object.c
extern struct id id_scrolls[SCROLLS] ; // object.c
extern struct id id_weapons[WEAPONS] ; // object.c
extern struct id id_armors[ARMORS] ; // object.c
extern struct id id_wands[WANDS] ; // object.c
extern struct id id_rings[RINGS] ; // object.c
extern object level_objects ; // object.c
extern object level_monsters ; // monster.c

/* *** LOCAL DATA DEFINITIONS ********************************************** */

// Minimum level on which each type of floor object will appear.
short fobj_level[FLOOROBJECTS] =
	{ 0, 8, 12, 14 } ;

// Base chance of each type appearing; these are added to the current level.
short fobj_chance[FLOOROBJECT] =
	{ 0, 42, -2, 6 } ;

/* *** LOCAL FUNCTION DECLARATIONS ***************************************** */

void place_floor_object( short fobtype ) ;
void fobj_alarm( void ) ;

void fobj_transmutate( object * ) ;
unsigned short make_general_item_pack( object *, unsigned short, unsigned short, boolean ) ;
unsigned short make_weapon_pack( object *, boolean identify ) ;
unsigned short make_armor_pack( object *, boolean identify ) ;
unsigned short make_wand_pack( object *, boolean unk ) ;
unsigned short make_ring_pack( object *, boolean unk ) ;
object* choose_transmutator_object( object * ) ;

void fobj_analyze( object * ) ;
unsigned short break_analyzed_object( object *, object * ) ;

void fobj_fount( object * ) ;
void initialize_fountain( object * ) ;
void inspect_fountain( object * ) ;
void drink_from_fountain( object * ) ;

/* *** FUNCTIONS *********************************************************** */

/*
 * (zerogue 0.4.2) Randomly places floor objects of all types.  Replaces
 * put_transmutator().
 */
void put_floor_objects(void)
{
	// Don't provide floor objects on the way up out of the dungeon.
	if( cur_level < max_level ) return ;

	unsigned short i ;

	// For each type of floor object, try to place one.
	// This starts at 1 because 0 is "broken" and won't be used yet.
	for( i = 1 ; i < FLOOROBJECTS ; i++ )
		place_floor_object(i) ;

	return ;
}

/*
 * (zerogue 0.4.3) Place a floor object of the given type.
 */
void place_floor_object( short fobtype )
{
	int row, col ;
	object *fob ;

	// Don't place the object if the player hasn't reached the minimum
	// level for the object's appearance.
	if( cur_level < fobj_level[fobtype] ) return ;

	// Each type of object has a random chance of appearing, which increases
	// the further the player goes into the dungeon.
	if( ! rand_percent( fobj_chance[fobtype] + cur_level ) ) return ;

	// All floor objects are actually wall-hangings.
	gr_row_col( &row, &col, (HORWALL|VERTWALL) ) ;
	dungeon[row][col] |= FLOOROBJECT ;
	fob = alloc_object() ;
	fob->row = row ;
	fob->col = col ;
	fob->what_is = FLOOROBJECT ;
	fob->which_kind = fobtype ;
	fob->identified = 0 ;
	(void)add_to_pack( fob, &level_objects, 0 ) ;

	return ;
}

/*
 * (zerogue 0.4.1) Interact with a floor object.
 */
void fobj_interact( short row, short col )
{
	object *fobj ;
	fobj = object_at( &level_objects, row, col ) ;

	switch( fobj->which_kind )
	{
		case BROKEN_FOBJ:
			message( "This ancient machine appears to be broken.", 1 ) ;
			break ;
		case TRANSMUTATOR:
			fobj_transmutate( fobj ) ;
			break ;
		case ANALYZER:
			fobj_analyze( fobj ) ;
			break ;
		case FOUNTAIN:
			fobj_fount( fobj ) ;
			break ;
		default:
			message( "It's a floor object.", 1 ) ;
	}
}

/*
 * (zerogue 0.4.1) Interact with a transmutator.
 */
void fobj_transmutate( object *tm )
{
	short ch ; // Input character.
	object *obj ; // Object to be traded or created.
	object *tm_pack ; // Holds items to be created.
	unsigned short i = 0 ; // Number of items created in tm_pack

	if( !tm->identified )
	{
		message( "This is a transmutator.", 1 ) ;
		tm->identified = 1 ;
	}
	message( "Feed it [G]old, or an [I]tem?", 0 ) ;
	while( ! is_valid_char( (ch=rgetchar()), "GgIi\033" ) )
		message( "Feed it [G]old, or an [I]tem?", 0 ) ;
	check_message() ;

	if( ch == ROGUE_KEY_CANCEL ) return ; // Cancel.
	else if( is_valid_char( ch, "Ii" ) ) // Transmute item into gold.
	{
		if( ( ch = pack_letter( "Transmute which item?", ALL_OBJECTS ) ) == ROGUE_KEY_CANCEL )
			return ;

		check_message() ;
		if( ! ( obj = get_letter_object(ch) ) ) return ; // Catch invalid letters.
		rogue.gold += get_value(obj) ;
		print_stats(STAT_GOLD) ;
		vanish( obj, 0, &rogue.pack ) ;
		message( "The transmutator takes the item and spits out a bag of gold.", 1 ) ;
		fobj_alarm() ;
	}
	else // Transmute gold into item.
	{
		tm_pack = alloc_object() ;
		tm_pack->next_object = (object *)0 ;

		message( "Type of object?", 0 ) ;

		while( r_index( "!?:)]=/\033", (ch = rgetchar()), 0 ) == -1 )
			sound_bell() ;

		check_message() ;

		if( ch == ROGUE_KEY_CANCEL ) return ;

		switch(ch)
		{
			case '!' : // Make a potion.
				i = make_general_item_pack( tm_pack, POTION, POTIONS, 0 ) ;
				break ;
			case '?' : // Make a scroll.
				i = make_general_item_pack( tm_pack, SCROLL, SCROLLS, 0 ) ;
				break ;
			case ':' : // Make food.
				i = make_general_item_pack( tm_pack, FOOD, 2, 0 ) ;
				break ;
			case ')' : // Make a weapon.
				i = make_weapon_pack( tm_pack, 1 ) ;
				break ;
			case ']' : // Make armor.
				i = make_armor_pack( tm_pack, 1 ) ;
				break ;
			case '/' : // Make a wand.
				i = make_wand_pack( tm_pack, 0 ) ;
				break ;
			case '=' : // Make a ring.
				i = make_ring_pack( tm_pack, 0 ) ;
				break ;
			default :
				message( "The transmutator beeps loudly.", 1 ) ;
				fobj_alarm() ;
				free_stuff( tm_pack ) ;
				return ;
		}

		if( i == 0 )
		{
			message( "You don't recognize any of these items.", 0 ) ;
			free_stuff( tm_pack ) ;
			return ;
		}

		obj = choose_transmutator_object( tm_pack ) ;

		if( obj )
		{
			if( get_value(obj) > rogue.gold )
			{
				message( "You can't afford that.", 1 ) ;
			}
			else if( pack_count(obj) >= MAX_PACK_COUNT )
			{
				message( "You couldn't fit that into your pack.", 1 ) ;
			}
			else
			{
				rogue.gold -= get_value(obj) ;
				print_stats( STAT_GOLD ) ;
				take_from_pack( obj, tm_pack ) ;
				obj = add_to_pack( obj, &rogue.pack, 1 ) ;
				obj->picked_up = 1 ;
				message( "The transmutator grinds loudly as it spits out the item.", 1 ) ;
				fobj_alarm() ;
			}
		}

		free_stuff( tm_pack ) ;
	}
}

/*
 * (zerogue 0.4.1) Makes a pack of items that don't have statistics.
 */
unsigned short make_general_item_pack( object *pack, unsigned short what, unsigned short kinds, boolean unk )
{
	unsigned short k ;
	unsigned short i = 0 ;
	object *obj ;
	struct id *id_table ;

	switch(what)
	{
		case POTION : id_table = id_potions ; break ;
		case SCROLL : id_table = id_scrolls ; break ;
		default : id_table = 0 ;
	}

	for( k = 0 ; k < kinds ; k++ )
	{
		if( !unk && ( what & (POTION|SCROLL) ) && ( id_table[k].id_status == UNIDENTIFIED ) ) continue ;
		obj = alloc_object() ;
		obj->what_is = what ;
		obj->which_kind = k ;
		obj->quantity = 1 ;
		(void)add_to_pack( obj, pack, 0 ) ;
		i++ ;
	}

	return i ;
}

/*
 * (zerogue 0.4.1) Makes a pack of weapons.
 */
unsigned short make_weapon_pack( object *pack, boolean identify )
{
	unsigned short k ;
	unsigned short i = 0 ;
	object *obj ;

	for( k = 0 ; k < WEAPONS ; k++ )
	{
		obj = alloc_object() ;
		obj->what_is = WEAPON ;
		obj->which_kind = k ;
		switch(k)
		{
			case ARROW :	obj->quantity = 30 ;	break ;
			case DAGGER :	obj->quantity = 5 ;		break ;
			case DART :		obj->quantity = 20 ;	break ;
			case SHURIKEN :	obj->quantity = 10 ;	break ;
			default :		obj->quantity = 1 ;
		}
		obj->hit_enchant = obj->d_enchant = obj->is_cursed = 0 ;
		set_weapon_damage(obj) ;
		if( identify )
		{
			id_weapons[k].id_status = IDENTIFIED ;
			obj->identified = 1 ;
		}
		(void)add_to_pack( obj, pack, 0 ) ;
		i++ ;
	}

	return i ;
}

/*
 * (zerogue 0.4.1) Makes a pack of armors.
 */
unsigned short make_armor_pack( object *pack, boolean identify )
{
	unsigned short k ;
	unsigned short i = 0 ;
	object *obj ;

	for( k = 0 ; k < ARMORS ; k++ )
	{
		obj = alloc_object() ;
		obj->what_is = ARMOR ;
		obj->which_kind = k ;
		obj->quantity = 1 ;
		obj->oclass = k+2 ;
		if( ( k == PLATE ) || ( k == SPLINT ) )
			obj->oclass-- ;
		obj->is_protected = obj->d_enchant = 0 ;
		if( identify )
		{
			id_armors[k].id_status = IDENTIFIED ;
			obj->identified = 1 ;
		}
			
		(void)add_to_pack( obj, pack, 0 ) ;
		i++ ;
	}

	return i ;
}

/*
 * (zerogue 0.4.1) Makes a pack of wands.
 */
unsigned short make_wand_pack( object *pack, boolean unk )
{
	unsigned short k ;
	unsigned short i = 0 ;
	object *obj ;

	for( k = 0 ; k < WANDS ; k++ )
	{
		if( !unk && ( id_wands[k].id_status == UNIDENTIFIED ) ) continue ;

		obj = alloc_object() ;
		obj->what_is = WAND ;
		obj->which_kind = k ;
		obj->quantity = 1 ;
		switch(k)
		{
			case MAGIC_MISSILE : obj->oclass = 9 ; break ;
			case CANCELLATION :  obj->oclass = 7 ; break ;
			default :            obj->oclass = 5 ;
		}
		(void)add_to_pack( obj, pack, 0 ) ;
		i++ ;
	}

	return i ;
}

/*
 * (zerogue 0.4.1) Make a pack of rings.
 */
unsigned short make_ring_pack( object *pack, boolean unk )
{
	unsigned short k ;
	unsigned short i = 0 ;
	object *obj ;

	for( k = 0 ; k < RINGS ; k++ )
	{
		if( !unk && ( id_rings[k].id_status == UNIDENTIFIED ) ) continue ;

		obj = alloc_object() ;
		obj->what_is = RING ;
		obj->which_kind = k ;
		obj->quantity = 1 ;
		if( k == ADD_STRENGTH || k == DEXTERITY )
			obj->oclass = 2 ;
		(void)add_to_pack( obj, pack, 0 ) ;
		i++ ;
	}

	return i ;
}

/*
 * (zerogue 0.4.1) If a floor object makes a noise, it wakes up all
 * the monsters within line of sight of the rogue.
 */
void fobj_alarm( void )
{
	object *monster ;

	for( monster = level_monsters.next_object ; monster ; monster = monster->next_object )
	{
		if( rogue_can_see( monster->row, monster->col ) )
		{
			monster->m_flags &= (~ASLEEP) ;
			monster->m_flags &= (~NAPPING) ;
			monster->nap_length = 0 ;
		}
	}
}

/*
 * (zerogue 0.4.1) Choose an object from the transmutator pack.
 * Ideally, this would present a menu that looks like the rogue's pack
 * inventory screen, but I haven't yet figured out a way to do that.
 */
object * choose_transmutator_object( object *pack )
{
	object *obj ;
	char mbuf[DCOLS], dbuf[DCOLS], ch ;
	int value ;

	for( obj = pack->next_object ; obj ; obj = obj->next_object )
	{
		check_message() ;
		get_desc( obj, dbuf, 0 ) ;
		value = get_value(obj) ;
		sprintf( mbuf, "Create %s(%i)? [Y/N]", dbuf, value ) ;
		message( mbuf, 0 ) ;
		while( ! is_valid_char( ch = rgetchar(), "YyNn\033" ) )
			message( mbuf, 0 ) ;
		if( is_valid_char( ch, "Yy" ) )
			return obj ;
		else if( ch == ROGUE_KEY_CANCEL )
			return 0 ;
	}

	message( "The transmutator offers no other choices.", 1 ) ;
	return 0 ;
}

/*
 * (zerogue 0.4.3) Interact with an analyzer.
 */
void fobj_analyze( object *anl )
{
	short ch ; // Input character.
	object *obj ; // Object to be traded or created.

	if( !anl->identified )
	{
		message( "The object consists of an item receptacle and a candy-red button.", 1 ) ;
		anl->identified = 1 ;
	}

	if( ( ch = pack_letter( "Place an item on the pedestal?", ALL_OBJECTS ) ) == ROGUE_KEY_CANCEL )
		return ;

	if( !( obj = get_letter_object(ch) ) ) return ; // Catch invalid letters.

	if( rand_percent( ANALYSIS_BREAK_CHANCE ) )
		if( break_analyzed_object( anl, obj ) )
			return ; // Item is broken; don't identify it.

	identify_item( ch ) ;

	if( rand_percent( ANALYSIS_STEAL_CHANCE ) )
	{
		message( "A clockwork gnome emerges from a hidden compartment and runs away.", 1 ) ;
		steal_gold(0) ;
	}

	return ;
}

/*
 * (zerogue 0.4.3) Breaks an analyzed object.
 */
unsigned short break_analyzed_object( object *anl, object *obj )
{
	switch( obj->what_is )
	{
		case ARMOR: // Only unprotected/unenchanted armor can be destroyed.
			if( obj->is_protected || ( obj->d_enchant > 0 ) )
				return 0 ; // Don't destroy enchanted/protedted objects.
			message( "The suit of armor rusts and crumbles before your eyes.", 1 ) ;
			break ;
		case WEAPON: // Only unprotected/unenchanted weapons can be destroyed.
			if( obj->is_protected || ( obj->d_enchant > 0 ) || ( obj->hit_enchant > 0 ) )
				return 0 ; // Don't destroy enchanted/protected objects.
			message( "The weapon rusts and crumbles before your eyes.", 1 ) ;
			break ;
		case SCROLL: // Any scroll might be destroyed.
			message( "The scroll crumbles to dust and is sucked into the machine.", 1 ) ;
			break ;
		case POTION: // Any potion might be destroyed.
			message( "The flask shatters, and the liquid inside drains away.", 1 ) ;
			break ;
		case FOOD: // Food may be destroyed.
			message( "The food rots instantly, then burns away in a flash.", 1 ) ;
			break ;
		case WAND: // Wands may break; if they do, they make noise.
			message( "The wand vibrates loudly, then snaps in half.", 1 ) ;
			fobj_alarm() ;
			break ;
		case RING: // Any ring may be destroyed.
			message( "The ring melts into a puddle, which drains into the machine.", 1 ) ;
			break ;
		case AMULET: // The Amulet breaks the analyzer.
			message( "The Amulet flashes brilliantly.", 1 ) ;
			anl->which_kind = BROKEN_FOBJ ;
			fobj_alarm() ;
			return 0 ;
		default: // An unrecognized object type sets off the alarm.
			message( "A loud warning buzzer sounds, accompanied by flashing lights.", 1 ) ;
			fobj_alarm() ;
			break ;
	}

	vanish( obj, 0, &rogue.pack ) ;

	return 1 ;
}

/*
 * (zerogue 0.4.3) Interact with a fountain.
 */
void fobj_fount( object *fnt )
{
	char ch ;

	if( !fnt->identified )
	{
		message( "There is a small fountain set into the wall here.", 1 ) ;
		initialize_fountain(fnt) ;
		fnt->identified = 1 ;
	}

	// If the fountain's type is set to the POTIONS constant, it indicates that
	// the potion is a dud; i.e., it dispenses only water.  Also, if a fountain
	// has no charges remaining, its contents have turned to plain water.
	if( fnt->oclass == POTIONS || fnt->quantity == 0 )
	{
		message( "It's a fountain.  It's quite pretty.  Ooo.  Pretty.", 0 ) ;
		return ;
	}

	// The player has the choice of inspecting the fountain to determine what
	// it might do to him, or simply drinking from it.
	do
	{
		message( "[D]rink from the fountain, or [I]nspect it? ", 0 ) ;
	}
	while( ! is_valid_char( (ch=rgetchar()), "DdIi\033" ) ) ;

	// Act on the user's decision.
	if( ch == ROGUE_KEY_CANCEL ) return ;
	else if( is_valid_char( ch, "Ii" ) )
		inspect_fountain(fnt) ;
	else if( is_valid_char( ch, "Dd" ) )
		drink_from_fountain(fnt) ;
	
	return ;
}

/*
 * (zerogue 0.4.3) Initialize a fountain floor object.  The type of potion
 * dispensed by the fountain is set by assigning one of the "which_kind"
 * constants to the fountain's own "oclass" field.  The fountain will dispense
 * only a limited number of these potions before changing to plain water; the
 * number of doses remaining is tracked in the "quantity" field of the
 * fountain.
 */
void initialize_fountain( object *fnt )
{
	// Choose a type of potion to produce.
	int ptype = get_rand( 1, 100 ) ;

	if( ptype == 1 )
	{
		fnt->oclass = RAISE_LEVEL ;
		fnt->quantity = get_rand( 1, 3 ) ;
	}
	else if( ptype <= 5 )
	{
		fnt->oclass = INCREASE_STRENGTH ;
		fnt->quantity = get_rand( 1, 3 ) ;
	}
	else if( ptype <= 25 )
	{
		fnt->oclass = EXTRA_HEALING ;
		fnt->quantity = get_rand( 1, 8 ) ;
	}
	else if( ptype <= 55 )
	{
		fnt->oclass = HEALING ;
		fnt->quantity = get_rand( 4, 16 ) ;
	}
	else if( ptype <= 65 )
	{
		fnt->oclass = DETECT_MONSTER ;
		fnt->quantity = 1 ;
	}
	else if( ptype <= 70 )
	{
		fnt->oclass = DETECT_OBJECTS ;
		fnt->quantity = 1 ;
	}
	else if( ptype <= 75 )
	{
		fnt->oclass = RESTORE_STRENGTH ;
		fnt->quantity = get_rand( 4, 16 ) ;
	}
	else if( ptype <= 80 )
	{
		fnt->oclass = RESTORE_LEVEL ;
		fnt->quantity = get_rand( 2, 8 ) ;
	}
	else if( ptype <= 85 )
	{
		fnt->oclass = POISON ;
		fnt->quantity = get_rand( 1, cur_level ) ;
	}
	else if( ptype <= 90 )
	{
		fnt->oclass = BLINDNESS ;
		fnt->quantity = get_rand( 1, cur_level ) ;
	}
	else if( ptype <= 95 )
	{
		fnt->oclass = HALLUCINATION ;
		fnt->quantity = get_rand( 1, cur_level ) ;
	}
	else
	{
		fnt->oclass = POTIONS ; // Should be handled appropriately by fobj_fount().
		fnt->quantity = 0 ;
	}

	return ;
}

/*
 * (zerogue 0.4.3) Provides an "inspect" function for the fountain.  This is
 * similar to the feedback provided by the "appraise" command for items.
 */
void inspect_fountain( object *fnt )
{
	check_message() ;

	if( fnt->quantity == 0 )
	{
		message( "The fluid seems to be plain water.", 0 ) ;
		return ;
	}

	switch( fnt->oclass )
	{
		case RAISE_LEVEL:
		case INCREASE_STRENGTH:
		case RESTORE_STRENGTH:
		case RESTORE_LEVEL:
			message( "The fluid is warm to the touch.", 0 ) ;
			break ;
		case HEALING:
		case EXTRA_HEALING:
			message( "The fountain's contents appear oily, like an unguent.", 0 ) ;
			break ;
		case DETECT_MONSTER:
		case DETECT_OBJECTS:
		case SEE_INVISIBLE:
			message( "The salty-smelling fluid sparkles brightly, even in the dim light.", 0 ) ;
			break ;
		case LEVITATION:
		case HASTE_SELF:
			message( "The liquid seems to be flowing upward...", 0 ) ;
			break ;
		case POISON:
		case BLINDNESS:
		case HALLUCINATION:
		case CONFUSION:
			message( "The smell of this liquid makes you light-headed.", 0 ) ;
			break ;
		default:
			message( "The fluid seems to be plain water.", 0 ) ;
	}

	return ;
}

/*
 * (zerogue 0.4.3) Handles the wonderful or terrible effects of drinking from a
 * fountain.
 */
void drink_from_fountain( object *fnt )
{
	check_message() ;

	if( fnt->oclass == POTIONS || fnt->quantity == 0 )
		message( "The water in the fountain is cold and refreshing.", 0 ) ;
	else
	{
		apply_potion( fnt->oclass ) ;
		print_stats( ( STAT_STRENGTH | STAT_HP | STAT_EXP ) ) ;
		fnt->quantity-- ;
	}

	return ;
}
