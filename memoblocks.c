/* 
.  .      _
|\/|     |_)
|  |EMORY|_)LOCKS


Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"

typedef unsigned char ubyte;

#define size 8
#define size2 16

byte py,px;
byte fy,fx; //the first tile
chtype colors[6]={0};
void rectangle(byte sy,byte sx){
	for(byte y=0;y<=size+1;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+size2+1,ACS_VLINE);
	}
	for(byte x=0;x<=size2+1;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+size+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+size+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+size2+1,ACS_URCORNER);
	mvaddch(sy+size+1,sx+size2+1,ACS_LRCORNER);
}
void logo(byte sy,byte sx){
	mvaddstr(sy,sx,  ".  .      _");
	mvaddstr(sy+1,sx,"|\\/|     |_)");
	mvaddstr(sy+2,sx,"|  |EMORY|_)LOCKS");
}
//convert integer to representing sign
char int2sgn(byte num){
	if(0< num && num <= 9)
		return num+'0';
	else if(10<=num && num <=35)
		return num-10+'a';
	else if(36<=num && num <=51)
		return num-36+'A';
	else if(52<=num && num<=64)
		return num-52+'!';
	return 0;
}
//display
void draw(byte sy,byte sx,chtype board[size][size2],bool show[size][size2]){
	rectangle(sy,sx);
	byte y,x;
	chtype prnt;
	for(y=0;y<size;++y){
		for(x=0;x<size2;++x){
			if(show[y][x] || (y==fy && x==fx) )
				prnt=board[y][x];
			else
				prnt='.';
			if(y==py && x==px)
				prnt|=A_STANDOUT;

			mvaddch(sy+1+y,sx+1+x,prnt);
		}
	}
}
void fill(chtype board[size][size2]){
	ubyte y,x,m;
	int n;
	for(y=0;y<size;++y){
		for(x=0;x<size2;++x){
			n=(y*size2+x)/2;
			if(size*size<193) //(1+0*64)%6 == (1+3*64)%6 so this won't work in n=193 and above
				m=n%6;
			else //this for default wouldn't be colorful enough blow n=193
				m=(n/64)%6;
			board[y][x]=int2sgn((n%64)+1)|colors[m];
			//fills with 1,1,2,2,.. with colored pairs
		}
	}
}
bool issolved(bool show[size][size2]){
	byte y,x;
	for(y=0;y<size;++y){
		for(x=0;x<size2;++x){
			if(!show[y][x])
				return 0;
		}
	}
	return 1;
}
void shuffle(chtype board[size][size2]){
	int n=size*size*3;
	chtype a;
	byte ay,ax,by,bx;
	for(int m=0;m<n;++m){
		ay=rand()%size;
		ax=rand()%(size2);
		by=rand()%size;
		bx=rand()%(size2);
		a=board[ay][ax];
		board[ay][ax]=board[by][bx];
		board[by][bx]=a;
	}
}
//peacefully close when ^C is pressed
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(void){
#ifndef NO_MOUSE
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4<size && minput.x-1<size2){
		py=minput.y-4;
		px=(minput.x-1);
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('\n');
#endif
}
void help(void){
	erase();
	logo(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(8,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"RETURN/ENTER : Reveal");
	mvprintw(5,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(6,0,"q : Quit");
	mvprintw(7,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(10,0,"Press a key to continue");
	refresh();
	getch();
	erase();
}
void gameplay(void){
	erase();
	logo(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	mvprintw(4,0,"Click on a tile to see the gylph it contains,\n");
	printw(      "then try to find a matching gylph the same way.\n");
	printw(      "They form a pair only when you click a tile\n");
	printw(	     "directly after the match. The game ends when \n");
	printw(	     "you have found all the matching pairs.\n");
	refresh();
	getch();
	erase();
}
int main(int argc,char** argv){
	if(argc>1){
		printf("This game doesn't take arguments");
	}
	signal(SIGINT,sigint_handler);
	srand(time(NULL)%UINT_MAX);
	initscr();
#ifndef NO_MOUSE
	mousemask(ALL_MOUSE_EVENTS,NULL);
#endif
	noecho();
	cbreak();
	keypad(stdscr,1);
	if(has_colors()){
		start_color();
		use_default_colors();
		if( has_colors() ){
			start_color();
			use_default_colors();
			init_pair(1,COLOR_YELLOW,-1);
			init_pair(2,COLOR_GREEN,-1);
	       		init_pair(3,COLOR_BLUE,-1);
			init_pair(4,COLOR_CYAN,-1);
			init_pair(5,COLOR_MAGENTA,-1);
			init_pair(6,COLOR_RED,-1);
			for(byte b=0;b<6;++b){
				colors[b]=COLOR_PAIR(b+1);
			}
		}
	}
    	chtype board[size][size2];
	bool show[size][size2];
	int input;
	time_t tstart,now;
	Start:
	tstart=time(NULL);
	py=px=0;
	fy=fx=-1;
	curs_set(0);
	memset(show,0,size*size2);
	fill(board);
	shuffle(board);
	while(1){
		erase();
		logo(0,0);
		draw(3,0,board,show);
		refresh();
		if(issolved(show))
			break;
		input = getch();
		if( input==KEY_F(1) || input=='?' )
			help();
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		if( input==KEY_MOUSE )
			mouseinput();
		if( (input=='k' || (input==KEY_UP||input=='w')) && py>0)
			--py;
		if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<size-1)
			++py;
		if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>0)
			--px;
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<size2-1)
			++px;
		if( (input=='q'||input==27))
			sigint_handler(0);
		if(input=='\n' || input==KEY_ENTER){
			if(fy!=-1 && board[py][px]==board[fy][fx] && !(fy==py && fx==px) )
				show[py][px]=show[fy][fx]=1;
			else{
				fy=py;
				fx=px;
			}
		}
	}
	now=time(NULL)-tstart;
	mvprintw(size+7,0,"Time spent: %d:%2d:%2d",now/3600,(now%3600)/60,now%60);
	mvprintw(size+5,0,"You solved it!");
	printw(" Wanna play again?(y/n)");
	refresh();
	curs_set(1);
	input=getch();
	if(input != 'N' && input != 'n' && input != 'q')
		goto Start;

	endwin();
	return EXIT_SUCCESS;
}
