//for easier access

//the default scorefiles
#ifdef Plan9
    #define PP_SCORES "/sys/lib/games/pp_scores"
    #define JW_SCORES "/sys/lib/games/jw_scores"
    #define FSH_SCORES "/sys/lib/games/fsh_scores"
    #define MNCH_SCORES "/sys/lib/games/mnch_scores"
    #define MT_SCORES "/sys/lib/games/mt_scores"
    #define DRT_SCORES "/sys/lib/games/drt_scores"
#else
    #define PP_SCORES "/usr/games/pp_scores"
    #define JW_SCORES "/usr/games/jw_scores"
    #define FSH_SCORES "/usr/games/fsh_scores"
    #define MNCH_SCORES "/usr/games/mnch_scores"
    #define MT_SCORES "/usr/games/mt_scores"
    #define DRT_SCORES "/usr/games/drt_scores"
#endif

#ifdef Plan9
	#define NO_VLA
	//Many ancient compilers don't have VLA support, including the Plan9 compiler
	//thought it would be nicer if it had its own flag instead of Plan9.
#endif


//#define NO_MOUSE
//it seems there wasn't mouse support in original curses, and the variants
//developed it indepedently, use if mouse doesn't work in your variant (e.g. BSD curses)



#include <stdlib.h>
#include <unistd.h>
#ifdef __unix__
	#define rand() random()
	#define srand(x) srandom(x)
	//At the time of writing, NetBSD's rand() is damn stupid.
	//rand()%4 constantly gives 0 1 2 3 0 1 2 3 0 1 2 3 0 1 2 3.
#endif

// It seems usleep is obsoleted in favor of nanosleep, 
// and some POSIX implementations lack it. but i refuse 
// to end using it! what the hell, filling a struct for
// something as trivial as sleeping 0.1 seconds??

// the function is written by Jens Staal for Plan9
#ifdef Plan9
	int microsleep(long usec){
		int second = usec/1000000;
		long nano = usec*1000 - second*1000000;
		struct timespec sleepy = {0};
		sleepy.tv_sec = second;
		sleepy.tv_nsec = nano;
		nanosleep(&sleepy, (struct timespec *) NULL);
		return 0;
	}
	#define usleep(x) microsleep(x)
#endif
