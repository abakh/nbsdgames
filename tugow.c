/*
_____   _
  |   .' '.  :   :
  |   :   :  : . :
  |UG '._.'F '.'.'AR


Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"
#define SAVE_TO_NUM 11
#define HOOKS 10
#define LEN 24
#define WID 80
#define HWID 40
#define DUDES_WID 32
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

char dudes[]=
	" O                           O \n"
	"/|\\                         /|\\\n"
	"\\--\\-----------------------/--/\n" 
	"/ \\                         / \\\n"
	"\\  \\                       /  /\n"
	"\f"
        "                             O\n"
	" O                          /| \\\n"
	"/|\\                        / | /\n"
	"\\--\\----------------------/---/ \n"
	"/ \\                         / \\\n"
	"\\  \\                       /   \\\n" 
	"                          /    /\n"
	"\f"
        "                              O\n"
        "                             /|\\\n"
	" O                          / | \\\n"
	"/|\\                        /  | /\n"
	"\\--\\----------------------/----/\n"
	"/ \\                          / \\\n"
	"\\  \\                        /   \\\n" 
	"                           /    /\n"
        "                          /    /\n"
        "\f"
        "                             O\n"
        "                            /| \\\n"
        "                           / |  \\\n"
	" O                        /  |  /\n"
	"/|\\                      /   | /\n"
	"\\--\\--------------------/-----/\n" 
	"/ \\                         / \\\n"
	"\\  \\                       /   \\\n" 
	"                          /     \\\n"
        "                         /      /\n"
        "                        /      / \n"
;



char logo[]=
	"_____   _           \n"  
	"  |   .' '.  :   :  \n"
	"  |   :   :  : . :  \n"
	"  |UG '._.'F '.'.'AR\n"
;


char choose_from[]="7894561230";
char type_str[10]={0};
byte offset=0;
byte level=0;
chtype colors[4]={A_NORMAL,A_STANDOUT};
unsigned long score=0;
int input;
void filled_rect(byte sy,byte sx,byte ey,byte ex){
	byte y,x;
	for(y=sy;y<ey;++y)
		for(x=sx;x<ex;++x)
			mvaddch(y,x,' ');
}
void blue_border(void){
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
void draw_sprite(byte sy, byte sx, char* str, byte frame){
	byte f=0;
	int index=0;
	while(f!=frame){
		if(str[index]=='\f'){
			++f;
		}
		if(str[index]=='\0'){
			break;
		}
		++index;
		if(f==frame){
			break;
		}
	}
	byte y=sy;
	byte x=sx+1;
	for(;str[index]!='\0'&&str[index]!='\f';++index){
		if(str[index]=='\n'){
			x=sx;
			++y;
		}
		else if(str[index]!=' '){
			mvaddch(y,x,str[index]);
		}
		++x;
	}


}
void draw(void){
	byte i,j;
	for(i=0;i<LEN;++i){
		mvaddch(i,HWID,ACS_VLINE);
	}
	mvprintw(15+level,offset-10,"Type!");
	mvprintw(16+level,offset-10,"%s",type_str);
	mvprintw(11+level,offset-10,"Score: %d",score);
	mvprintw(12+level,offset-10,"Level: %d",level);
	draw_sprite(0,0,logo,0);
	draw_sprite(12,offset,dudes,level%4);
}
byte save_score(void){
	return fallback_to_home("tugow_scores",score,SAVE_TO_NUM);
}


void show_scores(byte playerrank){
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	blue_border();
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
			blue_border();
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
	blue_border();
	mvprintw(1,HWID-4,"GAME PLAY");
	mvprintw(3,1,"Type those things and beat the other guy");
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
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_BLACK,COLOR_CYAN);
		init_pair(2,COLOR_BLACK,COLOR_BLUE);
		init_pair(3,COLOR_WHITE,COLOR_BLUE);
		init_pair(4,COLOR_BLUE,COLOR_WHITE);
		for(byte b=0;b<4;++b)
			colors[b]=COLOR_PAIR(b+1);
	}
	byte t;
	byte threshold;
	for(byte i=0;i<sizeof(type_str)-1;++i){
		type_str[i]=choose_from[rand()%(sizeof(choose_from)-1)];
	}
	Start:
	t=0;
	threshold=20/(1<<level);
	offset=(WID-DUDES_WID)/2;
	halfdelay(1);
	curs_set(0);
	while(1){
		erase();
		draw();
		refresh();
		
		input=getch();
		if(input==type_str[0]){
			for(byte i=0;i<sizeof(type_str)-2 ;++i){
				type_str[i]=type_str[i+1];
			}
			type_str[sizeof(type_str)-2]=choose_from[rand()%(sizeof(choose_from)-1)];	
			--offset;
			score+=(1<<level)*(1<<level)*(1<<level);
		}
		if(input!=ERR){
			flushinp();
			usleep(1e5);
		}
		++t;
		if(t>threshold){
			t=0;
			++offset;
		}
		if(offset>HWID-3){
			break;
		}
		if(offset<HWID-DUDES_WID+4){
			++level;
			goto Start;
		}
		if(input=='?' || input==KEY_F(1))
			help();
		
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
	if(input!='q' && input!='n' && input!='N'){
		score=0;
		level=0;
		goto Start;
	}
	endwin();
	return 0;
}
