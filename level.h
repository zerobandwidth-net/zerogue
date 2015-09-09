/*
 * level.h
 *
 * Created by Ashwin N.
 */

#ifndef _LEVEL_H_
#define _LEVEL_H_

void make_level(void);

void clear_level(void);

void put_player(int nr);

int drop_check(void);

int check_up(void);

void add_exp(int e, boolean promotion);

int hp_raise(void);

void show_average_hp(void);

short get_rogue_level(boolean apply_mod) ; // (zerogue 0.4.1)

#endif	/* _LEVEL_H_ */
