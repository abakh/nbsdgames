#include <curses.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#define LIGHT -1
#define DARK 1
#define KING 2
#define DOESNT_MATTER 1
#define IMAGINARY 0
#define NORMAL 1
#define ALT_IMG 2
#define ALT_NRM 3
#define WIN 100000
/* 
 .-.
|  '
'._.HECKERS

copyright Hossein Bakhtiarifar 2018 (c)
No rights are reserved and this software comes with no warranties of any kind to the extent permitted by law.


Compile with -lncurses
*/
typedef signed char byte;
byte py,px;//cursor
byte cy,cx;//selected(choosen) piece
int dpt;
byte game[8][8];
byte computer[2]={0,0};
byte score[2];//set by header()
bool endgame=false;
byte jumpagainy , jumpagainx;
bool kinged;//if a piece jumps over multiple others and becomes a king it cannot continue jumping
void rectangle(byte sy,byte sx){
	byte y,x;
	for(y=0;y<=8+1;y++){
		mvaddch(sy+y,sx,ACS_VLINE);
		mvaddch(sy+y,sx+8*2,ACS_VLINE);
	}
	for(x=0;x<=8*2;x++){
		mvaddch(sy,sx+x,ACS_HLINE);
		mvaddch(sy+8+1,sx+x,ACS_HLINE);
	}
	mvaddch(sy,sx,ACS_ULCORNER);
	mvaddch(sy+8+1,sx,ACS_LLCORNER);
	mvaddch(sy,sx+8*2,ACS_URCORNER);
	mvaddch(sy+8+1,sx+8*2,ACS_LRCORNER);
}
void header(void){
	score[0]=score[1]=0;
	byte y,x;
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			if(game[y][x]){
				if(game[y][x]<0)
					score[0]++;
				else
					score[1]++;
			}
		}
	}
	mvprintw(0,0," .-.");
	mvprintw(1,0,"|  '  %2d:%2d",score[0],score[1]);
	mvprintw(2,0,"'._,HECKERS ");
}
void draw(byte sy,byte sx){//the game's board
	rectangle(sy,sx);
	chtype ch ;
	byte y,x;
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			ch=A_NORMAL;
			if(y==py && x==px)
				ch |= A_STANDOUT;
			if(y==cy && x==cx)
				ch |= A_BOLD;
			if(game[y][x]){
				if(game[y][x]<0){
					if(has_colors())
						ch|=COLOR_PAIR(1);
					else
						ch |= A_UNDERLINE;
				}
				if(abs(game[y][x])<2)
					ch |='O';
				else
					ch |='K';
			}
			else if( (y%2) != (x%2) )                        
				ch|='.';
			else
				ch|=' ';
			mvaddch(sy+1+y,sx+x*2+1,ch);
		}
	}
}
void fill(void){
	byte y,x;
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			game[y][x]=0;
			if( (y%2) != (x%2)){
				if(y<3) game[y][x]=1;
				if(y>4) game[y][x]=-1;
			}
		}
	}
}
bool in(byte A[4],byte B[4],byte a,byte b){
	for(byte c=0;c<4;c++)
		if(A[c]==a && B[c]==b)
			return true;
	return false;
}
bool moves(byte ty,byte tx,byte mvy[4],byte mvx[4]){
	bool ret=0;
	byte ndx=0;
	byte t= game[ty][tx];
	move(15,0);
	byte dy,dx;
	for(dy=-1;dy<2;dy++){
		for(dx=-1;dx<2;dx++){
			if( !dy || !dx || (!ty && dy<0) || (!tx && dx<0) || (dy==-t) || (ty+dy>=8) || (tx+dx>=8) )
				;
			else if(!game[ty+dy][tx+dx]){
				ret=1;
				mvy[ndx]=ty+dy;
				mvx[ndx]=tx+dx;
				ndx++;
			}
			else
				ndx++;
		}
	}
	return ret;
}
bool can_move(byte side){//would be much faster than applying moves()  on every tile
	byte y , x ,t, dy , dx;
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			if( (t=game[y][x])*side > 0 ){
				for(dy=-1;dy<2;dy++){
					for(dx=-1;dx<2;dx++){
						if( !dy || !dx || (!y && dy<0) || (!x && dx<0) || (dy==-t) || (y+dy>=8) || (x+dx>=8) )
							;
						else if( !game[y+dy][x+dx] )
							return 1;
					}
				}
			}
		}
	}
	return 0;
	
}
bool jumps(byte ty,byte tx,byte mvy[4],byte mvx[4]){
	bool ret=0;
	byte ndx=0;
	byte ey,ex;
	byte t= game[ty][tx];
	byte dy,dx;
	for(dy=-1;dy<2;dy++){
		for(dx=-1;dx<2;dx++){
			ey = dy*2;
			ex = dx*2;
			if(!dy || !dx ||(dy==-t)|| (ty+ey<0) || (tx+ex<0) || (ty+ey>=8) || (tx+ex>=8) )
				;
			else if(!game[ty+ey][tx+ex] && game[ty+dy][tx+dx]*t<0){
				ret=1;
				mvy[ndx]=ty+ey;
				mvx[ndx]=tx+ex;
				ndx++;
			}
			else
				ndx++;
		}
	}
	return ret;
}
byte can_jump(byte ty,byte tx){
	byte dy,dx,t=game[ty][tx];
	byte ey,ex;
	byte ret=0;
	for(dy=-1;dy<2;dy++){
		for(dx=-1;dx<2;dx++){
			ey=dy*2;
			ex=dx*2;
			if((dy==-t)||(ty+ey<0)||(tx+ex<0)||(ty+ey>=8)||(tx+ex>=8) )
				;
			else if(!game[ty+dy*2][tx+dx*2]&&game[ty+dy][tx+dx]*t<0){
				ret++;
				if(ret>1)
					return ret;
			}
			
		}
	}
	return ret;
}
byte forced_jump(byte side){
	byte y,x;
	byte foo,ret;
	foo=ret=0;
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			if(game[y][x]*side>0 && (foo=can_jump(y,x)) )
				ret+=foo;
			if(ret>1)
				return ret;
		}
	}	
	return ret;
}
byte cmove(byte fy,byte fx,byte sy,byte sx){//really move/jump , move is a curses function
	byte a = game[fy][fx];
	byte ret=0;
	game[fy][fx]=0;
	game[sy][sx]=a;
	if(abs(fy-sy) == 2){
		ret =game[(fy+sy)/2][(fx+sx)/2]; 
		game[(fy+sy)/2][(fx+sx)/2]=0;
	}
	return ret;
}
bool king(byte y,byte x){
	byte t= (4-y)*game[y][x];
	if( (y==7 || !y) && t<0 && t>-5 ){
			game[y][x]*=2;
			return 1;
	}
	return 0;
}
double advantage(byte side){
	unsigned char own,opp;
	own=opp=0;
	byte foo;
	byte y,x;
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			foo=game[y][x]*side;
			if(foo>0){
				own++;//so it wont sacrfice two pawns for a king ( 2 kings == 3 pawns)
				own+=foo;
			}
			else if(foo<0){
				opp++;
				opp-=foo;
			}
		}
	}
	if(!own)
		return 0;
	else if(!opp)
		return WIN;
	else
		return (double)own/opp;
}
double posadvantage(byte side){
	double adv=0;
	double oppadv=0;
	byte foo;
	byte y,x;
	byte goal= (side>0)*7 , oppgoal=(side<0)*7;
	/*This encourages the AI to king its pawns and concentrate its kings in the center.
	The idea is : With forces in the center, movements to all of the board would be in the game tree horizon(given enough depth);
	and with forces being focused , its takes less movements to make an attack. */
	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			foo=game[y][x]*side;
			if(foo>0){
				adv+=foo;
				adv++;
				if(foo==1)
					adv+= 1/( abs(y-goal) );//adding positional value 
				else if(foo==2)
					adv+= 1/( fabs(y-3.5)+ fabs(x-3.5) );
			}
			else if( foo<0 ){
				oppadv-=foo;
				oppadv++;
				if(foo==-1)
					adv+=1/( abs(y-oppgoal) );
				else if(foo==-2)
					adv+= 1/( fabs(y-3.5)+ fabs(x-3.5) );
			}
		}
	}
	if(!adv)
		return 0;
	else if( !oppadv ) 
		return WIN;
	else
		return adv/oppadv;
	return adv;
}
double decide(byte side,byte depth,byte s){
	byte fj=forced_jump(side);//only one legal jump if returns 1
	byte nextturn;

	byte mvy[4],mvx[4];
	byte n;
	
	bool didking;
	byte captured;

	double adv=0;
	byte toy,tox;
	byte y,x;

	double wrstadv=WIN+1;

	double bestadv=0;
	byte besttoy,besttox;
	byte besty,bestx;
	bestx=besty=besttox=besttoy=-100;
	bool canmove=0;
	
	byte nexts ;
	if(s == IMAGINARY || s == NORMAL )
		nexts=IMAGINARY;
	else
		nexts=ALT_IMG;

	for(y=0;y<8;y++){
		for(x=0;x<8;x++){
			if(fj && (s==NORMAL || s==ALT_NRM) && jumpagainy>=0 && (jumpagainy!=y || jumpagainx!=x) )
				continue;
			if(game[y][x]*side>0){
				canmove=0;
				memset(mvy,-1,4);
				memset(mvx,-1,4);
				if(fj)
					canmove=jumps(y,x,mvy,mvx);
				else
					canmove=moves(y,x,mvy,mvx);
				if(canmove){
					for(n=0;n<4;n++){
						if(mvy[n] != -1){//a real move
							toy=mvy[n];
							tox=mvx[n];
							captured=cmove(y,x,toy,tox);//do the imaginary move
							if(fj && can_jump(toy,tox) ) //its a double jump
								nextturn=side;
							else
								nextturn=-side;
							didking=king(toy,tox);
							
							//see the advantage you get
							if(fj==1 && (s==ALT_NRM || s==NORMAL) )
								adv= DOESNT_MATTER;
							else if(!depth){
								if(s==IMAGINARY || s==NORMAL)
									adv=advantage(side);
								else
									adv=posadvantage(side);
							}
							else{
								if(nextturn==side)
									adv=decide(nextturn,depth,nexts);
								else{
									adv=decide(nextturn,depth-!fj,nexts);
									if(adv==WIN)
										adv=0;
									else if(adv)
										adv=1/adv;
									else
										adv=WIN;
								}
							}
							//undo the imaginary move
							if(didking)
								game[toy][tox]/=2;
							game[y][x]=game[toy][tox];
							game[toy][tox]=0;
							if(fj)
								game[(toy+y)/2][(tox+x)/2]=captured;

							if(besty<0 || adv>bestadv || (adv==bestadv && ( random()%2 )) ){
								besty=y;
								bestx=x;
								besttoy=toy;
								besttox=tox;
								bestadv=adv;
							}
							if(adv<wrstadv)
								wrstadv=adv;
							if(fj == 1)
								goto EndLoop;
						}
					}	
				}
			}
		}
	}
	EndLoop:
	if( (s==NORMAL || s==ALT_NRM) && besty >= 0 ){
		if(endgame && fj!=1 && s==NORMAL && bestadv==wrstadv ){
			if(wrstadv == WIN){//the randomization in the algorithm may cause an illusion of inevitable win
				if(depth > 1)
					decide(side,depth-1,NORMAL);
				else
					goto Move;
			}
			else
				decide(side,depth,ALT_NRM);
		}
		else{
			Move:
			cmove(besty,bestx,besttoy,besttox);
			kinged=king(besttoy,besttox);
			if(!kinged && can_jump(besttoy,besttox) ){
				jumpagainy = besttoy;
				jumpagainx = besttox;
			}
			else
				jumpagainy=jumpagainx=-1;
		}
	}
	return bestadv;
}
void sigint_handler(int x){
	endwin();
	puts("Quit.");
	exit(x);
}
int main(int argc,char** argv){
	dpt=3;
	if(argc>2){
		printf("Usage: %s [AIpower]\n",argv[0]);
		return EXIT_FAILURE;
	}
	if(argc==2){
		if(sscanf(argv[1],"%d",&dpt) && dpt<128 && dpt>0)
			;
		else{
			puts("That should be a number from 1 to 127.");
			return EXIT_FAILURE;
		}
	}
	initscr();
	noecho();
	cbreak();
	keypad(stdscr,1);
	int input ;
	printw("Black plays first.\n Choose type of the black player(H/c)\n" );
	input=getch();
	if(input=='c'){
		computer[0]=dpt;
		printw("Computer.\n");
	}
	else{
		computer[0]=0;
		printw("Human.\n");
	}
	printw("Choose type of the white player(h/C)\n");
	input=getch();
	if(input=='h'){
		computer[1]=0;
		printw("Human.\n");
	}
	else{
		computer[1]=dpt;
		printw("Computer.\n");
	}
	if(has_colors()){
		start_color();
		use_default_colors();
		init_pair(1,COLOR_RED,-1);
	}
	signal(SIGINT,sigint_handler);
	Start:
	srandom(time(NULL)%UINT_MAX);
	fill();
	cy=cx=-1;
	py=px=0;
	byte mvy[4],mvx[4];
	memset(mvy,-1,4);
	memset(mvx,-1,4);
	byte turn=1;
	bool t=1;
	bool fj;
	byte result;
	byte todraw=0;
	double adv = 1;//used to determine when the game is a draw
	double previousadv =1;	
	Turn:
	jumpagainy=jumpagainx=-1;
	kinged=0;
	turn =-turn;
	t=!t;//t == turn<0 that's turn in binary/array index format
	fj = forced_jump(turn);
	erase();
	flushinp();
	header();
	draw(3,0);
	if(t){
		previousadv=adv;
		adv= advantage(1) + (score[0]*score[1]);//just taking the dry scores to account too,nothing special
		if(previousadv==adv)
			todraw++;
		else 
			todraw=0;
	}
	if(!score[0] || (turn==-1 && !fj && !can_move(-1))){
		result=1;
		goto End;
	}
	else if(!score[1] || (turn==1 && !fj && !can_move(1))){
		result=-1;
		goto End;
	}
	else if(todraw==50){ // 50 turns without any gained advantage for each side
		result=0;
		goto End;
	}
	endgame= score[t]<=5 || score[!t]<=5;
	draw(3,0);
	refresh();
	while(computer[t]){
		mvprintw(13,0,"Thinking...");
		refresh();
		computer[t]=dpt+ (score[!t] != score[t]) + endgame;
		decide(turn,computer[t],1);
		if(!(fj && jumpagainy>=0 && !kinged )){
			goto Turn;
		}
	}
	while(1){
		erase();
		draw(3,0);
		header();
		refresh();
		input=getch();
		if( (input=='k' || input==KEY_UP) && py>0)
			py--;
		if( (input=='j' || input==KEY_DOWN) && py<7)
			py++;
		if( (input=='h' || input==KEY_LEFT) && px>0)
			px--;
		if( (input=='l' || input==KEY_RIGHT) && px<7)
			px++;
		if( input=='q'){
			result=2;
			goto End;
		}
		if(input=='\n'){
			if(game[py][px]*turn>0){
				cy=py;
				cx=px;
				memset(mvy,-1,4);
				memset(mvx,-1,4);
				if(!fj)
					moves(py,px,mvy,mvx);
				jumps(py,px,mvy,mvx);
			}
			if( in(mvy,mvx,py,px) && !(jumpagainy>=0 && (cy !=jumpagainy || cx != jumpagainx) ) ){
				memset(mvy,-1,4);
				memset(mvx,-1,4);
				cmove(cy,cx,py,px);
				kinged=king(py,px);
				cy=-1;
				cx=-1;
				if( !(fj && can_jump(py,px) && !kinged ) ){
					goto Turn;
				}
				else{
					jumpagainy=py;
					jumpagainx=px;
				}
			}
		}
	}
	End:
	move(13,0);
	switch(result){
		case -1:
			printw("The black side has won the game.");
			break;
		case 0:
			printw("Draw.");
			break;
		case 1:
			printw("The white side has won the game.");
			break;
		case 2:
			printw("You resigned.");
	}
	printw(" Wanna rematch?(y/N)");
	curs_set(1);
	input=getch();
	if(input=='y' || input=='Y'){
		byte b=computer[0];
		computer[0]=computer[1];
		computer[1]=b;
		goto Start;
	}
	endwin();
	return EXIT_SUCCESS;
}
