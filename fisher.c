/*
    O__/|
 ___|_/ |    __
|     / |   |__
	    |  ISHER

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"
#define SAVE_TO_NUM 11
#define HOOKS 10
#define LEN 24
#define HLEN LEN/2
#define WID 80
#define HWID WID/2
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
// 12 lines of water
// 80 columns

chtype colors[4]={A_NORMAL,A_STANDOUT};
byte fish[10]={0};//positions
byte caught=-1;
bool stop[10]={0};
unsigned int count[10]={0};
unsigned long score=0;
const char sym[]="~:=!@+><;&";
byte hook=0, hooknum=0;
byte clb,clbtime=0;

int input;
void filled_rect(byte sy,byte sx,byte ey,byte ex){
	byte y,x;
	for(y=sy;y<ey;++y)
		for(x=sx;x<ex;++x)
			mvaddch(y,x,' ');
}
void green_border(void){
	byte y,x;
	for(y=0;y<LEN;++y){
		mvaddch(y,WID-1,' '|colors[2]);
		mvaddch(y,0,' '|colors[2]);
	}
	for(x=0;x<WID;++x){
		mvaddch(LEN-1,x,' '|colors[2]);
		mvaddch(0,x,' '|colors[2]);
	}
		
}
void star_line(byte y){
	for(byte x=1;x<WID-1;++x)
		mvaddch(y,x,'.');
}
void draw(void){
	/*while(LEN< 15 || COL<80)
		mvprintw(0,0,"Screen size should at least be 80*15 characters");*/
	attron(colors[0]);
	filled_rect(0,0,12,80);
	byte y;
	mvprintw(0,0," __       Hooks:%d",hooknum);
	mvprintw(1,0,"|__       Score:%d",score);
	mvprintw(2,0,"|  ISHER");
	mvprintw(9,32, "    O__/");
	mvprintw(10,32," ___|_/ ");
	mvprintw(11,32,"|     / ");
	
	if(clbtime){
		if(count[clb]!=1){
			mvprintw(9,43,"%d ",count[clb]);
			switch(clb){
				case 0:
					addstr("plastic bags!");
				break;
				case 1:
					addstr("PVC bottles!");
				break;
				case 2:
					addstr("face masks!");
				break;
				case 3:
					addstr("shrimp!");
				break;
				case 4:
					addstr("algae!");
				break;
				case 5:
					addstr("jellyfish!");
				break;
				case 6:
					addstr("molluscs!");
				break;
				case 7:
					addstr("actual fish!");
				break;
				case 8:
					addstr("salmon!");
				break;
				case 9:
					addstr("tuna!");
				break;
			}
		}
		else{
			move(9,43);
			switch(clb){
				case 0:
					addstr("A plastic bag!");
				break;
				case 1:
					addstr("A PVC bottle!");
				break;
				case 2:
					addstr("A face mask!");
				break;
				case 3:
					addstr("A shrimp!");
				break;
				case 4:
					addstr("Algae!");
				break;
				case 5:
					addstr("A jellyfish!");
				break;
				case 6:
					addstr("A mollusc!");
				break;
				case 7:
					addstr("Actual fish!");
				break;
				case 8:
					addstr("A salmon!");
				break;
				case 9:
					addstr("A tuna!");
				break;
			}
		}
	}
	for(y=-3;y<0;++y)
		mvaddch(HLEN+y,HWID,ACS_VLINE);
	attroff(colors[0]);
	attron(colors[1]);
	filled_rect(HLEN,0,LEN,WID);
	for(y=0;y<hook;++y)
		mvaddch(HLEN+y,HWID,ACS_VLINE);
	if(caught==-1)
		mvaddch(HLEN+hook,HWID,')');
	else
		mvaddch(HLEN+hook,HWID,sym[caught]);
	for(y=0;y<10;++y)
		mvaddch(HLEN+1+y,fish[y],sym[y]);
	attroff(colors[1]);
	
}
byte save_score(void){
	return fallback_to_home("fisher_scores",score,SAVE_TO_NUM);

}


void show_scores(byte playerrank){
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	green_border();
	if(playerrank==FOPEN_FAIL){
		mvaddstr(1,0,"Could not open score file");
		mvprintw(2,0,"However, your score is %ld.",score);
		refresh();
		return;
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(score_file);
		fscanf(score_file,"%*s : %*d");
		if ( fscanf(score_file,"%s : %ld",formername,&formerscore)==2  && formerscore>0){
			byte a = (LEN-9)/2;
			star_line(1);
			star_line(LEN-2);
			mvaddstr(1,WID/2-8,"CONGRATULATIONS!!");
			mvprintw(a+1,HWID-10,"     _____You beat the");
			mvprintw(a+2,HWID-10,"   .'     |   previous");
			mvprintw(a+3,HWID-10," .'       |     record");
			mvprintw(a+4,HWID-10," |  .|    |         of");
			mvprintw(a+5,HWID-10," |.' |    |%11ld",formerscore);
			mvprintw(a+6,HWID-10,"     |    |    held by");
			mvprintw(a+7,HWID-10,"  ___|    |___%7s!",formername);
			mvprintw(a+8,HWID-10," |            |");
			mvprintw(a+9,HWID-10," |____________|");
			mvprintw(LEN-3,HWID-11,"Press a key to continue");
			refresh();
			do{
				input=getch();
			}while((input==KEY_UP||input=='w') || (input==KEY_DOWN||input=='s'));
			filled_rect(0,0,LEN,WID);
			green_border();
		}

	}
	//scorefile is still open with w+
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);	
	mvaddstr(1,WID/2-4,"HIGH SCORES");
	attron(colors[3]);
	while( rank<SAVE_TO_NUM && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		star_line(2+2*rank);
		move(2+2*rank,1);
		if(rank == playerrank)
			printw(">>>");
		printw("%s",pname);
		mvprintw(2+2*rank,WID-1-digit_count(pscore),"%d",pscore);
		++rank;
	}
	attroff(colors[3]);
	refresh();
}
void help(void){
	nocbreak();
	cbreak();
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	green_border();
	mvprintw(1,HWID-4,"GAME PLAY");
	mvprintw(3,1,"Catch a fish and reel it in for points");
	mvprintw(4,1,"The deeper the fish, the more points it is worth.");
	mvprintw(5,1,"If a fish hits your line, you lose a hook.");
	mvprintw(6,1,"When you run out of hooks, the game is over!");
	mvprintw(8,HWID-4,"CONTROLS");
	mvprintw(10,1,"UP & DOWN: Control the hook");
	mvprintw(11,1,"q: Quit");
	mvprintw(13,1,"This is a port of \"Deep Sea Fisher\", a MikeOS game.");
	attroff(colors[3]);
	refresh();
	getch();
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
	signal(SIGINT,sigint_handler);
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	srand(time(NULL)%UINT_MAX);
	for(byte n=0;n<10;++n)
		fish[n]=rand()%80;
	if(has_colors()){
		start_color();
		init_pair(1,COLOR_BLACK,COLOR_CYAN);
		init_pair(2,COLOR_BLACK,COLOR_BLUE);
		init_pair(3,COLOR_WHITE,COLOR_GREEN);
		init_pair(4,COLOR_BLACK,COLOR_WHITE);
		for(byte b=0;b<4;++b)
			colors[b]=COLOR_PAIR(b+1);
	}
	byte n;
	Start:
	halfdelay(1);
	curs_set(0);
	clbtime=0;
	hook=0;
	hooknum=HOOKS;
	score=0;
	memset(count,0,10*sizeof(unsigned int) );
	while(1){
		draw();
		refresh();
		input=getch();
		for(n=0;n<10;++n){
			if(stop[n]){
				if(rand()%(n+15)==0)//this is to make the fish move
					stop[n]=0;
				continue;
			}
			if(n%2)
				fish[n]--;
			else
				fish[n]++;
			if(fish[n]<0)
				fish[n]=79;
			if(fish[n]>79)
				fish[n]=0;//appear on the other end
			if(fish[n]==40){
				if(hook>n+1){
					caught= -1;
					hook=0;
					--hooknum;
				}
				if(hook==n+1 && caught==-1){
					caught=n;
					if(n%2)
						fish[n]=79;
					else
						fish[n]=0;
				}
			}
			if(rand()%(14-n)==0)//this is to make it stop
				stop[n]=1;
		}
		if((input==KEY_UP||input=='w')){
			if(hook>0)
				--hook;
			if(hook==0 && caught!=-1){
				count[caught]++;
				score+=(caught+1)*(caught+1);
				clb=caught;
				clbtime=10;//celebrate catching the fish
				caught=-1;
			}
		}
		if((input==KEY_DOWN||input=='s')){
			if(hook<11)
				++hook;
			if(fish[hook-1]==40 && caught==-1){
				caught=hook-1;
				if(n%2)
					fish[hook-1]=0;
				else
					fish[hook-1]=79;
			}
		}
		if(input=='?' || input==KEY_F(1))
			help();
		if((input=='q'||input==27))
			break;
		if(!hooknum)
			break;	
		if(input!=ERR){
			usleep(100000);
			flushinp();
		}
	}
	flushinp();
	nocbreak();
	cbreak();
	curs_set(1);
	show_scores(save_score());
	attron(colors[2]);
	mvprintw(LEN-1,HWID-11,"Wanna play again? (y/n)");
	attroff(colors[2]);
	do{
		input=getch();
	}while((input==KEY_UP||input=='w') || (input==KEY_DOWN||input=='s'));
	if(input!='q' && input!='n' && input!='N')
		goto Start;
	endwin();
	return 0;
}
