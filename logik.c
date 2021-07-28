#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#define NBHOLES  5
#define NBLINES    10
#define NBCOLORS 8
#define NBMINUS  5
unsigned char pion[] = "rgypmRGY";

int
convert(unsigned char a) {

	for(int i=0; i<NBCOLORS; i++)
		if( a == pion[i] ) 
			return i;
	return -1;
}

int
colorize(int a) {
	return a % NBMINUS + 31;
}

int
bold(int a) {
	return a <= 'Z'; 
}


int 
pastel(unsigned char a) {
	if( convert(a) < 0 )
		return -1;

#ifdef BGCOLOR
	/* if you want bg color define the macro BGCOLOR: */
	printf("\033[0;%dm%c\033[0m",  bold(a)*10 + colorize(convert(a)), a);
#else
	printf("\033[%d;%dm%c\033[0m",  bold(a), colorize(convert(a)), a);
#endif
	return 1;
}

int
main(void) {
	
	printf("\033[22;34m====================\n\r       LOGIK        \n\r====================\033[0m\n");

	printf("\rcolors\t");
	for( int i=0; i<NBCOLORS; i++)
		pastel( pion[i] );
	printf("\n\rx is backspace\n\rQQ to exit");


	int sol[NBHOLES], mix[NBCOLORS];
	srand( getpid() * ( 1 + getppid() ) + time(NULL) );
	for(int i=0; i<NBMINUS; i++) 
		sol[i] = rand() % NBCOLORS;	
	printf("\r");

	for(int i=0; i<NBCOLORS; i++)
		mix[i]=0;

	for(int i=0; i<NBHOLES; i++)
		mix[sol[i]]++;

	/* ----- */
	char c;
	char inc[NBHOLES], stop=0;
	int black, white;
	int tmp[NBCOLORS];
	system("/bin/stty raw");
	for(int l=1; l <= NBLINES; l++) { 
		printf("\n\r%d\t",l); /*due to stty raw*/
		for( int i=0;  i < NBHOLES ; ) {
			c = getchar();
			if( pastel(c) > 0 ) {
				inc[i] = convert(c);
				i++;
				stop = 0;
			}
			if( i > 0 & c == 'x' ) { /*aimed to backspace*/
				i--; 
				printf("\b");
				stop = 0;
			}
			if( c == 'Q' ) {
				stop++;
				if( stop == 2) 
					break;
			}
		}
		printf("\t");

		if( stop == 2 ) 
			break;

		black=0; white=0;
		/* black section : x : is in the right place */
		for(int i=0; i<NBHOLES; i++)
			if( inc[i] == sol[i] ) 
				black++;

		/* white section : o : the color is right but placed */
		for(int i=0; i<NBCOLORS; i++)
			tmp[i]=0;

		for(int i=0; i<NBHOLES; i++) 
			tmp[(int) inc[i]] += 1;

		white = 0;
		for(int i=0; i<NBCOLORS; i++)
			if(  (mix[i] > 0)  & (tmp[i] > 0) ) {
				if( tmp[i] <= mix[i] )
					white += tmp[i];
				else if ( mix[i] < tmp[i] )
					white += mix[i]; 
			}

		/* fmt */
		for(int i=0; i < NBHOLES - white ; i++)
			printf("-");

		for(int i=0; i < black; i++)
			printf("x");

		for(int i=0; i < white - black; i++)
			printf("o");

		if( black == 5 ) {
			system("/bin/stty cooked");
			printf("\n\tYOU WIN\n");
			return 0;
		}
		for(int i=0; i<NBHOLES; inc[i++]=-1)
			;
	}
	system("/bin/stty cooked");
	printf("\n\rYOU LOOSE\n\r");

	for(int i=0; i<NBHOLES; i++)
		pastel(pion[sol[i]]);
	printf("\n");
	return 0;
}
