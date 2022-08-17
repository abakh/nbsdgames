/* 
.-.
|_.'      
|  \EVENGE

Authored by abakh <abakh!tuta,io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

*/
#include "common.h"
enum {EMPTY=0,BLOCK,CAT_OLD,CAT,CAT_NEW,CAT_TRAPPED,CHEESE};
/* The Plan9 compiler can not handle VLAs */
#ifdef NO_VLA
#define size 20
#else
byte size=20;
#endif
#define SAVE_TO_NUM 10
long score=0;
byte py,px;
byte ey,ex; //the empty tile
chtype colors[6]={0};
void rectangle(byte sy,byte sx){
	for(byte y=0;y<=size+1;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+size*2,ACS_VLINE);
	}
	for(byte x=0;x<=size*2;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+size+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+size+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+size*2,ACS_URCORNER);
	mvaddch(sy+size+1,sx+size*2,ACS_LRCORNER);
}
void logo(byte sy,byte sx){
	mvaddstr(sy,sx,  ".-.");
	mvprintw(sy+1,sx,"|_.'");
	mvaddstr(sy+2,sx,"|  \\EVENGE ");
}

byte save_score(void){
	return fallback_to_home("revenge_scores",score,SAVE_TO_NUM);

}

void show_scores(byte playerrank){
	erase();
	logo(0,0);
	if(playerrank==FOPEN_FAIL){
		mvaddstr(3,0,"Could not open score file");
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
			mvprintw(4,0, "     _____ ");
			mvprintw(5,0, "   .'     |");
			mvprintw(6,0, " .'       |");
			mvprintw(7,0, " |  .|    |");
			mvprintw(8,0, " |.' |    |");
			mvprintw(9,0, "     |    |");
			mvprintw(10,0,"  ___|    |___");
			mvprintw(11,0," |            |");
			mvprintw(12,0," |____________|");
			b=(b+1)%6;
			if(getch()==ERR)
				goto Effect;
			nocbreak();
			cbreak();
			erase();
			logo(0,0);
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

//display
void draw(byte sy,byte sx,byte board[size][size]){
	rectangle(sy,sx);
	mvprintw(1,sx+12,"Score: %ld",score);
	chtype prnt;
	byte y,x;
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			switch(board[y][x]){
				case EMPTY:
					prnt=' ';
				break;
				case BLOCK:
					prnt='#'|colors[1];
				break;
				case CAT:
				case CAT_OLD:
				case CAT_TRAPPED:
					prnt='f'|colors[2];
				break;
				case CHEESE:
					prnt='%'|colors[2]|A_BOLD;
				break;
			}
			if(y==py && x==px && board[y][x]==EMPTY){
				prnt = 'r'|A_STANDOUT;
			}
			mvaddch(sy+1+y,sx+x*2+1,prnt);
		}
	}
}
void fill(byte board[size][size]){
	byte y,x;
	
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			if(y>3 && y<17 && x>3 && x<17){
				board[y][x]= BLOCK;
			}
			else{
				board[y][x]= EMPTY;
			}
		}
	}
	py=size/2;
	px=size/2;
	board[py][px]=EMPTY;
}
void put_cats(byte board[size][size], byte number){
	byte y,x;
	for(byte i=0;i<number;++i){
		do{
			y=rand()%size;
			x=rand()%size;
		}while(board[y][x]!=EMPTY || abs(y-py)<4 || abs(x-px)<4);
		board[y][x]=CAT;
	}
}
void turn_to_cheese(byte board[size][size]){
	byte y,x;
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			if(board[y][x]==CAT_OLD){
				board[y][x]=CHEESE;
				score+=75;
			}
		}
	}
}
#define MOVE_CAT if(!(y+dy<0 || x+dx<0 || y+dy>=size || x+dx>=size)&&(board[y+dy][x+dx]==EMPTY)){\
			board[y][x]=EMPTY;\
			board[y+dy][x+dx]=CAT_NEW;\
			goto Next;\
		}
byte cat_life(byte board[size][size]){
	byte y,x,dy,dx,predy,predx;
	byte only_old=1;
	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			if(board[y][x]==CAT_OLD){
				for(dy=-1;dy<2;++dy){
					for(dx=-1;dx<2;++dx){
						MOVE_CAT;
					}
				}
			}
			if(board[y][x]==CAT){
				only_old=0;
				board[y][x]=CAT_OLD;
				dy=dx=0;
				if(py<y){
					dy=-1;
				}
				if(px<x){
					dx=-1;
				}
				if(py>y){
					dy=1;
				}
				if(px>x){
					dx=1;
				}
				MOVE_CAT;

				predy=dy;
				predx=dx;
				dy=0;
				dx=predx;
				MOVE_CAT;

				dy=predy;
				dx=0;
				MOVE_CAT;

				dx=-1+(rand()%3);
				MOVE_CAT;

				dy=-1+(rand()%3);
				MOVE_CAT;

				for(dy=-1;dy<2;++dy){
					for(dx=-1;dx<2;++dx){
						MOVE_CAT;
					}
				}
				
			}
		Next: continue;
		}
	}

	for(y=0;y<size;++y){
		for(x=0;x<size;++x){
			if(board[y][x]==CAT_NEW){
				board[y][x]=CAT;
			}
		}
	}
	if(only_old){
		turn_to_cheese(board);
		return 0;
	}
	return 1;
}
void tile_push(byte board[size][size],byte dy, byte dx){
	byte y=py,x=px;
	y+=dy;
	x+=dx;
	byte first_tile=1;
	while(y>=0 && x>=0 && y<size && x<size){
		if(first_tile && board[y][x]==CHEESE){
			score+=	100;
		}
		if(board[y][x]==EMPTY || board[y][x]==CHEESE){
			board[y][x]=BLOCK;
			board[py+dy][px+dx]=EMPTY;
			py+=dy;
			px+=dx;
			break;
		}
		else if(board[y][x]!=BLOCK){
			break;
		}
		y+=dy;
		x+=dx;
	}

}
//peacefully close when ^C is pressed
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
void mouseinput(void){
#ifndef NO_MOUSE
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4<size && minput.x-1<size*2){
		py=minput.y-4;
		px=(minput.x-1)/2;
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('\n');
#endif
}
void help(void){
	erase();
	logo(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	mvprintw(8,0,"YOU CAN ALSO USE THE MOUSE!");
	attroff(A_BOLD);
	mvprintw(4,0,"RETURN/ENTER : Slide");
	mvprintw(5,0,"hjkl/ARROW KEYS : Move cursor");
	mvprintw(6,0,"q : Quit");
	mvprintw(7,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(10,0,"Press a key to continue");
	refresh();
	getch();
	erase();
}
void gameplay(void){
	erase();
	logo(0,0);
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	mvprintw(4,0,"Trap cats and don't be eaten by them.\n");
	refresh();
	getch();
	erase();
}
int main(int argc, char** argv){
	int opt;
	bool no_replay=0;
	while( (opt=getopt(argc,argv,"hns:"))!=-1){
		switch(opt){
			case 'n':
				no_replay=1;
			break;	
			case 'h':
			default:
				printf("Usage:%s [options]\n -h help\n -n don't ask for replay\n",argv[0]);
			break;
		}
	}
	signal(SIGINT,sigint_handler);
	srand(time(NULL)%UINT_MAX);
	initscr();
#ifndef NO_MOUSE
	mousemask(ALL_MOUSE_EVENTS,NULL);
#endif
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
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}

	}

	byte board[size][size];
	int input;
	time_t last_wave,last_move;
	byte waves_count;
	Start:
	waves_count=0;
	halfdelay(1);
	score=0;
	last_wave=last_move=time(0);
	py=px=0;
	ey=ex=size-1;
	curs_set(0);
	fill(board);
	put_cats(board,1);
	while(1){
		erase();
		logo(0,0);
		draw(3,0,board);
		refresh();
		input = getch();
		if( input==KEY_F(1) || input=='?' )
			help();
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		if( input==KEY_MOUSE )
			mouseinput();
		if( (input=='k' || (input==KEY_UP||input=='w')) && py>0){
			tile_push(board,-1,0);
		}
		if( (input=='j' || (input==KEY_DOWN||input=='s')) && py<size-1){
			tile_push(board,1,0);
		}
		if( (input=='h' || (input==KEY_LEFT||input=='a')) && px>0){
			tile_push(board,0,-1);
		}
		if( (input=='l' || (input==KEY_RIGHT||input=='d')) && px<size-1){
			tile_push(board,0,1);
		}
		if( input=='y'){
			tile_push(board,-1,-1);
		}
		if( input=='u'){
			tile_push(board,-1,1);
		}
		if(input=='b'){
			tile_push(board,1,-1);
		}
		if(input=='n'){
			tile_push(board,1,1);
		}
		if( input=='p'){
			nocbreak();
			cbreak();
			erase();
			logo(0,0);
			attron(A_BOLD);
			mvaddstr(2,12,"PAUSED");
			attroff(A_BOLD);
			getch();
			halfdelay(1);
		}
		if( (input=='q'||input==27)){
			sigint_handler(0);
		}
		if( last_move < time(0)){
			byte no_cat_life=!cat_life(board);
			if( no_cat_life || (time(0)> last_wave+300 && waves_count<5)){
				put_cats(board,2+(rand()%7));
				last_wave=time(0);
				waves_count++;
			}
			if(no_cat_life){
				waves_count=0;
			}
			last_move=time(0);
		}
		if(board[py][px]==CAT){
			break;
		}
	}
	flushinp();
	nocbreak();
	cbreak();
	logo(0,0);
	draw(3,0,board);
	refresh();
	move(25,0);
	printw("You lost The Game. Press a key to see high scores:");
	getch();
	show_scores(save_score());
	if(!no_replay){
		printw("You lost The Game. Wanna play again?(y/n)");
		refresh();
		curs_set(1);
		input=getch();
		while(input!='n'&&input!='N'&&input!='q'&&input!='Q'&&input!='y'&&input!='\n'){
			input=getch();
		}

		if(input != 'N' && input != 'n' && input != 'q')
			goto Start;
	}
	else{
		printw(" Press any key on this computer's keyboard if you want to continue.");
		getch();
	}
	endwin();
	return EXIT_SUCCESS;
}
