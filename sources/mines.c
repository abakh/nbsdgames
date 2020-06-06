#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#define FLAG 9
#define UNCLEAR 10
/* 
|\/|
|  |INES

Authored by Hossein Bakhtiarifar <abakh@tuta.io>
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
typedef signed char byte;
int len,wid,py,px,flags;
int untouched;
int mscount;
chtype colors[6]={0};
void rectangle(int sy,int sx){
	for(int y=0;y<=len+1;y++){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+wid*2,ACS_VLINE);
	}
	for(int x=0;x<=wid*2;x++){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+len+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+len+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+wid*2,ACS_URCORNER);
	mvaddch(sy+len+1,sx+wid*2,ACS_LRCORNER);
}
//display
void draw(int sy,int sx,byte board[len][wid]){
	rectangle(sy,sx);
	chtype attr ;
	char prnt;
	int y,x;
	for(y=0;y<len;y++){
		for(x=0;x<wid;x++){
			attr=A_NORMAL;
			if(y==py && x==px)
				attr |= A_STANDOUT;

			if(board[y][x]<0)
				prnt='.';
			else if(!board[y][x])
				prnt=' ';
			else if(board[y][x] < 8){
				attr |= colors[board[y][x]-1];
				prnt='0'+board[y][x];
			}
			else if(board[y][x]==9){
				attr |= colors[3];
				prnt='P';
			}
			else if(board[y][x]>9)
				prnt='?';

			mvaddch(sy+1+y,sx+x*2+1,attr|prnt);
		}
	}
}
//show the mines
void drawmines(int sy,int sx,byte board[len][wid],bool mines[len][wid]){
	int y,x;
	for(y=0;y<len;y++){
		for(x=0;x<wid;x++){
			if(mines[y][x]){
				if(y==py&&x==px)
					mvaddch(sy+y+1,sx+x*2+1,'X');
				else if(board[y][x]==9)
					mvaddch(sy+y+1,sx+x*2+1,'%');
				else
					mvaddch(sy+y+1,sx+x*2+1,'*');
			}
		}
	}
}
//place mines
void mine(bool mines[len][wid]){
	int y=rand()%len;
	int x=rand()%wid;
	for(int n=0;n<mscount;n++){
		while(mines[y][x]){
			y=rand()%len;
			x=rand()%wid;
		}
		mines[y][x]=true;
	}
}

bool click(byte board[len][wid],bool mines[len][wid],int ty,int tx){
	if(board[ty][tx]>=0 && board[ty][tx] <9)//it has been click()ed before
		return 0;
	else{//untouched
		if(board[ty][tx]==FLAG)
			flags--;
		board[ty][tx]=0;
		untouched--;
		
	}
	int y,x;
	for(y=ty-1;y<ty+2;y++){
		if(y<0)
			y=0;
		if(y>=len)
			break;
		for (x=tx-1;x<tx+2;x++){
			if(x<0)
				x=0;
			if(x>=wid)
				break;

			if(mines[y][x])
				board[ty][tx]++;
		}
	}

	if(!board[ty][tx]){//there are  no mines in the adjacent tiles
		for(y=ty-1;y<ty+2;y++){
			if(y<0)
				y=0;
			if(y>=len)
				break;
			for(x=tx-1;x<tx+2;x++){
					if(x<0)
						x=0;
					if(x>=wid)
						break;

					click(board,mines,y,x);
			}
	
		}
	}
	return 0;
}

void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(int sy, int sx){
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4-sy<len && minput.x-1-sx<wid*2){
		py=minput.y-4-sy;
		px=(minput.x-1-sx)/2;
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('\n');
	if(minput.bstate & (BUTTON2_CLICKED|BUTTON3_CLICKED) )
		ungetch(' ');
}
void help(void){
	erase();
	mvprintw(1,0,"|\\/|");
	mvprintw(2,0,"|  |INES");
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(10,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"RETURN/ENTER : Examine for bombs");
	mvprintw(5,0,"SPACE : Flag/Unflag");
	mvprintw(6,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(7,0,"q : Quit");
	mvprintw(8,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(9,0,"PgDn,PgUp,<,> : Scroll"); 
	mvprintw(12,0,"Press a key to continue");
	refresh();
	getch();
	erase();
}
void gameplay(void){
	erase();
	mvprintw(1,0,"|\\/|");
	mvprintw(2,0,"|  |INES");
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	mvprintw(4,0,"Try to find the landmines in the field\n");
	printw("with logical reasoning: When you click\n");
	printw("on a tile ( a '.' here), numbers may show\n");
	printw("up that indicate the number of landmines\n");
	printw("in adjacent tiles; you should find and \n");
	printw("avoid the landmines based on them; and\n");
	printw("clicking on a landmine would make you\n");
	printw("lose the game.");
	refresh();
	getch();
	erase();
}
int main(int argc, char** argv){
	signal(SIGINT,sigint_handler);
	if(argc>4 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s [len wid [minescount]]\n",argv[0]);
		return EXIT_FAILURE;
	}
	if(argc==2){
		puts("Give both dimensions.");
		return EXIT_FAILURE;
	}
	if(argc>=3){
		bool lool = sscanf(argv[1],"%d",&len) && sscanf(argv[2],"%d",&wid);
		if(!lool){
			puts("Invalid input.");
			return EXIT_FAILURE;
		}
		if(len<5 || wid<5 || len>1000 || wid>1000){
			puts("At least one of your given dimensions is either too small or too big.");
			return EXIT_FAILURE;
		}
	
	}
	else
		len=wid=8;
	if(argc==4){
		if( !sscanf(argv[3],"%d",&mscount)){
			puts("Invalid input.");
			return EXIT_FAILURE;
		}
		if( mscount<5 || mscount>= len*wid){
			puts("Too few/many mines.");
			return EXIT_FAILURE;
		}
	}
	else
		mscount = len*wid/6;
	srand(time(NULL)%UINT_MAX);
	initscr();
	mousemask(ALL_MOUSE_EVENTS,NULL);
	noecho();
	cbreak();
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
		for(byte b= 0;b<6;b++){
			colors[b]=COLOR_PAIR(b+1);
		}

	}
	byte board[len][wid];
	bool mines[len][wid];
	char result[70];
	int input;
	int sy,sx;		
	Start:
	sy=sx=0;
	py=px=0;
	untouched=len*wid;
	flags=0;
	curs_set(0);
	memset(board,-1,len*wid);
	memset(mines,false,len*wid);
	mine(mines);
	
	while(1){
		erase();
		mvprintw(sy+1,sx+0,"|\\/|     Flags:%d\n",flags);
		mvprintw(sy+2,sx+0,"|  |INES Mines:%d\n",mscount);
		draw(sy+3,sx+0,board);
		refresh();
		if(untouched<=mscount){
			strcpy(result,"YAY!!");
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
		if( input==KEY_F(1) || input=='?' )
			help();
		if( input==KEY_F(2) )
			gameplay();
		if( input==KEY_MOUSE )
			mouseinput(sy,sx);
		if( (input=='k' || input==KEY_UP) && py>0)
			py--;
		if( (input=='j' || input==KEY_DOWN) && py<len-1)
			py++;
		if( (input=='h' || input==KEY_LEFT) && px>0)
			px--;
		if( (input=='l' || input==KEY_RIGHT) && px<wid-1)
			px++;
		if( input=='q')
			sigint_handler(0);
		if(input=='x' && getch()=='y' && getch()=='z' && getch()=='z' && getch()=='y' ){
			strcpy(result,"It is now pitch dark. If you proceed you will likely fall into a pit.");
			break;
		}
		if(input=='\n' && board[py][px] < 9){
			if(mines[py][px]){
				switch( rand()%3){
					case 0:
						strcpy(result,"You lost The Game.");
						break;
					case 1:
						strcpy(result,"You exploded!");
						break;
					case 2:
						strcpy(result,"Bring your MRAP with you next time!");
				}
				break;
			}
			click(board,mines,py,px);
		}
		if(input==' '){
			 if(board[py][px] == -1){
				board[py][px]=FLAG;
				flags++;
			 }
			 else if(board[py][px] == FLAG){
				board[py][px]=UNCLEAR;
				flags--;
			 }
			 else if(board[py][px] == UNCLEAR)
				board[py][px]=-1;
		}
	}
	drawmines(sy+3,sx+0,board,mines);
	mvprintw(sy+len+5,sx+0,"%s Wanna play again?(y/n)",result);
	curs_set(1);
	input=getch();
	if(input != 'N' && input != 'n' && input != 'q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
