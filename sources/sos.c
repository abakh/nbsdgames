/* 
 _  _  _
(_'| |(_'
._):_:._)

Authored by abakh <abakh@tuta.io>
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#define NOTHING 123
typedef signed char byte;

#ifdef Plan9
#define len 5
#define wid 6
#else
int len,wid;
#endif

int py,px;
chtype colors[6]={A_BOLD};
int score[2] ={0};
int computer[2]={0};
char so[2] = {'S','O'};

char rd(char board[len][wid],int y, int x){
	if(y<0 || x<0 || y>= len || x>=wid)
		return NOTHING;
	else
		return board[y][x];
}
void color(byte colored[len][wid],int y,int x,bool side){
	if(colored[y][x] == !side || colored[y][x]==2)
		colored[y][x]=2;
	else
		colored[y][x]=side;
}
void rectangle(int sy,int sx){
	for(int y=0;y<=len+1;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+wid*2,ACS_VLINE);
	}
	for(int x=0;x<=wid*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+len+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+len+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+wid*2,ACS_URCORNER);
	mvaddch(sy+len+1,sx+wid*2,ACS_LRCORNER);
}

void draw(int sy,int sx,char board[len][wid],byte colored[len][wid]){
	rectangle(sy,sx);
	chtype attr ;
	char prnt;
	int y,x;
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			attr=A_NORMAL;
			if(y==py && x==px)
				attr |= A_STANDOUT;
			if(colored[y][x]>=0)
				attr |= colors[colored[y][x]];
			if( board[y][x] )
				prnt = board[y][x];
			else
				prnt = '_';
			mvaddch(sy+1+y,sx+x*2+1,attr|prnt);
		}
	}
}

byte did_sos(char board[len][wid], int y , int x ){
	byte dy,dx;
	byte soses=0;
	if(board[y][x]== 'S'){
		for(dy=-1;dy<2;++dy){
			for(dx=-1;dx<2;++dx){
				if(rd(board,y+dy,x+dx)=='O' && rd(board,y+2*dy,x+2*dx) == 'S' )
					++soses;
			}
		}
		return soses;
	}
	else if(board[y][x]== 'O'){
		for(dy=-1;dy<2;++dy){
			for(dx=-1;dx<2;++dx){
				if(rd(board,y+dy,x+dx)=='S' && rd(board,y-dy,x-dx) =='S')
					++soses;
			}
		}
		return soses/2;
	}
	return 0;
}
void color_sos(char board[len][wid],byte colored[len][wid], int y , int x ,bool side){
	byte dy,dx;
	if(board[y][x]== 'S'){
		for(dy=-1;dy<2;++dy){
			for(dx=-1;dx<2;++dx){
				if(rd(board,y+dy,x+dx)=='O' && rd(board,y+2*dy,x+2*dx) == 'S' ){
					color(colored,y,x,side);
			       		color(colored,y+dy,x+dx,side);
					color(colored,y+2*dy,x+2*dx,side);	
				}       
			}
		}
	}
	else if(board[y][x]== 'O'){
		for(dy=-1;dy<2;++dy){
			for(dx=-1;dx<2;++dx){
				if(rd(board,y+dy,x+dx)=='S' && rd(board,y-dy,x-dx) =='S'){
					color(colored,y,x,side);
			      		color(colored,y+dy,x+dx,side);
					color(colored,y-dy,x-dx,side);
				}
			}
		}
	}
}
void randmove(int* y,int* x,byte* c){
	*y=rand()%len;
	*x=rand()%wid;
	*c=rand()%2;
}
int decide ( char board[len][wid],byte colored[len][wid], byte depth , byte side ){ //the move is imaginary if side is negative
	int adv,bestadv;
	int oppadv;
	int besty,bestx;
	char bestchar;
	byte c;
	oppadv=adv=bestadv=INT_MIN;
	besty=bestx=-1;
	int y,x;

	int ry,rx;
	byte rc;
	randmove(&ry,&rx,&rc);//provides efficient randomization
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			if(!board[y][x]){
				for(c=0;c<2;++c){
					board[y][x]=so[c];
					adv=did_sos(board,y,x);
					if(depth>0)
						oppadv= decide(board,NULL,depth-1,-1);
					if(depth>0 && oppadv != INT_MIN)//this has no meanings if the opponet cannot move
						adv-=1*oppadv;
					if(besty<0 ||adv>bestadv || (adv==bestadv && y==ry && x==rx &&  c==rc /*c==0*/) ){
						bestadv=adv;
						besty=y;
						bestx=x;
						bestchar=so[c];		
					}
					board[y][x]=0;
				}
			}
		}
	}
	if(besty>=0 && side >= 0 ){
		board[besty][bestx]=bestchar;
		score[side]+= did_sos(board,besty,bestx);
		color_sos(board,colored,besty,bestx,side);
	}
	return bestadv;
}
bool isfilled(char board[len][wid]){
	int y,x;
	for(y=0;y<len;++y)
		for(x=0;x<wid;++x)
			if(!board[y][x])
				return 0;
	return 1;
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(int sy,int sx){
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4-sy <len && minput.x-1-sx<wid*2){
		py=minput.y-4-sy;
		px=(minput.x-1-sx)/2;
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('S');
	if(minput.bstate & (BUTTON2_CLICKED|BUTTON3_CLICKED) )
		ungetch('O');
}
void help(void){
	erase();
	mvprintw(0,0," _  _  _");
	mvprintw(1,0,"(_'| |(_' ");
	mvprintw(2,0,"._):_:._) ");
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(9,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(5,0,"S & O : Write S or O");
	mvprintw(6,0,"q : Quit");
	mvprintw(7,0,"F1 & F2: Help on controls & gameplay");
	mvprintw(8,0,"PgDn,PgUp,<,> : Scroll");
	mvprintw(11,0,"Press a key to continue");
	refresh();
	getch();
	erase();
}
void gameplay(void){
	erase();
	mvprintw(0,0," _  _  _");
	mvprintw(1,0,"(_'| |(_' ");
	mvprintw(2,0,"._):_:._) ");
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("The game is similiar to Tic Tac Toe:\n");
	printw("The players write S and O in the squares\n");
	printw("and making the straight connected sequence\n");
	printw("S-O-S makes you a score; obviously, the\n");
	printw("player with a higher score wins.");
	refresh();
	getch();
	erase();
}
int main(int argc, char** argv){
	int dpt=1;
	signal(SIGINT,sigint_handler);
#ifndef Plan9
	if(argc>4 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s [len wid [AIpower]]\n",argv[0]);
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
		if(len<3 || wid<3 || len>300 || wid>300){
			puts("At least one of your given dimensions is either too small or too big.");
			return EXIT_FAILURE;
		}
	
	}
	else{
		len=5;
		wid=6;
	}
	if(argc==4){
		if( !sscanf(argv[3],"%d",&dpt)){
			puts("Invalid input.");
			return EXIT_FAILURE;
		}
		if( dpt<1 || dpt>= 127){
			puts("That should be between 1 and 127.");
			return EXIT_FAILURE;
		}
	}
#else
	if(argc>2 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s [AIpower]\n",argv[0]);
		return EXIT_FAILURE;
	}

	if(argc==2){
		if( !sscanf(argv[1],"%d",&dpt)){
			puts("Invalid input.");
			return EXIT_FAILURE;
		}
		if( dpt<1 || dpt>= 127){
			puts("That should be between 1 and 127.");
			return EXIT_FAILURE;
		}
	}
	
#endif
	srand(time(NULL)%UINT_MAX);
	int input;		
	initscr();
	mousemask(ALL_MOUSE_EVENTS,NULL);
	curs_set(0);
	noecho();
	cbreak();
	keypad(stdscr,1);
	printw("Black plays first.\n Choose the type of the blue player(H/c)\n" );
	refresh();
	input=getch();
	if(input=='c'){
		computer[0]=dpt;
		printw("Computer.\n");
	}
	else{
		computer[0]=0;
		printw("Human.\n");
	}
	refresh();
	printw("Choose the type of the yellow player(h/C)\n");
	refresh();
	input=getch();
	if(input=='h'){
		computer[1]=0;
		printw("Human.\n");
	}
	else{
		computer[1]=dpt;
		printw("Computer.\n");
	}
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_BLUE,-1);
		init_pair(2,COLOR_YELLOW,-1);
		init_pair(3,COLOR_GREEN,-1);
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}

	}
	int sy,sx;
	Start:
	sy=sx=0;//for scrolling
	py=px=0;
	char board[len][wid];
	byte colored[len][wid];
	bool t=1;
	score[0]=score[1]=0;
	memset(board,0,len*wid);
	memset(colored,-1,len*wid);
	Turn:
	erase();
	mvprintw(sy+0,sx+0," _  _  _");
	mvprintw(sy+1,sx+0,"(_'| |(_'  %d vs %d \n",score[0],score[1]);
	mvprintw(sy+2,sx+0,"._):_:._) \n");
	draw(sy+3,sx+0,board,colored);
	if( isfilled(board) )
		goto End;
	refresh();
	t=!t;
	if(computer[t]){
		mvprintw(sy+len+5,sx+0,"Thinking...");
		refresh();
		decide(board,colored,dpt,t);
		goto Turn;
	}
	//else
	while(1){
		erase();
       		mvprintw(sy+0,sx+0," _  _  _");
		mvprintw(sy+1,sx+0,"(_'| |(_'  %d vs %d \n",score[0],score[1]);
		mvprintw(sy+2,sx+0,"._):_:._) \n");
		draw(sy+3,sx+0,board,colored);
		refresh();
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
		if( input==KEY_F(1) || input=='?')
			help();
		if( input==KEY_F(2) )
			gameplay();
		if( input==KEY_MOUSE )
			mouseinput(sy,sx);
		if( (input=='k' || input==KEY_UP) && py>0)
			--py;
		if( (input=='j' || input==KEY_DOWN) && py<len-1)
			++py;
		if( (input=='h' || input==KEY_LEFT) && px>0)
			--px;
		if( (input=='l' || input==KEY_RIGHT) && px<wid-1)
			++px;
		if( input=='q')
			sigint_handler(0);
		if(!board[py][px] && (input=='s'||input=='S'||input=='o'||input=='O') ){
			if(input=='s'||input=='S')
				board[py][px]='S';
			else
				board[py][px]='O';
			score[t]+=did_sos(board,py,px);
			color_sos(board,colored,py,px,t);
			goto Turn;
		}
	}
	End:
	if( score[1] == score[0])
		mvprintw(sy+len+5,sx+0,"Draw!!");
	else
		mvprintw(sy+len+5,sx+0,"Player %d won the game!",(score[1]>score[0]) +1);
	printw(" Wanna play again?(y/n)");
	curs_set(1);
	flushinp();
	input=getch();
	curs_set(0);
	if(input != 'N' && input != 'n' && input!='q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
