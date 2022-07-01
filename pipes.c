/* 
 _
|_)
| IPES

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


*/

#include "common.h"
#define UP 1
#define RIGHT 2
#define DOWN 4
#define LEFT 8
#define CROSSOVER 15
#define FILLED 16
#define FLOWDELAY 5
#define DELAY 3
#define SAVE_TO_NUM 10
#define SY 0
#define SX 7
typedef unsigned char bitbox;

/* The Plan9 compiler can not handle VLAs */
//#ifdef NO_VLA who uses that len wid arguments for this one? removed them
#define wid 20
#define len 14
//#else
//int len,wid;
//#endif

int py,px,fy,fx;//p: pointer f: fluid
bitbox tocome[5]={0};//the row of pipes in the left side
chtype green=A_BOLD;//will use bold font instead of green if colors are not available
long score;

void logo(void){
	mvprintw(0,0," _ ");
	mvprintw(1,0,"|_)");
	mvprintw(2,0,"| IPES");
}

byte save_score(void){
	return fallback_to_home("pipes_scores",score,SAVE_TO_NUM);

}

void show_scores(byte playerrank){
	erase();
	logo();
	if(playerrank==FOPEN_FAIL){
		mvaddstr(SY,SX,"Couldn't open scorefile");
		mvprintw(SY+1,SX,"However, your score is %ld.",score);
		refresh();
		return;
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(score_file);
		fscanf(score_file,"%*s : %*d");
		if ( fscanf(score_file,"%s : %ld",formername,&formerscore)==2  && formerscore>0){
			byte a = (len-9)/2;
			attron(A_BOLD);
			mvprintw(SY,SX,      "****                ***");
			mvprintw(SY+len+1,SX,"***********************");
			attroff(A_BOLD);
			attron(green);
			mvprintw(SY,SX+4,"CONGRATULATIONS!");
			attroff(green);

			mvprintw(SY+a+1,SX,"     _____You beat the");
			mvprintw(SY+a+2,SX,"   .'     |   previous");
			mvprintw(SY+a+3,SX," .'       |     record");
			mvprintw(SY+a+4,SX," |  .|    |         of");
			mvprintw(SY+a+5,SX," |.' |    |%11ld",formerscore);
			mvprintw(SY+a+6,SX,"     |    |    held by");
			mvprintw(SY+a+7,SX,"  ___|    |___%7s!",formername);
			mvprintw(SY+a+8,SX," |            |");
			mvprintw(SY+a+9,SX," |____________|");
			mvprintw(len+2,0,"Game over! Press a key to proceed:");
			refresh();
			getch();
			erase();
			logo();
		}

	}
	attron(A_BOLD);
	mvprintw(3,0," HIGH");
	mvprintw(4,0,"SCORES");
	attroff(A_BOLD);
	//scorefile is still open with w+
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);
	while( rank<SAVE_TO_NUM && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		move(SY+1+rank,SX+1);
		attron(green);
		if(rank == playerrank)
			printw(">>>");
		printw("%d",rank+1);
		attroff(green);
		printw(") %s : %ld",pname,pscore);
		++rank;
	}
	fclose(score_file);
	refresh();
}
//move in direction
void MID(bitbox direction){
	switch(direction){
		case UP:
			--fy;
			break;
		case DOWN:
			++fy;
			break;
		case LEFT:
			--fx;
			break;
		case RIGHT:
			++fx;
			break;
	}
}
bitbox opposite(bitbox direction){
	switch(direction){
		case  UP:
			return DOWN;
		case DOWN:
			return UP;
		case LEFT:
			return RIGHT;
		case RIGHT:
			return LEFT;
	}
	return 0;
}
void rectangle(void){
	for(int y=0;y<=len;++y){
		mvaddch(SY+y,SX,ACS_VLINE);
		mvaddch(SY+y,SX+wid+1,ACS_VLINE);
	}
	for(int x=0;x<=wid;++x){
		mvaddch(SY,SX+x,ACS_HLINE);
		mvaddch(SY+len+1,SX+x,ACS_HLINE);
	}
	mvaddch(SY,SX,ACS_ULCORNER);
	mvaddch(SY+len+1,SX,ACS_LLCORNER);
	mvaddch(SY,SX+wid+1,ACS_URCORNER);
	mvaddch(SY+len+1,SX+wid+1,ACS_LRCORNER);
}
//this generates the pipes...
bitbox pipegen(void){
	if(rand()%17){//17 so all forms have the same chance
		byte a=rand()%4;
		byte b;
		do{
			b=rand()%4;
		}while(b==a);
		return (1 << a) | ( 1 << b);
	}
	else
		return CROSSOVER;//could not be generated like that
	
}
//.. and this is only for display
void addpipe(int y,int x,bitbox pipe , bool highlight){
	bitbox p= pipe & ~FILLED;
	chtype foo ;
	switch(p){
		case  UP|RIGHT : 
			foo= ACS_LLCORNER;
			break;
		case  UP|DOWN : 
			foo=ACS_VLINE;
			break;
		case  UP|LEFT : 
			foo=ACS_LRCORNER;
			break;
		case DOWN|RIGHT : 
			foo =ACS_ULCORNER;
			break;
		case DOWN|LEFT :
			foo=ACS_URCORNER;
			break;
		case LEFT|RIGHT: 
			foo=ACS_HLINE;
			break;
		case RIGHT: 
			foo = '>';
			break;
		case LEFT:
			foo = '<';
			break;
		case UP: 
			foo = '^';
			break;
		case DOWN:
			foo = 'v';
			break;
		case CROSSOVER: //all
			foo = ACS_PLUS;
			break;
		default:
			foo = ' ';
			break;		
	}
	if( pipe & FILLED )
		foo |= green;
	mvaddch(y,x, foo|(highlight*A_REVERSE) );
}
//display
void draw(bitbox board[len][wid]){
	int y,x;
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
				addpipe(SY+1+y,SX+x+1,board[y][x], (y==py&&x==px) );//its highlighted when y==py and x==px
		}
	}
	rectangle();
}

void mouseinput(void){
#ifndef NO_MOUSE
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4 <len && minput.x-1<wid*2){
		py=minput.y-(1+SY);
		px=minput.x-(1+SX);
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('\n');
#endif //NO_MOUSE
}
//peacefully close when ^C is pressed
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void help(void){
	erase();
	logo();
	attron(A_BOLD);
	mvprintw(SY,SX+5,"-*            *-");
	mvprintw(3,0," HELP");
	mvprintw(4,0," PAGE");
	mvprintw(SY+7,SX,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	attron(green);
	mvprintw(SY,SX+7,"THE CONTROLS");
	attroff(green);
	mvprintw(SY+1,SX,"RETURN/ENTER : Place/Replace a pipe");
	mvprintw(SY+2,SX,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(SY+3,SX,"p : Pause");
	mvprintw(SY+4,SX,"q : Quit");
	mvprintw(SY+5,SX,"f : Toggle fast flow");
	mvprintw(SY+6,SX,"g : Go! (End the countdown.)");
	mvprintw(SY+6,SX,"F1 & F2 : Help on controls & gameplay");
	mvprintw(SY+9,SX,"Press a key to continue");
	refresh();
	while(getch()==ERR);
	erase();
}
void gameplay(void){
	erase();
	logo();
	attron(A_BOLD);
	mvprintw(SY,SX+5,"-*            *-");
	mvprintw(3,0," HELP");
	mvprintw(4,0," PAGE");
	attroff(A_BOLD);
	attron(green);
	mvprintw(SY,SX+7,"THE GAMEPLAY");
	attroff(green);
	mvprintw(SY+1,SX,"Keep maintaining the pipeline and");
	mvprintw(SY+2,SX,"don't let the sewage leak.");
	refresh();
	while(getch()==ERR);
	erase();
}
int main(int argc, char** argv){
	signal(SIGINT,sigint_handler);
	initscr();
#ifndef NO_MOUSE
	mousemask(ALL_MOUSE_EVENTS,NULL);
#endif
	time_t tstart , now, lasttime, giventime=len*wid/4;
	srand(time(NULL)%UINT_MAX);		
	bitbox direction,board[len][wid];
	int input;
	byte foo;
	bool flow,fast;
	Start:
	flow=0;
	fast=0;
	score=0;
	memset(board,0,len*wid);
	fy=1+(rand()%(len-2) );
	fx=1+(rand()%(wid-2) );
	board[fy][fx]= 1 << (rand()%4);
	direction= board[fy][fx];
	board[fy][fx]|=FILLED;
	for(foo=0;foo<5;++foo)
		tocome[foo]=pipegen();
	tstart = time(NULL);
	lasttime=0;
	initscr();
	curs_set(0);
	noecho();
	cbreak();
	halfdelay(DELAY);
	keypad(stdscr,1);
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(2,COLOR_GREEN,-1);
		green=COLOR_PAIR(2);

	}
	py=px=0;
	while(1){
		now=time(NULL);
		erase();
		logo();
		if(fast){
			attron(A_BOLD);
			mvprintw(3,0," FAST");
			attroff(A_BOLD);
		}

		if(!flow && giventime >= now-tstart){
			mvprintw(4,0,"Time:%ld",giventime-(now-tstart));
			mvprintw(5,0,"Score:");
			mvprintw(6,0,"%ld",score);
		}
		else{
			mvprintw(4,0,"Score:");
			mvprintw(5,0,"%ld",score);
		}
		for(foo=0;foo<5;++foo)
			addpipe(11-foo,4,tocome[foo],0); 
		draw(board);
		refresh();

		if(now-tstart == giventime){
			flow=1;
		}
		if(flow && (fast || ( !(now%FLOWDELAY)&& now!=lasttime ) )){
			lasttime = now;
			MID(direction);
			if(fy<len && fx<wid && fy>=0&& fx>=0 && ( board[fy][fx]&opposite(direction) ) ){
				if(board[fy][fx] != CROSSOVER && board[fy][fx] != (CROSSOVER|FILLED) )
					direction = board[fy][fx] & ~opposite(direction);
				++score;
				if(fast)
					++score;
			}
			else 
				goto End;
			board[fy][fx]|=FILLED;
		}

		input = getch();
		if( input == KEY_F(1) || input=='?' ){
			help();
			if(!flow)
				tstart += time(NULL)-now;
		}
		if( (input==KEY_F(2)||input=='!') ){
			gameplay();
			if(!flow)
				tstart += time(NULL)-now;
		}
		if( input == KEY_MOUSE )
			mouseinput();
		if( (input=='k' || (input==KEY_UP||input=='w')) && py>0 )
			--py;
		if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<len-1 )
			++py;
		if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>0 )
			--px;
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<wid-1 )
			++px;
		if( (input == '\n'||input==KEY_ENTER) && !(board[py][px] & FILLED) ){
			if(board[py][px])
				score-=3;
			board[py][px]=tocome[0];
			for(foo=0;foo<4;++foo)
				tocome[foo]=tocome[foo+1];
			tocome[4]= pipegen();
		}
		if( input=='f' ){
			if(fast){
				halfdelay(DELAY);
				fast=0;
			}
			else{
				halfdelay(1);
				fast=1;
			}
		}
		if( input=='p'){
			erase();
			logo();
			attron(A_BOLD);
			mvprintw(3,0,"PAUSED");
			attroff(A_BOLD);
			refresh();
			while(getch()==ERR);
			if(!flow)//pausing should not affect the countdown
				tstart += time(NULL)-now;//now is not right now
			continue;
		}
		if(!flow && input == 'g' )
			flow=1;
		if( score < -1000)
			goto End;
		if( (input=='q'||input==27)){
			nocbreak();
			cbreak();
			curs_set(1);
			mvprintw(len+2,0,"Do you want to see the high scores?(y/n)");
			input=getch();
			if(input == 'N' || input=='n' || (input=='q'||input==27))
				sigint_handler(EXIT_SUCCESS);
			
			show_scores(save_score());
			mvprintw(len+2,0,"Press a key to exit:");
			refresh();
			getch();
			sigint_handler(EXIT_SUCCESS);
		}
			
	}
	End:
	nocbreak();
	cbreak();
	curs_set(1);
	attron(A_BOLD|green);
	mvprintw(3,0," OOPS!");
	attroff(A_BOLD|green);
	draw(board);
	mvprintw(len+2,0,"Game over! Press a key to see the high scores:");
	getch();
	show_scores(save_score());
	mvprintw(len+2,0,"Game over!");
	printw(" Wanna play again?(y/n)");
	input=getch();
       	if( input!= 'N' &&  input!= 'n' && input!='q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
