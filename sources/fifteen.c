#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
/* 
.--
|__
|  IFTEEN

Authored by Hossein Bakhtiarifar <abakh@tuta.io>
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
typedef signed char byte;
byte size;
byte py,px;
byte ey,ex; //the empty tile
chtype green=A_BOLD; //bold when there is no color
void rectangle(byte sy,byte sx){
	for(byte y=0;y<=size+1;y++){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+size*2,ACS_VLINE);
	}
	for(byte x=0;x<=size*2;x++){
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
	for(y=0;y<size;y++){
		for(x=0;x<size;x++){
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
	for(y=0;y<size;y++){
		for(x=0;x<size;x++){
			board[y][x]= int2sgn(y*size+x+1);
		}
	}
	board[size-1][size-1]=' ';
}
void slide_one(char board[size][size],byte y,byte x){
	if( (y>=0 && y<size && x>=0 && x<size) &&(abs(y-ey)==1)^(abs(x-ex)==1) ){
		board[ey][ex]=board[y][x];
		board[y][x]=' ';
		ey=y;
		ex=x;
	}
}
void slide_multi(char board[size][size],byte y,byte x){
	byte dy,dx;
	if( (ey==y) ^ (ex==x) ){
		if(ey!=y)
			 dy=(y-ey)/abs(y-ey);
		if(ex!=x)
			 dx=(x-ex)/abs(x-ex);
		while(ex!=x || ey!=y)
			slide_one(board,ey+dy,ex+dx);//ey/x comes forth itself
		ey=y;
		ex=x;
	}
}
bool issolved(char board[size][size],char check[size][size]){
	byte y,x;
	for(y=0;y<size;y++){
		for(x=0;x<size;x++){
			if(board[y][x]!=check[y][x])
				return 0;
		}
	}
	return 1;
}
void shuffle(char board[size][size]){
	for(int m=0;m<1000;m++){
		switch(random()%4){
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
	MEVENT minput;
	getmouse(&minput);
	if( minput.y-4<size && minput.x-1<size*2){
		py=minput.y-4;
		px=(minput.x-1)/2;
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('\n');
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
	size=4;
	if(argc==2){
		if(!strcmp("help",argv[1])){
                        printf("Usage: %s [size]\n",argv[0]);
			return EXIT_SUCCESS;
		}
		size=atoi(argv[1]);
		if(size<3 || size>7){
			fprintf(stderr,"3<=size<=7\n");
			return EXIT_FAILURE;
		}
	}
	signal(SIGINT,sigint_handler);
	srandom(time(NULL)%UINT_MAX);
	initscr();
        mousemask(ALL_MOUSE_EVENTS,NULL);
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
		if( input==KEY_F(2) )
			gameplay();
		if( input==KEY_MOUSE )
			mouseinput();
		if( (input=='k' || input==KEY_UP) && py>0)
			py--;
		if( (input=='j' || input==KEY_DOWN) && py<size-1)
			py++;
		if( (input=='h' || input==KEY_LEFT) && px>0)
			px--;
		if( (input=='l' || input==KEY_RIGHT) && px<size-1)
			px++;
		if( input=='q')
			sigint_handler(0);
		if(input=='\n'){
			slide_multi(board,py,px);
		}
	}
	mvprintw(size+5,0,"You solved it! Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if(input != 'N' && input != 'n' && input != 'q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
