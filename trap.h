/*
 * trap.h
 *
 * Created by Ashwin N.
 */

#ifndef _TRAP_H_
#define _TRAP_H_

struct tr
{
	short trap_type;
	short trap_row, trap_col;
};

typedef struct tr trap;

void trap_player(short row, short col);

void add_traps(void);

void id_trap(void);

void show_traps(void);

short get_trap_at( short row, short col ) ; // (zerogue 0.4.0)

void disarm_trap( short count ) ; // (zerogue 0.4.0)

void search(short n, boolean is_auto);

#endif	/* _TRAP_H_ */
