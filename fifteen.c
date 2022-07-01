/* 
.--
|__
|  IFTEEN

Authored by abakh <abakh!tuta,io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"

/* The Plan9 compiler can not handle VLAs */
#ifdef NO_VLA
#define size 4
#else
byte size=4;
#endif
byte py,px;
byte ey,ex; //the empty tile
chtype green=A_BOLD; //bold when there is no color
void rectangle(byte sy,byte sx){
	for(byte y=0;y<=size+1;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+size*2,ACS_VLINE);
	}
	for(byte x=0;x<=size*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+size+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+size+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+size*2,ACS_URCORNER);
	mvaddch(sy+size+1,sx+size*2,ACS_LRCORNER);
}
void logo(byte sy,byte sx){
	mvaddstr(sy,sx,  ".--");
	mvaddstr(sy+1,sx,"|__");
	mvaddstr(sy+2,sx,"|  IFTEEN");
}
//convert integer to representing sign
char int2sgn(byte num){
	if(!num)
		return ' ';
	else if(0< num && num <= 9)
		return num+'0';
	else if(10<=num && num <=35)
		return num-10+'a';
	else if(36<=num && num <=51)
		return num-36+'A';
	return 0;
}
/*bool isinorder(byte board[size][size],byte y,byte x){ using check[][] is much cheaper
	return (board[y][x] == y*size+x+1);
} */

//display
void draw(byte sy,byte sx,char board[size][size],char check[size][size]){
	rectangle(sy,sx);
	chtype prnt;
	byte y,x;
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			prnt=board[y][x];
			if(check[y][x]==board[y][x] && check[y][x] != ' ')
				prnt |= green;
			if(y==py && x==px)
				prnt |= A_STANDOUT;
			mvaddch(sy+1+y,sx+x*2+1,prnt);
		}
	}
}
void fill(char board[size][size]){
	byte y,x;
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			board[y][x]= int2sgn(y*size+x+1);
		}
	}
	board[size-1][size-1]=' ';
}
void slide_one(char board[size][size],byte y,byte x){
	if( (y>=0 && y<size && x>=0 && x<size) && ((abs(y-ey)==1)^(abs(x-ex)==1)) ){
		board[ey][ex]=board[y][x];
		board[y][x]=' ';
		ey=y;//ey/x moves one tile
		ex=x;
	}
}
void slide_multi(char board[size][size],byte y,byte x){
	byte dy,dx;
	dy=dx=0;
	if( (ey==y) ^ (ex==x) ){
		if(ey!=y)//d's are steps from ey/x to y/x
			 dy=(y-ey >0)? 1:-1;
		if(ex!=x)
			 dx=(x-ex >0)? 1:-1;
		while(ex!=x || ey!=y){
			slide_one(board,ey+dy,ex+dx);//ey/x comes forth itself
		}
		ey=y;
		ex=x;
	}
}
bool issolved(char board[size][size],char check[size][size]){
	byte y,x;
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			if(board[y][x]!=check[y][x])
				return 0;
		}
	}
	return 1;
}
void shuffle(char board[size][size]){
	for(int m=0;m<10000;++m){
		switch(rand()%4){
			case 0:
				slide_one(board,ey,ex+1);
				break;
			case 1:
				slide_one(board,ey,ex-1);
				break;
			case 2:
				slide_one(board,ey+1,ex);
				break;
			case 3:
				slide_one(board,ey-1,ex);
				break;
		}
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
	if( minput.y-4<size && minput.x-1<size*2){
		py=minput.y-4;
		px=(minput.x-1)/2;
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
	mvprintw(4,0,"RETURN/ENTER : Slide");
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
	mvprintw(4,0,"Slide the tiles until the numbers and characters are\n");
	printw("in the right order.\n");
	refresh();
	getch();
	erase();
}
int main(int argc, char** argv){
	int opt;
	bool no_replay=0;
	while( (opt=getopt(argc,argv,"hns:"))!=-1){
		switch(opt){
#ifndef NO_VLA
			case 's':
				size=atoi(optarg);
				if(size<3 || size>7){
					fprintf(stderr,"3<=size<=7");
				}
			break;
#endif //NO_VLA
			case 'n':
				no_replay=1;
			break;	
			case 'h':
			default:
				printf("Usage:%s [options]\n -s size\n -h help\n -n don't ask for replay\n",argv[0]);
			break;
		}
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
		init_pair(1,COLOR_GREEN,-1);
		green=COLOR_PAIR(1);
	}
	char board[size][size];
	char check[size][size];
	fill(check);
	int input;
	Start:
	py=px=0;
	ey=ex=size-1;
	curs_set(0);
	fill(board);
	shuffle(board);
	while(1){
		erase();
		logo(0,0);
		draw(3,0,board,check);
		refresh();
		if(issolved(board,check))
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
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<size-1)
			++px;
		if( (input=='q'||input==27))
			sigint_handler(0);
		if(input=='\n'||input==KEY_ENTER){
			slide_multi(board,py,px);
		}
	}
	mvprintw(size+5,0,"You solved it!");
	if(!no_replay){
		printw(" Wanna play again?(y/n)");
		refresh();
		curs_set(1);
		input=getch();
		if(input != 'N' && input != 'n' && input != 'q')
			goto Start;
	}
	else{
		printw(" Press any key on this computer's keyboard if you want to continue.");
		getch();
	}
	endwin();
	return EXIT_SUCCESS;
}
