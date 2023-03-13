/*
 _        
| '.      
|  :      
|.' ARRT

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "common.h"
#define SAVE_TO_NUM 11
#define LEN 24
#define HLEN LEN/2
#define WID 80
#define HWID WID/2
#define SHOTS_WHEN_STARTING 10

#define randint(a,b) ((a)+(rand()%((b+1)-(a))))
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

chtype colors[3]={0};
long score=0;
FILE* scorefile;

chtype background[LEN][WID];

int input;
typedef struct aim{
	char sign;
	float y,x;
	float angle;
	float v;
	byte brake;
	bool visible;
}aim;

aim aims[26];
aim landed_aims[SHOTS_WHEN_STARTING];//so an aim couldn't pass below one that has already landed, doesn't make sense visually.

byte shots,aims_to_stop;

char msg[150]={0};
byte msg_show=0;
void filled_rect(byte sy,byte sx,byte ey,byte ex){
	byte y,x;
	for(y=sy;y<ey;++y)
		for(x=sx;x<ex;++x)
			mvaddch(y,x,' ');
}
void red_border(void){
	byte y,x;
	for(y=0;y<LEN;++y){
		mvaddch(y,WID-1,' '|A_STANDOUT|colors[0]);
		mvaddch(y,0,' '|A_STANDOUT|colors[0]);
	}
	for(x=0;x<WID;++x){
		mvaddch(LEN-1,x,' '|A_STANDOUT|colors[0]);
		mvaddch(0,x,' '|A_STANDOUT|colors[0]);
	}
		
}

void fill_aims(){
	for(byte i=0;i<26;++i){
		aims[i].y= randint(8,HLEN);
		aims[i].x= randint(0,HWID);
		aims[i].angle=randint(0,628)/100;
		aims[i].v=1;
		aims[i].sign='A'+i;
		aims[i].brake=0;
		aims[i].visible=1;
	}
}

float center_distance(byte y,byte x){
	//y distance is twice accounted for. visual reasons
	return sqrt( (y-HLEN)*(y-HLEN)+0.25*(x-HWID)*(x-HWID) );
}
long calculate_points(aim *a){
	int distance=center_distance((byte)a->y,(byte)a->x);
	long points;
	if(distance>HLEN){
		points=-2*pow(2,distance-HLEN);
	}
	else if((byte) a->y == HLEN && (byte) a->x == HWID){
		points=1000000;
	}
	else{
		points=pow(2,HLEN-distance);
	}
	return points;
}
void aim_lands(aim *a){
	landed_aims[SHOTS_WHEN_STARTING-aims_to_stop]=*a;
	--aims_to_stop;
	score+=calculate_points(a);
	a->visible=0;

	float distance= center_distance((byte)a->y,(byte)a->x);
	if((byte)a->y==HLEN && (byte)a->x==HWID){
		strcpy(msg,"Bravo!");
	}
	else if(distance<2){
		strcpy(msg,"Very close...");
	}
	else{
		goto NoMessage;
	}
	msg_show=30;
	NoMessage: return;
}
void move_aim(aim *a){
	if(a->brake==1){
		return;
	}
	else if(a->brake>0){
		--a->brake;
	}
	bool bounce;
	bounce=0;

	//bounce when hitting the borders, and don't get stuck there
	if(a->x<0 || (int)a->x>=WID-1 || ((int)a->x==13 && a->y<=7 ) ){
		a->angle =M_PI- a->angle;
		bounce=1;
	}
	if(a->y <0 || (int)a->y >= LEN-1 || (a->x<=13 && (int)a->y==7)){
		a->angle =0- a->angle;
		bounce=1;
	}
	if(a->x<0)//these are for getting unstuck
		a->x=1;
	if(a->y<0)
		a->y=1;
	if(a->x>=WID)
		a->x=WID-1;
	if(a->y>=LEN)
		a->y=LEN-1;

	if((int)a->x==13 && a->y<7) 
		a->x=14;

	if(a->x<=13 && (int)a->y==7) 
		a->y=8;

	while(a->angle<0){//preventing overflow
		a->angle +=M_PI*2;
	}
	
	//move
	a->x+=cos(a->angle)*a->v;
	a->y+=sin(a->angle)*a->v;


	if(bounce && a->x>=WID-1)//getting unstuck
		a->x=WID-1;
	if(bounce && a->y>=LEN-1)
		a->y=LEN-1;
	
	if(bounce){//bounce in a slightly different direction than it should be
		a->angle +=randint(-1,1)*0.1;
	}
	if(a->x<13 && a->y<7){// don't go into the logo area
		if(13 - a->x < 7 - a->y){
			a->y=8;
		}
		else{
			a->x=14;
		}
	}

	if(a->brake==1){//the aim has just been stopped
		aim_lands(a);
	}

}
void star_line(byte y){
	for(byte x=1;x<WID-1;++x)
		mvaddch(y,x,'.');
}
void make_background(){
	float d;
	for(byte y=0;y<LEN;++y){
		for(byte x=0;x<WID;++x){
			d=center_distance(y,x)/(HLEN/4);
			if(d<4){
				if( ((int)d) %2){
					background[y][x]='#';
				}
				else{
					background[y][x]='$'|colors[0];
				}
			}
			else{
				background[y][x]=' ';
			}
		}
	}
}
void draw_aim(aim a){
	if(!a.visible)
		return;

	chtype color;
	if(a.brake)
		color=colors[2]|A_BOLD;
	else
		color=colors[2];
	mvaddch((int) a.y,(int)a.x,a.sign|color);
}
void logo(){
	mvaddstr(0,0," _        ");
	mvaddstr(1,0,"| '.      ");
	mvaddstr(2,0,"|  :      ");
        mvaddstr(3,0,"|.' ARRT  ");
}
void draw(){
	for(byte y=0;y<LEN;++y){
		for(byte x=0;x<WID;++x){
			mvaddch(y,x,background[y][x]);
		}
	}

	logo();
	mvprintw(5,0,"Score: %d",score);
	mvprintw(6,0,"Shots: %d",shots);
	for(byte i=0;i<SHOTS_WHEN_STARTING-aims_to_stop;++i){
		draw_aim(landed_aims[i]);
	}
	for(byte i=0;i<26;++i){
		draw_aim(aims[i]);
	}
	if(msg_show){
		--msg_show;
		mvaddstr(LEN-1,0,msg);
	}
}

void end_scene(){
	for(byte y=0;y<LEN;++y){
		for(byte x=0;x<WID;++x){
			mvaddch(y,x,background[y][x]);
		}
	}

	logo();
	mvprintw(5,0,"Score: %d",score);
	for(byte i=0;i<SHOTS_WHEN_STARTING-aims_to_stop;++i){
		draw_aim(landed_aims[i]);
	}
	mvaddstr(LEN-1,0,"Press any key to continue:");
	getch();
	refresh();
}

byte save_score(void){
	return fallback_to_home("darrt_scores",score,SAVE_TO_NUM);

}

void show_scores(byte playerrank){
	attron(colors[3]);
	filled_rect(0,0,LEN,WID);
	red_border();
	if(playerrank==FOPEN_FAIL){
		mvaddstr(1,0,"Could not open score file.");
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
			red_border();
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
	red_border();
	mvprintw(1,HWID-4,"GAME PLAY");
	mvprintw(3,1,"If you hit a letter on keyboard, the letter on the");
	mvprintw(4,1,"screen will soon stop. You have to aim for the");
	mvprintw(5,1,"center of the target using the moving letters.");
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
		init_pair(1,COLOR_RED,COLOR_BLACK);
		init_pair(2,COLOR_YELLOW,COLOR_BLACK);
		init_pair(3,COLOR_GREEN,COLOR_BLACK);
		for(byte b=0;b<3;++b)
			colors[b]=COLOR_PAIR(b+1);
	}
	
	make_background();
	Start:
	erase();
	halfdelay(1);
	curs_set(0);
	score=0;
	msg_show=0;
	aims_to_stop=shots=SHOTS_WHEN_STARTING;
	fill_aims();
	while(1){
		draw();
		refresh();
		input=getch();

		if(input=='?' || input==KEY_F(1))
			help();
		if(input>='a' && input<='z'){
			input=input-'a'+'A';
		}
		if(input>='A' && input<='Z' && shots){
			if(!aims[input-'A'].brake){
				aims[input-'A'].brake=15;
				--shots;
			}
		}
		if(input=='Q'){
			strcpy(msg,"ESC or Ctrl-C to quit.");
			msg_show=50;
		}
		if(input==27){
			break;
		}
		if(input!=ERR){
			usleep(100000);
			flushinp();
		}
		if(!aims_to_stop){
			break;
		}
		for(int i=0;i<26;++i){
			move_aim(aims+i);
		}
	}
	flushinp();
	nocbreak();
	cbreak();
	curs_set(1);
	end_scene();
	show_scores(save_score());
	attron(colors[0]|A_STANDOUT);
	mvprintw(LEN-1,HWID-11,"Wanna play again? (y/n)");
	attroff(colors[0]|A_STANDOUT);
	do{
		input=getch();
	}while((input==KEY_UP||input=='w') || (input==KEY_DOWN||input=='s'));
	if(input!='q' && input!='n' && input!='N')
		goto Start;
	endwin();
	return 0;
}
