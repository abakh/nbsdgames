/*
 _        ___ 
(_'        |   
._)QUARE (_:UMP

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <curses.h>
#include <unistd.h>
#include "config.h"
#include "common.h"
#define SAVE_TO_NUM 10
#define LEN 24
#define HLEN LEN/2
#define WID 80
#define HWID WID/2
#define SIZE 12
#define SHOTS_WHEN_STARTING 10
#define DELAY 50000
#define byte int
#define randint(a,b) ((a)+(rand()%((b+1)-(a))))
#ifdef Plan9
int usleep(long usec) {
    int second = usec/1000000;
    long nano = usec*1000 - second*1000000;
    struct timespec sleepy = {0};
    sleepy.tv_sec = second;
    sleepy.tv_nsec = nano;
    nanosleep(&sleepy, (struct timespec *) NULL);
    return 0;
}
#endif
// 12 lines of water
// 80 columns
enum {LOSE,WIN};
chtype colors[5]={0};
long score=0,jumps=0;
FILE* scorefile;

chtype background[LEN][WID];

int input;
char msg[150]={0};
byte msg_show=0;

bool timed[3];
byte squarex[3];
byte squarey[3];
byte loops_left=0;
int oy=0;
int ox=0;
float rotation_angle=0;
int combo=1;
void filled_rect(byte sy,byte sx,byte ey,byte ex){
	byte y,x;
	for(y=sy;y<ey;++y)
		for(x=sx;x<ex;++x)
			mvaddch(y,x,' ');
}
void red_border(void){
	byte y,x;
	for(y=0;y<LEN;++y){
		mvaddch(y,WID-1,' '|A_STANDOUT|colors[0]);
		mvaddch(y,0,' '|A_STANDOUT|colors[0]);
	}
	for(x=0;x<WID;++x){
		mvaddch(LEN-1,x,' '|A_STANDOUT|colors[0]);
		mvaddch(0,x,' '|A_STANDOUT|colors[0]);
	}
		
}

float center_distance(byte y,byte x){
	//y distance is twice accounted for. visual reasons
	return sqrt( (y-HLEN)*(y-HLEN)+0.25*(x-HWID)*(x-HWID) );
}
void star_line(byte y){
	for(byte x=1;x<WID-1;++x)
		mvaddch(y,x,'.');
}
void logo(){
	mvprintw(0,0," _        ___  ");
	mvprintw(1,0,"(_'        |     Score: %d",score);
	mvprintw(2,0,"._)QUARE (_:UMP  Combo: %d",combo);
	addch('\n');
}

byte save_score(void){
	return fallback_to_home("sjump_scores",score,SAVE_TO_NUM);

}

void show_scores(byte playerrank){
	erase();
	logo();
	if(playerrank==FOPEN_FAIL){
		mvaddstr(3,0,"Could not open score file");
		printw("\nHowever, your score is %ld.",score);
		refresh();
		return;
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(score_file);
		fscanf(score_file,"%*s : %*d\n");
		move(4,0);
		byte b=0;
		if ( fscanf(score_file,"%s : %ld\n",formername,&formerscore)==2){
			halfdelay(1);
			printw("*****CONGRATULATIONS!****\n");
			printw("             You beat the\n");
			printw("                 previous\n");
			printw("                   record\n");
			printw("                       of\n");
			printw("           %14ld\n",formerscore);
			printw("                  held by\n");
			printw("              %11s\n",formername);
			printw("               \n");
			printw("               \n");
			printw("*************************\n");
			printw("Press a key to proceed:");
			Effect:
			move(4,0);
			mvprintw(4,0, "     _____ ");
			mvprintw(5,0, "   .'     |");
			mvprintw(6,0, " .'       |");
			mvprintw(7,0, " |  .|    |");
			mvprintw(8,0, " |.' |    |");
			mvprintw(9,0, "     |    |");
			mvprintw(10,0,"  ___|    |___");
			mvprintw(11,0," |            |");
			mvprintw(12,0," |____________|");
			b=(b+1)%6;
			if(getch()==ERR)
				goto Effect;
			nocbreak();
			cbreak();
			erase();
			logo();
		}
	}
	//scorefile is still open with w+
	move(4,0);
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);
	printw(">*>*>Top %d<*<*<\n",SAVE_TO_NUM);
	while( rank<SAVE_TO_NUM && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		if(rank == playerrank)
			printw(">>>");
		printw("%d) %s : %ld\n",rank+1,pname,pscore);
		++rank;
	}
	addch('\n');
	refresh();
}


void draw_square(byte sy,byte sx){
	for(byte y=0;y<SIZE;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+SIZE*2,ACS_VLINE);
	}
	for(byte x=0;x<SIZE*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+SIZE,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+SIZE,sx+SIZE*2,ACS_LRCORNER);
	mvaddch(sy+SIZE,sx,ACS_LLCORNER);
	mvaddch(sy,sx+SIZE*2,ACS_URCORNER);
}
void move_o(){
	if(ox==0){
		if(oy==0){
			ox++;
		}
		else{
			oy--;
		}
	}
	else if(ox==SIZE*2){
		if(oy==SIZE){
			ox--;
			combo=1;
		}
		else{
			oy++;
		}
	}
	else if(oy==SIZE){
		if(ox==0){
			oy--;
		}
		else{
			ox--;
		}
	}
	else if(oy==0){
		if(ox==SIZE*2){
			oy++;
		}
		else{
			ox++;
		}

	}

	//---->---
	//|      |
	//^      V
	//|      |
	//----<---

}
void draw_angle(byte sy,byte sx){
	int y=sy+oy;
	int x=sx+ox;
	attron(colors[0]);
	mvaddch(y,x,'O');
	attroff(colors[0]);
}
void draw(int sy,int sx){
	for(byte i=0;i<3;++i){
		draw_square(sy+squarey[i],sx+squarex[i]);
	}
	logo();
}

byte shooting_scene(){
	float dy=(oy-(SIZE/2))/(float)SIZE;
	float dx=(ox-(SIZE))/(float)(SIZE*2);
	dy/=2;//it was too hard :(
	float y=squarey[0]+oy;
	float x=squarex[0]+ox;
	float offsetx=0;
	float offsety=0;
	float doffsety=0;
	float doffsetx=-dx*2;
	byte reached_o=0;
	while(1){
		erase();
		draw(offsety,offsetx);
		if(round(x)<=-round(offsetx)+1){//slow the animation
			reached_o=1;
		}
		attron(colors[0]);
		mvaddch(round(y)+round(offsety),round(x)+round(offsetx),'O');
		attroff(colors[0]);
		for(byte i=0;i<1;i++){
			y+=dy;
			x+=dx;
			offsety+=doffsety;
			if(reached_o){
				offsetx=-x;
			}
			else{
				offsetx+=doffsetx;
			}

			if( y>=LEN-1 || y<=1|| x> squarex[1]+SIZE*3 || x<-SIZE){
				combo=1;
				return LOSE;
			}
			if(y>=squarey[1] && x>=squarex[1] && y<=squarey[1]+SIZE && round(x)<=squarex[1]+SIZE*2){
					if(round(y)==squarey[1] || round(y)==squarey[1]+SIZE){
						score+=combo*101;
					}
					else{
						score+=combo;
					}
					combo++;
					jumps++;
					oy=round(y)-squarey[1];
					ox=round(x)-squarex[1];
					for(byte i=0;i<3;++i){
						squarey[i]+=offsety;
						squarex[i]+=offsetx;
					}
					squarey[0]=squarey[1];
					squarex[0]=squarex[1];
					squarey[1]=squarey[2];
					squarex[1]=squarex[2];
					squarex[2]=squarex[1]+WID-SIZE;
					squarey[2]=5+ (rand()%(LEN-(5+SIZE)));
					//since square 0 always ends up at the center after the animation,
					//square 2 should be choosen in a way that it would still remain
					//in the screen after the animation in which it becomes square 1.
					return WIN;
			}
		}
		refresh();
		if(reached_o){
			usleep(DELAY/6);
		}
		else{
			usleep(DELAY/3);
		}
	}
}
void help(void){
	nocbreak();
	cbreak();
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	red_border();
	mvprintw(1,HWID-4,"GAME PLAY");
	mvprintw(3,1,"Jump from square to square.");
	attroff(colors[3]);
	refresh();
	getch();
	halfdelay(1);
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
int main(void){
	signal(SIGINT,sigint_handler);
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	srand(time(NULL)%UINT_MAX);
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_RED,-1);
		init_pair(2,COLOR_YELLOW,-1);
		init_pair(3,COLOR_GREEN,-1);
		init_pair(4,COLOR_MAGENTA,-1);
		init_pair(5,COLOR_BLUE,-1);

		for(byte b=0;b<5;++b)
			colors[b]=COLOR_PAIR(b+1);
	}
	Start:
	oy=ox=0;
	squarey[0]=5;
	squarex[0]=0;
	squarey[1]=5;
	squarex[1]=squarex[0]+WID-SIZE;
	squarey[2]=5;
	squarex[2]=squarex[1]+WID-SIZE;
	erase();
	nodelay(stdscr,1);
	curs_set(0);
	score=0;
	msg_show=0;
	while(1){
		erase();
		draw(0,0);
		draw_angle(squarey[0],squarex[0]);
		refresh();
		input=getch();
		move_o();
		if(input=='?' || input==KEY_F(1))
			help();
		if(input=='q'){
			break;
		}
		if(input=='\n'||input==KEY_ENTER){
			if(shooting_scene()==LOSE){
				break;
			}
		}
		usleep(DELAY);
		if(input!=ERR){
			flushinp();
		}
	}
	nodelay(stdscr,0);
	flushinp();
	nocbreak();
	cbreak();
	curs_set(1);

	mvprintw(LEN,0,"Press a key to see the high scores:");
	refresh();
	getch();

	show_scores(save_score());
	printw("\n\nWanna play again? (y/n)");
	do{
		input=getch();
	}while((input==KEY_UP||input=='w') || (input==KEY_DOWN||input=='s'));
	if(input!='q' && input!='n' && input!='N')
		goto Start;
	endwin();
	return 0;
}
