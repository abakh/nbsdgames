/* 
|\/|
|  |UNCHER

Authored by abakh <abakh@tuta.io>
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "config.h"
#define SAVE_TO_NUM 10
#define MINLEN 10
#define MAXLEN 24
#define MINWID 40
#define MAXWID 80
enum {UP=1,RIGHT,DOWN,LEFT,FOOD,SUPERFOOD,TRAP};
typedef signed char byte;

/* The Plan9 compiler can not handle VLAs */
#ifdef NO_VLA
#define len 10
#define wid 40

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


#else
int len,wid;
#endif//NO_VLA

int py,px;//pointer

byte pse_msg=20;//flashing animations might hurt some people
bool epilepsy=0;
char alt_animation[4]={'-','\\','|','/'};

int immunity;
byte direction;
long score;
chtype colors[6]={0};

FILE* scorefile;
char error[150]={0};

void logo(void){
	mvaddstr(1,0,"|\\/|");
	mvaddstr(2,0,"|  |UNCHER");
}
byte scorewrite(void){// only saves the top 10, returns the place in the chart
	bool deforno;
	if( !getenv("MNCH_SCORES") && (scorefile= fopen(MNCH_SCORES,"r")) ){
		deforno=1;
	}
	else{
		deforno=0;
		if( !(scorefile = fopen(getenv("MNCH_SCORES"),"r")) ){
			sprintf(error,"No accessible score files found. You can make an empty text file in %s or set MNCH_SCORES to such a file to solve this.",MNCH_SCORES);
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
		++location;

		memset(fuckingname,0,60);
		fuckingscore=0;
	}
	if(deforno)
		scorefile = fopen(MNCH_SCORES,"w+");//get rid of the previous text first
	else
		scorefile = fopen(getenv("MNCH_SCORES"), "w+") ;
	if(!scorefile){
		strcpy(error, "The file cannot be opened in w+. ");
		return -1;
	}

	byte itreached=location;
	byte ret = -1;
	bool wroteit=0;

	for(location=0;location<=itreached && location<SAVE_TO_NUM-wroteit;++location){
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
	erase();
	logo();
	if(*error){
		mvaddstr(3,0,error);
		printw("\nHowever, your score is %ld.",score);
		refresh();
		return;
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(scorefile);
		fscanf(scorefile,"%*s : %*d\n");
		move(3,0);
		byte b=0;
		if ( fscanf(scorefile,"%s : %ld\n",formername,&formerscore)==2){
			halfdelay(1);
			printw("*****CONGRATULATIONS!****\n");
			printw("              You bet the\n");
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
			attron(colors[b]);
			mvprintw(4,0, "     _____ ");
			mvprintw(5,0, "   .'     |");
			mvprintw(6,0, " .'       |");
			mvprintw(7,0, " |  .|    |");
			mvprintw(8,0, " |.' |    |");
			mvprintw(9,0, "     |    |");
			mvprintw(10,0,"  ___|    |___");
			mvprintw(11,0," |            |");
			mvprintw(12,0," |____________|");
			attroff(colors[b]);
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
	move(3,0);
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(scorefile);
	printw(">*>*>Top %d<*<*<\n",SAVE_TO_NUM);
	while( rank<SAVE_TO_NUM && fscanf(scorefile,"%s : %ld\n",pname,&pscore) == 2){
		if(rank == playerrank)
			printw(">>>");
		printw("%d) %s : %ld\n",rank+1,pname,pscore);
		++rank;
	}
	addch('\n');
	refresh();
}
void rectangle(void){
	for(int y=0;y<=len;++y){
		mvaddch(3+y,0,ACS_VLINE);
		mvaddch(4+y,1+wid,ACS_VLINE);
	}
	for(int x=0;x<=wid;++x){
		mvaddch(3,x,ACS_HLINE);
		mvaddch(4+len,x,ACS_HLINE);
	}
	mvaddch(3,0,ACS_ULCORNER);
	mvaddch(4+len,0,ACS_LLCORNER);
	mvaddch(3,1+wid,ACS_URCORNER);
	mvaddch(4+len,1+wid,ACS_LRCORNER);
}
void place_food(byte board[len][wid]){
	int y,x;
	do{
		y=rand()%len;
		x=rand()%wid;
	}while(y==py && x==px);
	board[y][x]=FOOD;
	
	byte num;
	if(score<300)
		num=rand()%2;
	else if(score<500)
		num=1+rand()%2;
	else if(score<1000)
		num=2+rand()%4;
	else if(score<2000)
		num=5+rand()%6;
	else
		num=10+rand()%11;

	while(num){
		Again:
		y=rand()%len;
		x=rand()%wid;
		if(abs(y-py)<4 && abs(x-px)<7)
			goto Again;
		if(board[y][x]==FOOD)
			goto Again;
		board[y][x]=TRAP;
		
		--num;
	}
	if(score>2000 && !(rand()%5)){
		do{
			y=rand()%len;
			x=rand()%wid;
		}while(y==py && x==px && board[y][x]!=FOOD);
		board[y][x]=SUPERFOOD;
	}
}
void draw(byte board[len][wid]){
	int y,x;
	static byte effect=0;
	chtype prnt;
	rectangle();
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			if(y==py && x==px){
				prnt='r'|colors[2]|A_STANDOUT;
				if(immunity){
					if(epilepsy)
						prnt='r';
					else
						prnt='r'|colors[effect]|A_BOLD;
				}
			}
			else if(board[y][x]==TRAP)
				prnt='^'|colors[((y*x)/15)%6];
			else if(board[y][x]==FOOD)
				prnt='%'|colors[(y+x)%6];
			else if(board[y][x]==SUPERFOOD){
				if(epilepsy)
					prnt=alt_animation[effect/10];
				else
					prnt='%'|colors[effect];
			}
			else 
				prnt= ' ';
			mvaddch(4+y,x+1,prnt);
		}
	}
	if(epilepsy)
		effect=(effect+1)%40;
	else
		effect=(effect+1)%6;
	if(pse_msg && !epilepsy){
		mvprintw(len+5,0,"Suffering PSE? Press e.");
		--pse_msg;
	}
}
void help(void){
	nocbreak();
	cbreak();
	erase();
	logo();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	attroff(A_BOLD);
	mvprintw(4,0,"hjkl/ARROW KEYS : Change direction");
	mvprintw(5,0,"q : Quit");
	mvprintw(6,0,"F1 & F2: Help on controls & gameplay");
	mvprintw(8,0,"Press a key to continue");
	refresh();
	getch();
	erase();
	halfdelay(1);
}
void gameplay(void){
	nocbreak();
	cbreak();
	erase();
	logo();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("Eat the food and avoid the traps.\n");
	refresh();
	erase();
	getch();
	halfdelay(1);
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
int main(int argc, char** argv){
	bool autoset=0;
	signal(SIGINT,sigint_handler);
#ifndef NO_VLA
	if(argc>3 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s [len wid]\n",argv[0]);
		return EXIT_FAILURE;
	}
	if(argc==2){
		puts("Give both dimensions.");
		return EXIT_FAILURE;
	}
	if(argc==3){
		bool lool = sscanf(argv[1],"%d",&len) && sscanf(argv[2],"%d",&wid);
		if(!lool){
			puts("Invalid input.");
			return EXIT_FAILURE;
		}
		if(len<MINLEN || wid<MINWID || len>500 || wid>500){
			puts("At least one of your given dimensions is either too small or too big.");
			return EXIT_FAILURE;
		}
	
	}
	else{
		autoset=1;
	}
#endif
	initscr();
#ifndef NO_VLA
	if(autoset){
		len=LINES-7;
		if(len<MINLEN)
			len=MINLEN;
		else if(len>MAXLEN)
			len=MAXLEN;

		wid=COLS-5;
		if(wid<MINWID)
			wid=MINWID;
		else if(wid>MAXWID)
			wid=MAXWID;
	}
#endif
	srand(time(NULL)%UINT_MAX);		
	byte board[len][wid];
	bool halfspeed=0;
	int constant=150*(80*24)/(len*wid);
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_BLUE,-1);
		init_pair(2,COLOR_GREEN,-1);
		init_pair(3,COLOR_YELLOW,-1);
		init_pair(4,COLOR_CYAN,-1);
		init_pair(5,COLOR_MAGENTA,-1);
		init_pair(6,COLOR_RED,-1);
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}

	}
	Start:
	curs_set(0);
	halfdelay(1);
	score=direction=immunity=0;
	py=len/2;
	px=wid/2;
	memset(board,0,len*wid);
	place_food(board);
	int preinput,input=0;
	while(1){
		erase();
		logo();
		mvprintw(1,11,"Score:%ld",score);
		if(immunity)
			mvprintw(2,11,"Immunity:%d",immunity);
		draw(board);
		refresh();
		if( board[py][px]==FOOD ){
			score+= constant;
			board[py][px]=0;
			if(!epilepsy){
				for(byte b=0;b<6;++b){
					mvaddch(4+py,px+1,'r'|colors[b]|A_STANDOUT);
					refresh();
					usleep(100000/5);
				}
			}
			place_food(board);
		}
		if( board[py][px]==SUPERFOOD ){
			immunity+=(len+wid)/2;
			board[py][px]=0;
		}
		if(board[py][px]==TRAP){
			if(immunity)
				board[py][px]=0;
			else
				break;
		}
		if(px<0 || px>=wid)
			break;
		halfspeed=!halfspeed;
		preinput=input;
		input = getch();
		if( input == KEY_F(1) || input=='?' )
			help();
		if( input==KEY_F(2) )
			gameplay();
		if( (input=='k' || input==KEY_UP) && py>0 ){
			direction=UP;
			halfspeed=1;
		}
		if( (input=='j' || input==KEY_DOWN) && py<len-1 ){
			direction=DOWN;
			halfspeed=1;
		}
		if( (input=='h' || input==KEY_LEFT) && px>0 )
			direction=LEFT;
		if( (input=='l' || input==KEY_RIGHT) && px<wid-1 )
			direction=RIGHT;
		if( input=='e')
			epilepsy=1;
		if( input=='q')
			sigint_handler(0);
		if( input=='p'){
			nocbreak();
			cbreak();
			erase();
			logo();
			attron(A_BOLD);
			mvaddstr(1,11,"PAUSED");
			attroff(A_BOLD);
			getch();
			halfdelay(1);
		}
		if(input!=ERR){
			if(preinput==input){//if it wasn't there, hitting two keys in less than 0.1 sec would not work
				usleep(100000);
				flushinp();
			}
		}
		if(direction==UP && halfspeed){
			if(!py)
				break;
			--py;
		}
		else if(direction==DOWN && halfspeed){
			if(py==len-1)
				break;
			++py;
		}
		else if(direction==LEFT){
			if(!px)
				break;
			--px;
		}
		else if(direction==RIGHT){
			if(px==wid-1)
				break;
			++px;
		}
		if(immunity)
			--immunity;
	}
	nocbreak();
	cbreak();
	draw(board);
	refresh();
	mvprintw(len+5,0,"Game over! Press a key to see the high scores:");
	getch();
	showscores(scorewrite());
	printw("Game over! Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if( input!= 'N' &&  input!= 'n' && input!='q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}

