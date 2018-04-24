/*
 * pack.h
 *
 * Created by Ashwin N.
 */

#ifndef _PACK_H_
#define _PACK_H_

object * add_to_pack(object *obj, object *pack, int condense);

void take_from_pack(object *obj, object *pack);

object * pick_up(int row, int col, short *status);

void drop(void);

void wait_for_ack(void);

short pack_letter(char *prompt, unsigned short mask);

void take_off(void);

void wear(void);

void unwear(object *obj);

void do_wear(object *obj);

void wield(void);

void do_wield(object *obj);

void unwield(object *obj);

void call_it(void);

short pack_count(object *new_obj);

boolean has_amulet(void);

void kick_into_pack(void);

/**
 * (0.4.5) Centralize a "select an item from a pack" function.
 * short *pch - Pointer to an input character value, so we can overwrite it.
 * char *prompt - Prompt to be displayed to the player.
 * unsigned short mask - category of objects to be selected, or ALL_OBJECTS
 */
object * select_from_pack( short *pch, char *prompt, unsigned short mask ) ;

#endif	/* _PACK_H_ */
