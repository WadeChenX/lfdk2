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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <ncurses.h>
#include <panel.h>

#include "../lfdd/lfdd.h"
#include "lfdk.h"
#include "libbase.h"

static BasePanel BaseScreen;


/*
        int (*start_win)(st_cmd_info *p_cmd, void *data);
        int (*lost_focus)(st_cmd_info *p_cmd, void *data);
        int (*get_focus)(st_cmd_info *p_cmd, void *data);
        int (*paint)(st_cmd_info *p_cmd, void *data);
        int (*key_press)(st_cmd_info *p_cmd, void *data);
        int (*destroy_win)(st_cmd_info *p_cmd, void *data);
        int (*exit)(st_cmd_info *p_cmd, void *data);
        */

void PrintBaseScreen( void ) {


    //
    // Background Color
    //
    PrintWin( BaseScreen, bg, 23, 80, 0, 0, WHITE_BLUE, "" );


    //
    // Base Screen
    //
    PrintWin( BaseScreen, logo, 1, 80, 0, 0, WHITE_RED, "Linux Firmware Debug Kit "LFDK_VERSION );
    PrintWin( BaseScreen, copyright, 1, 32, 0, 48, WHITE_RED, "Merck Hung <merckhung@gmail.com>" );
    PrintWin( BaseScreen, help, 1, 80, 23, 0, BLACK_WHITE, "(Q)uit (P)CI (M)emory (I)O CM(O)S" );


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

