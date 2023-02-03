/*
 _   _
|_) (_
| \ed_)quare

Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


*/
#include "common.h"
#define LEN 35 
#define WID 50
#define STALE_LIMIT 20
#define RLEN LEN //real
#define RWID WID
#define DEAD 0
#define ALIVE 1
#define RED 2
#define EMPTY_LINES 7
int level;
byte py,px;
byte cy,cx;//cross
bool coherent;//square's coherence
int anum,rnum;//reds and otherwise alive cell counts
int stale_cells,stale_for;//throw new cells if it is stale for a long time
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

int beginy,view_len;
void setup_scroll(){
	beginy=0;
	if(0<py+3-(LINES-EMPTY_LINES)){
		beginy=py+3-(LINES-EMPTY_LINES);
	}
	view_len=LEN;
	if(LINES-EMPTY_LINES<LEN){
		view_len=LINES-EMPTY_LINES;
	}
	if(beginy+view_len>LEN){
		beginy-=beginy+view_len-LEN;
	}
}

void rectangle(int sy,int sx){
	setup_scroll();
	for(int y=0;y<=view_len;++y){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+WID+1,ACS_VLINE);
	}
	for(int x=0;x<=WID;++x){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+view_len+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+view_len+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+WID+1,ACS_URCORNER);
	mvaddch(sy+view_len+1,sx+WID+1,ACS_LRCORNER);
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
byte get_cell(byte board[LEN][WID],int y,int x){
	return board[(y+LEN)%LEN][(x+WID)%WID];
}

//display
void draw(byte board[RLEN][RWID]){
	rectangle(3,0);
	chtype prnt;
	byte y,x;
	setup_scroll();
	if(stale_for){
		attron(colors[3]);
		mvprintw(4,0,"%d",stale_for);
		attroff(colors[3]);
	}
	for(y=beginy;y<beginy+view_len;++y){
		for(x=0;x<WID;++x){
			if(y==cy && x==cx){
				prnt='X';
				if(get_cell(board,y,x)==ALIVE)
					prnt|=A_STANDOUT;
				else if(get_cell(board,y,x)==RED)
					prnt|=colors[3]|A_STANDOUT;
			}
			else{
				if(get_cell(board,y,x)==ALIVE)
					prnt=ACS_BLOCK;
				else if(get_cell(board,y,x)==RED){
					if(coherent && (y==py||y==(py+LEN+1)%LEN)&& (x==px||x==(px+WID+1)%WID)){
						prnt=' '|A_STANDOUT|colors[3];
					}
					else{
						prnt='O'|colors[3];
					}
				}
				else
					prnt=' ';
			}
			mvaddch(4+y-beginy,x+1,prnt);
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
			add_line(board,5,"                #");
			add_line(board,6,"                ##");
			add_line(board,7,"               ##");
		break;
		default:
			srand(level);
			cy=rand()%(RLEN/2);
			cx=rand()%(RWID/2);
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
byte nothing_around(byte board[LEN][WID],byte fy, byte fx){
	byte count_reds=0;
	byte sy=fy-1,sx=fx-1;//s:start
	byte dy,dx;//d:delta
	for(dy=0;dy<4;dy++){
		for(dx=0;dx<4;dx++){
			if(get_cell(board,sy+dy,sx+dx)){
				count_reds++;
			}
		}
	}
	return (count_reds==4);
}

byte no_square(byte board[LEN][WID]){
	return !(get_cell(board,py,px) &&
		get_cell(board,py+1,px) &&
		get_cell(board,py,px+1) &&
		get_cell(board,py+1,px+1) &&
		nothing_around(board,py,px) );
}
void find_square(byte board[LEN][WID],byte fy, byte fx){//f:found
	byte dy,dx,ry,rx;
	for(dy=0;dy<2;++dy){
		for(dx=0;dx<2;++dx){
			ry=fy+dy;
			rx=fx+dx;
			if(get_cell(board,ry,rx)!=RED){
				//the square can be divided at both sides of the border, this prevents failing
				//it goes to look from the upper-left corner of the square as it would for other squares
				return;
			}
		}
	}
	if(nothing_around(board,fy,fx)){
		py=fy;
		px=fx;
		coherent=1;	
	}
}
//detect if there is a square and enable the player to move
void reemerge(byte board[LEN][WID]){
	byte y,x,dy,dx,ry,rx;
	for(y=0;y<LEN;++y){
		for(x=0;x<WID;++x){
			if(board[y][x]==RED){
				find_square(board,y,x);
			}
			if(coherent){
				return;
			}
		}
	}
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
int main(int argc,char** argv){
	if(argc>1){
		printf("This game doesn't take arguments");
	}
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
	int input=0;
	int prey,prex;
	int cinred;
	level=-1;
	Start:
	stale_cells=0;
	stale_for=0;
	curs_set(0);
	halfdelay(9);
	cinred=0;
	py=LEN*3/4;
	px=WID/2;
	curs_set(0);
	new_level(board);
	mk_square(board);
	while(1){
		switch(rand()%5){//move the X
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
		if(no_square(board)){
			coherent=0;
		}
		if(!coherent && rnum>=4)
			reemerge(board);
		erase();
		logo();
		draw(board);
		refresh();
		if(coherent || abs(stale_cells-(rnum+anum))<stale_cells/10){//if there is little variation it is stale
			stale_cells=rnum+anum;
		}
		else{
			stale_for+=1;
		}
		if(stale_for>STALE_LIMIT){
			for(int i=0;i<10;++i){
				board[rand()%LEN][rand()%WID]=RED;
			}
			for(int i=0;i<10;++i){
				board[rand()%LEN][rand()%WID]=ALIVE;
			}
			stale_for=0;
		}
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
		if(no_square(board))//the square has participated in life reactions if so
			coherent=0;
		if(!coherent)//there can be a square
			reemerge(board);

		if( input==KEY_F(1) || input=='?' )
			help();
		if( (input==KEY_F(2)||input=='!') )
			gameplay();
		prey=py;
		prex=px;
		if(input=='k' || (input==KEY_UP||input=='w')){
			--py;
			if(py==-1)
				py=LEN-1;
		}
		else if(input=='j' || (input==KEY_DOWN||input=='s')){
			++py;
			if(py==LEN)
				py=0;
		}
		else if(input=='h' || (input==KEY_LEFT||input=='a')){
			--px;
			if(px==-1)
				px=WID-1;
		}
		else if(input=='l' || (input==KEY_RIGHT||input=='d')){
			++px;
			if(px==WID)
				px=0;
		}
		else 
			goto DidntMove;
		if(!coherent){
			reemerge(board);
		}
		if(coherent){ 
			rm_square(board,prey,prex);
			mk_square(board);
		}
		DidntMove:
		if( (input=='q'||input==27)){
			sigint_handler(0);
		}
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
		if( input=='?' || input==KEY_F(1)){
			help();
		}
		if( input=='!' || (input==KEY_F(2)||input=='!')){
			gameplay();
		}
	}
	move(EMPTY_LINES+view_len,0);
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
