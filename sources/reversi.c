/* 
 _  
|_)
| \EVERSI

Authored by abakh <abakh@tuta.io>
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.

compile with -lncurses
*/
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
typedef signed char byte;

byte py,px;//cursor
const char piece[2] = {'O','X'};
char game[8][8];//main board
byte computer[2] = {0,0};
byte score[2];//set by header()

void rectangle(byte sy,byte sx){
	for(byte y=0;y<=8+1;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+8*2,ACS_VLINE);
	}
	for(byte x=0;x<=8*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+8+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+8+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+8*2,ACS_URCORNER);
	mvaddch(sy+8+1,sx+8*2,ACS_LRCORNER);
}

void header(void){//abuse, used to count the pieces on each side too
	score[0]=score[1]=0;
	for(byte y=0;y<8;++y){
		for(byte x=0;x<8;++x){
			if(game[y][x]){
				if(game[y][x]==piece[0])
					score[0]++;
				else
					score[1]++;
			}
		}
	}
	mvaddch(0,1,  '_');
	mvprintw(1,0,"|_) %2d:%2d",score[0],score[1]);
	mvprintw(2,0,"| \\EVERSI ");
}

void draw(byte sy,byte sx){//the game's board 
	rectangle(sy,sx);
	chtype attr ;
	for(byte y=0;y<8;++y){
		for(byte x=0;x<8;++x){
			attr=A_NORMAL;
			if(y==py && x==px)
				attr |= A_STANDOUT;
			if(game[y][x])
				mvaddch(sy+1+y,sx+x*2+1,attr|game[y][x]);
			else				
				mvaddch(sy+1+y,sx+x*2+1,attr|'.');
		}
	}
}

bool can_reverse(byte ty , byte tx,char board[8][8],char piece){//can place a piece there?
	byte y,x,count;
	if(board[ty][tx])
		return false;
	for(byte dy=-1;dy<2;++dy){ //changes the direction
		for(byte dx=-1;dx<2;++dx){
			if(dx==0&&dy==0)//it would be itself
				dx=1;
			count=0;
			y=ty+dy;
			x=tx+dx;
			while(1){
				if(y<0 || y>=8 ||x<0 || x>=8){//reaches edges of the board
					count=0;
					break;
				}
				if(!board[y][x]){//gap
					count=0;
					break;
				}

				if(board[y][x]!=piece){
					++count;
					y+=dy;
					x+=dx;
				}
				else
					break;//same color
			}
			if(count)
				return true;
		}
	}
	return false;
}

void reverse(byte ty,byte tx,char board[8][8],char piece){//place a piece there
	board[ty][tx]=piece;
	byte y,x;
	for(byte dy=-1;dy<2;++dy){//changes the direction
		for(byte dx=-1;dx<2;++dx){
			if(dy==0 && dx==0)
				dx=1;
			y=ty+dy;
			x=tx+dx;
			while(1){
				if(y<0 || y>=8 || x<0 || x>=8)
					break;
				if(!board[y][x])
					break;
				if(board[y][x]!=piece){
					y+=dy;
					x+=dx;
				}
				else{ //of same kind
					while(y!=ty || x!=tx){ //reverse the disks
						board[y][x]=piece;
						y-=dy;
						x-=dx;
					}
					break;
				}
			}
		}
	}
}

bool can_move(char board[8][8],char piece){//can move at all?
	for(byte y=0;y<8;++y)
		for(byte x=0;x<8;++x)
			if(can_reverse(y,x,board,piece))
				return true;
	return false;
}

double advantage(char board[8][8],char piece){
	double own=0;
	double opp=0;
	for(byte y=0;y<8;++y){
		for(byte x=0;x<8;++x){
			if(board[y][x]){
				if(board[y][x]==piece){
					++own;
					if( ((y==7 || y==0)&&(x!=7 && x!=0)) || ((x==7 || x==0)&&(y!=7 && y!=0)) )//edges
						own+=100;
					if( (y==7 || y==0)&&(x==7 || x==0) )//corners
						own+=10000;
				}
				else{
					++opp;
					if( ((y==7 || y==0)&&(x!=7 && x!=0)) || ((x==7 || x==0)&&(y!=7 && y!=0)) )
						opp+=100;
					if( (y==7 || y==0)&&(x==7 || x==0) )
						opp+=10000;
				}
			}
			
		}
	}
	return own/opp;
}

void cp(char A[8][8],char B[8][8]){//copy the board A to B
	for(byte y=0;y<8;++y)
		for(byte x=0;x<8;++x)
			B[y][x]=A[y][x];
}

double decide(char board[8][8],char piece,char opponet,byte depth){//AI algorithm
	if(!can_move(board,piece))
		return 0;
	char plan[8][8];
	double adv,bestadv;
	adv=bestadv=0;
	byte besty,bestx;
	for(byte y=0;y<8;++y){
		for(byte x=0;x<8;++x){
			if(can_reverse(y,x,board,piece) ){
				cp(board,plan);//backtrack
				reverse(y,x,plan,piece);
				if(depth){
					adv= decide(plan,opponet,piece,depth-1);//least benefit for the opponet
					if(adv) //the opponet can make a move
						adv = 1/adv;
					else
						adv=advantage(plan,piece);
				}
				else
					adv=advantage(plan,piece);
				if(adv>bestadv){
					bestadv=adv;
					besty=y;
					bestx=x;
				}
			}
		}
	}
	reverse(besty,bestx,board,piece);//do the move
	return bestadv;
}
//peacefully close when ^C is pressed
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(void){
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4 <8 && minput.x-1<16){
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
	header();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(8,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"RETURN/ENTER : Put the piece");
	mvprintw(5,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(6,0,"q : Quit");
	mvprintw(7,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(10,0,"Press a key to continue");
	curs_set(1);
	getch();
}
void gameplay(void){
	erase();
	header();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("Players take turns placing disks on the board:\n\n");
	printw("1) Any pieces of the opponet's color that is bounded\n");
	printw("   in a straight line between the piece just placed and\n");
	printw("   another piece of the current player's color would turn\n");
	printw("   to the current player's color.\n\n");
	printw("2) You can only put pieces if at least one of your \n");
	printw("   opponet's pieces turns into your color.\n\n");
	printw("3) The game ends when neither side can do a move and\n");
	printw("   the player with more pieces wins.\n");
	getch();
}
int main(int argc , char** argv){
	int depth=2;
	if(argc>2){
		printf("Usage:%s [AIpower]",argv[0]);
		return EXIT_FAILURE;
	}
	if(argc==2){
		if(sscanf(argv[1],"%d",&depth) && depth<128 && 0<depth)
			;//already done in sscanf
		else{
			printf("That should be a number from 1 to 127.\n");
			return EXIT_FAILURE;
		}
	} 
	signal(SIGINT,sigint_handler);
	initscr();
	mousemask(ALL_MOUSE_EVENTS,NULL);
	noecho();
	cbreak();
	keypad(stdscr,1);
	int input;
	printw("Black plays first:\n");
	printw("Choose type of the white player (H/c)\n");
	refresh();
	input=getch();
	if(input == 'c'){
		computer[0]=depth;
		printw("Computer.\n");
	}
	else{
		computer[1]=0;
		printw("Human.\n");
	}
	refresh();
	printw("Choose type of the black player(h/C)\n");
	refresh();
	input=getch();
	if(input == 'h'){
		computer[1]=0;
		printw("Human.\n");
	}
	else{
		computer[1]=depth;
		printw("Computer.\n");
	}
	Start: 
	curs_set(0);
	py=px=0;
	memset(game,0,64);
	bool turn=0;
	bool resign=0;
	byte cantmove=0;
	game[3][3]=piece[0];
	game[4][4]=piece[0];
	game[3][4]=piece[1];
	game[4][3]=piece[1];

	Turn:
	erase();
	flushinp();
	draw(3,0);
	header();
	refresh();
	if(cantmove >=2)//both sides cant move, the game ends
		goto End;

	turn = !turn;
	if(computer[turn]){
		if(can_move(game,piece[turn])){
			mvprintw(13,0,"Thinking...");
			refresh();
			decide(game,piece[turn],piece[!turn],computer[turn]);
			cantmove=0;
		}
		else
			++cantmove;
		goto Turn;
			
	}
	
	if(!can_move(game,piece[turn])){
		++cantmove;
		goto Turn;
	}
	else{
		cantmove=0;
		while(1){ //human control
			erase();
			draw(3,0);
			header();
			if(!(computer[0]||computer[1]))
				mvprintw(0,5,"%c's turn",piece[turn]);
			refresh();
			input=getch();
			if( input==KEY_F(1) || input=='?' )
				help();
			if( input==KEY_F(2) )
				gameplay();
			if( input==KEY_MOUSE )
				mouseinput();
			if( (input=='k' || input==KEY_UP) && py>0)
				--py;
			if( (input=='j' || input==KEY_DOWN) && py<7)
				++py;
			if( (input=='h' || input==KEY_LEFT) && px>0)
				--px;
			if( (input=='l' || input==KEY_RIGHT) && px<7)
				++px;
			if( input=='q'){
				resign=1;
				goto End;
			}
			if(input=='\n'){
				if(can_reverse(py,px,game,piece[turn])){
					reverse(py,px,game,piece[turn]);
					goto Turn;
				}
				
			}
		
		}
	}
	End:
	if(resign)
		mvprintw(13,0,"You resigned.");
	else if(score[0]==score[1])
		mvprintw(13,0,"Draw!!");
	else if(score[0] > score[1])
		mvprintw(13,0,"'%c' won.",piece[0]);
	else
		mvprintw(13,0,"'%c' won.",piece[1]);
		
	printw(" Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if( resign){
		if (input=='Y' || input=='y') 
			goto Start;
	}
	else if(input != 'N' && input != 'n' && input != 'q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
