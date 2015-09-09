/*
 * use.h
 *
 * Created by Ashwin N.
 */

#ifndef _USE_H_
#define _USE_H_

void quaff(void);

void apply_potion( unsigned short ) ; // (zerogue 0.4.3)

void read_scroll(void);

void vanish(object *obj, short rm, object *pack);

void eat(void);

void tele(void);

void hallucinate(void);

void unhallucinate(void);

void unblind(void);

void relight(void);

void take_a_nap(void);

void confuse(void);

void unconfuse(void);

void identify_item( short ichar ) ;

void sleepify( object *scroll ) ;

#endif	/* _USE_H_ */
