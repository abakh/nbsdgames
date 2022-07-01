/* 
|\/|
|  |IKETRON

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"
#define SAVE_TO_NUM 10
#define MINLEN 10
#define MAXLEN 24
#define MINWID 40
#define MAXWID 80

#define FLIGHT_TIME 16
#define NOTRAIL_TIME 30
#define BOMB_RANGE 8
enum {UP=1,RIGHT,DOWN,LEFT,FLIGHT,NOTRAIL,BOMB,SPAWN,STOP,SUPERFOOD,TRAIL};

/* The Plan9 compiler can not handle VLAs and usleep is a POSIX function */
#ifdef NO_VLA 
#define len 10
#define wid 40

#ifdef Plan9
int usleep(long usec) {
    int second = usec/1000000;
    long nano = usec*1000 - second*1000000;
    struct timespec sleepy = {0};
    sleepy.tv_sec = second;
    sleepy.tv_nsec = nano;
    nanosleep(&sleepy, (struct timespec *) NULL);
    return 0;
}
#endif

#else
int len=MINLEN,wid=MINWID;
#endif//NO_VLA

int py,px;
int immunity,flight,notrail;
byte direction;
long score;
chtype colors[6]={0};

byte pse_msg=100;
//no need to a epilepsy variable like in muncher as zeroing the colors suffices

FILE *scorefile;

void move_tron(void){
	switch(direction){
		case UP:
			--py;
		break;
		case DOWN:
			++py;
		break;
		case LEFT:
			--px;
		break;
		case RIGHT:
			++px;
		break;
	}
	if(py==-1)
		py=len-1;
	else if(py==len)
		py=0;
	if(px==-1)
		px=wid-1;
	else if(py==len)
		py=0;
}
void logo(void){
	mvaddstr(1,0,"|\\/|");
	mvaddstr(2,0,"|  |IKETRON");
}
byte save_score(void){
	return fallback_to_home("miketron_scores",score,SAVE_TO_NUM);

}

void show_scores(byte playerrank){
	erase();
	logo();
	if(playerrank==FOPEN_FAIL){
		mvaddstr(3,0,"Couldn't open scorefile.");
		printw("\nHowever, your score is %ld.",score);
		refresh();
		return;
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(score_file);
		fscanf(score_file,"%*s : %*d\n");
		move(3,0);
		byte b=0;
		if ( fscanf(score_file,"%s : %ld\n",formername,&formerscore)==2){
			halfdelay(1);
			printw("*****CONGRATULATIONS!****\n");
			printw("             You beat the\n");
			printw("                 previous\n");
			printw("                   record\n");
			printw("                       of\n");
			printw("           %14ld\n",formerscore);
			printw("                  held by\n");
			printw("              %11s\n",formername);
			printw("               \n");
			printw("               \n");
			printw("*************************\n");
			printw("Press a key to proceed:");
			Effect:
			move(4,0);
			attron(colors[b]);
			mvprintw(4,0, "     _____ ");
			mvprintw(5,0, "   .'     |");
			mvprintw(6,0, " .'       |");
			mvprintw(7,0, " |  .|    |");
			mvprintw(8,0, " |.' |    |");
			mvprintw(9,0, "     |    |");
			mvprintw(10,0,"  ___|    |___");
			mvprintw(11,0," |            |");
			mvprintw(12,0," |____________|");
			attroff(colors[b]);
			b=(b+1)%6;
			if(getch()==ERR)
				goto Effect;
			nocbreak();
			cbreak();
			erase();
			logo();
		}
	}
	//scorefile is still open with w+
	move(3,0);
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);
	printw(">*>*>Top %d<*<*<\n",SAVE_TO_NUM);
	while( rank<SAVE_TO_NUM && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		if(rank == playerrank)
			printw(">>>");
		printw("%d) %s : %ld\n",rank+1,pname,pscore);
		++rank;
	}
	addch('\n');
	refresh();
}
void put_stuff(byte board[len][wid],byte num){
	byte y,x;
	for(byte n=0;n<num;++n){
		do{
			y=rand()%len;
			x=rand()%wid;
		}while((y==py&&x==px)||board[y][x]==TRAIL);
		board[y][x]=STOP;
		if(!(rand()%40))
			board[y][x]=SUPERFOOD;
		else if(!(rand()%20))
			board[y][x]=SPAWN;
		else if(!(rand()%10))
			board[y][x]=BOMB;
		else if(!(rand()%10))
			board[y][x]=NOTRAIL;
		else if(!(rand()%4))
			board[y][x]=FLIGHT;
	}
}
void put_trail(byte board[len][wid],byte num){
	byte y,x;
	for(byte n=0;n<num;++n){
		do{
			y=rand()%len;
			x=rand()%wid;
		}while(y>py-5 && y<py+5 && x>px-10 && x<px+10);
		board[y][x]=TRAIL;
	}
}
void rectangle(void){
	for(int y=0;y<=len;++y){
		mvaddch(3+y,0,ACS_VLINE);
		mvaddch(4+y,1+wid,ACS_VLINE);
	}
	for(int x=0;x<=wid;++x){
		mvaddch(3,x,ACS_HLINE);
		mvaddch(4+len,x,ACS_HLINE);
	}
	mvaddch(3,0,ACS_ULCORNER);
	mvaddch(4+len,0,ACS_LLCORNER);
	mvaddch(3,1+wid,ACS_URCORNER);
	mvaddch(4+len,1+wid,ACS_LRCORNER);
}
void draw(byte board[len][wid]){
	int y,x;
	static byte effect=0;
	chtype prnt;
	rectangle();
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			if(board[y][x]<0){
				prnt=' '|A_STANDOUT|colors[abs(board[y][x])%6];
				board[y][x]++;
			}
			else if(y==py && x==px){
				prnt=ACS_PLUS;
				if(immunity)
					prnt|=colors[effect];
				else if(flight)
					prnt|=A_BOLD;
				else if(!notrail)
					prnt|=A_STANDOUT;
			}
			else if(board[y][x]==TRAIL)
				prnt=ACS_PLUS;
			else if(board[y][x]==NOTRAIL)
				prnt='&';
			else if(board[y][x]==FLIGHT)
				prnt='?';
			else if(board[y][x]==SUPERFOOD)
				prnt='%'|colors[effect];
			else if(board[y][x]==STOP)
				prnt='S';
			else if(board[y][x]==SPAWN)
				prnt='B';
			else if(board[y][x]==BOMB)
				prnt='8';
			else
				prnt= ' ';
			mvaddch(4+y,x+1,prnt);
		}
	}
	if(pse_msg>0){
		mvprintw(len+5,0,"Suffering PSE? Press e.");
		--pse_msg;
	}
	effect=(effect+1)%6;
}
void explode(byte board[len][wid],int by,int bx){
	board[by][bx]=0;//prevent endless recursion
	int sy=by-BOMB_RANGE/2;
	int sx=bx-BOMB_RANGE;
	int ey=by+BOMB_RANGE/2;
	int ex=bx+BOMB_RANGE;
	if(ey>=len)
		ey-=len;
	if(ex>=wid)
		ex-=wid;
	if(sy<0)
		sy+=len;
	if(sx<0)
		sx+=wid;
	int y=sy;
	int x=sx;
	while(y!=ey){
		while(x!=ex){
			++x;
			if(x==wid)
				x=0;
			if(board[y][x]==BOMB)
				explode(board,y,x);
			board[y][x]=-10;
		}
		x=sx;
		++y;
		if(y==len)
			y=0;
		
	}
}
void help(void){
	nocbreak();
	cbreak();
	erase();
	logo();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	attroff(A_BOLD);
	mvprintw(4,0,"hjkl/ARROW KEYS : Change direction");
	mvprintw(5,0,"q : Quit");
	mvprintw(6,0,"F1 & F2: Help on controls & gameplay");
	mvprintw(8,0,"Press a key to continue");
	refresh();
	getch();
	erase();
	halfdelay(1);
}
void gameplay(void){
	nocbreak();
	cbreak();
	erase();
	logo();
	attron(A_BOLD);
	mvprintw(3,0,"     **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("You are controlling a strange vehicle which can \n");
	printw("survive explosions but cannot cross the trail it has\n");
	printw("left behind. Keep it running as much as you can.");
	refresh();
	getch();
	erase();
	halfdelay(1);
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
#ifndef NO_VLA
	signal(SIGINT,sigint_handler);
#endif
	initscr();
#ifndef NO_VLA 
	len=LINES-7;
	if(len<MINLEN){
		len=MINLEN;
	}
	else if(len>MAXLEN){
		len=MAXLEN;
	}

	wid=COLS-5;
	if(wid<MINWID){
		wid=MINWID;
	}
	else if(wid>MAXWID){
		wid=MAXWID;
	}
#endif
	srand(time(NULL)%UINT_MAX);		
	byte board[len][wid];
	bool halfspeed=0;
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);

	if(has_colors()){
		start_color();
		use_default_colors();

		init_pair(1,COLOR_BLUE,-1);
		init_pair(2,COLOR_GREEN,-1);
		init_pair(3,COLOR_YELLOW,-1);
		init_pair(4,COLOR_CYAN,-1);
		init_pair(5,COLOR_MAGENTA,-1);
		init_pair(6,COLOR_RED,-1);
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}

	}
	Start:
	immunity=flight=notrail=0;
	curs_set(0);
	halfdelay(1);
	score=0;
	direction=LEFT;
	py=len/2;
	px=wid/2;
	memset(board,0,len*wid);
	put_stuff(board,20);

	int preinput=0,input=0;
	while(1){
		erase();
		logo();
		mvprintw(1,12,"Score:%ld",score);
		if(immunity)
			mvprintw(2,12,"Immunity:%ld",immunity);
		else if(flight)
			mvprintw(2,12,"Flight:%ld",flight);
		else if(notrail)
			mvprintw(2,12,"NoTrail:%ld",notrail);
		draw(board);
		refresh();
	
		preinput=input;
		input = getch();
		if(input!=ERR)//hide message when a key is entered
			pse_msg=0;

		if(board[py][px]==SPAWN)
			put_stuff(board,5);
		else if(board[py][px]==BOMB){
			explode(board,py,px);
			for(byte b=0;b<10;++b){
				draw(board);
				refresh();
				usleep(100000);
			}
		}
		else if(board[py][px]==STOP){
			mvaddch(4+py,px+1,ACS_PLUS|A_STANDOUT);
			refresh();
			nocbreak();
			cbreak();
			preinput=input;
			input=getch();
			halfdelay(1);
		}	
		else if(board[py][px]==SUPERFOOD)
			immunity+=len+wid;
		else if(board[py][px]==FLIGHT)
			flight+=FLIGHT_TIME;
		else if(board[py][px]==NOTRAIL)
			notrail+=NOTRAIL_TIME;	
		else
			goto NoFeatures;
		board[py][px]=0;//if one of conditions is true, it executes! keep nagging about goto being redundant!

		NoFeatures:
		if(board[py][px]==TRAIL){
			if(immunity)
				board[py][px]=0;
			else if(!flight)
				break;
		}
		if( input == KEY_F(1) || input=='?' )
			help();
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		halfspeed=!halfspeed;
		if( (input=='k' || (input==KEY_UP||input=='w')) ){
			direction=UP;
			halfspeed=1;
		}
		else if( (input=='j' || (input==KEY_DOWN||input=='s')) ){
			direction=DOWN;
			halfspeed=1;
		}
		else if( (input=='h' || (input==KEY_LEFT||input=='a')) )
			direction=LEFT;
		else if( (input=='l' || (input==KEY_RIGHT||input=='d')) )
			direction=RIGHT;
		if( (input=='q'||input==27))
			sigint_handler(0);
		if(input=='e'){
			for(int b=0;b<6;++b){
				colors[b]=0;
			}
			pse_msg=0;
		}
		if(input!=ERR){
			if(preinput==input){//if it wasn't there, hitting two keys in less than 0.1 sec would not work
				usleep(100000);
				flushinp();
			}
		}
		
		if( !((direction==UP||direction==DOWN)&&!halfspeed) && !immunity && !flight && !notrail)
			board[py][px]=TRAIL;

		if(direction==UP && halfspeed){
			--py;
			if(py==-1)
				py=len-1;
			halfspeed=1;
		}
		else if(direction==DOWN && halfspeed){
			++py;
			if(py==len)
				py=0;
		}
		else if(direction==LEFT){
			--px;
			if(px==-1)
				px=wid-1;
		}
		else if(direction==RIGHT){
			++px;
			if(px==wid)
				px=0;
		}
		++score;
		if(!(score%100)){
			put_stuff(board,5);
			put_trail(board,20);
		}
		if(immunity)
			--immunity;
		else if(flight)
			--flight;
		else if(notrail)
			--notrail;
	}
	nocbreak();
	cbreak();
	draw(board);
	refresh();
	mvprintw(len+5,0,"Game over! Press a key to see the high scores:");
	getch();
	show_scores(save_score());
	printw("Game over! Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if( input!= 'N' &&  input!= 'n' && input!='q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
