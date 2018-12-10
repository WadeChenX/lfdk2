#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include "lfdk.h"

#define DEFAULT_LOG_FILE "lfdk.log"

extern st_windows_manager_info win_manager;

FILE *debug_fd = NULL;

int debug_init()
{
        debug_fd = fopen(DEFAULT_LOG_FILE, "wb") ;
        if (!debug_fd) 
                return ERR_OPEN_FILE;
        return 0;
}

int debug_log(int lv, const char *fmt, ...)
{
        int ret = 0;

        if (debug_fd) {
                if (lv <= win_manager.cmd_info.debug_lv) {
                        va_list ap;
                        va_start(ap, fmt);
                        ret = vfprintf(debug_fd, fmt, ap);
                        va_end(ap);

                        /* Don't pass back error results.  */
                        if (ret < 0) {
                                ret = 0;
                        }
                }
        }
        return ret;
}

int debug_exit()
{
        if (debug_fd)
                fclose(debug_fd);
        return 0;
}
