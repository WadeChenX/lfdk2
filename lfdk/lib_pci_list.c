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
#define TAG "PCI-LIST"

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
#include "libpci.h"
#include "debug.h"

PCILPanel PCILScreen;
PCIData lfdd_pci_list[ LFDK_MAX_PCIBUF ];
uint64_t pcix_mmio_base=0;
int curr_index, pci_list_len;
const int RECORDS_PER_PAGE = 20; // 20 records per page

void PrintPCILScreen( void ) {

            int i;
            int page_start, page_end, lightbar;

            page_start = (curr_index / RECORDS_PER_PAGE) * RECORDS_PER_PAGE;
            page_end = page_start + RECORDS_PER_PAGE - 1;
            if (page_end >= pci_list_len) 
                    page_end = pci_list_len-1;


            //
            // Print Title
            //
            PrintFixedWin( PCILScreen, title, 1, 80, 1, 0, BLACK_GREEN, 
                "Name                                              Vendor  Device  Bus# Dev# Fun#" 
                );


            //
            // Print PCI device name
            //
            if( !PCILScreen.devname ) {

                    PCILScreen.devname = newwin( 20, 50, 2, 0 );
                    PCILScreen.p_devname = new_panel( PCILScreen.devname );
            }

            if( !PCILScreen.vendev ) {

                    PCILScreen.vendev = newwin( 20, 30, 2, 50 );
                    PCILScreen.p_vendev = new_panel( PCILScreen.vendev );
            }

            wbkgd( PCILScreen.devname, COLOR_PAIR( WHITE_BLUE ) );
            wbkgd( PCILScreen.vendev, COLOR_PAIR( WHITE_BLUE ) );

            mvwprintw( PCILScreen.devname, 0, 0, "" );
            mvwprintw( PCILScreen.vendev, 0, 0, "" );
            for( i=page_start; i<page_start+RECORDS_PER_PAGE; i++ ) {
                    if(i == curr_index) {
                            wattrset( PCILScreen.devname, COLOR_PAIR( BLACK_YELLOW ) | A_BOLD );
                    } else if (i<pci_list_len){
                            wattrset( PCILScreen.devname, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );
                    }
                    wattrset( PCILScreen.vendev, COLOR_PAIR( WHITE_BLUE ) | A_BOLD );

                    if (i<pci_list_len) {
                            char pcie_sign = lfdd_pci_list[i].phy_base ? '*' : ' ';
                            if( !lfdd_pci_list[ i ].ventxt[ 0 ] ) {
                                    wprintw( PCILScreen.devname, "%c %4.4X, ", pcie_sign, lfdd_pci_list[ i ].venid );
                            } else {
                                    wprintw( PCILScreen.devname, "%c %.10s, ", pcie_sign, lfdd_pci_list[ i ].ventxt );
                            }

                            if( !lfdd_pci_list[ i ].devtxt[ 0 ] ) {
                                    wprintw( PCILScreen.devname, "%4.4X\n", lfdd_pci_list[ i ].devid );
                            } else {
                                    wprintw( PCILScreen.devname, "%.35s\n", lfdd_pci_list[ i ].devtxt );
                            }
                            wprintw( PCILScreen.vendev, "%4.4X    %4.4X     %2.2X   %2.2X   %2.2X\n", 
                                            lfdd_pci_list[ i ].venid, lfdd_pci_list[ i ].devid, 
                                            lfdd_pci_list[ i ].bus, lfdd_pci_list[ i ].dev, lfdd_pci_list[ i ].fun );
                    } else {
                            wprintw( PCILScreen.devname, "\n", i);
                            wprintw( PCILScreen.vendev, "\n", i);
                    }

                    wattrset( PCILScreen.devname, A_NORMAL );
                    wattrset( PCILScreen.vendev, A_NORMAL );
            }
            return;
}

void ClearPCILScreen( void ) {

        DestroyWin( PCILScreen, title );
        DestroyWin( PCILScreen, devname );
        DestroyWin( PCILScreen, vendev );
}

int pci_list_handle;
static int pci_list_get_focus(st_cmd_info *p_cmd, void *data);
static int pci_list_lost_focus(st_cmd_info *p_cmd, void *data);
static int pci_list_paint(st_cmd_info *p_cmd, void *data);
static int pci_list_key_press(st_cmd_info *p_cmd, void *data);

static st_window_info  pci_list_win_info  = {
        .name = "PCI-LIST",
        .init = NULL,
        .short_key = 'p',
        .get_focus = pci_list_get_focus,
        .lost_focus = pci_list_lost_focus,
        .key_press = pci_list_key_press,
        .paint = pci_list_paint,
        .destroy_win = NULL 
};


static int pci_list_get_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        return 0;
}
static int pci_list_key_press(st_cmd_info *p_cmd, void *data)
{
        uint32_t *p_key_code = data;

        switch(*p_key_code){
                case KEY_UP:
                        curr_index--;
                        if (curr_index < 0) curr_index = 0;
                        break;

                case KEY_DOWN:
                        curr_index++;
                        if (curr_index >= pci_list_len) curr_index = pci_list_len-1;
                        break;

                case KEY_NPAGE:
                        curr_index += RECORDS_PER_PAGE;
                        if (curr_index >= pci_list_len) curr_index = pci_list_len-1;
                        break;

                case KEY_PPAGE:
                        curr_index -= RECORDS_PER_PAGE;
                        if (curr_index < 0) curr_index = 0;
                        break;

                case 0x0a: //enter
                        request_xfer_control(&pci_list_win_info, pci_list_handle, "PCI-DEV");
                        break;

                default:
                        ;;

        }
        return 0;
}

static int pci_list_lost_focus(st_cmd_info *p_cmd, void *data)
{
        log_i("%s\n", __func__);
        ClearPCILScreen();
        return 0;
}

static int pci_list_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&pci_list_win_info, pci_list_handle) == WM_FOREGROUND) {
                log_v("%s\n", __func__);
                PrintPCILScreen();
                top_panel(PCILScreen.p_title);
                top_panel(PCILScreen.p_devname);
                top_panel(PCILScreen.p_vendev);
                update_panels();
        }
        return 0;
}

module_init(pci_list_win_info, pci_list_handle)

