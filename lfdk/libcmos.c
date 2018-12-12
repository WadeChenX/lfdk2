/*
 * LFDK - Linux Firmware Debug Kit
 * File: libcmos.c
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
#define TAG "CMOS"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/io.h>

#include <ncurses.h>
#include <panel.h>

#include "../lfdd/lfdd.h"
#include "lfdk.h"
#include "debug.h"
#include "libcmos.h"

typedef enum INPUT_MODE {
        NO_INPUT = 0,
        INPUT_CMOS,
        INPUT_MODE_MAX,
} INPUT_MODE;

CmosPanel CmosScreen;
static struct lfdd_io_t lfdd_io_data;

static int x, y;
static INPUT_MODE input;
static uint32_t wbuf;
static char offset_text[512] = {0};
static int32_t cmos_addr = 0;


void WriteCmosByteValue() 
{
        outb( x * LFDK_BYTE_PER_LINE + y, LFDK_CMOS_ADDR_PORT );
        outb( wbuf, LFDK_CMOS_DATA_PORT );
}

void ReadCmos256Bytes( char *buf ) {

        int i;

        for( i = 0 ; i < LFDK_CMOS_RANGE_BYTES ; i++ ) {
                outb( i, LFDK_CMOS_ADDR_PORT );
                buf[ i ] = (char)inb( LFDK_CMOS_DATA_PORT );
        }
}


void ClearCmosScreen() 
{
        DestroyWin( CmosScreen, offset );
        DestroyWin( CmosScreen, info );
        DestroyWin( CmosScreen, value );
        DestroyWin( CmosScreen, ascii );
}

static char *CreateOffsetText()
{
        int addr_x_y = 0;

        addr_x_y = y * LFDK_BYTE_PER_LINE + x;
        sprintf(offset_text,
                "%4.4X 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0",
                addr_x_y
        );
        return offset_text;
}

void PrintCmosScreen() 
{

        int i, j;
        char tmp;
        void *p_value = NULL;
        uint32_t result_value;

        //
        // Print Offset Text
        //
        PrintFixedWin( CmosScreen, offset, 17, 53, 4, 1, RED_BLUE, CreateOffsetText());


        //
        // Print memory address
        //
        if( !CmosScreen.info ) {
                CmosScreen.info = newwin( 1, 47, 22, 0 );
                CmosScreen.p_info = new_panel( CmosScreen.info );
        }
        wbkgd( CmosScreen.info, COLOR_PAIR( WHITE_BLUE ) );
        wattrset( CmosScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
        mvwprintw( CmosScreen.info, 0, 0, "Type: CMOS" );


        //
        // Read memory space 256 bytes
        //
        ReadCmos256Bytes( lfdd_io_data.mass_buf );


        //
        // Print ASCII content
        //
        if( !CmosScreen.ascii ) {
                CmosScreen.ascii = newwin( 17, 16, 4, 58 );
                CmosScreen.p_ascii = new_panel( CmosScreen.ascii );
        }


        wbkgd( CmosScreen.ascii, COLOR_PAIR( CYAN_BLUE ) );
        wattrset( CmosScreen.ascii, COLOR_PAIR( CYAN_BLUE ) | A_BOLD );
        mvwprintw( CmosScreen.ascii, 0, 0, "" );

        wprintw( CmosScreen.ascii, "0123456789ABCDEF" );
        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j++ ) {
                        tmp = ((unsigned char)lfdd_io_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ]);
                        if( (tmp >= '!') && (tmp <= '~') ) {
                                wprintw( CmosScreen.ascii, "%c", tmp );
                        } else {
                                wprintw( CmosScreen.ascii, "." ); 
                        }
                } //for j
        } //for i

        wattrset( CmosScreen.ascii, A_NORMAL );

        //
        // Print 256bytes content
        //
        if( !CmosScreen.value ) {
                CmosScreen.value = newwin( 17, 50, 5, 6 );
                CmosScreen.p_value = new_panel( CmosScreen.value );
        }

        wbkgd( CmosScreen.value, COLOR_PAIR( WHITE_BLUE ) );
        mvwprintw( CmosScreen.value, 0, 0, "" );

        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j++ ) {
                        //
                        // Change Color Pair and Read Value
                        //
                        if (input && y == i && x == j) {
                                p_value = &wbuf;
                                wattrset( CmosScreen.value, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                        } else {
                                p_value =  &lfdd_io_data.mass_buf[ i * LFDK_BYTE_PER_LINE + j];
                                if (y == i && x == j) {
                                        wattrset( CmosScreen.value, COLOR_PAIR( BLACK_YELLOW ) | A_BOLD ); 
                                } else {
                                        wattrset( CmosScreen.value, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                                }
                        }
                        result_value = *((uint8_t *)p_value) & 0x0FF;

                        wprintw( CmosScreen.value, "%2.2X", result_value );
                        //
                        // End of color pair
                        //
                        wattrset( CmosScreen.value, A_NORMAL );
                        //
                        // Move to next byte
                        //
                        if( j+1 < LFDK_BYTE_PER_LINE ) {
                                wprintw( CmosScreen.value, " ");
                        }
                } //for j
                wprintw( CmosScreen.value, "\n" );
        } //for i
}

int cmos_handle;
static int cmos_get_focus(st_cmd_info *p_cmd, void *data);
static int cmos_lost_focus(st_cmd_info *p_cmd, void *data);
static int cmos_paint(st_cmd_info *p_cmd, void *data);
static int cmos_key_press(st_cmd_info *p_cmd, void *data);
static int cmos_init(st_cmd_info *p_cmd, void *data);

static st_window_info  cmos_win_info  = {
        .name = "CMOS-SPACE",
        .init = cmos_init,
        .short_key = 'o',
        .get_focus = cmos_get_focus,
        .lost_focus = cmos_lost_focus,
        .key_press = cmos_key_press,
        .paint = cmos_paint,
        .destroy_win = NULL 
};

static int cmos_key_press(st_cmd_info *p_cmd, void *data)
{
        uint32_t *p_key_code = data;

        if (input == INPUT_CMOS) {
                switch(*p_key_code){
                        case 0x0a: //enter
                                WriteCmosByteValue(p_cmd->fd_lfdd);
                                input = NO_INPUT;
                                wbuf = 0;
                                break;

                        case KEY_BACKSPACE:
                                wbuf >>= 4;
                                break;

                        default:
                                if (isxdigit(*p_key_code)){
                                        wbuf <<= 4;
                                        wbuf &= 0x0f0;
                                        if( *p_key_code <= '9' ) {
                                                wbuf |= *p_key_code - 0x30;
                                        } else if( *p_key_code > 'F' ) {
                                                wbuf |= *p_key_code - 0x60 + 9;
                                        } else {
                                                wbuf |= *p_key_code - 0x40 + 9;
                                        }
                                }else {
                                        // out input mode
                                        wbuf = 0;
                                        input = NO_INPUT;
                                }
                                break;
                }

        } else { //display mode
                switch(*p_key_code){
                        case KEY_UP:
                                if (y>0) y--;
                                break;

                        case KEY_DOWN:
                                if (y < 15) y++;
                                break;

                        case KEY_LEFT:
                                x--;
                                if (x<0) x=0;
                                break;

                        case KEY_RIGHT:
                                x++;
                                if (x >= LFDK_BYTE_PER_LINE) x = LFDK_BYTE_PER_LINE-1;
                                break;

                        default:
                                if (isxdigit(*p_key_code)){
                                        input = INPUT_CMOS;
                                        if( *p_key_code <= '9' ) {
                                                wbuf = *p_key_code - 0x30;
                                        } else if( *p_key_code > 'F' ) {
                                                wbuf = *p_key_code - 0x60 + 9;
                                        } else {
                                                wbuf = *p_key_code - 0x40 + 9;
                                        }
                                }
                } //switch
        } //endif 

        log_i("key: %d, x: %.2X, y: %.02X, wbuf: 0x%04X\n", 
                *p_key_code, x, y, wbuf);
        return 0;
}

static int cmos_init(st_cmd_info *p_cmd, void *data)
{
        // Enable IO Permission
        if(ioperm(LFDK_CMOS_IO_START, LFDK_CMOS_IO_END, 1)) {
                fprintf( stderr, "Failed to Execute ioperm()\n" );
                return ERR_REQ_IO;
        }
        return 0;
}

static int cmos_get_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        input = NO_INPUT;
        wbuf = 0;
        return 0;
}

static int cmos_lost_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        input = NO_INPUT;
        wbuf = 0;
        ClearCmosScreen();
        return 0;
}

static int cmos_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&cmos_win_info, cmos_handle) == WM_FOREGROUND) {
                log_v("%s\n", __func__);
                PrintCmosScreen();
                top_panel( CmosScreen.p_offset);
                top_panel( CmosScreen.p_info );
                top_panel( CmosScreen.p_ascii);
                top_panel( CmosScreen.p_value );
                update_panels();
        }
        return 0;
}

module_init(cmos_win_info, cmos_handle)

