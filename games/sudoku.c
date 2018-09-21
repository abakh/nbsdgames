#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  //to seed random
#include <limits.h>
#include <signal.h>
/*
 _
(_
 _)UDOKU 


copyright Hossein Bakhtiarifar 2018 (c)
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
typedef unsigned char byte;
byte size,s,ds;//s=size*size
byte py,px;
byte diff;
unsigned int filled;
chtype colors[6]={0};
void cross(byte sy,byte sx,chtype start,chtype middle,chtype end){ //to simplify drawing tables
		mvaddch(sy,sx,start);
		byte f = 2*size;
		for(char n=1;n<size;n++){
			mvaddch(sy,sx+f,middle);
			f+=2*size;
		}
		mvaddch(sy,sx+f,end);
}

void table(byte sy,byte sx){ //empty table
		byte l;//line
		for(l=0;l<=size;l++){
			for(byte y=0;y<=ds;y++)
				mvaddch(sy+y,sx+l*size*2,ACS_VLINE);
			for(byte x=0;x<=s*2;x++)
				mvaddch(sy+(size+1)*l,sx+x,ACS_HLINE);
		}
		cross(sy,sx,ACS_ULCORNER,ACS_TTEE,ACS_URCORNER);
		for(l=1;l<size;l++)
			cross(sy+l*size+l,sx,ACS_LTEE,ACS_PLUS,ACS_RTEE);
		cross(sy+l*size+l,sx,ACS_LLCORNER,ACS_BTEE,ACS_LRCORNER);
}

byte sgn2int(char sgn){
	if('0'<sgn && sgn <='9')
		return sgn-'0';
	if('a'<=sgn && sgn <='z')
		return sgn-'a'+10;
	if('A'<=sgn && sgn <= 'Z')
		return sgn-'A'+36;
        return 0;
}

char int2sgn(byte num){//integer to representing sign
	if(0< num && num <= 9)
		return num+'0';
	else if(10<=num && num <=35)
		return num-10+'a';
	else if(36<=num && num <=51)
		return num-36+'A';
	return 0;
}

bool isvalid(byte ty,byte tx,char board[s][s]){ //is it legal to place that char there?
	char t= board[ty][tx];
	if(!t)
		return 0;
	byte y,x;
	for(y=0;y<s;y++){
		if(board[y][tx] == t && y!=ty)
			return 0;
	}
	for(x=0;x<s;x++){
		if(board[ty][x] == t && x!= tx)
			return 0;
	}
	byte sy=size*(ty/size);//square
	byte sx=size*(tx/size);
	for(y=0;y<size;y++){
		for(x=0;x<size;x++){
			if(board[sy+y][sx+x]==t && sy+y != ty && sx+x != tx)
				return 0;
		}
	}				
	return 1;
}

void genocide(char board[s][s],char victim){
	for(byte y=0;y<s;y++)
		for(byte x=0;x<s;x++)
			if(board[y][x]==victim)
				board[y][x]=0;
}
bool fill_with(char board[s][s],char fillwith){//returns 1 on failure
	byte firstx,x,tries=0;
	Again:
	tries++;
	if (tries>s)
		return 1;
	for(byte y=0;y<s;y++){
		firstx=x=random()%s;
		while(1){
			if(!board[y][x]){
				board[y][x]=fillwith;
				if(isvalid(y,x,board)){
					break;
				}
				else{
					board[y][x]=0;
					goto Next;
				}
			}
			else{
				Next:
				x++;
				if(x==s)
					x=0;
				if(x==firstx){
					genocide(board,fillwith);
					goto Again;
				}
			}
		}
	}
	refresh();
	return 0;
}
void fill(char board[s][s]){
	for(byte num=1;num<=s;num++){
		if ( fill_with(board,int2sgn(num) ) ){
			memset(board,0,s*s);
			num=0;
		}
	}
}
void mkpuzzle(char board[s][s],char empty[s][s],char game[s][s]){//makes a puzzle to solve
	for(byte y=0;y<s;y++){
		for(byte x=0;x<s;x++){
			if( !(random()%diff) ){
				empty[y][x]=board[y][x];
				game[y][x]=board[y][x];
			}
		}
	}
}

void header(byte sy,byte sx){
	mvaddch(sy, sx+1, '_');
	mvprintw(sy+1,sx,"(_       Solved:%d/%d",filled,s*s);
	mvprintw(sy+2,sx," _)UDOKU Left  :%d/%d",s*s-filled,s*s);
}
	
void draw(byte sy,byte sx,char empty[s][s],char board[s][s]){
	chtype attr;
	table(sy,sx);
	filled=0;
	for(byte y=0;y<s;y++){
		for(byte x=0;x<s;x++){
			attr=A_NORMAL;
			if(x==px && y==py)
				attr |= A_STANDOUT;
			if(empty[y][x])
				attr |= A_BOLD;
			if(board[y][x]){
				if(!isvalid(y,x,board))
					attr |= colors[5];
				else{
					attr |= colors[board[y][x]%5];
					filled++;
				}
				mvaddch(sy+y+y/size+1,sx+x*2+1,attr|board[y][x]);
			}
			else
				mvaddch(sy+y+y/size+1,sx+x*2+1,attr|' ');
		}
	}
}

void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
	
int main(int argc,char** argv){
	signal(SIGINT,sigint_handler);
	if(argc>3 || (argc==2 && !strcmp("help",argv[1])) ){
		printf("Usage: %s [size [ diff]]\n",argv[0]);
		return EXIT_FAILURE;
	}

	if(argc>1 ){
		if(strlen(argv[1])>1 || argv[1][0]-'0'>7 || argv[1][0]-'0'< 2){ 
			printf("2 <= size <= 7\n");
			return EXIT_FAILURE;
		}
		else
			size = *argv[1]-'0';
	}	
	else
		size=3;
	if(argc>2){ 
		if (strlen(argv[2])>1 || argv[2][0]-'0'>4 || argv[2][0]-'0'<= 0 ){
			printf("1 <= diff <=4\n");
			return EXIT_FAILURE;
		}
		else
			diff = *argv[2]-'0'+1;
	}
	else
		diff=2;



	Start:
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	curs_set(0);
	srandom(time(NULL)%UINT_MAX);
	if( has_colors() ){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_YELLOW,-1);
		init_pair(2,COLOR_GREEN,-1);
		init_pair(3,COLOR_BLUE,-1);
		init_pair(4,COLOR_CYAN,-1);
		init_pair(5,COLOR_MAGENTA,-1);
		init_pair(6,COLOR_RED,-1);
		for(byte b=0;b<6;b++){
			colors[b]=COLOR_PAIR(b+1);
		}
	}
	filled =0;
	s=size*size;
	ds=s+size;
	char board[s][s];
	char empty[s][s]; 
	char game[s][s];
	memset(board,0,s*s);
	memset(empty,0,s*s);
	memset(game,0,s*s);
	int input=0 ;
	fill(board);
	mkpuzzle(board,empty,game);
	py=px=0;

	while(1){
		erase();
		draw(3,0,empty,game);
		header(0,0);
		refresh();
		if(filled == s*s)
			break;
		input = getch();
		if(input == KEY_UP && py)
			py--;
		if(input == KEY_DOWN && py<s-1)
			py++;
		if(input == KEY_LEFT && px)
			px--;
		if(input == KEY_RIGHT && px<s-1)
			px++;
		if(!empty[py][px]){
			if(input == ' ' )
				game[py][px]=0;
			else if(input<=CHAR_MAX && sgn2int(input) && sgn2int(input)<=s )
				game[py][px]=input;
		}
		if(input == 'q' && size<5)
			sigint_handler(EXIT_SUCCESS);
		if(input == 'x' && getch()=='y' && getch()=='z' && getch()=='z' && getch()=='y')
			game[py][px]=board[py][px];
	}
	mvprintw(ds+4,0,"YAY!! Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if(input == 'Y' || input == 'y')
			goto Start;
	endwin();
	return EXIT_SUCCESS;
}
