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
#define TAG "PCI-DEV"

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
#include "libpci.h"
#include "debug.h"

#define LFDK_BYTE_PER_LINE		16

typedef enum DISPLAY_MODE {
        BYTE_MODE = 0,
        WORD_MODE,
        DWORD_MODE,
        MODE_MAX,
} DISPLAY_MODE;

extern PCIData lfdd_pci_list[ LFDK_MAX_PCIBUF ];
extern int pci_list_len;
extern int curr_index, pci_list_len;

PCIPanel PCIScreen;
struct lfdd_pci_t lfdd_pci_data;
static int x, y;

static uint32_t wbuf;
static int input;
static int display_mode = BYTE_MODE;
static char offset_text[512] = {0};

static char *CreateOffsetText()
{
        if (display_mode == BYTE_MODE) {
                sprintf(offset_text,
                  "0000 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0" 
                  );
        } else if (display_mode == WORD_MODE) {
                sprintf(offset_text,
                  "0000 0000  0002  0004  0006  0008  000A  000C  000E\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0" 
                  );
        } else {
                sprintf(offset_text,
                  "0000 00000000     00000004     00000008     0000000C\n"
                  "0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0" 
                  );
        }
        return offset_text;
}

void PrintPCIScreen( int fd ) {

        int i, j;
        const char mode_pattern[MODE_MAX][8] = {
                "%2.2X",
                "%4.4X",
                "%8.8X"
        };
        const char gap_pattern[MODE_MAX][8] = {
                "%.1s",
                "%.2s",
                "%.5s"
        };
        void *p_value = NULL;
        uint32_t result_value;


        // Print Device Name
        //
        PrintFixedWin( PCIScreen, ven, 1, 70, 1, 1, CYAN_BLUE, "%70s", " " );
        if( lfdd_pci_list[ curr_index ].ventxt[ 0 ] ) {
                PrintFixedWin( PCIScreen, ven, 1, 70, 1, 1, CYAN_BLUE, "Vendor: %.62s", lfdd_pci_list[ curr_index ].ventxt );
        } else {
                PrintFixedWin( PCIScreen, ven, 1, 70, 1, 1, CYAN_BLUE, "Unknown Vendor" );
        }

        PrintFixedWin( PCIScreen, dev, 1, 70, 2, 1, CYAN_BLUE, "%70s", " " );
        if( lfdd_pci_list[ curr_index ].devtxt[ 0 ] ) {
                PrintFixedWin( PCIScreen, dev, 1, 70, 2, 1, CYAN_BLUE, "Device: %.62s", lfdd_pci_list[ curr_index ].devtxt );
        } else {
                PrintFixedWin( PCIScreen, dev, 1, 70, 2, 1, CYAN_BLUE, "Unknown Device" );
        }


        //
        // Print Offset Text
        //
        PrintFixedWin( PCIScreen, offset, 17, 53, 4, 1, RED_BLUE, CreateOffsetText()  );


        //
        // Print PCI bus, device, function number
        //
        PrintFixedWin( PCIScreen, info, 1, 47, 22, 0, WHITE_BLUE, 
           "Type: PCI    Bus %2.2X    Device %2.2X    Function %2.2X",
           lfdd_pci_list[ curr_index ].bus, lfdd_pci_list[ curr_index ].dev, lfdd_pci_list[ curr_index ].fun );


        //
        // Read PCI configuration space 256 bytes
        //
        lfdd_pci_data.bus = lfdd_pci_list[ curr_index ].bus;
        lfdd_pci_data.dev = lfdd_pci_list[ curr_index ].dev;
        lfdd_pci_data.fun = lfdd_pci_list[ curr_index ].fun;
        lfdd_pci_data.reg = 0;
        LFDD_IOCTL( fd, LFDD_PCI_READ_256BYTE, lfdd_pci_data );



        //
        // Print PCI information
        //
        if( !PCIScreen.rtitle ) {
                PCIScreen.rtitle = newwin( 17, 24, 4, 56 );
                PCIScreen.p_rtitle = new_panel( PCIScreen.rtitle );
        }

        mvwprintw( PCIScreen.rtitle, 0, 0, "" );
        wbkgd( PCIScreen.rtitle, COLOR_PAIR( CYAN_BLUE ) );
        wattrset( PCIScreen.rtitle, COLOR_PAIR( CYAN_BLUE ) | A_BOLD );

        wprintw( PCIScreen.rtitle, "Refresh    : ON\n\n" );
        wprintw( PCIScreen.rtitle, "Data Width : 8 bits\n\n" );

        wprintw( PCIScreen.rtitle, "VID:DID = %4.4X:%4.4X\n", 
                lfdd_pci_list[ curr_index ].venid, lfdd_pci_list[ curr_index ].devid );
        wprintw( PCIScreen.rtitle, "Rev ID        : %2.2X\n", 
                (unsigned char)lfdd_pci_data.mass_buf[ 0x08 ] );
        wprintw( PCIScreen.rtitle, "Int Line (IRQ): %2.2X\n", 
                (unsigned char)lfdd_pci_data.mass_buf[ 0x3c ] );
        wprintw( PCIScreen.rtitle, "Int Pin       : %2.2X\n\n", 
                (unsigned char)lfdd_pci_data.mass_buf[ 0x3c + 8 ] );

        wprintw( PCIScreen.rtitle, "Mem: 00000000 00000000\n" );
        wprintw( PCIScreen.rtitle, "Mem: 00000000 00000000\n" );
        wprintw( PCIScreen.rtitle, "Mem: 00000000 00000000\n" );
        wprintw( PCIScreen.rtitle, "Mem: 00000000 00000000\n" );
        wprintw( PCIScreen.rtitle, "Mem: 00000000 00000000\n" );
        wprintw( PCIScreen.rtitle, "Mem: 00000000 00000000\n\n" );

        wprintw( PCIScreen.rtitle, "ROM: 00000000\n" );
        wattrset( PCIScreen.rtitle, A_NORMAL );



        //
        // Print 256bytes PCI Configuration Space
        //
        if( !PCIScreen.value ) {
                PCIScreen.value = newwin( 17, 50, 5, 6 );
                PCIScreen.p_value = new_panel( PCIScreen.value );
        }


        wbkgd( PCIScreen.value, COLOR_PAIR( WHITE_BLUE ) );
        mvwprintw( PCIScreen.value, 0, 0, "" );


        for( i = 0 ; i < LFDK_BYTE_PER_LINE ; i++ ) {
                for( j = 0 ; j < LFDK_BYTE_PER_LINE ; j+=(1<<display_mode) ) {
                        //
                        // Change Color Pair and Read Value
                        //
                        if (input && y == i && x == j) {
                                p_value = &wbuf;
                                wattrset( PCIScreen.value, COLOR_PAIR( YELLOW_RED ) | A_BOLD );
                        } else {
                                p_value =  &lfdd_pci_data.mass_buf[ i * LFDK_BYTE_PER_LINE + j];
                                if (y == i && x == j) {
                                        wattrset( PCIScreen.value, COLOR_PAIR( BLACK_YELLOW ) | A_BOLD ); 
                                } else {
                                        wattrset( PCIScreen.value, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                                }
                        }
                        if (display_mode == BYTE_MODE) {
                                result_value = *((uint8_t *)p_value) & 0x0FF;
                        }else if (display_mode == WORD_MODE) {
                                result_value = *((uint16_t *)p_value) & 0x0FFFF;
                        } else { //DWORD
                                result_value = *(uint32_t *)p_value;
                        }

                        wprintw( PCIScreen.value, mode_pattern[display_mode], result_value );
                        //
                        // End of color pair
                        //
                        wattrset( PCIScreen.value, A_NORMAL );
                        //
                        // Move to next byte
                        //
                        if( j+(1<<display_mode) < LFDK_BYTE_PER_LINE ) {
                                wprintw( PCIScreen.value, gap_pattern[display_mode], "               " );
                        } 
                } //for j
                wprintw( PCIScreen.value, "\n" );
        } // for i
}

void WritePCIByteValue(int fd) {


        lfdd_pci_data.bus = lfdd_pci_list[ curr_index ].bus;
        lfdd_pci_data.dev = lfdd_pci_list[ curr_index ].dev;
        lfdd_pci_data.fun = lfdd_pci_list[ curr_index ].fun;
        lfdd_pci_data.reg = y * LFDK_BYTE_PER_LINE + x;
        lfdd_pci_data.buf = wbuf;

        LFDD_IOCTL( fd, LFDD_PCI_WRITE_BYTE, lfdd_pci_data );
}


void ClearPCIScreen() 
{

        DestroyWin( PCIScreen, ven );
        DestroyWin( PCIScreen, dev );
        DestroyWin( PCIScreen, offset );
        DestroyWin( PCIScreen, info );
        DestroyWin( PCIScreen, rtitle );
        DestroyWin( PCIScreen, value );
}

int pci_dev_handle;
static int pci_dev_get_focus(st_cmd_info *p_cmd, void *data);
static int pci_dev_lost_focus(st_cmd_info *p_cmd, void *data);
static int pci_dev_paint(st_cmd_info *p_cmd, void *data);
static int pci_dev_key_press(st_cmd_info *p_cmd, void *data);

static st_window_info  pci_dev_win_info  = {
        .name = "PCI-DEV",
        .init = NULL,
        .get_focus = pci_dev_get_focus,
        .lost_focus = pci_dev_lost_focus,
        .key_press = pci_dev_key_press,
        .paint = pci_dev_paint,
        .destroy_win = NULL 
};


static int pci_dev_get_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        return 0;
}
static int pci_dev_key_press(st_cmd_info *p_cmd, void *data)
{
        uint32_t *p_key_code = data;

        if (input) {
                switch(*p_key_code){
                        case 0x0a: //enter
                                WritePCIByteValue(p_cmd->fd_lfdd);
                                input = 0;
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
                                } else {
                                        // out input mode
                                        wbuf = 0;
                                        input = 0;
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
                                if (curr_index < pci_list_len-1) curr_index++;
                                break;

                        case KEY_PPAGE:
                                if (curr_index > 0) curr_index--;
                                break;

                        case KEY_F(7):
                                display_mode++;
                                if (display_mode >= MODE_MAX) display_mode = BYTE_MODE;
                                //alignment
                                x = (x>>display_mode)<<display_mode;
                                break;


                        default:
                                if (isxdigit(*p_key_code)){
                                        input = 1;
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

        log_i("key: %d, x: %.2X, y: %.02X, cur: %.02X\n", *p_key_code, x, y, curr_index);
        return 0;
}

static int pci_dev_lost_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        x = y = input = wbuf = 0;
        ClearPCIScreen();
        return 0;
}

static int pci_dev_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&pci_dev_win_info, pci_dev_handle) == WM_FOREGROUND) {
                log_v("%s\n", __func__);
                PrintPCIScreen(p_cmd->fd_lfdd);
                top_panel( PCIScreen.p_ven );
                top_panel( PCIScreen.p_dev );
                top_panel( PCIScreen.p_offset );
                top_panel( PCIScreen.p_info );
                top_panel( PCIScreen.p_rtitle );
                top_panel( PCIScreen.p_value );
                update_panels();
        }
        return 0;
}

module_init(pci_dev_win_info, pci_dev_handle)

