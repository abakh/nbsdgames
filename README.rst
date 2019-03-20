New BSD Games
-------------
*You have a computing machine from 1980's  and you wonder how can you use it?*
 *You deal with a GUI-less machine at work and are looking for ways to kill time?*
  *You have to make a Reversi AI for your homework and you don't know where to copy it from?*
   *You have been so excited about the bsdgames, but have grown tired of playing tetris, snake and robots for billions of times?*
    *You feel they have fooled you by bundling stuff like phantasia with a package you except to contain GAMES?*


**Don't worry** anymore as you've got nbsdgames now!

I originally made these to be added to NetBSD (but the few i talked with preferred to have games in the repositories rather than in /usr/games itself).


These include:

Jewels (A game with a gameplay kinda similiar to that of Tetris, NOT my invention)
Sudoku
Mines (Minesweeper)
Reversi
Checkers
Battleship

Post Made-for-NetBSD games:

SOS
Rabbithole (A maze-exploring game where you have to gather items from all around the maze rather reaching an end,the idea maybe mine)
Pipes (Same as the famous Pipe Mania, unplayable on the environments that don't support the line characters)
Prerequisites
-------------

* make (optional)
* A C compiler with C99 enabled 
* The standard library
* libncurses (the dev package if you are on debian-based distros)

How to run
----------

1) Download the files
2) Go to the sources directory
3) Set the environment variable PREFIX to the address you want them to be in
4) Install

Like this:

.. code::
	cd ~/Downloads/sources
	export PREFIX= ~/bin
	make install 


How do these look like
-----------------------
.. image:: https://raw.githubusercontent.com/untakenstupidnick/new-bsd-games/master/screenshot.png


License
-------
No rights reserved.

I am living outside the Berne convention and therefore no meaningful licensing can be applied (meaning that it's public domain in most of the world).


