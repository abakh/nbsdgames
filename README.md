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

## Features
* Modern looking TUI
* Color
* Mouse support
* Multiplayer (not network)
* Each game containing help pages on the controls and the gameplay
* Very permissive license (Public Domain equivalent)
* Clean code with documentation
* Small size, with the deb package being less than 50 KB as I write this
* Low in dependencies and very simple build, even in comparison to bsdgames
* Low in CPU usage (Low if you are playing against AI, virtually nothing otherwise)
* Low as possible in memory footprint
* Diversity, you will like one at least
* Portability
* Flexiblity

They natively run on Linux, BSD, MacOS and are known to work on Windows as well (using PDCurses, thanks to Laura Michaels for providing advice).

The difficulty and/or dimensions are adjustable through simple command line options, you can play a minesweeper game that take hours to complete, or exprience hexadecimal sudoku and 8x8 fifteen-like puzzles!

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

It's been made available for openSUSE thanks to Jan Brezina: https://build.opensuse.org/package/show/home:Zinjanthropus/nbsdgames
## How do these look like
![Screenshot from 4 games in tmux](https://raw.githubusercontent.com/untakenstupidnick/new-bsd-games/master/screenshot.png)


## How to contribute
Oh, so kind! You can...
* Share these with your friends
* Tell me your feature requests, bug reports, games you want to be added etc. (Think of games in spirit of those already included and look if there is not a good  terminal game for it already)
* Make a package for your distro (or put it on repos and tell me afterwards)
* Tell me if you're interested in porting it to non-Unix (Possible in theory since there is PDCurses for SDL, and SDL for everything)
* With mouse support, PDCurses, SDL and NDK existing, now it is possible to port it to Android proper, feel free to tell me if you are doing it.
* Pull requests are of course, welcome too!
