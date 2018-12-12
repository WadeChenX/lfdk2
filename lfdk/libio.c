/*
 * LFDK - Linux Firmware Debug Kit
 * File: libio.c
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
#define TAG "IO"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <ncurses.h>
#include <panel.h>

#include "libio.h"
#include "../lfdd/lfdd.h"
#include "lfdk.h"

#include "debug.h"

#define LFDK_BYTE_PER_LINE		16

typedef enum DISPLAY_MODE {
        BYTE_MODE = 0,
        WORD_MODE,
        DISPLAY_MODE_MAX,
} DISPLAY_MODE;

typedef enum INPUT_MODE {
        NO_INPUT = 0,
        INPUT_IO,
        INPUT_ADDR,
        INPUT_MODE_MAX,
} INPUT_MODE;

IOPanel IOScreen;
struct lfdd_io_t lfdd_io_data;


static int x, y;
static INPUT_MODE input;
static uint32_t wbuf;
static int display_mode = BYTE_MODE;
static int32_t ioaddr = 0;
static char offset_text[512] = {0};


void WriteIOValue( int fd ) 
{
        //TODO: need to verify this function
        lfdd_io_data.addr = ioaddr + x * LFDK_BYTE_PER_LINE + y;
        lfdd_io_data.buf = wbuf;
        if (display_mode == BYTE_MODE) {
                LFDD_IOCTL( fd, LFDD_IO_WRITE_BYTE, lfdd_io_data );
        }else {
                LFDD_IOCTL( fd, LFDD_IO_WRITE_WORD, lfdd_io_data );
        }
}


void ClearIOScreen() {

    DestroyWin( IOScreen, offset );
    DestroyWin( IOScreen, info );
    DestroyWin( IOScreen, value );
    DestroyWin( IOScreen, ascii );
}

static char *CreateOffsetText()
{
        int ioaddr_x_y = 0;
        ioaddr_x_y = ioaddr + y * LFDK_BYTE_PER_LINE + x;
        if (display_mode == BYTE_MODE) {
                sprintf(offset_text,
                  "%4.4X 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0",
                  ioaddr_x_y
                );
        } else  {
                sprintf(offset_text,
                  "%4.4X 0000  0002  0004  0006  0008  000A  000C  000E\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0", 
                  ioaddr_x_y
                );
        } 
        return offset_text;
}

void PrintIOScreen( int fd ) 
{
        int i, j;
        char tmp;
        const char mode_pattern[DISPLAY_MODE_MAX][8] = {
                "%2.2X",
                "%4.4X",
        };
        const char gap_pattern[DISPLAY_MODE_MAX][8] = {
                "%.1s",
                "%.2s",
        };
        void *p_value = NULL;
        uint32_t result_value;


        //
        // Print Offset Text
        //
        PrintFixedWin( IOScreen, offset, 17, 53, 4, 1, RED_BLUE, CreateOffsetText());

        //
        // Print memory address
        //
        if( !IOScreen.info ) {
                IOScreen.info = newwin( 1, 47, 22, 0 );
                IOScreen.p_info = new_panel( IOScreen.info );
        }
        wbkgd( IOScreen.info, COLOR_PAIR( WHITE_BLUE ) );
        wattrset( IOScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
        mvwprintw( IOScreen.info, 0, 0, "Type: I/O Space Address:     " );


        if(input == INPUT_ADDR) {
                wattrset( IOScreen.info, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                wprintw( IOScreen.info, "%2.2X", wbuf);
                wattrset( IOScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                wprintw( IOScreen.info, "00" );
        } else {
                wattrset( IOScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                wprintw( IOScreen.info, "%4.4X", ioaddr );
        }

        wattrset( IOScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
        wprintw( IOScreen.info, "h" );
        wattrset( IOScreen.info, A_NORMAL );
        //
        // Read memory space 256 bytes
        //
        if(input == INPUT_ADDR ) {
                memset( lfdd_io_data.mass_buf, 0xff, LFDD_MASSBUF_SIZE );
        } else {
                lfdd_io_data.addr = ioaddr;
                LFDD_IOCTL( fd, LFDD_IO_READ_256BYTE, lfdd_io_data );
        }
        //
        // Print ASCII content
        //
        if( !IOScreen.ascii ) {
                IOScreen.ascii = newwin( 17, 16, 4, 58 );
                IOScreen.p_ascii = new_panel( IOScreen.ascii );
        }

        wbkgd( IOScreen.ascii, COLOR_PAIR( CYAN_BLUE ) );
        wattrset( IOScreen.ascii, COLOR_PAIR( CYAN_BLUE ) | A_BOLD );
        mvwprintw( IOScreen.ascii, 0, 0, "" );

        wprintw( IOScreen.ascii, "0123456789ABCDEF" );
        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j++ ) {
                        tmp = ((unsigned char)lfdd_io_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ]);
                        //if (!isprint(tmp)) {
                        //        tmp = '.';
                        //}
                        if( (tmp >= '!') && (tmp <= '~') ) {
                                wprintw( IOScreen.ascii, "%c", tmp );
                        } else {
                                wprintw( IOScreen.ascii, "." ); 
                        }
                } //for j
        } //for i

        wattrset( IOScreen.ascii, A_NORMAL );


        //
        // Print 256bytes content
        //
        if( !IOScreen.value ) {

                IOScreen.value = newwin( 17, 50, 5, 6 );
                IOScreen.p_value = new_panel( IOScreen.value );
        }

        wbkgd( IOScreen.value, COLOR_PAIR( WHITE_BLUE ) );
        mvwprintw( IOScreen.value, 0, 0, "" );
        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j+=(1<<display_mode) ) {
                        //
                        // Change Color Pair and Read Value
                        //
                        if (input == INPUT_IO && y == i && x == j) {
                                p_value = &wbuf;
                                wattrset( IOScreen.value, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                        } else {
                                p_value =  &lfdd_io_data.mass_buf[ i * LFDK_BYTE_PER_LINE + j];
                                if (input == NO_INPUT && y == i && x == j) {
                                        wattrset( IOScreen.value, COLOR_PAIR( BLACK_YELLOW ) | A_BOLD ); 
                                } else {
                                        wattrset( IOScreen.value, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                                }
                        }
                        if (display_mode == BYTE_MODE) {
                                result_value = *((uint8_t *)p_value) & 0x0FF;
                        }else if (display_mode == WORD_MODE) {
                                result_value = *((uint16_t *)p_value) & 0x0FFFF;
                        } 

                        wprintw( IOScreen.value, mode_pattern[display_mode], result_value );
                        //
                        // End of color pair
                        //
                        wattrset( IOScreen.value, A_NORMAL );
                        //
                        // Move to next byte
                        //
                        if( j+(1<<display_mode) < LFDK_BYTE_PER_LINE ) {
                                wprintw( IOScreen.value, gap_pattern[display_mode], "               " );
                        } 
                } //for j
                wprintw( IOScreen.value, "\n" );
        } //for i
}

int io_handle;
static int io_get_focus(st_cmd_info *p_cmd, void *data);
static int io_lost_focus(st_cmd_info *p_cmd, void *data);
static int io_paint(st_cmd_info *p_cmd, void *data);
static int io_key_press(st_cmd_info *p_cmd, void *data);

static st_window_info  io_win_info  = {
        .name = "IO-SPACE",
        .init = NULL,
        .short_key = 'i',
        .get_focus = io_get_focus,
        .lost_focus = io_lost_focus,
        .key_press = io_key_press,
        .paint = io_paint,
        .destroy_win = NULL 
};

static int io_key_press(st_cmd_info *p_cmd, void *data)
{
        uint32_t *p_key_code = data;

        if (input == INPUT_ADDR) {
                switch(*p_key_code){
                        case 0x0a: //enter
                                input = NO_INPUT;
                                ioaddr = (wbuf<<8);
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
                                }
                                break;
                } //switch

        } else if (input == INPUT_IO) {
                switch(*p_key_code){
                        case 0x0a: //enter
                                WriteIOValue(p_cmd->fd_lfdd);
                                input = NO_INPUT;
                                wbuf = 0;
                                break;

                        case KEY_BACKSPACE:
                                wbuf >>= 4;
                                break;

                        default:
                                if (isxdigit(*p_key_code)){
                                        wbuf <<= 4;
                                        if (display_mode == BYTE_MODE) {
                                                wbuf &= 0x0f0;
                                        } else if (display_mode == WORD_MODE) {
                                                wbuf &= 0x0fff0;
                                        }
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
                                x -= (1<<display_mode);
                                if (x<0) x=0;
                                break;

                        case KEY_RIGHT:
                                x += (1<<display_mode);
                                if (x >= LFDK_BYTE_PER_LINE) x -= (1<<display_mode);
                                break;

                        case KEY_NPAGE:
                                ioaddr += LFDK_BYTES_PER_PAGE;
                                if (ioaddr > 0x0ffff) ioaddr = 0x0ff00;
                                break;

                        case KEY_PPAGE:
                                ioaddr -= LFDK_BYTES_PER_PAGE;
                                if (ioaddr < 0) ioaddr = 0;
                                break;

                        case KEY_F(7):
                                display_mode++;
                                if (display_mode >= DISPLAY_MODE_MAX) display_mode = BYTE_MODE;
                                //alignment
                                x = (x>>display_mode)<<display_mode;
                                break;

                        default:
                                if (*p_key_code == io_win_info.short_key) {
                                        input = INPUT_ADDR;
                                        wbuf = 0;
                                } else if (isxdigit(*p_key_code)){
                                        input = INPUT_IO;
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

        log_i("key: %d, x: %.2X, y: %.02X, ioaddr: 0x%04X, wbuf: 0x%04X\n", 
                *p_key_code, x, y, ioaddr, wbuf);
        return 0;
}

static int io_get_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        input = INPUT_ADDR;
        wbuf = 0;
        return 0;
}

static int io_lost_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        input = NO_INPUT;
        wbuf = 0;
        ClearIOScreen();
        return 0;
}

static int io_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&io_win_info, io_handle) == WM_FOREGROUND) {
                log_v("%s\n", __func__);
                PrintIOScreen(p_cmd->fd_lfdd);
                top_panel( IOScreen.p_offset);
                top_panel( IOScreen.p_info );
                top_panel( IOScreen.p_ascii);
                top_panel( IOScreen.p_value );
                update_panels();
        }
        return 0;
}

module_init(io_win_info, io_handle)
