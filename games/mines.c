#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
/* 
|\/|
|  |INES


copyright Hossein Bakhtiarifar 2018 (c)
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
typedef signed char byte;
int len,wid,py,px;
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
void mine(bool mines[len][wid]){
	int y=random()%len;
	int x=random()%wid;
	for(int n=0;n<mscount;n++){
		while(mines[y][x]){
			y=random()%len;
			x=random()%wid;
		}
		mines[y][x]=true;
	}
}

bool click(byte board[len][wid],bool mines[len][wid],int ty,int tx){
	if(board[ty][tx]>=0 && board[ty][tx] <9)
		return 0;
	
	if(board[ty][tx]<0 || board[ty][tx]>8){//untouched
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

	if(!board[ty][tx]){ 
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
			puts("At least one of your given dimensions is too small or too big.");
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
	srandom(time(NULL)%UINT_MAX);		
	Start:
	initscr();
	curs_set(0);
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
	py=px=0;
	untouched=len*wid;
	int flags=0;
	byte board[len][wid];
	bool mines[len][wid];
	char result[70];
	int input;

	memset(board,-1,len*wid);
	memset(mines,false,len*wid);
	mine(mines);
	
	while(1){
		erase();
		mvprintw(1,0,"|\\/|     Flags:%d\n",flags);
		mvprintw(2,0,"|  |INES Mines:%d\n",mscount);
		draw(3,0,board);
		refresh();
		if(untouched<=mscount){
			strcpy(result,"You won!");
			break;
		}
		input = getch();
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
				strcpy(result,"You lost The Game.");
				break;
			}
			click(board,mines,py,px);
		}
		if(input==' '){
			 if(board[py][px] == -1){
				board[py][px]=9;//flag
				flags++;
			 }
			 else if(board[py][px] == 9){
				board[py][px]=10;//unclear
				flags--;
			 }
			 else if(board[py][px] == 10)
				board[py][px]=-1;
		}
	}
	drawmines(3,0,board,mines);
	mvprintw(len+5,0,"%s Wanna play again?(y/n)",result);
	curs_set(1);
	input=getch();
	if(input == 'Y' || input == 'y')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
