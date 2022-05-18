# New BSD Games
 *You have a computing machine from 1980's  and you wonder how you can use it? <br/>
  You are a bored sysadmin with no work, and need to kill time looking busy with terminal?  <br/>
  You have Plan9 dual-booted with OpenBSD and have kept the OpenBSD just for gaming? <br/>
  Your port of Linux to a fancy platform has no GUI, but you still want to find a use for it?<br/>
  You the DSL developer and have cancelled the project because you lacked games? <br/>
  You have to make a Reversi AI for your homework and you don't know where to copy it from? <br/>
  Your port of Linux to a fancy platform has no GUI, but you still want nice screenshots?<br/>
  You have been so excited about the bsdgames, but have grown tired of playing tetris, snake and robots for billions of times? <br/>
  Are you feeling that betrayed you by bundling stuff like phantasia in a package you expected to contain GAMES?*<br/>
**Don't worry** anymore as you've got nbsdgames now!

The games include:
 
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
* Darrt (with original gameplay!)
* Snakeduel
* Tugow (Numpad practice game)

The difficulty and/or dimensions are adjustable through simple command line options, you can play a minesweeper game that take hours to complete, or exprience hexadecimal sudoku and 8x8 fifteen-like puzzles!

Or just enter "nbsdgames" at your terminal to get a fancy menu and play all sorts of games from there.

 
Play on xterm for best experience.

## Prerequisites

* git (optional)
* POSIX make (optional)
* A C compiler with C99 enabled 
* The standard library
* ncurses (libncurses5-dev if you are on debian-based distros)

To install them all on debian-base :

``` sh
        sudo apt install git make gcc libncurses5-dev
```
## How to run

1) Download the files
2) Go to the sources directory
3) Install

Like this:

``` sh
        git clone https://github.com/abakh/nbsdgames
        cd ./nbsdgames
        make
        sudo make install # or use the binaries already compiled
```
## Platforms

They natively run on Linux, BSD, MacOS and are known to work on Windows as well (using PDCurses, thanks to Laura Michaels for providing advice).

They have been ported to Plan9 thanks to Jens Staal!

Thanks to PDCurses they even work on DOS and every platform with SDL.

They should theoretically work on OS/2 as well but I have not verified that yet.

## Packages

Packages are available for multiple distributions thanks to individual packagers.

[![Packaging status](https://repology.org/badge/vertical-allrepos/nbsdgames.svg)](https://repology.org/project/nbsdgames/versions)

## How do these look like
Linux+xterm+tmux
![Screenshot from 4 games in tmux](https://raw.githubusercontent.com/abakh/junk/master/screenshot.png)

Plan9
![Screenshot from the games in Plan9](https://raw.githubusercontent.com/abakh/junk/master/screenshot_plan9.png)

Windows
![Screenshot from the games in Windows 7](https://raw.githubusercontent.com/abakh/junk/master/screenshot_windows.jpg)

## How to contribute
* Share these with your friends and others
* Your stars make the repo more findable in Github :star:
* Tell me your feature requests, bug reports, etc.
* Tell me the games you want to be added (but in the same genre, I can't port Angry Birds to curses! :)
* Make a package for your distro (or put it on repos if the package is not there)
* Getting it to Redhat and SUSE repos would be nice.
* Tell distro developers to consider adding these as default games, nbsdgames packs a lot of fun games in a few hundreds of kilobytes.
Also thank to all the people who helped in the previous versions, all what I requested was done! I didn't expect such an amount of assistance on this project :heart:
