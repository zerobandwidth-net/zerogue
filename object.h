/*
 * object.h
 *
 * Created by Ashwin N.
 */

#ifndef _OBJECT_H_
#define _OBJECT_H_

void put_objects(void);

void place_at(object *obj, int row, int col);

object * object_at(object *pack, short row, short col);

object * get_letter_object(short ch);

void free_stuff(object *objlist);

char * name_of(object *obj);

object * gr_object(void);

void get_food(object *obj, boolean force_ration);

void put_stairs(void);

int get_armor_class(object *obj);

object * alloc_object(void);

void free_object(object *obj);

void show_objects(void);

void put_amulet(void);

void new_object_for_wizard(void);

boolean is_stackable( object * ) ; // (zerogue 0.4.0)

boolean could_stack_items( object *, object * ) ; // (zerogue 0.4.0)

#endif	/* _OBJECT_H_ */
