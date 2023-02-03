/* 
_____
  |
  |REASURE
Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


compile with -lncurses
*/
#include "common.h"
#define FOUND 9
#define UNTOUCHED -1
#define MINLEN 8
#define MINWID 8
#define MAXLEN 1000
#define MAXWID 1000
#define EMPTY_LINES 7
#define MAX_REPEATS 5
#ifdef NO_VLA //The Plan9 compiler can not handle VLAs
#define len 8
#define wid 8
#else
int len=8,wid=8;
#endif
int py,px,flags;
int mscount;
long scores[2];
char sides[2]={'h','h'};

chtype colors[6]={0};
int beginy,view_len;
int turn=0;
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
void logo(int sy, int sx){
	mvprintw(sy,sx,     "_____");
	mvprintw(sy+1,sx+0, "  |  ");
	mvprintw(sy+2,sx,   "  |REASURE    %ld:%ld",scores[0],scores[1]);
 
	
	if(turn==0){
		attron(colors[1]);
		mvprintw(sy+1,sx+11,"Percent's Turn");
		attroff(colors[1]);
	}
	if(turn==1){
		attron(colors[2]);
		mvprintw(sy+1,sx+11,"Square's Turn");
		attroff(colors[2]);
	}
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
				attr |= colors[1];
				prnt='%';
			}
			else if(get_cell(board,y,x)==10){
				attr |= colors[2];
				prnt='#';
			}
			mvaddch(sy+1+(y-beginy),sx+x*2+1,attr|prnt);
		}
	}
}
byte click(byte board[len][wid],byte mines[len][wid],int ty,int tx){
	if(mines[ty][tx]){
		board[ty][tx]=9+turn;
		scores[turn]+=1;
		return 1;
	}

	if(board[ty][tx]>=0 && board[ty][tx] <9)//it has been click()ed before
		return 0;
	else{
		board[ty][tx]=0;
		
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


//count discovered mines around the number being inspected
float hit_probablity(byte board [len][wid],byte mines[len][wid],int ny,int nx){//n:number
	int y,x;
	float empty=0;
	float bombs=0;
	for(y=ny-1;y<ny+2;++y){
		for(x=nx-1;x<nx+2;++x){
			if(y<0 || y>=len || x<0 || x>=wid){
				continue;
			}
			if(board[y][x]==UNTOUCHED){
				++empty;			
				if(mines[y][x]==1){
					++bombs;
				}
			}
		}
	}
	if(empty==0){
		return 0;
	}
	return bombs/empty;

}
//AI algorithm
byte decide(byte board[len][wid],byte mines[len][wid]){
	float maxp=0;
	float p=0;
	int targety=-1, targetx=-1;
	int hity,hitx;
	int y,x;
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			if(0<board[y][x] &&  board[y][x]<9){
				refresh();
				p=hit_probablity(board,mines,y,x);
				if(p>maxp){
					targety=y;
					targetx=x;
				}
				if(p==1.0){
					goto Skip;
				}
			}
		}
	}
	Skip:
	if(-1==targety){
		do{
			hity=rand()%len;
			hitx=rand()%wid;
		}while(board[hity][hitx]!=-1);
	}
	else{
		do{
			hity=targety-1+(rand()%3);
			hitx=targetx-1+(rand()%3);
		}while(board[hity][hitx]!=UNTOUCHED ||hitx<0 || hitx>=wid || hity<0 || hity>=len);
	}
	return click(board,mines,hity,hitx);
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
	logo(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(9,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"RETURN/ENTER : Examine for bombs");
	mvprintw(5,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(6,0,"q : Quit");
	mvprintw(7,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(8,0,"PgDn,PgUp,<,> : Scroll"); 
	mvprintw(11,0,"Press a key to continue");
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
	mvprintw(4,0,"Like Mines but you need to find stuff\n");
	printw("instead of avoiding them.\n");
	refresh();
	getch();
	erase();
}
int main(int argc, char** argv){
	signal(SIGINT,sigint_handler);
#ifndef	NO_VLA 
	int opt;
	int input;
	int sides_chosen=0,size_chosen=0;
	while( (opt=getopt(argc,argv,"hnm:l:w:"))!=-1){
		switch(opt){
			case '1':
			case '2':
				if(!strcmp("c",optarg) || !strcmp("h",optarg)){
					sides[opt-'1']=optarg[0];
					sides_chosen=1;
				}
				else{
					puts("That should be either h or c\n");
					return EXIT_FAILURE;
				}
			break;
			case 'm':
				mscount=atoi(optarg);
				if(mscount<0 || mscount>len*wid){
					fprintf(stderr,"Too few/many mines.\n");
				}
			break;
			case 'l':
				size_chosen=1;
				len=atoi(optarg);
				if(len<MINLEN || len>MAXLEN){
					fprintf(stderr,"Length too high or low.\n");
				}
			break;
			case 'w':
				size_chosen=1;
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
#ifndef NO_VLA
	if(!size_chosen){
		if((LINES-7) < 5){
			len=5;
		}
		else{
			len=15;
		}
		if((COLS-5)/2 < 20){
			wid=20;
		}
		else{
			wid=15;
		}
	}
	if(!mscount){
		mscount=len*wid/8;
	}
#else
	mscount=len*wid/8;
#endif

	if(!sides_chosen){
		printw("Choose type of the # player(H/c)\n" );
		refresh();
		input=getch();
		if(input=='c'){
			sides[0]='c';
			printw("Computer.\n");
		}
		else{
			sides[0]='h';
			printw("Human.\n");
		}
		printw("Choose type of the %% player(h/C)\n");
		refresh();
		input=getch();
		if(input=='h'){
			sides[1]='h';
			printw("Human.\n");
		}
		else{
			sides[1]='c';
			printw("Computer.\n");
		}
	}

	byte board[len][wid];
	byte mines[len][wid];
	char result[70];
	int sy,sx;
	byte repeats;
	bool first_click;	
	byte won;
	Start:
	won=-1;
	scores[0]=scores[1]=0;
	sy=sx=0;
	py=px=0;
	flags=0;
	curs_set(0);
	for(int y=0;y<len;++y){
		for(int x=0;x<wid;++x){
			board[y][x]=UNTOUCHED;
			mines[y][x]=0;
		}
	}
	mine(mines);
	turn=1;
	Turn:
	erase();
	logo(sy,sx);
	draw(sy+3,sx+0,board);
	refresh();
	if(scores[0]>mscount/2){
		won=0;
		goto End;
	}
	if(scores[1]>mscount/2){
		won=1;
		goto End;
	}
	repeats=0;
	turn=!turn;
	if(sides[turn]=='c'){
		bool first_time=1;
		do{
			++repeats;
			erase();
			logo(sy,sx);
			draw(sy+3,sx+0,board);
			refresh();
			if(!first_time){
				usleep(500000);//it is demoralising to see it find 5 mines in a split second
			}
			else{
				first_time=0;
			}
			if(scores[0]>mscount/2 || scores[1]>mscount/2){
				goto Turn;
			}

		}while(decide(board,mines) && repeats<MAX_REPEATS);
		goto Turn;
	}

	MyTurnAgain:
	if(scores[0]>mscount/2 || scores[1]>mscount/2){
		goto Turn;
	}
	++repeats;
	if(repeats>=MAX_REPEATS){
		goto Turn;
	}
	while(1){
		erase();
		logo(sy,sx);
		draw(sy+3,sx+0,board);
		refresh();
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
		if((input=='\n'||input==KEY_ENTER) && board[py][px]==UNTOUCHED){
			if(click(board,mines,py,px)){
				goto MyTurnAgain;
			}
			else{
				goto Turn;
			}
		}
	}
	End:
	move(sy+view_len+5,sx+0);
	if(won==0){
		attron(colors[1]);
		printw("Percent won.");
		attroff(colors[1]);
	}
	else{
		attron(colors[2]);
		printw("Square won.");
		attroff(colors[2]);
	}
	printw(" Wanna play again?(y/n)",result);

	curs_set(1);
	input=getch();
	if(input != 'N' && input != 'n' && input != 'q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
