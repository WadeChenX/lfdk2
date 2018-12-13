/*
 * LFDK - Linux Firmware Debug Kit
 * File: libpci.c
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
#define TAG "BASE"

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

#include "../lfdd/lfdd.h"
#include "lfdk.h"
#include "libbase.h"
#include "debug.h"

static BasePanel BaseScreen;

void PrintBaseScreen( void ) 
{
        struct tm *nowtime;
        time_t timer;
        int last_sec;


        //
        // Background Color
        //
        PrintFixedWin( BaseScreen, bg, 23, 80, 0, 0, WHITE_BLUE, "" );


        //
        // Base Screen
        //
        PrintFixedWin( BaseScreen, logo, 1, 80, 0, 0, WHITE_RED, "Linux Firmware Debug Kit "LFDK_VERSION );
        PrintFixedWin( BaseScreen, help, 1, 80, 23, 0, BLACK_WHITE, "(Q)uit (P)CI (M)emory (I)O CM(O)S" );

        // Update timer
        //
        time( &timer );
        nowtime = localtime( &timer );

        // Skip redundant update of timer
        if( last_sec != nowtime->tm_sec ) {
                last_sec = nowtime->tm_sec;
                PrintFixedWin( BaseScreen, time, 1, 8, 0, 71, WHITE_RED, "%2.2d:%2.2d:%2.2d", 
                        nowtime->tm_hour, nowtime->tm_min,  nowtime->tm_sec );
        }

        top_panel(BaseScreen.p_bg);
        top_panel(BaseScreen.p_logo);
        top_panel(BaseScreen.p_help);
        top_panel(BaseScreen.p_time);

        update_panels();
}

int base_handle = -1;
static int baseboard_init(st_cmd_info *p_cmd, void *data)
{
        return 0;
}

static int baseboard_start_win(st_cmd_info *p_cmd, void *data)
{
        PrintBaseScreen();
        return 0;
}

static int baseboard_paint(st_cmd_info *p_cmd, void *data)
{
        log_v("%s\n", __func__);

        PrintBaseScreen();
        return 0;
}


static st_window_info  baseboard_win_info  = {
        .name = "baseboard",
        .init = baseboard_init,
        .start_win = baseboard_start_win,
        .paint = baseboard_paint
};
module_init(baseboard_win_info, base_handle)

