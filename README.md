# New BSD Games
 *You have a computing machine from 1980's  and you wonder how you can use it? <br/>
  You deal with a GUI-less machine at work and are looking for ways to kill time? <br/>
   You are the DSL developer and have cancelled the project because you lacked games? <br/>
    You have to make a Reversi AI for your homework and you don't know where to copy it from? <br/>
     You have been so excited about the bsdgames, but have grown tired of playing tetris, snake and robots for billions of times? <br/>
     You feel they have betrayed you by bundling stuff like phantasia with a package you expect to contain GAMES?* <br/>


**Don't worry** anymore as you've got nbsdgames now!

I originally made these in hope of them becoming added to NetBSD (but the few i talked to preferred to have games in the repositories rather than in /usr/games itself).

These include:

* Jewels (A game with a gameplay kinda similiar to that of Tetris, NOT my invention)
* Sudoku
* Mines (Minesweeper)
* Reversi
* Checkers
* Battleship
* SOS
* Rabbithole (A maze-exploring game where you have to gather items from all around the maze rather than reaching an end, the idea maybe mine)
* Pipes (Same as the famous Pipe Mania, unplayable on the environments that don't support the line characters)
* Fifteen
* Memoblocks (or Memory blocks. A similar game was included in Windows 7)
* Fisher
* Muncher
* Miketron
* Redsquare (Conway's Game of Life made playable!)

They natively run on Linux, BSD, MacOS and are known to work on Windows as well (using PDCurses, thanks to Laura Michaels for providing advice).

They have been ported to Plan9 by Jens Staal!

The difficulty and/or dimensions are adjustable through simple command line options, you can play a minesweeper game that take hours to complete, or exprience hexadecimal sudoku and 8x8 fifteen-like puzzles!

I have encounterd Fisher, Muncher and Miketron on MikeOS and RedSquare on KolibriOS. 
## Prerequisites

* git (optional)
* POSIX make (optional)
* A C compiler with C99 enabled 
* The standard library
* ncurses (libncurses5-dev if you are on debian-based distros)

## How to run

1) Download the files
2) Go to the sources directory
3) Set the environment variable PREFIX to the address you want them to be in
4) Install

Like this:

``` sh
	 git clone https://github.com/untakenstupidnick/nbsdgames
        cd ./nbsdgames/sources
        export PREFIX= ~/bin
        make install
```

Also, If you are on a debian-based OS on a 64-bit PC you can download the deb package and simply install it with dpkg or apt.
the deb package: https://github.com/untakenstupidnick/nbsdgames/releases/download/v2.0/nbsdgames_amd64.deb

It's available on AUR thanks to Elias Riedel Gårding: https://aur.archlinux.org/packages/nbsdgames-git/
(The commands start with  nbsd_ to avoid conflict)

It's been made available for openSUSE thanks to Zinjanthropus: https://build.opensuse.org/package/show/home:Zinjanthropus/nbsdgames
## How do these look like
Linux+xterm+tmux
![Screenshot from 4 games in tmux](https://raw.githubusercontent.com/untakenstupidnick/nbsdgames/master/screenshot.png)

Plan9
![Screenshot from the games in Plan9](https://raw.githubusercontent.com/untakenstupidnick/nbsdgames/master/screenshot_plan9.png)

## How to contribute
* Share these with your friends and others
* Your stars make the repo more findable in github
* Tell me your feature requests, bug reports, etc.
* Tell me the games you want to be added (but in the same genre, i can't port Angry Birds to curses! :)
* Make a package for your distro (or put it on repos if the package is not there)
* Does anyone understand debian's .orig.tar.gz and the process to make packages to the repos? Any help would be appreciated.


Also thank to all the people who helped in the previous versions, all what i requested was done! i didn't expect such an amount of assistance on this project :heart:
