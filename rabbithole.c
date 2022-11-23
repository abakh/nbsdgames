/* 
 _
|_)
| \ABBITHOLE

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


compile with -lncurses
*/

#include "common.h"
#define UP 1
#define RIGHT 2
#define DOWN 4
#define LEFT 8
#define VISITED 16
#define CARROT 32
typedef unsigned char bitbox;

/* The Plan9 compiler can not handle VLAs */
#ifdef NO_VLA
#define len 10
#define wid 20
#else
int len,wid;
#endif
int py,px;

chtype colors[6]={0};

typedef struct point{
	int y;
	int x;
} point; 

point MID(int y,int x,bitbox direction){//move in direction
	point pt = {y,x};
	switch(direction){
		case UP:
			--pt.y;
			return pt;
		case DOWN:
			++pt.y;
			return pt;
		case LEFT:
			--pt.x;
			return pt;
		case RIGHT:
			++pt.x;
			return pt;
	}
	return pt;
}
void rectangle(int sy,int sx){
	for(int y=0;y<=len*2;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+wid*2,ACS_VLINE);
	}
	for(int x=0;x<=wid*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+len*2,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+len*2,sx,ACS_LLCORNER);
	mvaddch(sy,sx+wid*2,ACS_URCORNER);
	mvaddch(sy+len*2,sx+wid*2,ACS_LRCORNER);
}

void draw(int sy,int sx,bitbox board[len][wid]){
	int y,x;
	bitbox d;
	chtype prnt;
	point pt;
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			prnt=0;
			if( board[y][x] & CARROT )
				prnt='%'|A_BOLD|colors[3];
			else if(y==py && x==px)
				prnt= 'r'|A_REVERSE;
			if( board[y][x] & VISITED ){
				if(y!=py || x!=px)	
					prnt='.'|A_REVERSE;
				for(d=1;d<32;d=d << 1){
					if(board[y][x] & d){
						pt=MID(sy+1+y*2,sx+x*2+1,d);
						mvaddch(pt.y,pt.x,' '|A_REVERSE);
					}
				}
			}
			if(prnt)
				mvaddch(sy+1+y*2,sx+x*2+1,prnt);
		}
	}
	rectangle(sy,sx);
}
void make_maze(bitbox board[len][wid],point f){
	byte ds_tried=0;
	byte dnumber=rand()%4;
	bitbox direction= 1 << (dnumber);
	while( direction == board[f.y][f.x] )
		direction= 1 << (dnumber=rand()%4);
	
	point pt = MID(f.y,f.x,direction);
	while(ds_tried<4){
		if(pt.y<0 || pt.y==len || pt.x<0 || pt.x==wid || board[pt.y][pt.x])
			;
		else{ //if the tile exists and is empty
			board[f.y][f.x] |= direction;
			board[pt.y][pt.x]= 1 << ( (dnumber+2)%4 );//direction's reverse
			make_maze(board,pt);//recursion
		}
		direction= 1 << (dnumber= (dnumber+1)%4 );
		pt= MID(f.y,f.x,direction);
		++ds_tried;
	}
}
void carrotify(bitbox board[len][wid],int count){
	int y,x,c=count;
	while(c){
		y=rand()%len;
		x=rand()%wid;
		while( board[y][x] & CARROT ){
			y=rand()%len;
			x=rand()%wid;
		}
		board[y][x] |= CARROT;
		--c;
	}
}
void help(void){
	erase();
	mvprintw(0,0," _ ");
	mvprintw(1,0,"|_)");
	mvprintw(2,0,"| \\ABBITHOLE");	
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	attroff(A_BOLD);
	mvprintw(4,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(5,0,"q : Quit");
	mvprintw(6,0,"F1 & F2: Help on controls & gameplay (viewing these pages doesn't pause the timer!)");
	mvprintw(7,0,"PgDn,PgUp,<,> : Scroll");
	mvprintw(9,0,"Press a key to continue");
	
	refresh();
	while ( getch()==ERR );
	erase();
}
void gameplay(void){
	erase();
	mvprintw(0,0," _ ");
	mvprintw(1,0,"|_)");
	mvprintw(2,0,"| \\ABBITHOLE");
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("Try to gather all the carrots in the maze\n");
	printw("in the given time. The determining factors\n");
	printw("are your choice of paths and the speed of\n ");
	printw("your fingers.\n");
	refresh();
	while ( getch()==ERR );
	erase();
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
int main(int argc,char** argv){
	if(argc>1){
		printf("This game doesn't take arguments");
	}
	signal(SIGINT,sigint_handler);
	initscr();
#ifndef NO_VLA
	if((LINES-7)/2 < 5){
		len=5;
	}
	else{
		len=(LINES-7)/2;
	}
	if((COLS-5)/2 < 20){
		wid=20;
	}
	else{
		wid=(COLS-5)/2;
	}
#endif
	int carrot_count= (len*wid)/50;
	int carrots_found;
	time_t tstart , now, giventime=len*wid/5;
	srand(time(NULL)%UINT_MAX);		
	point start={0,0};
	bitbox board[len][wid];
	int sy,sx;
	Start:
	tstart = time(NULL);
	carrots_found=0;
	initscr();
	curs_set(0);
	noecho();
	cbreak();
	halfdelay(3);
	keypad(stdscr,1);
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_BLUE,-1);
		init_pair(2,COLOR_GREEN,-1);
		init_pair(3,COLOR_YELLOW,-1);
		init_pair(4,COLOR_RED,-1);
		init_pair(5,COLOR_RED,COLOR_YELLOW);
		init_pair(6,COLOR_RED,COLOR_MAGENTA);
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}

	}
	sy=sx=0;
	py=px=0;
	memset(board,0,len*wid);
	make_maze(board,start);
	carrotify(board,carrot_count);
	int input;
	while(1){
		board[py][px] |= VISITED;
		if( board[py][px] & CARROT ){
			++carrots_found;
			board[py][px] &= ~CARROT;
		}
		now=time(NULL);
		erase();
		mvprintw(sy+0,sx+0," _ ");
		mvprintw(sy+1,sx+0,"|_)          Time left    :%ld",giventime-(now-tstart));
		mvprintw(sy+2,sx+0,"| \\ABBITHOLE Carrots left :%d",carrot_count-carrots_found);
		draw(sy+3,sx+0,board);
		refresh();
		if(carrots_found==carrot_count || now-tstart == giventime){
			flushinp();
			break;
		}
		input = getch();
		if( input==KEY_PPAGE && LINES< len+3){//the board starts in 3
			sy+=10;
			if(sy>0)
				sy=0;
		}
		if( input==KEY_NPAGE && LINES< len+3){
			sy-=10;
			if(sy< -(len+3) )
				sy=-(len+3);
		}
		if( input=='<' && COLS< wid*2+1){
			sx+=10;
			if(sx>0)
				sx=0;
		}
		if( input=='>' && COLS< wid*2+1){
			sx-=10;
			if(sx< -(wid*2+1))
				sx=-(wid*2+1);
		}
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		if( input == KEY_F(1) || input=='?' )
			help();
		if( (input=='k' || (input==KEY_UP||input=='w')) && py>0 && (board[py][px]&UP) )
			--py;
		if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<len-1 && (board[py][px]&DOWN) )
			++py;
		if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>0 && (board[py][px]&LEFT) )
			--px;
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<wid-1 && (board[py][px]&RIGHT) )
			++px;
		if( (input=='q'||input==27))
			sigint_handler(0);
		if( board[py][px] & CARROT ){
			++carrots_found;
			board[py][px] &= ~CARROT;
		}
	}
	End:
	nocbreak();
	cbreak();
	draw(3,0,board);
	refresh();
	if(carrots_found==carrot_count)
		mvprintw(len*2+4,0,"YAY!!");
	else
		mvprintw(len*2+4,0,"You gathered %2.1f%% of the carrots in %d seconds.",(float) carrots_found*100/carrot_count,giventime);

	printw(" Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if(input == 'Y' || input == 'y')
		goto Start;
	else if( input!= 'N' &&  input!= 'n' && input!='q')
		goto End;
	endwin();
	return EXIT_SUCCESS;
}
