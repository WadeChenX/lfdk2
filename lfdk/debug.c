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
int k_log_fd = 0;
char k_buf[256];

char *msg_name(MESSAGE MSG)
{
        switch(MSG) {
                case MSG_NEED_FOCUS: 
                        return "MSG_NEED_FOCUS";
                case MSG_DESTROY_WINDOW:
                        return "MSG_DESTROY_WINDOW";
                case MSG_RELEASE_FOCUS:
                        return "MSG_RELEASE_FOCUS";
                case MSG_XFER_CONTROL:
                        return "MSG_XFER_CONTROL";
                default:
                        return "MSG_UNKNOWN";
        }
        return NULL;
}

int debug_init(st_cmd_info *p_cmd)
{
        debug_fd = fopen(DEFAULT_LOG_FILE, "wb") ;
        if (!debug_fd) 
                return ERR_OPEN_FILE;

        k_log_fd = p_cmd->fd_lfdd;
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

int debug_k_log(int lv, const char *fmt, ...)
{
        int ret = 0;

        if (k_log_fd) {
                if (lv <= win_manager.cmd_info.debug_lv) {
                        va_list ap;

                        va_start(ap, fmt);
                        ret = vsnprintf(k_buf, sizeof(k_buf), fmt, ap);
                        va_end(ap);

                        ret = write(k_log_fd, k_buf, sizeof(k_buf));
                        if (ret != sizeof(k_buf)) {
                                goto err_out;
                        }
                        ret = 0;
                }
        }
err_out:
        return ret;
}

int debug_exit(st_cmd_info *p_cmd)
{
        if (debug_fd)
                fclose(debug_fd);
        k_log_fd = 0;
        return 0;
}
