/* 
Authored by abakh <abakh@tuta.io>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include "config.h"
#define FOPEN_FAIL -10
#define ENV_VAR_OR_USERNAME (getenv("NB_PLAYER")?getenv("NB_PLAYER"):getenv("USER"))
#define MAXPATHSIZE 1000
FILE* score_file;
byte score_write(const char* path, long wscore, byte save_to_num){// only saves the top 10, returns the place in the chart
	score_file=fopen(path,"r");
	if(!score_file){
		score_file=fopen(path,"w");
		if(!score_file){
			return FOPEN_FAIL;
		}
	}
	#ifdef NO_VLA 
		#define save_to_num_ 10
	#else //such a dirty cpp hack
		byte save_to_num_=save_to_num;
	#endif
	char name_buff[save_to_num_][60];
	long score_buff[save_to_num_];
    char tmp_path[MAXPATHSIZE + 8] = {0};
    strcpy(tmp_path, path);
    strcat(tmp_path, ".XXXXXX");

	memset(name_buff,0,save_to_num_*60*sizeof(char) );
	memset(score_buff,0,save_to_num_*sizeof(long) );

	long scanned_score =0;
	char scanned_name[60]={0};
	byte location=0;

	while( fscanf(score_file,"%59s : %ld\n",scanned_name,&scanned_score) == 2 && location<save_to_num){
		strcpy(name_buff[location],scanned_name);
		score_buff[location] = scanned_score;
		++location;//so it doesn't save more scores than intented

		memset(scanned_name,0,60);
		scanned_score=0;
	}
    FILE* tmp_score_file = fdopen(mkstemp(tmp_path), "w");
	if(!tmp_score_file){
		return FOPEN_FAIL;
	}
	byte scores_count=location;//if 5 scores were scanned, it is 5. the number of scores it reached
	byte ret = -1;
	bool wrote_it=0;

	for(byte i=0;i<=scores_count && i<save_to_num_-wrote_it;++i){
		if(!wrote_it && (i>=scores_count || wscore>=score_buff[i]) ){
			fprintf(tmp_score_file,"%s : %ld\n",ENV_VAR_OR_USERNAME,wscore);
			ret=i;
			wrote_it=1;
		}
		if(i<save_to_num_-wrote_it && i<scores_count){
			fprintf(tmp_score_file,"%s : %ld\n",name_buff[i],score_buff[i]);
		}
	}
	fflush(tmp_score_file);
    fclose(score_file);
    if (rename(tmp_path, path) < 0) {
        return FOPEN_FAIL;
    }
    fclose(tmp_score_file);
    score_file=fopen(path,"r");
    if (!score_file) {
        return FOPEN_FAIL;
    }
	return ret;
}

byte fallback_to_home(const char* name,long wscore,byte save_to_num){// only saves the top 10, returns the place in the chart
	byte ret;
	char full_path[MAXPATHSIZE]={0};
	if(getenv("NB_SCORES_DIR")){
		snprintf(full_path,MAXPATHSIZE,"%s/%s",getenv("NB_SCORES_DIR"),name);
		ret=score_write(full_path,wscore,save_to_num);
		if(ret==FOPEN_FAIL){
			return ret;//do not fallback this
		}
	}
	snprintf(full_path,MAXPATHSIZE,"%s/%s",SCORES_DIR,name);
	ret=score_write(full_path,wscore,save_to_num);
	if(ret==FOPEN_FAIL){
		snprintf(full_path,MAXPATHSIZE,"%s/.%s",getenv("HOME"),name);
		ret=score_write(full_path,wscore,save_to_num);
	}
	return ret;
}

byte digit_count(int num){
	byte ret=0;
	do{
		++ret;
		num/=10;
	}while(num);
	return ret;
}

