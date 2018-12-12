/*
 * LFDK - Linux Firmware Debug Kit
 * File: libmem.c
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

#define TAG MEM

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

#include "../lfdd/lfdd.h"
#include "lfdk.h"
#include "libmem.h"

#include "debug.h"

typedef enum DISPLAY_MODE {
        BYTE_MODE = 0,
        WORD_MODE,
        DWORD_MODE,
        DISPLAY_MODE_MAX,
} DISPLAY_MODE;

typedef enum INPUT_MODE {
        NO_INPUT = 0,
        INPUT_MEM,
        INPUT_ADDR,
        INPUT_MODE_MAX,
} INPUT_MODE;

MemPanel MemScreen;
struct lfdd_mem_t lfdd_mem_data;

static int x, y;
static INPUT_MODE input;
static uint32_t wbuf;
static int display_mode = BYTE_MODE;
static char offset_text[512] = {0};

static uint32_t phyaddr = 0;


void WriteMemValue(int fd ) 
{
        //TODO: need verify
        lfdd_mem_data.addr = phyaddr + y * LFDK_BYTE_PER_LINE + x;
        lfdd_mem_data.buf = wbuf;

        if (display_mode == BYTE_MODE) {
                log_i("Write byte 0x%02X\n", wbuf);
                LFDD_IOCTL( fd, LFDD_MEM_WRITE_BYTE, lfdd_mem_data );
        } else if (display_mode == WORD_MODE) {
                log_i("Write word 0x%04X\n", wbuf);
                LFDD_IOCTL( fd, LFDD_MEM_WRITE_WORD, lfdd_mem_data );
        } else {
                log_i("Write dword 0x%08X\n", wbuf);
                LFDD_IOCTL( fd, LFDD_MEM_WRITE_DWORD, lfdd_mem_data );
        }
}


void ClearMemScreen() {

    DestroyWin( MemScreen, offset );
    DestroyWin( MemScreen, info );
    DestroyWin( MemScreen, value );
    DestroyWin( MemScreen, ascii );
}

static char *CreateOffsetText()
{
        uint32_t mem_addr_x_y = 0;
        mem_addr_x_y = (phyaddr+ y * LFDK_BYTE_PER_LINE + x) & 0x0ffff;

        if (display_mode == BYTE_MODE) {
                sprintf(offset_text,
                  "%4.4X 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0",
                  mem_addr_x_y
                );
        } else if (display_mode == WORD_MODE) {
                sprintf(offset_text,
                  "%4.4X 0000  0002  0004  0006  0008  000A  000C  000E\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0",
                  mem_addr_x_y
                );
        } else {
                sprintf(offset_text,
                  "%4.4X 00000000     00000004     00000008     0000000C\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0", 
                  mem_addr_x_y
                );
        }
        return offset_text;
}

void PrintMemScreen( int fd ) {

        int i, j;
        char tmp;
        const char mode_pattern[DISPLAY_MODE_MAX][8] = {
                "%2.2X",
                "%4.4X",
                "%8.8X"
        };
        const char gap_pattern[DISPLAY_MODE_MAX][8] = {
                "%.1s",
                "%.2s",
                "%.5s"
        };
        void *p_value = NULL;
        uint32_t result_value;

        //
        // Print Offset Text
        //
        PrintFixedWin( MemScreen, offset, 17, 53, 4, 1, RED_BLUE, CreateOffsetText());


        //
        // Print memory address
        //
        if( !MemScreen.info ) {
                MemScreen.info = newwin( 1, 47, 22, 0 );
                MemScreen.p_info = new_panel( MemScreen.info );
        }
        wbkgd( MemScreen.info, COLOR_PAIR( WHITE_BLUE ) );
        wattrset( MemScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
        mvwprintw( MemScreen.info, 0, 0, "Type: Memory    Address: " );

        if(input == INPUT_ADDR) {
                wattrset( MemScreen.info, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                wprintw( MemScreen.info, "%6.6X", wbuf);
                wattrset( MemScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                wprintw( MemScreen.info, "00" );
        } else {
                wattrset( MemScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                wprintw( MemScreen.info, "%8.8X", phyaddr);
        }

        wattrset( MemScreen.info, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
        wprintw( MemScreen.info, "h" );
        wattrset( MemScreen.info, A_NORMAL );


        //
        // Read memory space 256 bytes
        //
        if(input == INPUT_ADDR) {
                memset( lfdd_mem_data.mass_buf, 0xff, LFDD_MASSBUF_SIZE );
        }
        else {
                lfdd_mem_data.addr = phyaddr;
                LFDD_IOCTL( fd, LFDD_MEM_READ_256BYTE, lfdd_mem_data );
        }


        //
        // Print ASCII content
        //
        if( !MemScreen.ascii ) {
                MemScreen.ascii = newwin( 17, 16, 4, 58 );
                MemScreen.p_ascii = new_panel( MemScreen.ascii );
        }

        wbkgd( MemScreen.ascii, COLOR_PAIR( CYAN_BLUE ) );
        wattrset( MemScreen.ascii, COLOR_PAIR( CYAN_BLUE ) | A_BOLD );
        mvwprintw( MemScreen.ascii, 0, 0, "" );

        wprintw( MemScreen.ascii, "0123456789ABCDEF" );
        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j++ ) {
                        tmp = ((unsigned char)lfdd_mem_data.mass_buf[ (i * LFDK_BYTE_PER_LINE) + j ]);
                        if( (tmp >= '!') && (tmp <= '~') ) {
                                wprintw( MemScreen.ascii, "%c", tmp );
                        } else {
                                wprintw( MemScreen.ascii, "." ); 
                        }
                } //for j
        } //for i

        wattrset( MemScreen.ascii, A_NORMAL );


        //
        // Print 256bytes content
        //
        if( !MemScreen.value ) {
                MemScreen.value = newwin( 17, 50, 5, 6 );
                MemScreen.p_value = new_panel( MemScreen.value );
        }


        wbkgd( MemScreen.value, COLOR_PAIR( WHITE_BLUE ) );
        mvwprintw( MemScreen.value, 0, 0, "" );

        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j+=(1<<display_mode) ) {
                        //
                        // Change Color Pair and Read Value
                        //
                        if (input == INPUT_MEM && y == i && x == j) {
                                p_value = &wbuf;
                                wattrset( MemScreen.value, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                        } else {
                                p_value =  &lfdd_mem_data.mass_buf[ i * LFDK_BYTE_PER_LINE + j];
                                if (input == NO_INPUT && y == i && x == j) {
                                        wattrset( MemScreen.value, COLOR_PAIR( BLACK_YELLOW ) | A_BOLD ); 
                                } else {
                                        wattrset( MemScreen.value, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                                }
                        }
                        if (display_mode == BYTE_MODE) {
                                result_value = *((uint8_t *)p_value) & 0x0FF;
                        }else if (display_mode == WORD_MODE) {
                                result_value = *((uint16_t *)p_value) & 0x0FFFF;
                        } else {
                                result_value = *((uint32_t *)p_value);
                        }

                        wprintw( MemScreen.value, mode_pattern[display_mode], result_value );
                        //
                        // End of color pair
                        //
                        wattrset( MemScreen.value, A_NORMAL );
                        //
                        // Move to next byte
                        //
                        if( j+(1<<display_mode) < LFDK_BYTE_PER_LINE ) {
                                wprintw( MemScreen.value, gap_pattern[display_mode], "               " );
                        } 
                } //for j
                wprintw( MemScreen.value, "\n" );
        } //for i
}

int mem_handle;
static int mem_get_focus(st_cmd_info *p_cmd, void *data);
static int mem_lost_focus(st_cmd_info *p_cmd, void *data);
static int mem_paint(st_cmd_info *p_cmd, void *data);
static int mem_key_press(st_cmd_info *p_cmd, void *data);

static st_window_info  mem_win_info  = {
        .name = "MEM-SPACE",
        .init = NULL,
        .short_key = 'm',
        .get_focus = mem_get_focus,
        .lost_focus = mem_lost_focus,
        .key_press = mem_key_press,
        .paint = mem_paint,
        .destroy_win = NULL 
};

static int mem_key_press(st_cmd_info *p_cmd, void *data)
{
        uint32_t *p_key_code = data;

        if (input == INPUT_ADDR) {
                switch(*p_key_code){
                        case 0x0a: //enter
                                input = NO_INPUT;
                                phyaddr = (wbuf<<8);
                                wbuf = 0;
                                break;

                        case KEY_BACKSPACE:
                                wbuf >>= 4;
                                break;

                        default:
                                if (isxdigit(*p_key_code)){
                                        wbuf <<= 4;
                                        wbuf &= 0x00fffff0; //only 6 digit allowed
                                        if( *p_key_code <= '9' ) {
                                                wbuf |= *p_key_code - 0x30;
                                        } else if( *p_key_code > 'F' ) {
                                                wbuf |= *p_key_code - 0x60 + 9;
                                        } else {
                                                wbuf |= *p_key_code - 0x40 + 9;
                                        }
                                } else {
                                        // out input mode
                                        wbuf = 0;
                                        input = NO_INPUT;
                                }
                                break;
                } //switch

        } else if (input == INPUT_MEM) {
                switch(*p_key_code){
                        case 0x0a: //enter
                                WriteMemValue(p_cmd->fd_lfdd);
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
                } //switch

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
                                if (phyaddr + LFDK_BYTES_PER_PAGE > phyaddr)
                                        phyaddr += LFDK_BYTES_PER_PAGE;
                                break;

                        case KEY_PPAGE:
                                if (phyaddr - LFDK_BYTES_PER_PAGE < phyaddr)
                                        phyaddr -= LFDK_BYTES_PER_PAGE;
                                break;

                        case KEY_F(7):
                                display_mode++;
                                if (display_mode >= DISPLAY_MODE_MAX) display_mode = BYTE_MODE;
                                //alignment
                                x = (x>>display_mode)<<display_mode;
                                break;

                        default:
                                if (*p_key_code == mem_win_info.short_key) {
                                        input = INPUT_ADDR;
                                        wbuf = 0;
                                } else if (isxdigit(*p_key_code)){
                                        input = INPUT_MEM;
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

        log_i("key: %d, x: %.2X, y: %.02X, mem_addr: 0x%08X, wbuf: 0x%08X\n", 
                *p_key_code, x, y, phyaddr, wbuf);
        return 0;
}

static int mem_get_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        input = INPUT_ADDR;
        wbuf = 0;
        return 0;
}

static int mem_lost_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        input = NO_INPUT;
        wbuf = 0;
        ClearMemScreen();
        return 0;
}

static int mem_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&mem_win_info, mem_handle) == WM_FOREGROUND) {
                log_v("%s\n", __func__);
                PrintMemScreen(p_cmd->fd_lfdd);
                top_panel( MemScreen.p_offset);
                top_panel( MemScreen.p_info );
                top_panel( MemScreen.p_ascii);
                top_panel( MemScreen.p_value );
                update_panels();
        }
        return 0;
}

module_init(mem_win_info, mem_handle)

