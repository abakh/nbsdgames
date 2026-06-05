/*
 _        
(_
 _)CISSOR
Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "common.h"
#define SAVE_TO_NUM 11
#define LEN 24
#define WID 80
#define HLEN (LEN/2)
#define HWID (WID/2)
#define ITEMS_COUNT 26
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
enum {ROCK=0,PAPER,SCISSOR,MAGIC};
chtype colors[3]={0};
long score=0;
FILE* scorefile;

int input;
typedef struct item{
	byte type;
	float y,x;
	float angle;
	float vy,vx;
	bool player;
	struct item *last_collision;//so they don't stick together
}item;

item items[ITEMS_COUNT];


char msg[150]={0};
byte msg_show=0;

void filled_rect(byte sy,byte sx,byte ey,byte ex){
	byte y,x;
	for(y=sy;y<ey;++y)
		for(x=sx;x<ex;++x)
			mvaddch(y,x,' ');
}
void magenta_border(void){
	byte y,x;
	for(y=0;y<LEN;++y){
		mvaddch(y,WID-1,' '|A_STANDOUT|colors[2]);
		mvaddch(y,0,' '|A_STANDOUT|colors[2]);
	}
	for(x=0;x<WID;++x){
		mvaddch(LEN-1,x,' '|A_STANDOUT|colors[2]);
		mvaddch(0,x,' '|A_STANDOUT|colors[2]);
	}
		
}
void add_random_angle(item *a){
	a->angle+=((rand()%200)-100)*0.01;
	a->vy=sin(a->angle)/2;
	a->vx=cos(a->angle);
}

void fill_items(){
	item *a=NULL;
	for(byte i=0;i<ITEMS_COUNT;++i){
		a=items+i;
		/*a->y=(float)(LEN*i/ITEMS_COUNT);
		a->x=(float)(WID*i/ITEMS_COUNT);*/
		a->y=rand()%LEN;
		a->x=rand()%WID;
		a->angle=(rand()%4)*(6.28/4);
		a->vy=sin(a->angle);
		a->vx=cos(a->angle);
		a->type=i%3;
		if(i<1){
			a->type=MAGIC;
		}
		a->player=0;
		a->last_collision=NULL;
	}
}
void move_item(item *a){
	bool bounce;
	bounce=0;
	//bounce when hitting the borders, and don't get stuck there
	if( a->x<0 || (int)a->x>=WID-1 || ((int)a->x==13 && a->y<=7 ) ){
		a->angle =M_PI- a->angle;
		a->vy=sin(a->angle);
		a->vx=cos(a->angle);
		bounce=1;
	}
	if( a->y <0 || (int)a->y >= LEN-1 || (a->x<=13 && (int)a->y==7)){
		a->angle =0- a->angle;
		a->vy=sin(a->angle);
		a->vx=cos(a->angle);
		bounce=1;
	}

	if(a->x<0){//these are for getting unstuck
		a->x=1;
	}
	if(a->y<0){
		a->y=1;
	}
	if(a->x>=WID){
		a->x=WID-1;
	}	
	if(a->y>=LEN){
		a->y=LEN-1;
	}
	if((int)a->x==13 && a->y<7){
		a->x=14;
	}
	if(a->x<=13 && (int)a->y==7){
		a->y=8;
	}
	while(a->angle<0){//preventing overflow
		a->angle +=M_PI*2;
	}
	
	//move
	a->x+=a->vx;
	a->y+=a->vy;


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

}
void swap_items(item *a, item *b){
	item s= *a;
	*a=*b;
	*b=s;
}
void sort_items(){
	byte pos=0;//sort for y
	while(pos<ITEMS_COUNT){
		if(pos==0 || items[pos].y > items[pos-1].y || ((int) items[pos].y == (int) items[pos-1].y) && (items[pos].x > items[pos-1].x)){
			pos+=1;
		}
		else{
			swap_items(&items[pos],&items[pos-1]);
		}
	}
}
void guide_item(item *a, int input){
		if( (input=='k' || (input==KEY_UP||input=='w'))){
			a->vy=-1;
			a->vx=0;
		}
		if( (input=='j' || (input==KEY_DOWN||input=='s'))){
			a->vy=1;
			a->vx=0;
		}
		if( (input=='h' || (input==KEY_LEFT||input=='a'))){
			a->vy=0;
			a->vx=-1;
		}
		if( (input=='l' || (input==KEY_RIGHT||input=='d'))){
			a->vy=0;
			a->vx=1;
		}
		if( input=='y'){//intended behavior, moves faster diagonally. legit "cheating"
			a->vy=-1;
			a->vx=-1;
		}
		if( input=='u'){
			a->vy=-1;
			a->vx=1;
		}
		if(input=='b'){
			a->vy=1;
			a->vx=-1;
		}
		if(input=='n'){
			a->vy=1;
			a->vx=1;
		}
}
void collide(item *a, item *b){
	if(a==b){
		printf("RIIIIDII");
	}
	if(a->last_collision==b){
		return;
	}
	if(a->player || b->player){
		if(a->type!=b->type){
			score+=47;
		}
	}
	float svy=a->vy;
	float svx=a->vx;
	float sangle=a->angle;
	a->vy=b->vy;
	a->vx=b->vx;
	a->angle=b->angle;
	b->vy=svy;
	b->vx=svx;
	b->angle=sangle;
	add_random_angle(a);
	add_random_angle(b);
	a->last_collision=b;
	b->last_collision=a;
	if(a->type==MAGIC){
		if(b->type==SCISSOR){
			b->type=PAPER;
			b->player=0;
		}
		else if(b->type==ROCK){
			b->type=SCISSOR;
		}
		else if(b->type==PAPER){
			b->type=ROCK;
		}
	}
	else if(a->type==ROCK){
		if(b->type==SCISSOR){
			b->type=ROCK;
			b->player=0;
		}
		else if(b->type==PAPER){
			a->type=PAPER;
		}
	}
	else if(a->type==PAPER){
		if(b->type==ROCK){
			b->type=PAPER;
		}
		else if(b->type==SCISSOR){
			a->type=SCISSOR;
		}
	}
	else if(a->type==SCISSOR){
		if(b->type==PAPER){
			b->type=SCISSOR;
		}
		else if(b->type==ROCK){
			a->type=ROCK;
			a->player=0;
		}
	}
}
void collisions(){
	item *a,*b;
	for(byte i=0;i<ITEMS_COUNT;++i){
		a=&items[i];
		b=a->last_collision;
		/*if(b && fabs(a->y-b->y)<3 && fabs(a->x-b->x)<3){
			a->last_collision=NULL;
		}*/
		for(byte j=i+1;j<ITEMS_COUNT;++j){
			b=&items[j];
			if(fabs(a->y-b->y)<1.5 && fabs(a->x-b->x)<1.5){
				collide(a,b);
			}
		}
		
	}

}
void mechanics(){
	collisions();
	static byte slow_motion=0;
	slow_motion=(slow_motion+1)%1;
	for(byte i=0;i<ITEMS_COUNT;++i){
		if(slow_motion==0 || items[i].player){
			move_item(&items[i]);
		}
	}
	collisions();
}
void star_line(byte y){
	for(byte x=1;x<WID-1;++x)
		mvaddch(y,x,'.');
}
void draw_item(item a){
	chtype shape;
	switch(a.type){
		case ROCK:
			shape='O'|colors[0];
		break;
		case PAPER:
			shape='#'|A_BOLD;
		break;
		case SCISSOR:
			shape='>'|colors[1];
		break;
		case MAGIC:
			shape='?'|A_BOLD|colors[2];
		break;
		default:
			printf("ASDAFARHGRHETHTHDHDFG");
	}
	if(a.player){
		shape|=A_STANDOUT;
	}
	mvaddch((int) a.y,(int)a.x,shape);
}
void find_scissor(){
	item *a=NULL;
	item *make_player=NULL;
	for(byte i=0;i<ITEMS_COUNT;++i){
		a=items+i;
		if(a->type==SCISSOR){
			make_player=a;
		}
		if(a->player){
			return;
		}
	}
	if(make_player){
		make_player->player=1;
	}
}
void logo(){
	mvaddstr(0,0,"         ");
	mvaddstr(1,0," _      ");
	mvaddstr(2,0,"(_'      ");
        mvaddstr(3,0,"._)CISSOR");
}

void draw(){
	logo();
	mvprintw(5,0,"Score: %ld",score);
	for(byte i=0;i<ITEMS_COUNT;++i){
		draw_item(items[i]);
	}
	if(msg_show){
		--msg_show;
		mvaddstr(LEN-1,0,msg);
	}
}

byte save_score(void){
	return fallback_to_home("scissor_scores",score,SAVE_TO_NUM);

}

void show_scores(byte playerrank){
	filled_rect(0,0,LEN,WID);
	magenta_border();
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
			magenta_border();
		}

	}
	//scorefile is still open with w+
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);	
	mvaddstr(1,WID/2-4,"HIGH SCORES");
	while( rank<SAVE_TO_NUM && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		star_line(2+2*rank);
		move(2+2*rank,1);
		if(rank == playerrank)
			printw(">>>");
		printw("%s",pname);
		mvprintw(2+2*rank,WID-1-digit_count(pscore),"%ld",pscore);
		++rank;
	}
	refresh();
}
void help(void){
	nocbreak();
	cbreak();
	filled_rect(0,0,LEN,WID);
	magenta_border();
	mvaddstr(1,WID/2-4,"GAMEPLAY");
	mvprintw(3,1,"This is rock-paper-scissor game evolved to a");
	mvprintw(4,1,"super-hyper-great live action strategy game.");
	mvprintw(5,1,"You have extra lives as long as there are scissors");
	mvprintw(6,1,"on screen. Hit and dodge rocks and papers and manage");
	mvprintw(7,1,"their populations. The more you hit, the more you win.");
	mvprintw(8,1,"Patience and vigilance could lead to very high scores.");
	refresh();
	getch();
	halfdelay(1);
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
int avoid_accidental_pass(){
	int input;
	Again:
	input=getch();
	if( input==ERR){
		goto Again;
	}
	if( (input=='k' || (input==KEY_UP||input=='w'))){
		goto Again;
	}
	else if( (input=='j' || (input==KEY_DOWN||input=='s')) ){
		goto Again;
	}
	else if( (input=='h' || (input==KEY_LEFT||input=='a'))){
		goto Again;
	}
	else if( (input=='l' || (input==KEY_RIGHT||input=='d'))){
		goto Again;
	}
	return input;

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
		init_pair(3,COLOR_MAGENTA,COLOR_BLACK);
		for(byte b=0;b<3;++b)
			colors[b]=COLOR_PAIR(b+1);
	}
	
	Start:
	erase();
	halfdelay(1);
	curs_set(0);
	score=0;
	msg_show=0;
	fill_items();
	byte found_player=0;
	while(1){
		erase();
		draw();
		refresh();
		input=getch();

		if(input=='?' || input==KEY_F(1))
			help();
		if(input=='p'){
			nocbreak();
			cbreak();
			erase();
			logo();
			attron(A_BOLD);
			addstr("\n  PAUSED");
			attroff(A_BOLD);
			refresh();

			getch();

			halfdelay(1);
	
		}
		if(input=='q'){
			break;
		}
		find_scissor();
		found_player=0;
		for(int i=0;i<ITEMS_COUNT;i++){
			if((items+i)->type==SCISSOR && !(rand()%2)){
				guide_item(items+i,input);
			}
			if((items+i)->player){
				guide_item((items+i),input);
				found_player=1;
				break;
			}
		}
		if(!found_player){
			break;
		}
		if(input==27){
			break;
		}
		if(input!=ERR){
			usleep(100000);
			flushinp();
		}
		mechanics();
	}

	flushinp();
	nocbreak();
	cbreak();
	draw();
	refresh();
	move(LEN-1,0);
	printw("You have lost The Game. Press a key to contintue.");
	refresh();
	avoid_accidental_pass();
	curs_set(1);
	show_scores(save_score());
	attron(colors[2]|A_STANDOUT);
	mvprintw(LEN-1,HWID-11,"Wanna play again? (y/n)");
	attroff(colors[2]|A_STANDOUT);
	input=avoid_accidental_pass();
	if(input!='q' && input!='n' && input!='N')
		goto Start;
	endwin();
	return 0;
}
