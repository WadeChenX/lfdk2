/*
 * LFDK - Linux Firmware Debug Kit
 * File: lfdk.h
 *
 * Copyright (C) 2006 - 2010 Merck Hung <merckhung@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LFDK_H__
#define __LFDK_H__

#include <stdint.h>

#include "list.h"


#define ERROR_MIN  -32
typedef enum error {
        ERR_BUF_OVERFLOW = ERROR_MIN,
        ERR_OPEN_DEV,
        ERR_OPEN_FILE,
        ERR_REQ_IO,
        ERR_INVALID_PARAM,
} error_t;


#define LFDK_VERSION            "2.0.2"
#define LFDK_PROGNAME           "lfdk"
#define LFDK_VERTEXT            LFDK_PROGNAME" version "LFDK_VERSION", Linux Firmware Debug Kit"
#define LFDK_MAX_PATH           40
#define LFDK_MAX_READBUF        512
#define LFDK_BYTE_PER_LINE		16

#define LFDD_IOCTL( FDESC, IOCTL_CMD, DATA ) {              \
                                                            \
    if( ioctl( FDESC, IOCTL_CMD, &DATA ) ) {                \
                                                            \
        endwin();                                           \
        fprintf( stderr, "Cannot execute command\n\n" );    \
        exit( 1 );                                          \
    }                                                       \
}

#define PrintWin( RESRC, NAME, LINE, COLUMN, X, Y, COLORPAIR, FORMAT, ARGS... ) {   \
                                                                                    \
    RESRC.NAME = newwin( LINE, COLUMN, X, Y );                                      \
    RESRC.p_##NAME = new_panel( RESRC.NAME );                                       \
    wbkgd( RESRC.NAME, COLOR_PAIR( COLORPAIR ) );                                   \
    wattrset( RESRC.NAME, COLOR_PAIR( COLORPAIR ) | A_BOLD );                       \
    wprintw( RESRC.NAME, FORMAT, ##ARGS );                                          \
    wattrset( RESRC.NAME, A_NORMAL );                                               \
}


#define PrintFixedWin( RESRC, NAME, LINE, COLUMN, X, Y, COLORPAIR, FORMAT, ARGS... ) {  \
                                                                                        \
    if( !RESRC.NAME ) {                                                                 \
                                                                                        \
        RESRC.NAME = newwin( LINE, COLUMN, X, Y );                                      \
        RESRC.p_##NAME = new_panel( RESRC.NAME );                                       \
    }                                                                                   \
    wbkgd( RESRC.NAME, COLOR_PAIR( COLORPAIR ) );                                       \
    wattrset( RESRC.NAME, COLOR_PAIR( COLORPAIR ) | A_BOLD );                           \
    mvwprintw( RESRC.NAME, 0, 0, FORMAT, ##ARGS );                                      \
    wattrset( RESRC.NAME, A_NORMAL );                                                   \
}


#define DestroyWin( RESRC, NAME ) {     \
                                        \
    if( RESRC.p_##NAME ) {              \
                                        \
        del_panel( RESRC.p_##NAME );    \
        RESRC.p_##NAME = NULL;          \
    }                                   \
                                        \
    if( RESRC.NAME ) {                  \
                                        \
        delwin( RESRC.NAME );           \
        RESRC.NAME = NULL;              \
    }                                   \
}

enum {

    WHITE_RED = 1,
    WHITE_BLUE,
    BLACK_WHITE,
    CYAN_BLUE,
    RED_BLUE,
    YELLOW_BLUE,
    BLACK_GREEN,
    BLACK_YELLOW,
    YELLOW_RED,
    YELLOW_BLACK,
    WHITE_YELLOW
};


typedef enum win_state {
        WM_NOT_START = 0,
        WM_INITED,
        WM_STARTED,
        WM_POST_STARTED,
        WM_DISABLED,
        WM_BACKGROUND,
        WM_FOREGROUND,
        WM_DESTROYED,
        WM_EXIT,
} WIN_STATE;

typedef enum message {
        MSG_NO_USED = 0,
        MSG_NEED_FOCUS,
        MSG_RELEASE_FOCUS,
        MSG_DESTROY_WINDOW,
} MESSAGE;


typedef struct {
        int fd_lfdd;
        int debug_lv;
} st_cmd_info;

typedef struct {
        char name[32];
        uint32_t short_key;
        int (*init)(st_cmd_info *p_cmd, void *data);
        int (*start_win)(st_cmd_info *p_cmd, void *data);
        int (*post_start_win)(st_cmd_info *p_cmd, void *data);
        int (*lost_focus)(st_cmd_info *p_cmd, void *data);
        int (*get_focus)(st_cmd_info *p_cmd, void *data);
        int (*paint)(st_cmd_info *p_cmd, void *data);
        int (*key_press)(st_cmd_info *p_cmd, void *data);
        int (*destroy_win)(st_cmd_info *p_cmd, void *data);
        int (*module_exit)(st_cmd_info *p_cmd, void *data);
}st_window_info;

typedef struct {
        st_window_info *p_win;
        int win_state;
} st_win_info;

typedef struct message_info {
        MESSAGE msg;
        int sender_handle;
        void *data;
}msg_info;

#define WINDOWS_POOL_SIZE  32
#define MSG_SIZE 32
typedef struct windows_manager_info {
        st_cmd_info  cmd_info;
        st_win_info windows_pool[WINDOWS_POOL_SIZE];
        int windows_used_len;
        msg_info   msg_box[MSG_SIZE];
        int msg_used_len;
        int cur_fore_window_handle;
} st_windows_manager_info;


int register_windows(st_window_info *p_win);
int request_windows_focus(st_window_info *p_win, int handle);
int request_destroy_windows(st_window_info *p_win, int handle);
WIN_STATE request_window_state(st_window_info *p_win, int handle);
int release_windows_focus(st_window_info *p_win, int handle);

#define module_init(info, handle)                                                 \
static void __attribute__((constructor)) do_lfdk_init_ ## info(void)            \
{                                                                                   \
            handle = register_windows(&info);                                             \
}

#endif //__LFDK_H__
