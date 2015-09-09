/*
 * score.h
 *
 * Created by Ashwin N.
 */

#ifndef _SCORE_H_
#define _SCORE_H_

void killed_by(object *monster, int other);

void win(void);

void quit(boolean from_intrpt);

void put_scores(object *monster, int other);

int is_vowel(char ch);

void name_cipher(char *buf, int n);

long char_cipher(boolean st);

#endif	/* _SCORE_H_ */
