zerogue
=======

A fork of LinuxRogue 0.3.2.

Well, I had the commercial version of Rogue on a 5.25" floppy disk, and played
it like crazy when I was a kid.  So, when I grew up into a halfway-competent
hacker, I found a copy of the Rogue 5.3 clone's source, and decided to use it
as a coding exercise.  Also, I needed something to do when work was slow, and
what the heck, it's C.  Everybody loves C, right?  Sure they do.

Anyway, enjoy this version of Rogue, with its new modern-RPG-inspired features.

Pasha Paterson <pasha.paterson@gmail.com>

Changes Since the Fork
----------------------

* Hit messages now show damage dealt.
* Lots of tweaks to monster damage ratings.
* Adjusted the effect of healing potions.
* Rogue can now "appraise" an item to get a hint of what it does.
* Added indicators for whether an armor is blessed/cursed.
* Adjusted hunger rate, especially as it related to rings.
* Identical items will now stack, if stacking them makes sense.
* Rogue can now "disarm" a visible trap.
* Throwing a harmful status potion at a monster can inflict the status.
* Gaining a level now heals the rogue's HP. (Ding!)
* Revised the mechanic for the scroll of sleep.
* Gold stolen by leprechauns may be recovered by defeating one.
* Items stolen by nymphs may be recovered by defeating one.
* Rogues teleported by traps are now briefly confused.
* Lower floors of the dungeon contain various "floor objects":
  - The "transmutator" converts items to gold, and vice versa.
  - The "analyzer" identifies an item... or might break it.
  - The "fountain" can act as a potion.
* The level drain effect has been adjusted and is curable.
* Revised item stacking logic to be more similar to modern games.
* Weapons and armor self-identify when equipped.
