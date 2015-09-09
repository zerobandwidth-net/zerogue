/*
 * main.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "rogue.h"
#include "init.h"
#include "level.h"
#include "main.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "play.h"
#include "trap.h"
#include "floorobject.h" // (zerogue 0.4.2)

extern short party_room;
extern object level_objects;
extern object level_monsters;

int saved_uid;
int true_uid;

void turn_into_games(void)
{
	if(setuid(saved_uid) == -1)
	{
		perror("setuid restore failed!");
		clean_up("");
	}
}

void turn_into_user(void)
{
	if(setuid(true_uid)==-1)
	{
		perror("setuid(restore)");
		clean_up("");
	}
}

int main(int argc, char *argv[])
{
	/* Save the setuid we have got, then turn back into the player */
	saved_uid = geteuid();
	setuid(true_uid = getuid());

	if (init(argc, argv))	/* restored game */
	{
		/* These are the three lines from the game's main loop that
		   need to be executed when restoring a saved game's state. */
		play_level() ;
		free_stuff(&level_objects);
		free_stuff(&level_monsters);
	}

	for (;;)
	{
		clear_level();
		make_level();
		put_objects();
		put_stairs();
		put_floor_objects() ; // (zerogue 0.4.2)
		add_traps();
		put_mons();
		put_player(party_room);
		print_stats(STAT_ALL);
		
		play_level();
		free_stuff(&level_objects);
		free_stuff(&level_monsters);
	}

	return 0;
}
