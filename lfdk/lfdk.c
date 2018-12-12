/*
 * LFDK - Linux Firmware Debug Kit
 * File: lfdk.c
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

#define TAG "LFDK"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>

#include <ncurses.h>
#include <panel.h>

#include "debug.h"

#include "../lfdd/lfdd.h"
#include "lfdk.h"
#include "libpci.h"

int32_t ibuf;
int maxpcibus = 255;
char pciname[ LFDK_MAX_PATH ];


st_windows_manager_info win_manager = {
        .cur_fore_window_handle = -1,
        .cmd_info = {
                .debug_lv = DEBUG_INFO,
        },
};

static void usage( void ) {

    fprintf( stderr, "\n"LFDK_VERTEXT"\n" );
	fprintf( stderr, "Copyright (C) 2006 - 2010, Merck Hung <merckhung@gmail.com>\n" );
    fprintf( stderr, "Usage: "LFDK_PROGNAME" [-h] [-d /dev/lfdd] [-n ./pci.ids] [-b 255]\n" );
    fprintf( stderr, "\t-n\tFilename of PCI Name Database, default is /usr/share/misc/pci.ids\n" );
    fprintf( stderr, "\t-d\tDevice name of Linux Firmware Debug Driver, default is /dev/lfdd\n" );
    fprintf( stderr, "\t-b\tMaximum PCI Bus number to scan, default is 255\n" );
    fprintf( stderr, "\t-h\tprint this message.\n");
    fprintf( stderr, "\n");
}


void InitColorPairs( void ) {

    init_pair( WHITE_RED, COLOR_WHITE, COLOR_RED );
    init_pair( WHITE_BLUE, COLOR_WHITE, COLOR_BLUE );
    init_pair( BLACK_WHITE, COLOR_BLACK, COLOR_WHITE ); 
    init_pair( CYAN_BLUE, COLOR_CYAN, COLOR_BLUE );
    init_pair( RED_BLUE, COLOR_RED, COLOR_BLUE );
    init_pair( YELLOW_BLUE, COLOR_YELLOW, COLOR_BLUE );
    init_pair( BLACK_GREEN, COLOR_BLACK, COLOR_GREEN );
    init_pair( BLACK_YELLOW, COLOR_BLACK, COLOR_YELLOW );
    init_pair( YELLOW_RED, COLOR_YELLOW, COLOR_RED );
    init_pair( YELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK );
    init_pair( WHITE_YELLOW, COLOR_WHITE, COLOR_YELLOW );
}



int register_windows(st_window_info *p_win) 
{
        int handle;

        if (win_manager.windows_used_len + 1 > WINDOWS_POOL_SIZE) {
                return ERR_BUF_OVERFLOW;
        }

        win_manager.windows_pool[win_manager.windows_used_len].p_win = p_win;
        win_manager.windows_pool[win_manager.windows_used_len].win_state = WM_NOT_START;
        handle = win_manager.windows_used_len;
        win_manager.windows_used_len++;

        return handle;
}

int request_windows_focus(st_window_info *p_win, int handle)
{
        st_win_info *p_win_info = NULL;

        if (!p_win || handle < 0) return ERR_INVALID_PARAM;
        if (handle >= win_manager.windows_used_len) return ERR_INVALID_PARAM;

        p_win_info = &win_manager.windows_pool[handle];

        if (strcmp(p_win_info->p_win->name, p_win->name)) {
                return ERR_INVALID_PARAM;
        }

        if (win_manager.msg_used_len >= MSG_SIZE) {
                return ERR_BUF_OVERFLOW;
        }

        win_manager.msg_box[win_manager.msg_used_len].msg = MSG_NEED_FOCUS;
        win_manager.msg_box[win_manager.msg_used_len].sender_handle = handle;
        win_manager.msg_used_len++;

        return 0;
}

int request_xfer_control(st_window_info *p_win, int handle, char *func_name)
{
        st_win_info *p_win_info = NULL;
        int i, found;

        if (!p_win || handle < 0) return ERR_INVALID_PARAM;
        if (handle >= win_manager.windows_used_len) return ERR_INVALID_PARAM;

        p_win_info = &win_manager.windows_pool[handle];

        if (strcmp(p_win_info->p_win->name, p_win->name)) {
                return ERR_INVALID_PARAM;
        }

        if (win_manager.msg_used_len >= MSG_SIZE) {
                return ERR_BUF_OVERFLOW;
        }

        if (!func_name) {
                return ERR_INVALID_PARAM;
        }

        for (i=0,found = -1; i<win_manager.windows_used_len; i++) {
                if (!strcmp(win_manager.windows_pool[i].p_win->name, func_name)) {
                        found = i;
                        break;
                }
        }

        win_manager.msg_box[win_manager.msg_used_len].msg = MSG_XFER_CONTROL;
        win_manager.msg_box[win_manager.msg_used_len].sender_handle = handle;
        win_manager.msg_box[win_manager.msg_used_len].data.target_handle = found;
        win_manager.msg_used_len++;


}

int request_destroy_windows(st_window_info *p_win, int handle)
{
        st_win_info *p_win_info = NULL;

        if (!p_win || handle < 0) return ERR_INVALID_PARAM;
        if (handle >= win_manager.windows_used_len) return ERR_INVALID_PARAM;

        p_win_info = &win_manager.windows_pool[handle];

        if (strcmp(p_win_info->p_win->name, p_win->name)) {
                return ERR_INVALID_PARAM;
        }

        if (win_manager.msg_used_len >= MSG_SIZE) {
                return ERR_BUF_OVERFLOW;
        }

        win_manager.msg_box[win_manager.msg_used_len].msg = MSG_DESTROY_WINDOW;
        win_manager.msg_box[win_manager.msg_used_len].sender_handle = handle;
        win_manager.msg_used_len++;

}

int release_windows_focus(st_window_info *p_win, int handle)
{
        st_win_info *p_win_info = NULL;

        if (!p_win || handle < 0) return ERR_INVALID_PARAM;
        if (handle >= win_manager.windows_used_len) return ERR_INVALID_PARAM;

        p_win_info = &win_manager.windows_pool[handle];

        if (strcmp(p_win_info->p_win->name, p_win->name)) {
                return ERR_INVALID_PARAM;
        }

        if (win_manager.msg_used_len >= MSG_SIZE) {
                return ERR_BUF_OVERFLOW;
        }

        win_manager.msg_box[win_manager.msg_used_len].msg = MSG_RELEASE_FOCUS;
        win_manager.msg_box[win_manager.msg_used_len].sender_handle = handle;
        win_manager.msg_used_len++;

        return 0;
}

WIN_STATE request_window_state(st_window_info *p_win, int handle)
{
        st_win_info *p_win_info = NULL;

        if (!p_win || handle < 0) return ERR_INVALID_PARAM;
        if (handle >= win_manager.windows_used_len) return ERR_INVALID_PARAM;

        p_win_info = &win_manager.windows_pool[handle];

        if (strcmp(p_win_info->p_win->name, p_win->name)) {
                return ERR_INVALID_PARAM;
        }

        return p_win_info->win_state;
}

static int switch_key_press(int key)
{
        int i;
        int found = 0;
        st_window_info *p_window = NULL;

        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state != WM_BACKGROUND)
                        continue;

                p_window = win_manager.windows_pool[i].p_win;
                if (p_window->short_key == (uint32_t)key) {
                        request_windows_focus(p_window, i);
                        found = 1;
                        break;
                }
        }
        return found;
}

static int handle_key_event(uint32_t key)
{
        int i;

        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state != WM_FOREGROUND)
                        continue;
                if (win_manager.windows_pool[i].p_win->key_press)
                        win_manager.windows_pool[i].p_win->key_press(&win_manager.cmd_info, &key);
                break;
        }
        return 0;
}

static int handle_init_event()
{
        int i;
        int ret = 0;

        log_i("%s\n", __func__);

        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].p_win->init) {
                        ret = win_manager.windows_pool[i].p_win->init(&win_manager.cmd_info, NULL);
                        if (ret) {
                                log_c("%s, %s fail\n", __func__, win_manager.windows_pool[i].p_win->name);
                                break;
                        };
                }
                win_manager.windows_pool[i].win_state = WM_INITED;
        }
        return ret;
}

static int handle_start_win_event()
{
        int i;

        log_i("%s\n", __func__);

        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state != WM_INITED)
                        continue;
                if (win_manager.windows_pool[i].p_win->start_win)
                        win_manager.windows_pool[i].p_win->start_win(&win_manager.cmd_info, NULL);
                win_manager.windows_pool[i].win_state = WM_STARTED;
        }
        return 0;
}

static int handle_post_start_win_event()
{
        int i;

        log_i("%s\n", __func__);

        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state != WM_STARTED)
                        continue;
                if (win_manager.windows_pool[i].p_win->post_start_win)
                        win_manager.windows_pool[i].p_win->post_start_win(&win_manager.cmd_info, NULL);
                win_manager.windows_pool[i].win_state = WM_BACKGROUND;
        }
        return 0;
}

static int handle_message_event()
{
        int i;
        msg_info msg_tmp;
        st_win_info *p_cur_wnd=NULL, *p_next_wnd=NULL;

        while (win_manager.msg_used_len) {
                win_manager.msg_used_len--;
                msg_tmp = win_manager.msg_box[win_manager.msg_used_len];
                //clear the end unit
                memset(&win_manager.msg_box[win_manager.msg_used_len], sizeof(msg_info), 0x00);
                //handle message
                log_i("%s, sender: %s\n", msg_name(msg_tmp.msg), win_manager.windows_pool[msg_tmp.sender_handle].p_win->name);
                switch(msg_tmp.msg) {
                        case MSG_NEED_FOCUS:
                                if (msg_tmp.sender_handle == win_manager.cur_fore_window_handle) {
                                        // foreground module no need to switch 
                                        continue;
                                }
                                if (win_manager.cur_fore_window_handle >= 0) {
                                        p_cur_wnd = &win_manager.windows_pool[win_manager.cur_fore_window_handle];
                                }
                                p_next_wnd = &win_manager.windows_pool[msg_tmp.sender_handle];
                                //lost focust event
                                if (p_cur_wnd) {
                                        if (p_cur_wnd->p_win->lost_focus) 
                                                p_cur_wnd->p_win->lost_focus(&win_manager.cmd_info, NULL);
                                        p_cur_wnd->win_state = WM_BACKGROUND;
                                }
                                //got focus event
                                win_manager.cur_fore_window_handle = msg_tmp.sender_handle;
                                p_next_wnd->win_state = WM_FOREGROUND;
                                if (p_next_wnd->p_win->get_focus)
                                        p_next_wnd->p_win->get_focus(&win_manager.cmd_info, NULL);

                                break;

                        case MSG_RELEASE_FOCUS:
                                if (win_manager.cur_fore_window_handle != msg_tmp.sender_handle) {
                                        // TODO: I think bug here ...
                                }
                                p_next_wnd = &win_manager.windows_pool[msg_tmp.sender_handle];
                                if (p_next_wnd->win_state == WM_FOREGROUND) {
                                        if (p_next_wnd->p_win->lost_focus) {
                                                p_next_wnd->p_win->lost_focus(&win_manager.cmd_info, NULL);
                                        }
                                        p_next_wnd->win_state = WM_BACKGROUND;
                                        win_manager.cur_fore_window_handle = -1;
                                }

                                break;

                        case MSG_XFER_CONTROL:
                                if (msg_tmp.sender_handle == msg_tmp.data.target_handle) {
                                        // foreground module no need to switch 
                                        log_w("xfer warning, sender == target: %s\n", win_manager.windows_pool[msg_tmp.sender_handle].p_win->name);
                                        continue;
                                }
                                if (win_manager.cur_fore_window_handle >= 0) {
                                        p_cur_wnd = &win_manager.windows_pool[win_manager.cur_fore_window_handle];
                                }

                                if (p_cur_wnd && msg_tmp.sender_handle != win_manager.cur_fore_window_handle) {
                                        log_w("xfer warning, sender != foreground: %s, %s\n", 
                                                win_manager.windows_pool[msg_tmp.sender_handle].p_win->name,
                                                p_cur_wnd->p_win->name
                                        );
                                        continue;
                                }
                                p_next_wnd = &win_manager.windows_pool[msg_tmp.data.target_handle];
                                log_i("xfer to: %s\n", p_next_wnd->p_win->name);
                                //lost focust event
                                if (p_cur_wnd) {
                                        if (p_cur_wnd->p_win->lost_focus) 
                                                p_cur_wnd->p_win->lost_focus(&win_manager.cmd_info, NULL);
                                        p_cur_wnd->win_state = WM_BACKGROUND;
                                }
                                //got focus event
                                win_manager.cur_fore_window_handle = msg_tmp.data.target_handle;
                                p_next_wnd->win_state = WM_FOREGROUND;
                                if (p_next_wnd->p_win->get_focus)
                                        p_next_wnd->p_win->get_focus(&win_manager.cmd_info, NULL);

                                break;

                        case MSG_DESTROY_WINDOW:
                                p_next_wnd = &win_manager.windows_pool[msg_tmp.sender_handle];
                                if (msg_tmp.sender_handle == win_manager.cur_fore_window_handle)
                                        win_manager.cur_fore_window_handle = -1;
                                if (p_next_wnd->p_win->destroy_win)
                                        p_next_wnd->p_win->destroy_win(&win_manager.cmd_info, NULL);
                                p_next_wnd->win_state = WM_DESTROYED;

                                break;

                        default:
                                ;;
                }

        }
        return 0;
}

static int handle_paint_windows()
{
        int i;
        st_win_info *p_wind = NULL;

        //paint background 
        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state != WM_BACKGROUND)
                        continue;
                if (win_manager.windows_pool[i].p_win->paint)
                        win_manager.windows_pool[i].p_win->paint(&win_manager.cmd_info, NULL);

        }
        //paint foreground
        if (win_manager.cur_fore_window_handle >= 0) {
                p_wind = &win_manager.windows_pool[win_manager.cur_fore_window_handle];
                if (p_wind->p_win->paint) {
                        p_wind->p_win->paint(&win_manager.cmd_info, NULL);
                }
        }
        return 0;
}

static int handle_destroy_windows()
{
        int i;

        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state == WM_DESTROYED ||
                                win_manager.windows_pool[i].win_state == WM_EXIT)
                        continue;

                if (win_manager.windows_pool[i].p_win->destroy_win)
                        win_manager.windows_pool[i].p_win->destroy_win(&win_manager.cmd_info, NULL);
                win_manager.windows_pool[i].win_state = WM_DESTROYED;
        }
        return 0;
}

static int handle_module_exit()
{
        int i;
        for (i=0; i<win_manager.windows_used_len; i++) {
                if (win_manager.windows_pool[i].win_state == WM_EXIT)
                        continue;

                if (win_manager.windows_pool[i].p_win->module_exit)
                        win_manager.windows_pool[i].p_win->module_exit(&win_manager.cmd_info, NULL);
                win_manager.windows_pool[i].win_state = WM_EXIT;
        }
        return 0;
}

int main( int argc, char **argv ) {

    char c, device[ LFDK_MAX_PATH ];
    int i, fd, orig_fl, ret;

    struct tm *nowtime;
    time_t timer;
    int last_sec;
    int debug_level;


    //
    // Initialize & set default value
    //
    strncpy( device, LFDD_DEFAULT_PATH, LFDK_MAX_PATH );
    strncpy( pciname, LFDK_DEFAULT_PCINAME, LFDK_MAX_PATH );


    while( (c = getopt( argc, argv, "D:b:n:d:h" )) != EOF ) {
        switch( c ) {
            //
            // Change default path of device
            //
            case 'd' :
                strncpy( device, optarg, LFDK_MAX_PATH );
                break;
            //
            // Change default path of PCI name database
            //
            case 'n' :
                strncpy( pciname, optarg, LFDK_MAX_PATH );
                break;

            case 'b' :
                maxpcibus = atoi( optarg );
                if( maxpcibus >= LFDK_MAX_PCIBUS ) {
                    fprintf( stderr, "Maximum PCI Bus value must be less than %d\n", LFDK_MAX_PCIBUS );
                    return 1;
                }
                break;
            case 'D':
                debug_level = atoi(optarg);
                if (debug_level < DEBUG_MAX) {
                        win_manager.cmd_info.debug_lv = debug_level;
                }
                break;
            case 'h' :
            default:
                usage();
                return 1;
        }
    }

    //init debug function
    if (win_manager.cmd_info.debug_lv) {
            ret = debug_init();
            if (ret) {
                    fprintf(stderr, "Init debug function error\n");
                    goto err_init_dbg;
            }
            log_c("Debug init done\n");
    }



    //
    // Start communicate with LFDD I/O control driver
	//
    fd = open( device, O_RDWR );
    if( fd < 0 ) {
        fprintf( stderr, "Cannot open device: %s\n", device );
        ret = ERR_OPEN_DEV;
        goto err_open_dev;
    }
    win_manager.cmd_info.fd_lfdd = fd;



    ret = handle_init_event();
    if (ret) {
            fprintf( stderr, "Error occur in init event\n");
            goto err_init_ev;
    }


    //
    // Ncurse start
    //
    initscr();
    start_color();
    cbreak();
    noecho();
    nodelay( stdscr, 1 );
    keypad( stdscr, 1 );
    curs_set( 0 );


    //
    // Initialize color pairs for later use
    //
    InitColorPairs();

    ret = handle_start_win_event();
    if (ret) {
            fprintf( stderr, "Error occur in start-window event\n");
            goto err_out;
    }

    ret = handle_post_start_win_event();
    if (ret) {
            fprintf( stderr, "Error occur in post-start-window event\n");
            goto err_out;
    }

    doupdate();

    for( ; ; ) {
            //handle keyEvent
            ibuf = getch();
            //if (ibuf > 0) 
            //        printf("%s - %d\r\n", keyname(ibuf), ibuf);
            if( (ibuf == 'q') || (ibuf == 'Q') ) {
                    //
                    // Exit when ESC pressed
                    //
                    break;
            }
            if (!switch_key_press(ibuf)) {
                    if (ibuf > 0)
                            handle_key_event(ibuf);
            }

            //handle message event
            handle_message_event();

            //paint window
            handle_paint_windows();

            // update window
            doupdate();

            usleep( 1000 );
    }

    //destroy windows 
    handle_destroy_windows();

    //module exit
    handle_module_exit();

err_out:
    endwin();
err_init_ev:
    close( fd );
err_open_dev:
    debug_exit();
err_init_dbg:
    return ret;
}


