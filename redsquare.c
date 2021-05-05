/*
 _   _
|_) (_
| \ed_)quare

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


*/
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include "config.h"
#define LEN 35 
#define WID 50
#define RLEN LEN //real
#define RWID WID
#define DEAD 0
#define ALIVE 1
#define RED 2
typedef signed char byte;

int level;
byte py,px;
byte cy,cx;//cross
bool coherent;//square's coherence
int anum,rnum;//reds and otherwise alive cell counts
chtype colors[6]={0};
void cp(byte a[RLEN][RWID],byte b[RLEN][RWID]){
	byte y,x;
	for(y=0;y<RLEN;++y)
		for(x=0;x<RWID;++x)
			b[y][x]=a[y][x];
}
void logo(void){
	move(0,0);
	addstr(" _   _\n");
	addstr("|_) (_\n");
	addstr("| \\ED_)QUARE");
}
void rectangle(int sy,int sx){
	for(int y=0;y<=LEN;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+WID+1,ACS_VLINE);
	}
	for(int x=0;x<=WID;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+LEN+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+LEN+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+WID+1,ACS_URCORNER);
	mvaddch(sy+LEN+1,sx+WID+1,ACS_LRCORNER);
}
void count(byte board[LEN][WID]){
	byte y,x;
	anum=rnum=0;
	for(y=0;y<LEN;++y){
		for(x=0;x<WID;++x){
			if(board[y][x]==ALIVE)
				++anum;
			else if(board[y][x]==RED)
				++rnum;
		}
	}
}
//display
void draw(byte board[RLEN][RWID]){
	rectangle(3,0);
	chtype prnt;
	byte y,x;
	for(y=0;y<LEN;++y){
		for(x=0;x<WID;++x){
			if(y==cy && x==cx){
				prnt='X';
				if(board[y][x]==ALIVE)
					prnt|=A_STANDOUT;
				else if(board[y][x]==RED)
					prnt|=colors[3]|A_STANDOUT;
			}
			else{
				if(board[y][x]==ALIVE)
					prnt=ACS_BLOCK;
				else if(board[y][x]==RED){
					if(coherent)
						prnt=' '|A_STANDOUT|colors[3];
					else
						prnt='O'|colors[3];
				}
				else
					prnt=' ';
			}
			mvaddch(4+y,x+1,prnt);
		}
	}
}
void rand_level(byte board[RLEN][RWID]){
	byte y,x;
	for(y=0;y<LEN/2;++y){
		for(x=0;x<WID;++x){
			if(rand()%2){
				if(rand()%3)
					board[y][x]=ALIVE;
			}
			else
				board[y][x]=DEAD;
		}
	}
}
void live(byte board[RLEN][RWID]){
	byte y,x;
	byte dy,dx;//delta
	byte ry,rx;
	byte alives,reds;
	byte preboard[RLEN][RWID];
	cp(board,preboard);
	for(y=0;y<LEN;++y){
		for(x=0;x<WID;++x){
			alives=reds=0;
			for(dy=-1;dy<2;++dy){
				for(dx=-1;dx<2;++dx){
					if(!dy && !dx)
						continue;
					ry=y+dy;
					rx=x+dx;
					if(ry==-1)
						ry=LEN-1;
					else if(ry==LEN)
						ry=0;
					if(rx==-1)
						rx=WID-1;
					else if(rx==WID)
						rx=0;

					if(preboard[ry][rx]==ALIVE)
						++alives;
					else if(preboard[ry][rx]==RED)
						++reds;
				}
			}
			if(board[y][x]){
				if(alives+reds==2 || alives+reds==3){
					if(reds>alives)
						board[y][x]=RED;
					else if(alives>reds)
						board[y][x]=ALIVE;
				}
				else{
					if(coherent && board[y][x]==RED)
						coherent=0;
					board[y][x]=DEAD;
				}
			}
			else if(alives+reds==3){
				if(alives>reds)
					board[y][x]=ALIVE;
				else
					board[y][x]=RED;
			}
		}
	}
}
void add_line(byte board[LEN][WID],byte line,const char* str){
	for(byte x=0;str[x]!='\0';++x){
		if(str[x]=='#')
			board[line][x]=ALIVE;
		/*else	
			board[line][x]=0;*/
	}
}
void new_level(byte board[LEN][WID]){
	++level;
	memset(board,0,RLEN*RWID);
	switch(level){
		case 0:
			cy=12;
			cx=RWID/2;
			add_line(board,5, "                ####   #");
			add_line(board,6, "             ####      #");
			add_line(board,7, "                #      #          "); 
			add_line(board,8, "                #  ##  # ##  # ##");
			add_line(board,9, "                # #  # ##  # ##  #");
			add_line(board,10,"            #   # #  # #   # #   #");
			add_line(board,11,"             ###   ##  #   # #   #");
			
			add_line(board,15,"     ####       ");
			add_line(board,16,"    #    #               ");
			add_line(board,17,"    #        ##   # ##  #     #  ##   #  #");
			add_line(board,18,"    #       #  #  ##  #  # # #  #  #  #  #");
			add_line(board,19,"    #    #  #  #  #   #  # # #  #  #  #  #");
			add_line(board,20,"     ####    ##   #   #   # #    ## #  ###");
			add_line(board,21,"                                         #");
			add_line(board,22,"                                      #  #");
			add_line(board,23,"                                       ##");
		break;
		case 1:
			cy=12;
			cx=RWID/2;
			add_line(board,5, " #     #  #           #");
			add_line(board,6, " #     # ##           #     ");
			add_line(board,7, "  #   #   #   ##    ###  #  # ## ##  #  # ##");
			add_line(board,8, "  #   #   #  #  #  #  #  #  ##  #  # #  ##");
			add_line(board,9, "   # #    #  #  #  #  #  #  #   #  # #  #");			
			add_line(board,10,"    #      #  ## #  ## #  # #   #  #  # #");

			add_line(board,15,"   ####                        # ");
			add_line(board,16,"  #    #                       #  ");
			add_line(board,17,"  #    #  # ##  #  #   #  ##   #   ##  #   #");
			add_line(board,18,"  #####   ##    #   # #  #  #  #  #  #  # #");
			add_line(board,19,"  #       #     #   # #  #  #  #  #  #  # #");
			add_line(board,20,"  #       #      #   #    ## #  #  ##    #");
		break;
		case 2:
			cy= 12;
			cx= 10;
			add_line(board,3, "             ##             #        #");
			add_line(board,4, "             ##            #        # ");
			add_line(board,5, "                           #        # ");
			add_line(board,6, "                           #  #     #  # ");
			add_line(board,7, "                           ###      ### ");
			add_line(board,17,"      ##     ## ");
			add_line(board,18,"     #  #   #  #");
			add_line(board,19,"      #  # #  # ");
			add_line(board,20,"         # #                          ");
			add_line(board,21,"       ### ###                                 ");
			add_line(board,22,"     ###     ###                               ");
			add_line(board,23,"     ##       ##                               ");
			add_line(board,24,"     ##       ##                               ");
			add_line(board,25,"      # ## ## #                                ");	
			add_line(board,26,"      ###   ###");
			add_line(board,27,"       #     #");

			add_line(board,30,"             ##");
			add_line(board,31,"             ##");
		break;
		case 3:
			cy=RLEN/2;
			cx=RWID/2;
			add_line(board,0, "                                               ");
			add_line(board,1, "   #                     #                     ");
			add_line(board,2, "    #                     #                    ");
			add_line(board,3, "  ###                   ###                    ");
			add_line(board,4, "        #                     #                ");
			add_line(board,5, "         #                     #               ");
			add_line(board,6, "       ###                   ###               ");
			add_line(board,7, "             #                     #           ");
			add_line(board,8, "              #                     #          ");
			add_line(board,9, "            ###                   ###          ");
			add_line(board,10,"                  #                     #      ");
			add_line(board,11,"                   #                     #     ");
			add_line(board,12,"                 ###                   ###     ");
			add_line(board,13,"                       #                     # ");
			add_line(board,14,"                        #                     #");
			add_line(board,15,"                      ###                   ###");
			add_line(board,17,"                                               ");
			add_line(board,18,"      #                                        ");
			add_line(board,19,"       #                                       ");
			add_line(board,20,"     ###                                       ");
			add_line(board,21,"           #                                   ");
			add_line(board,22,"            #                                  ");
			add_line(board,23,"          ###                                  ");
			add_line(board,24,"                #                              ");
			add_line(board,25,"                 #                             ");
			add_line(board,26,"               ###                             ");
			add_line(board,27,"                     #                         ");
			add_line(board,28,"                      #                        ");
			add_line(board,29,"                    ###                        ");
			add_line(board,30,"                          #                    ");
			add_line(board,31,"                           #                   ");
			add_line(board,32,"                         ###                   ");
		break;
		case 4:
			cy=rand()%(RLEN/2);
			cx=rand()%(RWID/2);
			add_line(board,0, "                                               ");
			add_line(board,1, "                                               ");
			add_line(board,2, "                                               ");
			add_line(board,3, "                                               ");
			add_line(board,4, "                                               ");
			add_line(board,5, "                                               ");
			add_line(board,6, "                                               ");
			add_line(board,0, "    #                     #       #       #     ");
			add_line(board,1, "     #  |           |      #       #       #    ");
			add_line(board,2, " #   #  |           |  #   #   #   #   #   #    ");
			add_line(board,3 ,"  ####  |           |   ####    ####    ####    ");
			add_line(board,11,"                                               ");
			add_line(board,12,"                                               ");
			add_line(board,13,"                                               ");
			add_line(board,8 ,"    #              #       #       #           ");
			add_line(board,9 ,"     #              #       #       #          ");
			add_line(board,10," #   #          #   #   #   #   #   #          ");
			add_line(board,11,"  ####           ####    ####    ####          ");
			add_line(board,19,"                                               ");
			add_line(board,20,"                                               ");
			add_line(board,16,"     #                      #       #      #   ");
			add_line(board,17,"      #|               |     #       #      #  ");
			add_line(board,18,"  #   #|               | #   #   #   #  #   #  ");
			add_line(board,19,"   ####|               |  ####    ####   ####  ");
			add_line(board,25,"                                               ");
			add_line(board,26,"                                               ");
			add_line(board,27,"                                               ");
			add_line(board,28,"                                               ");
			add_line(board,25,"    #                                   #      ");
			add_line(board,26,"     #                                   #     ");
			add_line(board,27," #   #                               #   #     ");
			add_line(board,28,"  ####                                ####     ");
			//add_line(board,5,"                #");
			//add_line(board,6,"                ##");
			//add_line(board,7,"               ##");
		break;
		default:
			srand(level);
			rand_level(board);
	}
}
void rm_square(byte board[LEN][WID],byte prey,byte prex){
	byte dy,dx,ry,rx;
	for(dy=0;dy<2;++dy){
		for(dx=0;dx<2;++dx){
			ry=prey+dy;
			if(ry==-1)
				ry=LEN-1;
			else if(ry==LEN)
				ry=0;
			rx=prex+dx;
			if(rx==-1)
				rx=WID-1;
			else if(rx==WID)
				rx=0;
			board[ry][rx]=DEAD;
		}
	}
}
void mk_square(byte board[LEN][WID]){
	byte dy,dx,ry,rx;
	for(dy=0;dy<2;++dy){
		for(dx=0;dx<2;++dx){
			ry=py+dy;
			if(ry==-1)
				ry=LEN-1;
			else if(ry==LEN)
				ry=0;
			rx=px+dx;
			if(rx==-1)
				rx=WID-1;
			else if(rx==WID)
				rx=0;
			board[ry][rx]=RED;
		}
	}
}
//detect if there is a square and enable the player to move
void reemerge(byte board[LEN][WID]){
	byte y,x,dy,dx,ry,rx;
	for(y=0;y<LEN;++y)
		for(x=0;x<WID;++x)
			if(board[y][x]==RED)
				goto FoundTheFirst;
	FoundTheFirst:
	for(dy=0;dy<2;++dy){
		for(dx=0;dx<2;++dx){
			ry=y+dy;
			if(ry==-1)
				ry=LEN-1;
			else if(ry==LEN)
				ry=0;
			rx=x+dx;
			if(rx==-1)
				rx=WID-1;
			else if(rx==WID)
				rx=0;
			if(board[ry][rx]!=RED){
				if(!y){
					y=LEN-1;//the square can be divided at both sides of the border, this prevents failing
					//it goes to look from the upper-left corner of the square as it would for other squares
					goto FoundTheFirst;
				}
				if(!x){
					x=WID-1;
					goto FoundTheFirst;
				}
				return;
			}
		}
	}
	py=y;
	px=x;
	coherent=1;	
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
/*void mouseinput(int sy, int sx){
	MEVENT minput;
	#ifdef PDCURSES
	nc_getmouse(&minput);
	#else
	getmouse(&minput);
	#endif
	if( minput.y-4-sy<LEN && minput.x-1-sx<WID*2){
		py=minput.y-4-sy;
		px=(minput.x-1-sx)/2;
	}
	else
		return;
	if(minput.bstate & BUTTON1_CLICKED)
		ungetch('\n');
	if(minput.bstate & (BUTTON2_CLICKED|BUTTON3_CLICKED) )
		ungetch(' ');
}*/
void help(void){
	erase();
	logo();
	nocbreak();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE CONTROLS ****");
	attroff(A_BOLD);
	mvprintw(4,0,"hjkl/ARROW KEYS : Move square");
	mvprintw(5,0,"q : Quit");
	mvprintw(6,0,"F1 & F2 : Help on controls & gameplay");
	mvprintw(8,0,"Press a key to continue");
	refresh();
	cbreak();
	getch();
	halfdelay(1);
	erase();
}
void gameplay(void){
	erase();
	logo();
	nocbreak();
	attron(A_BOLD);
	mvprintw(3,0,"  **** THE GAMEPLAY ****");
	attroff(A_BOLD);
	mvprintw(4,0,"Move the square and catch the X or outnumber the\n");
	printw(      "white cells with those of your own,\n");
	printw(      "in the environment of Conway's game of life.\n");
	refresh();
	cbreak();
	getch();
	halfdelay(1);
	erase();
}
int main(void){
	signal(SIGINT,sigint_handler);
	srand(time(NULL)%UINT_MAX);
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
		init_pair(4,COLOR_RED,-1);
		init_pair(5,COLOR_RED,COLOR_YELLOW);
		init_pair(6,COLOR_RED,COLOR_MAGENTA);
		for(byte b= 0;b<6;++b){
			colors[b]=COLOR_PAIR(b+1);
		}

	}
	byte board[RLEN][RWID];
	memset(board,0,RLEN*RWID);
	char result[70];
	int input=0;
	int prey,prex;
	int cinred;
	Start:
	curs_set(0);
	halfdelay(9);
	cinred=0;
	py=LEN*3/4;
	px=WID/2;
	curs_set(0);
	level=-1;
	new_level(board);
	mk_square(board);
	while(1){
		switch(rand()%5){//move the cross
			case 0:
				++cx;
				if(cx==WID)
					cx=0;
			break;
			case 1:
				--cy;
				if(cy==-1)
					cy=LEN-1;
			break;
			case 2:
				--cx;
				if(cx==-1)
					cx=WID-1;
			break;
			case 3:
				++cy;
				if(cy==LEN)
					cy=0;
			break;
			case 4:
			;//stay there
		}
		if(board[cy][cx]==RED)
			++cinred;
		else
			cinred=0;	
		count(board);
		if(rnum!=4)
			coherent=0;
		if(!coherent && rnum==4)
			reemerge(board);
		erase();
		logo();
		draw(board);
		refresh();
		if(rnum>anum||cinred==2){
			mvprintw(LEN+5,0,"Well done! Press a key to continue:");
			curs_set(1);
			getch();
			curs_set(0);
			new_level(board);
			py=LEN*3/4;
			px=WID/2;
			mk_square(board);
			continue;
		}
		else if(!rnum){
			move(LEN+5,0);
			printw("You have lost The Game");
			if(rand()%5==0)
				printw(" (and RedSquare)");
			printw(". ");
			break;
		}
		halfdelay(9);
		input = getch();
		live(board);
		count(board);//apparently this should come at both sides of live+draw. resulting from trial and error.
		if(rnum!=4)//the square has participated in life reactions if so
			coherent=0;
		if(!coherent && rnum==4)//there can be a square
			reemerge(board);

		if( input==KEY_F(1) || input=='?' )
			help();
		if( input==KEY_F(2) )
			gameplay();
		prey=py;
		prex=px;
		if(input=='k' || input==KEY_UP){
			--py;
			if(py==-1)
				py=LEN-1;
		}
		else if(input=='j' || input==KEY_DOWN){
			++py;
			if(py==LEN)
				py=0;
		}
		else if(input=='h' || input==KEY_LEFT){
			--px;
			if(px==-1)
				px=WID-1;
		}
		else if(input=='l' || input==KEY_RIGHT){
			++px;
			if(px==WID)
				px=0;
		}
		else 
			goto DidntMove;
		if(coherent){ 
			rm_square(board,prey,prex);
			mk_square(board);
		}
		DidntMove:
		if( input=='q')
			sigint_handler(0);
		if( input=='p'){
			nocbreak();
			cbreak();
			erase();
			logo();
			attron(A_BOLD);
			addstr("\n    PAUSED");
			attroff(A_BOLD);
			refresh();

			getch();

			halfdelay(9);
		}

	}
	
	printw("Wanna play again?(y/n)");
	nocbreak();
	cbreak();
	curs_set(1);
	flushinp();
	
	input=getch();

	if(input != 'N' && input != 'n' && input != 'q')
		goto Start;
	endwin();
	return EXIT_SUCCESS;
}
