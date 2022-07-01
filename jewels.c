/*
Jewels


Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


A pair of jewels appear on top of the window, And you can move and rotate them while they are falling down.
If you make a vertical or horizontal row of 4 jewels they will explode and add up to your score.
Like Tetris,You will lose the game when the center of the uppermost row is filled.

TODO make it like puyo puyo instead of the remake of what i poorly remembered*/
#include "common.h"
#define LEN 17
#define WID 19
#define DELAY 2
#define SAVE_TO_NUM 10

chtype board[LEN][WID];
chtype colors[6]={0};
chtype next1,next2;
byte jx,jy; //first jewel's position
byte kx,ky;//second jewel's position in relation to that of j
long score=0;
char* controls = "j,l-Move k-Rotate p-Pause q-Quit";

byte save_score(void){
	return fallback_to_home("jewels_scores",score,SAVE_TO_NUM);

}
void show_scores(byte playerrank){
	if(playerrank==FOPEN_FAIL){
		printf("Could not open scorefile.");
		abort();
	}
	if(playerrank == 0){
		char formername[60]={0};
		long formerscore=0;
		rewind(score_file);
		fscanf(score_file,"%*s : %*d\n");
		if ( fscanf(score_file,"%s : %ld\n",formername,&formerscore)==2){
			printf("\n*****CONGRATULATIONS!****\n");
			printf("     _____   You beat the\n");
			printf("   .'     |      previous\n");
			printf(" .'       |        record\n");
			printf(" |  .|    |            of\n");
			printf(" |.' |    |%14ld\n",formerscore);
			printf("     |    |       held by\n");
			printf("  ___|    |___%11s\n",formername);
			printf(" |            |\n");
			printf(" |____________|\n");
			printf("*************************\n");
		}
		
	}
	//scorefile is still open with w+
	char pname[60] = {0};
	long pscore=0;
	byte rank=0;
	rewind(score_file);
	printf("\n>*>*>Top %d<*<*<\n",SAVE_TO_NUM);
	while( rank<SAVE_TO_NUM && fscanf(score_file,"%s : %ld\n",pname,&pscore) == 2){
		if(rank == playerrank)
			printf(">>>");
		printf("%d) %s : %ld\n",rank+1,pname,pscore);
		++rank;
	}
	putchar('\n');
	fclose(score_file);
}
//apply gravity
bool fall(void){
	bool jfall,kfall,ret;
	jfall=kfall=ret=0;
	for(int y=LEN-1;y>0;--y){
		chtype c,d;
		for(int x=WID-1;x>=0;--x){
			c=board[y][x];
			d=board[y-1][x];
			if(!c && d){
				board[y-1][x]=0;
				board[y][x]=d;
				if(y-1==jy && x==jx)
					jfall=1;
				if((y-1==jy+ky) && (x==jx+kx))
					kfall=1;
				ret=1;
			}
		}
	}
	if(jfall&&kfall)
		++jy;
	else
		jy = LEN+1;
	return ret;
}
// rotate 90d clockwise in ky/x format
void clockwise(byte* y,byte* x){
		/*
		 o		x
		 x	xo	o    ox*/
		chtype fx,fy;
		if(*y){
			fy=0;
			fx=-*y;
		}
		if(*x){
			fx=0;
			fy=*x;
		}
		*y=fy;
		*x=fx;
}
			
//rtt jwls
bool rotate(void){//f:future
		if(jy>LEN)
			return 0;
		byte fy,fx;
		fy=ky;fx=kx;
		clockwise(&fy,&fx);
		if( jy+fy<0 || jy+fy>=LEN || jx+fx<0 || jx+fx>=WID )
			return 0;
		if(board[jy+fy][jx+fx])
			return 0;
		chtype a = board[jy+ky][jx+kx];
		board[jy+ky][jx+kx]=0;
		ky=fy;
		kx=fx;
		board[jy+ky][jx+kx]=a;
		return 1;
}
//mv jwls
bool jmove(byte dy,byte dx){
		if(jy>LEN)
			return 0;

		if(jx+dx>=WID || jx+dx<0 || jx+kx+dx>=WID ||jx+kx+dx<0 || jy+dx<0 ||jx+dx+kx<0)
			return 0;
		if( board[jy+ky+dy][jx+kx+dx] )
			if( !(jy+ky+dy == jy && jx+kx+dx==jx) )
				return 0;

		if( board[jy+dy][jx+dx])
				if(!(dx==kx && dy==ky))
					return 0;
		//still alive? 
		chtype a = board[jy][jx];
		chtype b = board[jy+ky][jx+kx];
		board[jy][jx]=0;
		board[jy+ky][jx+kx]=0;
		board[jy+dy][jx+dx]=a;
		board[jy+ky+dy][jx+kx+dx]=b;
		jy+=dy;jx+=dx;
		return 1;
}	
//scoring algorithm
bool explode(byte combo){
	bool ret =0;
	chtype c,uc;
	byte n;
	byte y,x;
	for(y=0;y<LEN;++y){
		c=uc=n=0;
		for(x=0;x<WID;++x){
			uc = c; 
			c  = board[y][x];
			if(c && c == uc){
				++n;
				if(n>=3 && x==WID-1){//the chain ends because the row ends
					++x;
					goto HrExplsn;
				}
			}
			else if(n>=3){
				HrExplsn:
				score+=n*10*(n-2)*combo;
				ret=1;
				for(;n>=0;--n)
					board[y][x-1-n]=0;
				n=0;
			}
			else
				n=0;
		}
	}
	for(x=0;x<WID;++x){
		c=uc=n=0;
		for(byte y=0;y<LEN;++y){
			uc=c;
			c = board[y][x];
			if(c && c == uc){
				++n;
				if(n>=3 && y==LEN-1){
					++y;
					goto VrExplsn;
				}
			}
			else if(n>=3){
				VrExplsn:
				score+=n*10*(n-2)*combo;
				ret=1;
				for(;n>=0;--n)
					board[y-1-n][x]=0;
				n=0;
			}
			else
				n=0;
		}
	}
	return ret;
}

//display
void draw(void){
	erase();
	int middle = (COLS/2-1)-(WID/2);
	chtype a=A_STANDOUT|' ';
	mvhline(LEN,middle-2,a,WID+4);
	mvvline(0,middle-1,a,LEN);
	mvvline(0,middle-2,a,LEN);
	mvvline(0,middle+WID,a,LEN);
	mvvline(0,middle+WID+1,a,LEN);
	mvprintw(0,0,"Score:%d",score);
	mvaddstr(1,0,"Next:");
	addch(next1);
	addch(next2);
	for(byte y=0;y<LEN;++y){
		for(byte x=0;x<WID;++x){
			chtype c = board[y][x];
			if(c)
				mvaddch(y,middle+x,c);
		}
	}
	mvaddstr(LINES-2,middle-5,controls);
	refresh();
}
int main(int argc,char** argv){
	if(argc>1){
		printf("This game doesn't take arguments");
	}
	initscr();
	cbreak();
	halfdelay(DELAY);
	noecho();
	curs_set(0);
	wnoutrefresh(stdscr);
	keypad(stdscr,1);
	int input;
	bool falls;
	byte stop=0 , combo;
	char jwstr[] = "*^~\"$V";
        if(has_colors()){
                start_color();
                use_default_colors();
                init_pair(1,COLOR_RED,-1);
                init_pair(2,COLOR_GREEN,-1);
                init_pair(3,COLOR_MAGENTA,-1);
                init_pair(4,COLOR_BLUE,-1);//array this thing
                init_pair(5,COLOR_YELLOW,-1);
                init_pair(6,COLOR_CYAN,-1);
                for(byte n=0;n<6;++n){
                        colors[n] = COLOR_PAIR(n+1);
                }
        }

        srand(time(NULL)%UINT_MAX);
        byte ran1= rand()%6;
        byte ran2= rand()%6;
        next1= colors[ran1]|jwstr[ran1];
        next2= colors[ran2]|jwstr[ran2];
        while(1){
                chtype a,b;
                a=board[0][WID/2];
                b=board[0][WID/2-1];
                if(a || b ){
                        goto Lose;
                }
                jy=ky=0;
                jx=WID/2;
                kx=-1;
                board[jy][jx]=next2;
                board[jy+ky][jx+kx]=next1;
                ran1= rand()%6;
                ran2= rand()%6;
                next1=colors[ran1]|jwstr[ran1];
                next2=colors[ran2]|jwstr[ran2];
                falls = 1;
                while(falls){
                        input = getch();

                        if(input != ERR)
                                stop+=1;

                        if( stop >= 10){
                                falls=fall();
                                stop=0;
                        }
                        else if(input=='l' || (input==KEY_RIGHT||input=='d'))
                                jmove(0,+1);
                        else if(input=='j' || (input==KEY_LEFT||input=='a') )
                                jmove(0,-1);
                        else if(input=='k' || (input==KEY_UP||input=='w'))
                                rotate();
                        else if(input=='p'){
                                mvaddstr(LINES-2,COLS/2-15,"Paused - Press a key to continue   ");
                                refresh();
                                nocbreak();
                                cbreak();
                                getch();
                                halfdelay(DELAY);
                        }
                        else if((input=='q'||input==27))
                                goto Lose;
                        else if(input==' ')
                                while( (falls=fall()) )
                                        stop=0;
                        else{
                                falls=fall();
                                stop=0;
                        }
                        draw();
                 }
                combo=1;
                while(explode(combo)){ // explode, fall, explode, fall until nothing is left        
                        ++combo;
                        while(fall());
                        draw();
                }
        }
        Lose:
        nocbreak();
        endwin();
        printf("%s _Jewels_ %s\n",jwstr,jwstr);
        printf("You have scored %ld points.\n",score);
        show_scores(save_score());
        return EXIT_SUCCESS;
}
