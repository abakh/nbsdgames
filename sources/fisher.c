#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <curses.h>
#include <unistd.h>
#include "config.h"
#define SAVE_TO_NUM 11
#define HOOKS 10
#define LEN 24
#define HLEN LEN/2
#define WID 80
#define HWID WID/2

#ifdef Plan9
int usleep(long usec) {
    int seconds = usec/1000000;
    long nano = usec*1000 - seconds*1000000;
    struct timespec sleepy = {0};
    sleepy.ts_sec = second;
    sleepy.ts_nsec = nano;
    nanosleep(&sleepy, (struct timespec *) NULL);
    return 0;
}
#endif

typedef signed char byte;
chtype colors[4]={A_NORMAL,A_STANDOUT};
byte fish[10]={0};//positions
byte caught=-1;
bool stop[10]={0};
unsigned int count[10]={0};
unsigned long score=0;
const char sym[]="~:=!@+><;&";
byte hook=0, hooknum=0;
char error [150]={0};
byte clb,clbtime=0;
FILE* scorefile;

int input;
/*
    O__/|
 ___|_/ |    __
|     / |   |__
	    |  ISHER*/
// 12 lines of water
// 80 columns
byte digit_count(int num){
	byte ret=0;
	do{
		ret++;
		num/=10;
	}while(num);
	return ret;
}
void filled_rect(byte sy,byte sx,byte ey,byte ex){
	byte y,x;
	for(y=sy;y<ey;y++)
		for(x=sx;x<ex;x++)
			mvaddch(y,x,' ');
}
void green_border(void){
	byte y,x;
	for(y=0;y<LEN;y++){
		mvaddch(y,WID-1,' '|colors[2]);
		mvaddch(y,0,' '|colors[2]);
	}
	for(x=0;x<WID;x++){
		mvaddch(LEN-1,x,' '|colors[2]);
		mvaddch(0,x,' '|colors[2]);
	}
		
}
void star_line(byte y){
	for(byte x=1;x<WID-1;x++)
		mvaddch(y,x,'.');
}
void draw(void){
	/*while(LEN< 15 || COL<80)
		mvprintw(0,0,"Screen size should at least be 80*15 characters");*/
	attron(colors[0]);
	byte y,x;
	filled_rect(0,0,12,80);
	mvprintw(0,0," __       Hooks:%d",hooknum);
	mvprintw(1,0,"|__       Score:%d",score);
	mvprintw(2,0,"|  ISHER");
	mvprintw(9,32, "    O__/");
	mvprintw(10,32," ___|_/ ");
	mvprintw(11,32,"|     / ");
	
	if(clbtime){
		if(count[clb]!=1){
			mvprintw(2,10,"%d ",count[clb]);
			switch(clb){
				case 0:
					addstr("plastic bags!");
				break;
				case 1:
					addstr("PVC bottles!");
				break;
				case 2:
					addstr("face masks!");
				break;
				case 3:
					addstr("shrimp!");
				break;
				case 4:
					addstr("algae!");
				break;
				case 5:
					addstr("jellyfish!");
				break;
				case 6:
					addstr("molluscs!");
				break;
				case 7:
					addstr("actual fish!");
				break;
				case 8:
					addstr("salmon!");
				break;
				case 9:
					addstr("tuna!");
				break;
			}
		}
		else{
			switch(clb){
				case 0:
					addstr("A plastic bag!");
				break;
				case 1:
					addstr("A PVC bottle!");
				break;
				case 2:
					addstr("A face mask!");
				break;
				case 3:
					addstr("A shrimp!");
				break;
				case 4:
					addstr("Algae!");
				break;
				case 5:
					addstr("A jellyfish!");
				break;
				case 6:
					addstr("A mollusc!");
				break;
				case 7:
					addstr("Actual fish!");
				break;
				case 8:
					addstr("A salmon!");
				break;
				case 9:
					addstr("A tuna!");
				break;
			}
		}
	}
	for(y=-3;y<0;y++)
		mvaddch(HLEN+y,HWID,ACS_VLINE);
	attroff(colors[0]);
	attron(colors[1]);
	filled_rect(HLEN,0,LEN,WID);
	for(y=0;y<hook;y++)
		mvaddch(HLEN+y,HWID,ACS_VLINE);
	if(caught==-1)
		mvaddch(HLEN+hook,HWID,')');
	else
		mvaddch(HLEN+hook,HWID,sym[caught]);
	for(y=0;y<10;y++)
		mvaddch(HLEN+1+y,fish[y],sym[y]);
	attroff(colors[1]);
	
}
byte scorewrite(void){// only saves the top 10, returns the place in the chart
	bool deforno;
	if( !getenv("FSH_SCORES") && (scorefile= fopen(FSH_SCORES,"r")) ){
		deforno=1;
	}
	else{
		deforno=0;
		if( !(scorefile = fopen(getenv("FSH_SCORES"),"r")) ){
			sprintf(error,"No accessible score files found. You can make an empty text file in %s or set FSH_SCORES to such a file to solve this.",FSH_SCORES);
			return -1;
		}
	}

	char namebuff[SAVE_TO_NUM][60];
	long scorebuff[SAVE_TO_NUM];

	memset(namebuff,0,SAVE_TO_NUM*60*sizeof(char) );
	memset(scorebuff,0,SAVE_TO_NUM*sizeof(long) );

	long fuckingscore =0;
	char fuckingname[60]={0};
	byte location=0;

	while( fscanf(scorefile,"%59s : %ld\n",fuckingname,&fuckingscore) == 2 && location<SAVE_TO_NUM ){
		strcpy(namebuff[location],fuckingname);
		scorebuff[location] = fuckingscore;
		location++;

		memset(fuckingname,0,60);
		fuckingscore=0;
	}
	if(deforno)
		scorefile = fopen(FSH_SCORES,"w+");//get rid of the previous text first
	else
		scorefile = fopen(getenv("FSH_SCORES"), "w+") ;
	if(!scorefile){
		strcpy(error, "The file cannot be opened in w+. ");
		return -1;
	}

	byte itreached=location;
	byte ret = -1;
	bool wroteit=0;
	for(location=0;location<=itreached && location<SAVE_TO_NUM-wroteit;location++){
		if(!wroteit && (location>=itreached || score>=scorebuff[location]) ){
			fprintf(scorefile,"%s : %ld\n",getenv("USER"),score);
			ret=location;
			wroteit=1;
		}
		if(location<SAVE_TO_NUM-wroteit && location<itreached)
			fprintf(scorefile,"%s : %ld\n",namebuff[location],scorebuff[location]);
	}
	fflush(scorefile);
	return ret;
}
void showscores(byte playerrank){
	byte y,x;
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	green_border();
	if(*error){
		mvaddstr(1,0,error);
		mvprintw(2,0,"However, your score is %ld.",score);
		refresh();
		return;
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(scorefile);
		fscanf(scorefile,"%*s : %*d");
		if ( fscanf(scorefile,"%s : %ld",formername,&formerscore)==2  && formerscore>0){
			byte a = (LEN-9)/2;
			star_line(1);
			star_line(LEN-2);
			mvaddstr(1,WID/2-8,"CONGRATULATIONS!!");
			mvprintw(a+1,HWID-10,"     _____ You bet the");
			mvprintw(a+2,HWID-10,"   .'     |   previous");
			mvprintw(a+3,HWID-10," .'       |     record");
			mvprintw(a+4,HWID-10," |  .|    |	 of");
			mvprintw(a+5,HWID-10," |.' |    |%11ld",formerscore);
			mvprintw(a+6,HWID-10,"     |    |    held by");
			mvprintw(a+7,HWID-10,"  ___|    |___%7s!",formername);
			mvprintw(a+8,HWID-10," |	    |");
			mvprintw(a+9,HWID-10," |____________|");
			mvprintw(LEN-3,HWID-11,"Press a key to continue");
			refresh();
			do{
				input=getch();
			}while(input==KEY_UP || input==KEY_DOWN);
			filled_rect(0,0,LEN,WID);
			green_border();
		}

	}
	//scorefile is still open with w+
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(scorefile);	
	mvaddstr(1,WID/2-4,"HIGH SCORES");
	attron(colors[3]);
	while( rank<SAVE_TO_NUM && fscanf(scorefile,"%s : %ld\n",pname,&pscore) == 2){
		star_line(2+2*rank);
		move(2+2*rank,1);
		if(rank == playerrank)
			printw(">>>");
		printw("%s",pname);
		mvprintw(2+2*rank,WID-1-digit_count(pscore),"%d",pscore);
		rank++;
	}
	attroff(colors[3]);
	refresh();
}
void help(void){
	nocbreak();
	cbreak();
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	green_border();
	mvprintw(1,HWID-4,"GAME PLAY");
	mvprintw(3,1,"Catch a fish and reel it in for points");
	mvprintw(4,1,"The deeper the fish, the more points it is worth.");
	mvprintw(5,1,"If a fish hits your line, you lose a hook.");
	mvprintw(6,1,"When you run out of hooks, the game is over!");
	mvprintw(8,HWID-4,"CONTROLS");
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
void main(void){
	signal(SIGINT,sigint_handler);
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	srand(time(NULL)%UINT_MAX);
	for(byte n=0;n<10;n++)
		fish[n]=rand()%80;
	if(has_colors()){
		start_color();
		init_pair(1,COLOR_BLACK,COLOR_CYAN);
		init_pair(2,COLOR_BLACK,COLOR_BLUE);
		init_pair(3,COLOR_WHITE,COLOR_GREEN);
		init_pair(4,COLOR_BLACK,COLOR_WHITE);
		for(byte b=0;b<4;b++)
			colors[b]=COLOR_PAIR(b+1);
	}
	byte n;
	Start:
	halfdelay(1);
	curs_set(0);
	hook=0;
	hooknum=HOOKS;
	score=0;
	memset(count,0,10*sizeof(unsigned int) );
	while(1){
		draw();
		refresh();
		input=getch();
		for(n=0;n<10;n++){
			if(stop[n]){
				if(rand()%(n+15)==0)//this is to make the fish move
					stop[n]=0;
				continue;
			}
			if(n%2)
				fish[n]--;
			else
				fish[n]++;
			if(fish[n]<0)
				fish[n]=79;
			if(fish[n]>79)
				fish[n]=0;//appear on the other end
			if(fish[n]==40){
				if(hook>n+1){
					caught= -1;
					hook=0;
					hooknum--;
				}
				if(hook==n+1 && caught==-1){
					caught=n;
					if(n%2)
						fish[n]=79;
					else
						fish[n]=0;
				}
			}
			if(rand()%(14-n)==0)//this is to make it stop
				stop[n]=1;
		}
		if(input==KEY_UP)
			if(hook>0)
				hook--;
			if(hook==0 && caught!=-1){
				count[caught]++;
				score+=(caught+1)*(caught+1);
				clb=caught;
				clbtime=10;//celebrate catching the fish
				caught=-1;
			}
				
		if(input==KEY_DOWN){
			if(hook<11)
				hook++;
			if(fish[hook-1]==40 && caught==-1){
				caught=hook-1;
				if(n%2)
					fish[hook-1]=0;
				else
					fish[hook-1]=79;
			}
		}
		if(input=='?' || input==KEY_F(1))
			help();
		if(input=='q')
			break;
		if(!hooknum)
			break;	
		if(input!=ERR){
			usleep(100000);
			flushinp();
		}
	}
	End:
	flushinp();
	nocbreak();
	cbreak();
	curs_set(1);
	showscores(scorewrite());
	attron(colors[2]);
	mvprintw(LEN-1,HWID-11,"Wanna play again? (y/n)");
	attroff(colors[2]);
	do{
		input=getch();
	}while(input==KEY_UP || input==KEY_DOWN);
	if(input!='q' && input!='n' && input!='N')
		goto Start;
	endwin();
}
