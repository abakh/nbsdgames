/* 
|\/|
|  |INES

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


compile with -lncurses
*/
#include "common.h"
#define FLAG 9
#define UNCLEAR 10
#define MINLEN 8
#define MINWID 8
#define MAXLEN 1000
#define MAXWID 1000
#define EMPTY_LINES 7 
#ifdef NO_VLA //The Plan9 compiler can not handle VLAs
#define len 8
#define wid 8
#else
int len=8,wid=8;
#endif
int py,px,flags;
int untouched;
int mscount;
chtype colors[6]={0};
int beginy,view_len;
void setup_scroll(){
	beginy=0;
	if(0<py+3-(LINES-EMPTY_LINES)){
		beginy=py+3-(LINES-EMPTY_LINES);
	}
	view_len=len;
	if(LINES-EMPTY_LINES<len){
		view_len=LINES-EMPTY_LINES;
	}
	if(beginy+view_len>len){
		beginy-=beginy+view_len-len;
	}
}

void rectangle(int sy,int sx){
	setup_scroll();
	for(int y=0;y<=view_len;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+wid*2,ACS_VLINE);
	}
	for(int x=0;x<=wid*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+view_len+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+view_len+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+wid*2,ACS_URCORNER);
	mvaddch(sy+view_len+1,sx+wid*2,ACS_LRCORNER);
}
byte get_cell(byte board[len][wid],int y,int x){
	return board[(y+len)%len][(x+wid)%wid];
}
//display
void draw(int sy,int sx,byte board[len][wid]){
	rectangle(sy,sx);
	chtype attr ;
	char prnt;
	int y,x;
	setup_scroll();
	for(y=beginy;y<beginy+view_len;++y){
		for(x=0;x<wid;++x){
			attr=A_NORMAL;
			if(y==py && x==px){
				attr |= A_STANDOUT;
			}
			if(get_cell(board,y,x)<0){
				prnt='.';
			}
			else if(!get_cell(board,y,x)){
				prnt=' ';
			}
			else if(get_cell(board,y,x) < 8){
				attr |= colors[board[y][x]-1];
				prnt='0'+get_cell(board,y,x);
			}
			else if(get_cell(board,y,x)==9){
				attr |= colors[3];
				prnt='P';
			}
			else if(get_cell(board,y,x)>9){
				prnt='?';
			}
			mvaddch(sy+1+(y-beginy),sx+x*2+1,attr|prnt);
		}
	}
}
//show the mines
void drawmines(int sy,int sx,byte board[len][wid],byte mines[len][wid]){
	int y,x;
	setup_scroll();
	for(y=beginy;y<beginy+view_len;++y){
		for(x=0;x<wid;++x){
			if(mines[y][x]){
				if(y==py&&x==px)
					mvaddch(sy+y-beginy+1,sx+x*2+1,'X');
				else if(get_cell(board,y,x)==9)
					mvaddch(sy+y-beginy+1,sx+x*2+1,'%');
				else
					mvaddch(sy+y-beginy+1,sx+x*2+1,'*');
			}
		}
	}
}
//place mines
void mine(byte mines[len][wid]){
	int y=rand()%len;
	int x=rand()%wid;
	mines[py][px]=1;//so it doesn't place mines where you click first
	for(int n=0;n<mscount;++n){
		while(mines[y][x]){
			y=rand()%len;
			x=rand()%wid;
		}
		mines[y][x]=1;
	}
	mines[py][px]=0;
}

byte click(byte board[len][wid],byte mines[len][wid],int ty,int tx){
	if(board[ty][tx]>=0 && board[ty][tx] <9)//it has been click()ed before
		return 0;
	else{//untouched
		if(board[ty][tx]==FLAG)
			--flags;
		board[ty][tx]=0;
		--untouched;
		
	}
	int y,x;
	for(y=ty-1;y<ty+2;++y){
		if(y<0)
			y=0;
		if(y>=len)
			break;
		for (x=tx-1;x<tx+2;++x){
			if(x<0)
				x=0;
			if(x>=wid)
				break;

			if(mines[y][x])
				board[ty][tx]++;
		}
	}

	if(!board[ty][tx]){//there are  no mines in the adjacent tiles
		for(y=ty-1;y<ty+2;++y){
			if(y<0)
				y=0;
			if(y>=len)
				break;
			for(x=tx-1;x<tx+2;++x){
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
#ifndef NO_MOUSE
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
#endif
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
#ifndef	NO_VLA 
	int opt;
	while( (opt=getopt(argc,argv,"hnm:l:w:"))!=-1){
		switch(opt){
			case 'm':
				mscount=atoi(optarg);
				if(mscount<0 || mscount>len*wid){
					fprintf(stderr,"Too few/many mines.\n");
				}
			break;
			case 'l':
				len=atoi(optarg);
				if(len<MINLEN || len>MAXLEN){
					fprintf(stderr,"Length too high or low.\n");
				}
			break;
			case 'w':
				wid=atoi(optarg);
				if(wid<MINWID || wid>MAXWID){
					fprintf(stderr,"Width too high or low.\n");
				}
			break;
			case 'h':
			default:
				printf("Usage:%s [options]\n -l length\n -w width\n -m number of mines\n -h help\n",argv[0]);
				return EXIT_FAILURE;
			break;
		}
	}
	if(!mscount){
		mscount=len*wid/6;
	}
#else
	mscount=len*wid/6;
#endif
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
	byte board[len][wid];
	byte mines[len][wid];
	char result[70];
	int input;
	int sy,sx;
	bool first_click;	
	Start:
	first_click=1;
	sy=sx=0;
	py=px=0;
	untouched=len*wid;
	flags=0;
	curs_set(0);
	for(int y=0;y<len;++y){
		for(int x=0;x<wid;++x){
			board[y][x]=-1;
			mines[y][x]=0;
		}
	}
	
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
			if(sy>0){
				sy=0;
			}
		}
		if( input==KEY_NPAGE && LINES< len+3){
			sy-=10;
			if(sy< -(len+3) ){
				sy=-(len+3);
			}
		}
		if( input=='<' && COLS< wid*2+1){
			sx+=10;
			if(sx>0){
				sx=0;
			}
		}
		if( input=='>' && COLS< wid*2+1){
			sx-=10;
			if(sx< -(wid*2+1)){
				sx=-(wid*2+1);
			}
		}	
		if( input==KEY_F(1) || input=='?' )
			help();
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		if( input==KEY_MOUSE )
			mouseinput(sy,sx);
		if( (input=='k' || (input==KEY_UP||input=='w')) && py>0)
			--py;
		if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<len-1)
			++py;
		if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>0)
			--px;
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<wid-1)
			++px;
		if( (input=='q'||input==27))
			sigint_handler(0);
		if(input=='x' && getch()=='y' && getch()=='z' && getch()=='z' && getch()=='y' ){
			if(first_click){
				strcpy(result,"That is for Windows.");
			}
			else{
				strcpy(result,"It is now pitch dark. If you proceed you will likely fall into a pit.");
			}
			break;
		}
		if((input=='\n'||input==KEY_ENTER) && board[py][px] < 9){
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



			if(first_click){
				mine(mines);
				first_click=0;
			}
			click(board,mines,py,px);
		}
		if(input==' '){
			 if(board[py][px] == -1){
				board[py][px]=FLAG;
				++flags;
			 }
			 else if(board[py][px] == FLAG){
				board[py][px]=UNCLEAR;
				--flags;
			 }
			 else if(board[py][px] == UNCLEAR)
				board[py][px]=-1;
		}
	}
	drawmines(sy+3,sx+0,board,mines);
	move(sy+view_len+5,sx+0);
	printw("%s Wanna play again?(y/n)",result);

	curs_set(1);
	input=getch();
	if(input != 'N' && input != 'n' && input != 'q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
