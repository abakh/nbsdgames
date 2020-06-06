#ifdef Plan9
    #define PP_SCORES "/sys/lib/games/pp_scores"
    #define JW_SCORES "/sys/lib/games/jw_scores"
    #define FSH_SCORES "/sys/lib/games/fsh_scores"
    #define MNCH_SCORES "/sys/lib/games/mnch_scores"
    #define MT_SCORES "/sys/lib/games/mt_scores"
#else
    #define PP_SCORES "/usr/games/pp_scores"
    #define JW_SCORES "/usr/games/jw_scores"
    #define FSH_SCORES "/usr/games/fsh_scores"
    #define MNCH_SCORES "/usr/games/mnch_scores"
    #define MT_SCORES "/usr/games/mt_scores"
#endif
//for easier access
