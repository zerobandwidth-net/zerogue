/*
 * inventory.h
 *
 * Created by Ashwin N.
 */

#ifndef _INVENTORY_H_
#define _INVENTORY_H_

void inventory(object *pack, unsigned short mask);

void mix_colors(void);

void make_scroll_titles(void);

void get_desc(object *obj, char *desc, boolean capitalize );

void get_wand_and_ring_materials(void);

void single_inv(short ichar);

struct id * get_id_table(object *obj);

int get_id_table_dim( object *obj ) ;

void inv_armor_weapon(boolean is_weapon);

#endif	/* _INVENTORY_H_ */
