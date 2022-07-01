/* 
 _  
|_)
|_)ATTLESHIP

Authored by abakh <abakh@tuta.io> 
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 

*/
#include "common.h"
#define MISS -2
#define SEA -1 
#define HIT 0
#define NOTHING -1
#define ALL 0x7c
#define RED 3
#define CYAN 2
#define ENGLISH_LETTERS 26
typedef unsigned char bitbox;

bool multiplayer;
byte py,px;//cursor

chtype colors[4]={0};

byte game[2][10][10];//main board
bool computer[2] = {0};
byte score[2] = {0};//set by header()
bitbox sunk[2]={0};
byte just_sunk[2]={0};//to be displayed for human players

byte firstinrowy , firstinrowx ;
byte lastinrowy ,lastinrowx;
byte goindirection;
byte shotinvain;
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(bool ingame){
#ifndef NO_MOUSE
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if(minput.bstate & (BUTTON1_CLICKED|BUTTON1_RELEASED)){
		if( minput.y-4 < 10){
			if( (ingame && minput.x-23<20 && minput.x-23>=0 ) || (!ingame && minput.x-1<20) ){//it most be on the trackboard if ingame is true
				py=minput.y-4;
				px=(minput.x-1-(ingame*2)) /2;
			}
		}
		else
			return;
	}
	if(minput.bstate & (BUTTON1_CLICKED|BUTTON1_RELEASED))
		ungetch('\n');
	if(minput.bstate & (BUTTON2_CLICKED|BUTTON2_RELEASED|BUTTON3_CLICKED|BUTTON3_RELEASED) )
		ungetch('r');
#endif
}
void rectangle(byte sy,byte sx){
	for(byte y=0;y<=10+1;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+10*2,ACS_VLINE);
	}
	for(byte x=0;x<=10*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+10+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+10+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+10*2,ACS_URCORNER);
	mvaddch(sy+10+1,sx+10*2,ACS_LRCORNER);
}
void print_type(byte type){
	switch(type){
		case(2):
			addstr("patrol boat");
			break;
		case(3):
			addstr("destroyer");
			break;
		case(4):
			addstr("battleship");
			break;
		case(5):
			addstr("carrier");
			break;
		case(6):
			addstr("submarine");
			break;
	}
}
void MID(byte *y , byte *x, byte direction){
	switch(direction){
		case(0):
			*x=*x-1;
			break;
		case(1):
			*y=*y-1;
			break;
		case(2):
			*x=*x+1;
			break;
		case(3):
			*y=*y+1;
			break;
	}	
}
void genocide(bool side , byte type){
	byte y,x;
	for(y=0;y<10;++y){
		for(x=0;x<10;++x){
			if(game[side][y][x] == type)
				game[side][y][x] = SEA;
		}
	}
}
void header(bool side){
	score[0]=score[1]=0;
	byte y,x;
	for(y=0;y<10;++y){
		for(x=0;x<10;++x){
			if(game[!side][y][x] == HIT)
					score[side]++;
			if(game[side][y][x] == HIT)
					score[!side]++;
		}
	}
	mvaddch(0,1,  '_');
	mvprintw(1,0,"|_) %2d:%2d",score[side],score[!side]);
	mvprintw(2,0,"|_)ATTLESHIP ");
	if(multiplayer){
		attron(colors[side]);
		if(side)
			printw("Yellow's turn");
		else
			printw("Green's turn");
		attroff(colors[side]);
	}
}

void draw(bool side,byte sy,byte sx,bool regular){//the game's board 
	rectangle(sy,sx);
	chtype ch ;
	byte y,x;
	for(y=0;y<10;++y){
		for(x=0;x<10;++x){
			ch =A_NORMAL;
			if(y==py && x==px)
				ch |= A_STANDOUT;
			if(game[side][y][x] == HIT)
				ch |= 'X'|colors[RED];
			else if(game[side][y][x] > 0 && !(multiplayer&&regular) )
				ch |= ACS_BLOCK|colors[side];
			else if(game[side][y][x]== MISS)
				ch |= 'O'|colors[CYAN];
			else if(!(multiplayer&&regular))
				ch |= '~'|colors[CYAN];
			else
				ch |=' ';

			mvaddch(sy+1+y,sx+x*2+1,ch);
		}
	}
}
void draw_trackboard(bool side,byte sy,byte sx){
	rectangle(sy,sx);
	chtype ch ;
	byte y,x;
	for(y=0;y<10;++y){
		for(x=0;x<10;++x){
			ch =A_NORMAL;
			if(y==py && x==px-10)
				ch |= A_STANDOUT;

			if(game[!side][y][x] == HIT)
				ch |= '*'|colors[RED];
			else if(game[!side][y][x]== MISS)
				ch |= '~'|colors[CYAN];
			else
				ch |= '.';

			mvaddch(sy+1+y,sx+x*2+1,ch);
		}
	}
	refresh();	
}

void autoset(bool side){
	byte y=0,x=0,direction=0, invain=0;
	byte realy,realx;
	byte l;
	for(byte type=2;type<7;++type){
		SetLocation:
		realy=rand()%10;
		realx=rand()%10;
		invain=0;
		SetDirection:
		y=realy;
		x=realx;
		direction=rand()%4;
		for(l=0;(type != 6 && l<type) || (type==6 && l<3) ; ++l){//there are two kinds of ship sized 3 tiles
			if( y<0 || x<0 || y>=10 || x>=10 || game[side][y][x] != SEA ){
				genocide(side,type);
				++invain;
				direction= (direction+1)%4;
				if(invain<4)
					goto SetDirection;
				else
					goto SetLocation;//endless loop
			}
			else{
				game[side][y][x]=type;
				MID(&y,&x,direction);
			}
		}
	}
}

void set_the_board(bool side){
	if( computer[side] ){
		autoset(side);
		return;
	}
	erase();
	mvaddch(0,1,  '_');
	mvaddstr(1,0,"|_) Set your board");
	mvaddstr(2,0,"|_)ATTLESHIP");
	mvaddstr(16,0,"Press RETURN to specify the location and press R to rotate the ship.");
	int input;
	byte y=0,x=0,direction=0, invain=0;
	byte realy,realx;
	byte l;
	py=px=0;
	for(byte type=2;type<7;++type){
		mvaddstr(15,0,"Put your ");
		print_type(type);
		addstr(" in its position:    ");
		SetLocation:
		while(1){
			draw(side,3,0,false);
			refresh();
			input = getch();
			if( input == KEY_MOUSE )
				mouseinput(0);
			if( (input=='k' || (input==KEY_UP||input=='w')) && py>0)
				--py;
	  	      	if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<9)
				++py;
	     		if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>0)
				--px;
			if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<9)
				++px;
			if( input=='\n'||input==KEY_ENTER )
				break;
			if( (input=='q'||input==27) )
				sigint_handler(EXIT_SUCCESS);
		}


		realy=y=py;
		realx=x=px;
		invain=0;
		SetDirection:
		y=realy;
		x=realx;
		for(l=0;(type != 6 && l<type) || (type==6 && l<3) ; ++l){//there are two kinds of ship sized 3 tiles
			if( y<0 || x<0 || y>=10 || x>=10 || game[side][y][x] != SEA ){
				genocide(side,type);
				++invain;
				direction= (direction+1)%4;
				if(invain<4)
					goto SetDirection;
				else
					goto SetLocation;//endless loop
			}
			else{
				game[side][y][x]=type;
				MID(&y,&x,direction);
			}
		}
		while(1){
			invain=0;
			draw(side,3,0,false);
			input=getch();
			if( input== 'r' || input == 'R' ){
				genocide(side,type);
				direction= (direction+1)%4;
				goto SetDirection;
			}
			else if(input == KEY_MOUSE)
				mouseinput(0);
			else
				break;
		}
	}
}

void turn_shift(void){
	if(!multiplayer)
		return;
	char key = 'a'+(rand()%ENGLISH_LETTERS);
	int input1,input2,input3;
	input1=input2=input3=0;
	erase();
	beep();
	mvaddch(0,1,  '_');
	mvaddstr(1,0,"|_) Anti-cheater");
	mvaddstr(2,0,"|_)ATTLESHIP");
	mvaddstr(4,0,"********************");
	mvprintw(5,0," Type '%c' 3 times  ",key);
	mvaddstr(6,0,"       before ");
	mvaddstr(7,0,"      proceeding   ");
	mvaddstr(8,0,"     to the  game   ");
	mvaddstr(10,0,"********************");
	refresh();
	while(1){
		input3=input2;
		input2=input1;
		input1=getch();
		if( (input1==input2) && (input2==input3) && (input3==key) )
			break;
	}
	erase();
}
byte shoot(bool turn, byte y , byte x){
	if( y<0 || x<0 || y>9 || x>9 ){ //didn't shoot at all
		return NOTHING;
	}
	byte s = game[!turn][y][x];
	if(s==HIT || s==MISS)
		return NOTHING;
	if(s>0){
		game[!turn][y][x]=HIT;
		return 1;
	}
	else{
		game[!turn][y][x]=MISS;
		return 0;
	}

}
void sink_announce(bool side){
	byte type,y,x;
	for(type=2;type<7;++type){
		for(y=0;y<10;++y){
			for(x=0;x<10;++x){
				if( game[!side][y][x] == type )
					goto Next;
			}
		}
		//there is no instance of 'type' in the opponent's board
		if( ( (1 << type) | sunk[!side] ) != sunk[!side] ){//if it is not yet announced as sunk
			sunk[!side] |= (1 << type);
			if(computer[side]){
				lastinrowy=lastinrowx=firstinrowy=firstinrowx=-1;
				shotinvain=0;
			}
			else{
				just_sunk[!side]=type;//leave to be displayed by you_sunk
			}
			return;
		}


		Next:
		continue;
	}
}
void you_sunk(bool side){
	if( just_sunk[!side] == 3)
		mvaddstr(15,0,"You have destroyed my destroyer!!");
	else if( just_sunk[!side]){
		mvaddstr(15,0,"You have sunk my ");
		print_type(just_sunk[!side]);
		addstr("!!");
	}
	just_sunk[!side]=0;
}
void cheat(bool side){
	/* its actually an anti-cheat, the player can place all their ships adjacent to one another and in the same direction,
	and the algorithm will often play in a way that it will be left with one or two isolated tiles being unshot (with their respective ships being shot before).
	in a such a situation a human will *very easily* find the tiles with logical thinking, but the computer shoots randomly and it will take such a long time for it
	that it will often lose the winning game.

	this function still doesn't make a win,it's randomly executed.
		
	if i implemented the logical thinking thing, it would become a difficult, unenjoyable game.*/
	byte y,x;
	for(y=0;y<10;++y){
		for(x=0;x<10;++x){
			if(game[!side][y][x]>0){
				shoot(side,y,x);
				firstinrowy=y;
				firstinrowx=x;	
				return;
			}
		}
	}
}
void decide(bool side){// sink_announce is responsible for unsetting the global variables involved
	byte y,x,r;
	Again:
	if( firstinrowy == NOTHING ){
		if( score[side] > 14 && score[side]<score[!side] && rand()%2 ){
			cheat(side);
			return;
		}
		while(1){
			y = rand()%10;
			x = rand()%10;
			r = shoot(side,y,x);
			if(r == 1){
				firstinrowy=y;
				firstinrowx=x;
			}	
			if(r != NOTHING)
				return;
		}
	}
	else if( lastinrowy ==NOTHING ){
		if(goindirection == NOTHING)
			goindirection = rand()%4;
		while(1){
			y= firstinrowy;//we know there is hit already
			x= firstinrowx;
			MID(&y,&x,goindirection);
			r= shoot(side,y,x);
			if( r != 1 ){ 
				goindirection = (goindirection+1)%4;//the ship is oriented in another way then
				++shotinvain;

				if(shotinvain==4){ // this only occurs in case of a ship being shot before but not sunk ( e.g. in exprimenting for the direction)
					shotinvain=0;
					y=firstinrowy;
					x=firstinrowx;
					goindirection = (goindirection+1)%4;
					while(game[!side][y][x]==HIT){//go till you reach an unshot tile
						MID(&y,&x,goindirection);
						if( (y<0 || x<0 || y>9 || x>9) && r==NOTHING){
							goto Again;
						}
					}
					r= shoot(side,y,x);//(y,x) may be MISS, but its impossible for it to be empty water, as executing this means it has tested every direction before 
					if(r==1){
						lastinrowy=y;//continue from the imaginary firstinrow
						lastinrowx=x;
					}
					if(r==NOTHING)
						goto Again;
				}

			}
			else{
				lastinrowy= y;
				lastinrowx= x;
			}

			if( r != NOTHING )
				return;
		}
	}
	else{
		y=lastinrowy;
		x=lastinrowx;
		MID(&y,&x,goindirection);
		r=shoot(side,y,x);
		if( r == 1 ){
			lastinrowy=y;
			lastinrowx=x;
		}
		else{
			lastinrowy=lastinrowx=NOTHING;
			goindirection=(goindirection+2)%4;
		}
		if( r != NOTHING )
			return;
		else{
			goto Again;
		}	
	}
}
void help(bool side){//side is only there to feed header()
	erase();
	header(side);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(9,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"RETURN/ENTER : Shoot");
	mvprintw(5,0,"R : Rotate");
	mvprintw(6,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(7,0,"q : Quit");
	mvprintw(8,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(11,0,"Press a key to continue");
	getch();
	erase();
}
void gameplay(bool side){//side is only there to feed header()
	erase();
	header(side);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("Guess the location of your opponent's\n");
	printw("ships and sink them! The player\n");
	printw("who sinks all the opponent's ships wins.");
	getch();
	erase();
}
int main(int argc,char** argv){
	if(argc>1){
		printf("This game doesn't take arguments");
	}
	initscr();
#ifndef NO_MOUSE
	mousemask(ALL_MOUSE_EVENTS,NULL);
#endif
	curs_set(0);
	noecho();
	cbreak();
	keypad(stdscr,1);
	if( has_colors() ){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_GREEN,-1);
		init_pair(2,COLOR_YELLOW,-1);
		init_pair(3,COLOR_CYAN,-1);
		init_pair(4,COLOR_RED,-1);
		for(byte b=0;b<4;++b)
			colors[b]=COLOR_PAIR(b+1);
	}
	int input;
	printw("Choose type of the game:\n");
	printw("1 : Single Player*\n");
	printw("2 : Multi Player\n");
	refresh();
	input=getch();
	if(input == '2'){
		multiplayer=1;
		computer[1]=computer[0]=0;
	}
	else{
		multiplayer=0;
		computer[1]=1;
		computer[0]=0;
	}
	Start:
	firstinrowy=firstinrowx=lastinrowy=lastinrowx=goindirection=NOTHING;
	shotinvain=0;
	sunk[0]=sunk[1]=0;
	memset(game,SEA,200);
	srand(time(NULL)%UINT_MAX);
	erase();

	set_the_board(0);	
	turn_shift();
	set_the_board(1);
	bool won;
	bool turn=1;
	Turn:
	px=10;
	py=0;
	sink_announce(turn);
	if( sunk[0]==ALL ){
		won=1;
		goto End;
	}
	else if( sunk[1]==ALL ){
		won=0;
		goto End;
	}
	//the turn starts HERE
	turn=!turn;
	//turn_shift();
	if( computer[turn] ){
		decide(turn);
		goto Turn;
	}
	else{
		erase();
		you_sunk(turn);
		while(1){
			header(turn);
			draw(turn,3,0,true);
			draw_trackboard(turn,3,22);
			refresh();
			input=getch();
			if(input == KEY_F(1) || input=='?' )
				help(turn);
			if((input==KEY_F(2)||input=='!') )
				gameplay(turn);
			if(input == KEY_MOUSE)
				mouseinput(1);
			if( (input=='k' || (input==KEY_UP||input=='w')) && py>0)
				--py;
			if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<9)
				++py;
			if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>10)
				--px;
			if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<19)
				++px;
			if( (input=='q'||input==27))
				sigint_handler(EXIT_SUCCESS);
			if( input=='\n' || input==KEY_ENTER){
				byte r=shoot(turn,py,px-10);
				if(r != NOTHING){
					goto Turn;
				}
			}	
		}
	}
	End:
	erase();
	header(won);
	draw(won,3,0,false);
	draw_trackboard(won,3,22);
	if( computer[won] )
		mvaddstr(15,0,"Hahaha! I won! ");
	else
		mvprintw(15,0,"Player %d won the game.",won+1);
	addstr(" Wanna play again? (y/n)");
	refresh();
	curs_set(1);
	input=getch();
	if( input!='n' &&  input !='N' && input!='q' ){
		curs_set(0);
		goto Start;
	}
	endwin();
	return 0;
}
