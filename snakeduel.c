/* 
 _       _
(_      | :  
 _)NAKE |.'UEL


Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"
#define SAVE_TO_NUM 10
#define MINLEN 10
#define MAXLEN 127
#define MINWID 40
#define MAXWID 127
#define LOSE -(MAXWID*MAXLEN)
#define WIN_LIMIT 5
//#define REPORT 0 
#ifdef REPORT
	#define reportif(x) if(x){fprintf(lol,#x" is true\n");fflush(lol);}
	#define reportd(x) if(REPORT){fprintf(lol, #x": %ld\n",(long)x);fflush(lol);}
	#define reports(x) if(REPORT){fprintf(lol, "line %d: %s\n",__LINE__,x);fflush(lol);}
#else
	#define reportif(x)
	#define reportd(x)
	#define reports(x)
#endif

enum {UP=0,RIGHT,DOWN,LEFT};
enum {BLOCK=0,SURVIVAL,MIRROR,IMITATE};
/* The Plan9 compiler can not handle VLAs */
#ifdef NO_VLA
#define len 36
#define wid 80

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
typedef struct snake{
	int y;
	int x;
	byte direction;
	byte fp;
	byte strategy;
	byte score;
	chtype color;
} snake;
snake p;//player
snake c;//computer
byte pscore;
byte cscore;
chtype colors[6]={0};
byte constant_change={0};

bool must_win=0;
FILE* lol;

void logo(void){
	mvaddstr(0,0," _       _");
	mvaddstr(1,0,"(_      | :  ");
	mvaddstr(2,0," _)NAKE |.'UEL");
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

void swap(byte* a,byte* b){
	byte s= *a;
	*a=*b;
	*b=s;
}
void swap_long(long* a,long* b){
	long s= *a;
	*a=*b;
	*b=s;
}
byte opposite(byte direction){
	switch(direction){
		case UP:
			return DOWN;
		case DOWN:
			return UP;
		case LEFT:
			return RIGHT;
		case RIGHT:
			return LEFT;
		default:
			abort();
	}
}
snake fake_move(snake s){
	switch(s.direction){
		case UP:
			s.y=s.y-1;
		break;
		case DOWN:
			s.y=s.y+1;
		break;
		case LEFT:
			s.x=s.x-1;
		break;
		case RIGHT:
			s.x=s.x+1;
		break;
	}
	return s;
}

bool blocked(byte board[len][wid],snake s){
	s=fake_move(s);
	return ( s.y<0 || s.y >=len || s.x<0 || s.x>=wid || board[s.y][s.x] );
}
bool better_change_way(byte board[len][wid],snake s){
	if(blocked(board,s)){
		return 1;
	}
	s=fake_move(s);
	if(blocked(board,s)){
		return 1;
	}
	return 0;
}
void putfp(byte board[len][wid],snake s){
	if(s.x>=0 && s.y>=0 && s.x<wid && s.y<len){
		board[s.y][s.x]=s.fp+opposite(s.direction);//putting direction for wiping
	}
}
void move_snake(byte board[len][wid],snake *s){
	assert(!blocked(board,*s));
	*s=fake_move(*s);
	putfp(board,*s);
}
void purs(snake me,int y,int x,byte directions[4]){
	if(me.y<y){
		directions[0]=DOWN;
		directions[3]=UP;
	}
	else{
		directions[0]=UP;
		directions[3]=DOWN;
	}

	if(me.x<x){
		directions[1]=RIGHT;
		directions[2]=LEFT;
	}
	else{
		directions[1]=LEFT;
		directions[2]=RIGHT;
	}
	int x_dist=abs(x-me.x);
	int y_dist=abs(y-me.y);

	if(x_dist>y_dist){
		swap(&directions[0],&directions[1]);
	}
	if(x_dist==y_dist && x_dist<3 && directions[0]==me.direction){
		swap(&directions[0],&directions[1]);
	}
	
}
void avoid(snake me,int y, int x, byte directions[4]){
	purs(me,y,x,directions);
	for(byte i=0;i<4;++i){
		directions[i]=opposite(directions[i]);
	}
}
void shuffle(byte directions[4]){
	byte a=rand()%4;
	byte b=rand()%4;
	swap(&directions[a],&directions[b]);
}
void enemy_avoid(snake me,snake enemy,byte directions[4]){
	avoid(me,enemy.y,enemy.x,directions);
}
void enemy_pursue(snake me,snake enemy,byte directions[4]){
	purs(me,enemy.y,enemy.x,directions);
}
void enemy_block(byte board[len][wid],snake me, snake enemy,byte directions[4]){
	snake ahead=enemy;
	switch(enemy.direction){
		case UP:
			if(me.y>enemy.y)//me is to the down of the enemy, so cannot plan to block it's way in advance
				goto JustPursue;
			break;
		case DOWN:
			if(me.y<enemy.y)
				goto JustPursue;
			break;
		case RIGHT:
			if(me.x<enemy.x)
				goto JustPursue;
			break;
		case LEFT:
			if(me.x>enemy.x)
				goto JustPursue;
			break;
		default:
			abort();
	}

	for(byte i=0;i<10;++i){
		if(blocked(board,ahead)||ahead.y==me.y||ahead.x==me.x){
			purs(me,ahead.y,ahead.x,directions);
			return;
		}
		ahead=fake_move(ahead);
	}
	JustPursue:
	purs(me,ahead.y,ahead.x,directions);
}
void enemy_mirror(snake me,snake enemy,byte directions[4]){
	int y,x;
	y=len-1-enemy.y;
	x=wid-1-enemy.x;
	purs(me,y,x,directions);
}
void enemy_block_mirror(snake me,snake enemy,byte directions[4]){
	int y_dist=abs(me.y-enemy.y);
	int x_dist=abs(me.x-enemy.x);

	if(y_dist>x_dist){
		purs(me,len-1-enemy.y,enemy.x,directions);
	}
	else{
		purs(me,enemy.y,wid-1-enemy.x,directions);
	}
}
void move_to_top(byte array[4],byte index){
	byte newtop=array[index];
	for(byte i=index;i>0;--i){
		array[i]=array[i-1];
	}
	array[0]=newtop;
}
void leave_escapes(byte board[len][wid],snake me,byte directions[4]){
	byte s=3;
	for(byte i=0;i<4;i++){
		me.direction=directions[s];
		if(!better_change_way(board,me)){
			move_to_top(directions,s);
		}
		else{
			--s;
		}
	}
}
long go_deep(byte board[len][wid],snake me,bool randomize){
	reports("****go deep***");
	if(randomize){
		reports("randomize");
	}
	long m=0;
	byte bumps=0;
	static byte inc=1;
	if(randomize){
		inc=-inc;
	}
	while(!blocked(board,me)){
		me=fake_move(me);
		++m;
		if(m>len+wid){
			return m;
		}
		if(blocked(board,me)||(randomize&&!(rand()%10))){
			snake f=me;
			byte i;
			
			if(randomize){
				f.direction=rand()%4;
			}

			for(i=0;i<4;++i){
				if(f.direction!=opposite(me.direction) || blocked(board,f)){
					me=f;
					break;
				}
				else{
					f.direction+=4+inc;
					f.direction%=4;
				}
			}

			reports("***BUMP!***");
			reportd(bumps);
			reportd(m);

			if(bumps==4){
				return m;
			}
			else{
				++bumps;
			}

		}
	}
	return m;

}
long mnvrblty(byte board[len][wid],snake me,byte depth){
	long m=0;
	long max=0,n,max_n;
	while(m<=4 && !blocked(board,me)){
		me=fake_move(me);
		++m;
		if(depth){
			snake f=me;
			max_n=0;
			for(byte i=0;i<4;++i){
				n=0;
				if(i==opposite(me.direction)){
					continue;
				}
				f.direction=i;
				for(byte j=0;j<10;++j){
					n=go_deep(board,f,j%2);
					if(max_n<n){
						max_n=n;
					}
					if(max_n>len+wid){
						return max_n;
					}
				}
				reports("Then the maximum became:");
				reportd(max_n);
			}
			if(max<m+max_n){
				max=m+max_n;
			}
		}
	}
	return max;
}
void sort_directions(long data[4],byte directions[4]){
	bool not_sorted=1;
	while(not_sorted){
		not_sorted=0;
		for(byte i=0;i<3;++i){
			if(data[i]<data[i+1]){
				swap_long(&data[i],&data[i+1]);
				swap(&directions[i],&directions[i+1]);
				not_sorted=1;
			}
		}
	}
}
void rank_for_survival(byte board[len][wid],snake me,long advantages[4],byte directions[4]){
	long max_adv,adv,sum,sum_positives;
	for(byte i=0;i<4;++i){
		reports("inspecting various directions");
		reportd(i);
		adv=sum=sum_positives=0;
		max_adv=LONG_MIN;
		me.direction= directions[i];
		adv=mnvrblty(board,me,2);//advantage(board,*me,*enemy,depth-1);
		reports("advantage is");
		reportd(adv);
		if(max_adv<adv){
			max_adv=adv;
		}
		advantages[i]=max_adv;
			
		reportd(advantages[i]);
	}

	sort_directions(advantages,directions);
	reportd(advantages[0]);
	reportd(directions[0]);
	reportd(advantages[1]);
	reportd(directions[1]);
	reportd(advantages[2]);
	reportd(directions[2]);
	reportd(advantages[3]);
	reportd(directions[3]);
}
void draw(byte board[len][wid]){
	int y,x;
	rectangle();
	mvprintw(1,16,"Computer's wins: %d",c.score);
	mvprintw(2,16,"Your wins: %d",p.score);
	for(y=0;y<len;++y){
		for(x=0;x<wid;++x){
			switch(board[y][x]/4){
				case 1:
					mvaddch(4+y,x+1,' '|A_STANDOUT|c.color);
				break;
				case 2:
					mvaddch(4+y,x+1,' '|A_STANDOUT|p.color);	
				break;
			}
			if(board[y][x]<0)
				mvaddch(4+y,x+1,'0'-board[y][x]);
		}
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
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	move(4,0);
	printw("Don't hit the walls, the other snake and yourself. Kill the other snake.\n");
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
long decide(byte board[len][wid],snake *me,snake *enemy){
	//do the move that gives the enemy least advantage (or move randomly at depth 0)
	//return 0 if you fail to find a way out
	snake f=*me;//f:future 
	static long turn=0;
	reports(" **MOVE***********");
	reportd(turn);
	++turn;
	reportd(me->direction);
	int y_dist=(abs(me->y-enemy->y));
	int x_dist=(abs(me->x-enemy->x));
	int dist=(y_dist+x_dist);
	long g=go_deep(board,*me,1);
	reportd(g);
	byte directions[4]={0,1,2,3};
	long advantages[4]={0};
	if(me->strategy==IMITATE ){
		if(abs(me->y-(len-1-enemy->y))+abs(me->x-(wid-1-enemy->x))>3){
			me->strategy=SURVIVAL;
		}
		else{
			me->strategy=IMITATE;
		}
	}
	else if(g<20){
		 me->strategy=SURVIVAL;
	}
	else if( dist<20){
		me->strategy=BLOCK;
	}
	else{
		me->strategy=MIRROR;
	}
	bool change_path=0;
	if(better_change_way(board,*me)){
		change_path=1;
	}
	else if(me->strategy==IMITATE){
		change_path=1;
	}
	else if(me->strategy==SURVIVAL){
		reports("SURVIVAL!@#");
		change_path=1;
	}
	else if(me->strategy==MIRROR){
		change_path=better_change_way(board,*me) || ((me->x%2)&&(me->y%3==2)) || ((me->x%2==0)&&(me->y%3==0));
		if(better_change_way(board,*me) && !change_path){
			reports("fuck you");
		}
	}
	else if(me->strategy==BLOCK){
		reports("BLOCK!@#");
	       	change_path= !(rand()%(dist+1)) || !(rand()%(x_dist+1)) || !(rand()%(y_dist+1));//this one wants to leave escapes
		
		if(!change_path && dist<40 && !(rand()%(dist/2+1))){//this one wants to kill
			change_path=1;
		}
	}
	
	if(change_path){
		if(me->strategy==IMITATE){
			enemy_mirror(*me,*enemy,directions);
		}
		if(me->strategy==MIRROR){
			enemy_mirror(*me,*enemy,directions);
			//shuffle(directions);
			leave_escapes(board,*me,directions);
			reports("did the leave escapes shit");
			reports("MIRROR");
		}
		else if(me->strategy==BLOCK){
			if(dist<7){
				enemy_pursue(*me,*enemy,directions);
			}
			/*else if(dist<20){
				enemy_block(board,*me,*enemy,directions);
			}*/
			else{
				enemy_block(board,*me,*enemy,directions);
			}
			leave_escapes(board,*me,directions);
			reports("BLOCK");
		}
		
		else if(me->strategy==SURVIVAL){
			rank_for_survival(board,*me,advantages,directions);
			reports("SURVIVAL and I am acting upon it");

		}

		for(byte i=0;i<4;++i){//if one way is blocked, go for others
			reportd(directions[i]);
			f.direction=directions[i];
			if(!blocked(board,f)){
				if(better_change_way(board,f)){
					reports("YET THIS MOTHER FUCKER CHOSE:");
					reportd(i);
				}
				*me=f;
				move_snake(board,me);
				return 1;
			}
			else{
				reports("this fucker didn't choose:");
				reportd(directions[i]);
				reports("because that way was supposedly blocked.");
			}
		}
		return LOSE;
	}

	reports("went on");
	move_snake(board,me);
	return 1;
}
void init_game(byte board[len][wid]){
	if(p.score>c.score+2 && rand()%2){
		must_win=1;
	}
	if(must_win && p.score>c.score){
		c.strategy=IMITATE;
	}
	else{
		c.strategy=MIRROR;
	}

	c.direction=0;
	c.y=len/2;
	c.x=wid*9/20;
	c.fp=4;
	c.color=colors[rand()%6];
	p.direction=0;
	p.y=len/2;
	p.x=wid*11/20;
	p.fp=8;

	do{
		p.color=colors[rand()%6];
	}while(p.color==c.color);

	for(byte y=0;y<len;++y){
		for(byte x=0;x<wid;++x){
			board[y][x]=0;
		}
	}
}
int main(int argc, char** argv){
	#ifdef REPORT
	lol=fopen("lol","w");
	fflush(lol);
	#endif 
	bool autoset=1;
	signal(SIGINT,sigint_handler);
	#ifndef NO_VLA
	int opt;
	while( (opt=getopt(argc,argv,"hnl:w:"))!=-1){
		switch(opt){
			case 'l':
				len=atoi(optarg);
				if(len<MINLEN || len>MAXLEN){
					fprintf(stderr,"Length too high or low.\n");
				}
				autoset=0;
			break;
			case 'w':
				wid=atoi(optarg);
				if(wid<MINWID || wid>MAXWID){
					fprintf(stderr,"Width too high or low.\n");
				}
				autoset=0;
			break;
			case 'h':
			default:
				printf("Usage:%s [options]\n -l length\n -w width\n -h help\n",argv[0]);
				return EXIT_FAILURE;
			break;
		}
	}
	#endif
	initscr();
	#ifndef NO_VLA
	if(autoset){
		len=LINES-7;
		if(len<MINLEN)
			len=MINLEN;
		else if(len>MAXLEN)
			len=MAXLEN;

		wid=COLS-5;
		if(wid<MINWID)
			wid=MINWID;
		else if(wid>MAXWID)
			wid=MAXWID;
	}
	#endif
	srand(time(NULL)%UINT_MAX);		
	byte board[len][wid];
	byte win_limit=WIN_LIMIT;

	reportd(len);
	reportd(wid);
	noecho();
	cbreak();
	keypad(stdscr,1);
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_RED,-1);
		init_pair(2,COLOR_YELLOW,-1);
		init_pair(3,COLOR_GREEN,-1);
		init_pair(4,COLOR_CYAN,-1);
		init_pair(5,COLOR_MAGENTA,-1);
		init_pair(6,COLOR_BLUE,-1);
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}
		colors[1]|=A_BOLD;
	}
	Start:
	if(c.score==win_limit || p.score==win_limit){
		win_limit=WIN_LIMIT;
		c.score=p.score=0;
		must_win=0;
	}
	if(c.score==p.score && p.score==win_limit-1){
		++win_limit;
	}
	curs_set(0);
	halfdelay(1);
	init_game(board);
	erase();
	int preinput=0,input=0;
	while(1){
		logo();
		draw(board);
		refresh();
		preinput=input;
		input = getch();
		if( input == KEY_F(1) || input=='?' )
			help();
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		if( (input=='k' || (input==KEY_UP||input=='w')) && p.y>0 && p.direction != DOWN ){
			p.direction=UP;
		}
		if( (input=='j' || (input==KEY_DOWN||input=='s')) && p.y<len-1 && p.direction != UP ){
			p.direction=DOWN;
		}
		if( (input=='h' || (input==KEY_LEFT||input=='a')) && p.x>0 && p.direction != RIGHT){
			p.direction=LEFT;
		}
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && p.x<wid-1 && p.direction != LEFT){
			p.direction=RIGHT;
		}
		if( (input=='q'||input==27))
			sigint_handler(0);
		if( input=='p'){
			nocbreak();
			cbreak();
			erase();
			logo();
			attron(A_BOLD);
			mvaddstr(1,13,"PAUSED");
			attroff(A_BOLD);
			getch();
			halfdelay(1);
		}
		if(input!=ERR){
			if(preinput==input){//if it wasn't there, hitting two keys in less than 0.1 sec would not work
				usleep(100000);
				flushinp();
			}
		}


		for(byte i=0;i<2;++i){
			if(blocked(board,p)){
				++c.score;
				reports("player died");
				goto Die;
			}
			else{
				move_snake(board,&p);
			}

			/*if(decide(board,&p,&c) == LOSE){//move, if failed die.
				++c.score;
				reports("computer died");
				goto Die;
			}*/
			if(decide(board,&c,&p) == LOSE){//move, if failed die.
				++p.score;
				reports("computer died");
				goto Die;
			}
		}
		refresh();
	}
	Die:
	nocbreak();
	cbreak();
	draw(board);
	refresh();
	mvprintw(5+len,0,"Game over! Wanna play again?(y/n)");
	curs_set(1);
	input=getch();
	if( input!= 'N' &&  input!= 'n' && input!='q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
