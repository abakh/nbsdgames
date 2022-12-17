APE=/sys/src/ape
<$APE/config

#comment out all that depend on variable size arrays
TARG=\
    battleship \
    checkers \
    darrt \
    fifteen \
    fisher \
    jewels \
    memoblocks \
    miketron \
    mines \
    muncher \
    pipes \
    rabbithole \
    redsquare \
    reversi \
    sos \
    snakeduel \
    sudoku \
    tugow \
    trsr \
    revenge \
    sjump \

BIN=/$objtype/bin/games

UPDATE=\
	mkfile\
	${OFILES:%.$O=%.c}\

</sys/src/cmd/mkmany

#function draw() conflicts with libdraw
CFLAGS= -c -D_POSIX_SOURCE -D_BSD_EXTENSION -DPlan9 -Ddraw=nb_draw

$O.out:	/$objtype/lib/ape/libcurses.a
