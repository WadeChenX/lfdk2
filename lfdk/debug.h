#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "lfdk.h"

typedef enum debug_level {
        NO_DEBUG = 0,
        DEBUG_CRIT,
        DEBUG_ERROR,
        DEBUG_WARN,
        DEBUG_INFO,
        DEBUG_VERBOSE,
        DEBUG_MAX,
} DEBUG_LVL;

int debug_init();
int debug_log(int lv, const char *fmt, ...);
int debug_exit();
char *msg_name(MESSAGE MSG);

#define _log_c(...) \
        debug_log(DEBUG_CRIT, __VA_ARGS__)

#define _log_e(...) \
        debug_log(DEBUG_ERROR, __VA_ARGS__)

#define _log_w(...) \
        debug_log(DEBUG_WARN, __VA_ARGS__)

#define _log_i(...) \
        debug_log(DEBUG_INFO, __VA_ARGS__)

#define _log_v(...) \
        debug_log(DEBUG_VERBOSE, __VA_ARGS__)

#ifndef TAG
#define TAG "<>"
#endif

#define log_c(...) \
        debug_log(DEBUG_CRIT, TAG ": " __VA_ARGS__)

#define log_e(...) \
        debug_log(DEBUG_ERROR, TAG ": " __VA_ARGS__)

#define log_w(...) \
        debug_log(DEBUG_WARN, TAG ": " __VA_ARGS__)

#define log_i(...) \
        debug_log(DEBUG_INFO, TAG  ": "  __VA_ARGS__)

#define log_v(...) \
        debug_log(DEBUG_VERBOSE, TAG  ": "  __VA_ARGS__)

#endif //__DEBUG_H__
