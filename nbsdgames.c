/*
The Menu

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <curses.h>
#include <unistd.h>
#include "config.h"
#include "common.h"
#define LEN 24 
#define HLEN LEN/2
#define WID 100
#define HWID WID/2
#ifndef NB 
	#define NB
#endif

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

chtype colors[4]={A_NORMAL,A_STANDOUT};
char main_menu[]={
	"See High Scores\n"
	NB"jewels\n"
	NB"sudoku\n"
	NB"mines\n"
	NB"reversi\n"
	NB"checkers\n"
	NB"battleship\n"
	NB"rabbithole\n"
	NB"sos\n"
	NB"pipes\n"
	NB"fifteen\n"
	NB"memoblocks\n"
	NB"fisher\n"
	NB"muncher\n"
	NB"miketron\n"
	NB"redsquare\n"
	NB"darrt\n"
	NB"snakeduel\n"
	NB"tugow\n"
	NB"revenge\n"
	NB"sjump\n"
	NB"trsr\n"
};
char ascii_art[]={
"     ######                    ######    "
"   ####################################  "
"  ###################################### "
" ########################################"
" ########################################"
"  ###################################### "
"  ###################################### "
"  ######## ####            #### ######## "
" #########                      #########"
" ########                        ########"
" ######                            ######"
};
char scores_menu[]={
	"pipes_scores\n"
	"jewels_scores\n"
       	"miketron_scores\n"
       	"muncher_scores\n"
       	"fisher_scores\n"
       	"darrt_scores\n" 
	"tugow_scores\n"
	"revenge_scores\n"
	"sjump_scores\n"
};
char choice_str[100]={0};
char name[100]={0};

void fancy_background(){
	int y,x;
	int lines=LINES,cols=COLS;
	#define ABS(x) (((x)<0)?-(x):(x))
	for(y=0;y<lines;++y){
		for(x=0;x<cols;++x){
			mvaddch(y,x,'#'|colors[abs((abs((y-lines/2)*(y-lines/2)/10)+x)/10)%4]);
		}
	}
	for(int i=0;ascii_art[i]!='\0';++i){
		if(ascii_art[i]!=' '){
			mvaddch(lines/2-5+(i/41),cols/2-21+(i%41),' ');
		}
	}
}

void filled_rect(int sy,int sx,int ey,int ex){
	int y,x;
	for(y=sy;y<ey;++y)
		for(x=sx;x<ex;++x)
			mvaddch(y,x,' ');
}

void green_border(void){
	int y,x;
	int lines=LINES,cols=COLS;
	for(y=0;y<lines;++y){
		x=cols-1;
		mvaddch(y,x,' '|A_REVERSE|colors[(x+y)%4]);
		x=0;
		mvaddch(y,x,' '|A_REVERSE|colors[(x+y)%4]);
	}
	for(x=0;x<cols;++x){
		y=lines-1;
		mvaddch(lines-1,x,' '|A_REVERSE|colors[(x+y)%4]);
		y=0;
		mvaddch(0,x,' '|A_REVERSE|colors[(x+y)%4]);
	}
}


void show_scores(FILE* score_file){
	erase();
	filled_rect(0,0,LINES,COLS);
	green_border();
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);
	attron(colors[1]);
	mvaddstr(1,COLS/2-6,"High Scores");
	attroff(colors[1]);
	while( rank<10 && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		move(2+rank,1);
		if(!strcmp(pname,(getenv("NB_PLAYER")?getenv("NB_PLAYER"):getenv("USER")))){
			attron(colors[(2+rank)%4]);
		}
		printw("%s",pname);
		if(!strcmp(pname,(getenv("NB_PLAYER")?getenv("NB_PLAYER"):getenv("USER")))){
			attroff(A_REVERSE);
		}

		mvprintw(2+rank,COLS-1-digit_count(pscore),"%d",pscore);
		++rank;
		if(!strcmp(pname,(getenv("NB_PLAYER")?getenv("NB_PLAYER"):getenv("USER")))){
			attroff(colors[(2+2*rank)%4]);
		}

		attroff(colors[(2+2*rank)%4]);
	}
	refresh();
	fclose(score_file);
	flushinp();
	getch();
}
void help(void){
	int lines=LINES,cols=COLS;
	nocbreak();
	cbreak();
	attron(colors[3]);
	filled_rect(0,0,LINES,cols);
	green_border();
	mvprintw(1,(cols/2)-4,"GAME PLAY");
	mvprintw(3,1,"Catch a fish and reel it in for points");
	mvprintw(4,1,"The deeper the fish, the more points it is worth.");
	mvprintw(5,1,"If a fish hits your line, you lose a hook.");
	mvprintw(6,1,"When you run out of hooks, the game is over!");
	mvprintw(8,(cols/2)-4,"CONTROLS");
	mvprintw(10,1,"UP & DOWN: Control the hook");
	mvprintw(11,1,"q: Quit");
	mvprintw(13,1,"This is a port of \"Deep Sea Fisher\", a MikeOS game.");
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
void get_entry(char* entries,int choice,char* target_str){
	int entry=0;
	int index=0;
	while(entry!=choice){
		if(entries[index]=='\n'){
			++entry;
		}
		++index;
		if(entry==choice){
			break;
		}
	}
	for(int target_index=0;;++target_index,++index){
		if(entries[index]=='\n'){
			target_str[target_index]='\0';
			return;
		}
		else{
			target_str[target_index]=entries[index];
		}
	}
}

int menu(char* entries,char* title){
	int chosen=0;
	int first_entry=0;
	int line=0;
	int lines=LINES,cols=COLS;
	Refresh:
	erase();
	green_border();
	attron(colors[1]);
	mvaddstr(1,(cols-strlen(title))/2,title);
	attroff(colors[1]);
	line=2;
	if(chosen<0){
		chosen=0;
	}
	if(chosen> first_entry+(lines-3)){
		first_entry=chosen-(lines-3);
	}
	if(chosen<first_entry){
		first_entry=chosen;
	}
	if(first_entry<0){
		first_entry=0;
	}
	int entry=0;
	int char_index=0;
	while(entry!=first_entry){
		if(entries[char_index]=='\n'){
			++entry;
		}
		if(entries[char_index]=='\0'){
			chosen=entry-1;
		}
		if(entry==first_entry){
			break;
		}
		++char_index;
	}
	int x=1;
	int y=2;
	
	for(;entry<first_entry+lines-3 ;++char_index){
		if(entries[char_index]=='\n'){
			++y;
			++entry;
			x=1;
		}
		else if(entries[char_index]=='\0'){
			if(entry-1<chosen){
				chosen=entry-1;
				goto Refresh;
			}
			break;
		}
		else{
			mvaddch(y,x,entries[char_index]|((entry==chosen)*(A_STANDOUT|colors[y%4])));
			++x;
		}
	}
	refresh();
	int input=0;
	input=getch();
	if((input==KEY_UP||input=='w')){
		--chosen;
	}
	if((input==KEY_DOWN||input=='s')){
		++chosen;
	}
	if(input=='\n'){
		return chosen;
	}
	if((input=='q'||input==27)){
		return -1;
	}
	goto Refresh;
}
void enter_name(){
	snprintf(name,25,"%s",getenv("USER"));
	int input=0;
	int index=strlen(name);
	while(1){
		erase();
		fancy_background();
		mvaddstr(LINES/2-2,COLS/2-15,"Enter a name, then press enter");
		mvaddstr(LINES/2,(COLS-strlen(name))/2,name);
		refresh();
		input=getch();
		if(input=='\n'){
			break;
		}
		if(input==KEY_BACKSPACE||input==8){
			if(index>=1){
				name[index-1]='\0';
				--index;
			}
		}
		if(('a'<=input && input<='z') || ('0'<=input && input<='9')){
			if(index<25){
				name[index]=input;
				index++;
			}
		}
	}
	setenv("NB_PLAYER",name,1);
}
void scores(){
	int choice;
	char address[1000]={0};
	FILE* score_file;
	while(1){
		switch(choice=menu(scores_menu,"High Scores")){
			case -1:
				erase();
				return;
			break;
			default:
				get_entry(scores_menu,choice,choice_str);
				snprintf(address,999,"%s/%s",SCORES_DIR,choice_str);
				if((score_file=fopen(address,"r"))){
					show_scores(score_file);
				}
				snprintf(address,999,"%s/.%s",getenv("HOME"),choice_str);
				if((score_file=fopen(address,"r"))){
					show_scores(score_file);
				}
			break;
		}
	}
	
}

int main(int argc,char** argv){
	printf("\x1b]2;%s\x07","NBSDGAMES!");//change the title
	printf("\x1b]0;%s\x07","NBSDGAMES!");
	if(argc>1){
		printf("This game doesn't take arguments");
	}
	char path[1000];
	snprintf(path,999,"%s:.",path);//include current dir at the end
	signal(SIGINT,sigint_handler);
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	srand(time(NULL)%UINT_MAX);
	if(has_colors()){
		start_color();
		init_pair(1,COLOR_RED,COLOR_BLACK);
		init_pair(2,COLOR_MAGENTA,COLOR_BLACK);
		init_pair(3,COLOR_YELLOW,COLOR_BLACK);
		init_pair(4,COLOR_GREEN,COLOR_BLACK);
		for(int b=0;b<4;++b){
			colors[b]=COLOR_PAIR(b+1);
		}
	}
	int n;
	Start:
	curs_set(0);
	enter_name();
	int choice;
	while(1){
		switch(choice=menu(main_menu,"Main Menu")){
			case -1:
				sigint_handler(EXIT_SUCCESS);
			break;
			case 0:
				scores();
			break;
			default:
				def_prog_mode();
				endwin();
				get_entry(main_menu,choice,choice_str);
				system(choice_str);
				reset_prog_mode();
			break;
		}
	}
	curs_set(1);
	//show_scores(save_score());

	endwin();
	return 0;
}
