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
#include "libpci.h"


char read_buffer[ LFDK_MAX_READBUF ];
PCIData lfdd_pci_list[ LFDK_MAX_PCIBUF ];
PCIPanel PCIScreen;
PCILPanel PCILScreen;
struct lfdd_pci_t lfdd_pci_data;


extern int x, y;
extern int curr_index, last_index;
extern int input;
extern unsigned int counter;
extern int ibuf;
extern char wbuf;
extern int func;
extern int maxpcibus;
extern char pciname[ LFDK_MAX_PATH ];


int rpp = 20; // 20 records per page
int lightbar = 0;


int ReadLine( int fd ) {

    char buf;
    int i;
    int tabs;


    memset( read_buffer, 0, LFDK_MAX_READBUF );

    for( i = 0, tabs = 0 ; read( fd, &buf, 1 ) ; ) {

        if( buf == '#' ) {

            //
            // Skip comment
            //
            while( read( fd, &buf, 1 ) && (buf != '\n') );
            return -1;
        }
        else if( buf == '\n' ) {

            if( strlen( read_buffer ) ) {

                return tabs;
            }

            return -1;
        }
        else if( buf == '\t' ) {

            tabs++;
        }
        else {

            read_buffer[ i ] = buf;
            i++;
        }
    }

    return -2;
}


int CompareID( unsigned int id ) {

    int i;
    char idstr[ 5 ];
    char temp[ 5 ];


    //
    // Read ID string
    //
    for( i = 0 ; i < 4 ; i++ ) {

        idstr[ i ] = read_buffer[ i ];
    }
    idstr[ i ] = 0;


    //
    // Convert ASCII to binary
    //
    snprintf( temp, 5, "%4.4x", id );


    return strncmp( temp, idstr, 4 );
}


void GetVendorAndDeviceTexts( int venid, int devid, char *ventxt, char *devtxt ) {

    int fd;
    int tabs;
    int i;
    int done;
    int findven;


    fd = open( pciname, O_RDONLY );
    if( fd < 0 ) {
    
        return;
    }


    for( findven = 0, done = 0 ; ; ) {


        //
        // Parse PCI Database file
        //
        switch( tabs = ReadLine( fd ) ) {

            case 0:
                if( !CompareID( venid ) ) {

                    strncpy( ventxt, (read_buffer + 6), LFDK_MAX_PCINAME );
                    findven = 1;
                }
                break;

            case 1:
                if( findven ) {

                    if( !CompareID( devid ) ) {

                        strncpy( devtxt, (read_buffer + 6), LFDK_MAX_PCINAME );
                        done = 1;
                    }
                }
                break;

            default:
                break;
        }


        if( done ) {

            break;
        }


        //
        // End of File
        //
        if( tabs == -2 ) {

            break;
        }
    }


    close( fd );
}


int ScanPCIDevice( int fd ) {

    int i;
    unsigned char bus = 0, dev = 0, fun = 0;


    //
    // Scan PCI memory space
    //
    last_index = 0;
    for( bus = 0 ; bus < maxpcibus ; bus++ ) {
        for( dev = 0 ; dev <= 0x1f ; dev++ ) {
            for( fun = 0 ; fun <= 0x07 ; fun++ ) {


                PrintFixedWin( PCILScreen, scan, 1, 36, 10, 25, WHITE_BLUE, "Bus = %2.2X, Dev = %2.2X, Fun = %2.2X", bus, dev, fun );
                update_panels();
                doupdate();

                lfdd_pci_data.bus = bus;
                lfdd_pci_data.dev = dev;
                lfdd_pci_data.fun = fun;
                lfdd_pci_data.reg = 0;
                LFDD_IOCTL( fd, LFDD_PCI_READ_WORD, lfdd_pci_data );

                if( (lfdd_pci_data.buf & 0xffff) != 0xffff ) {


                    //
                    // Yes, it's a PCI device
                    //
                    lfdd_pci_list[ last_index ].bus   = bus;
                    lfdd_pci_list[ last_index ].dev   = dev;
                    lfdd_pci_list[ last_index ].fun   = fun;
                    lfdd_pci_list[ last_index ].venid = (unsigned short int)lfdd_pci_data.buf;

                    //
                    // Read and record Device ID
                    //
                    lfdd_pci_data.reg += 0x02;
                    LFDD_IOCTL( fd, LFDD_PCI_READ_WORD, lfdd_pci_data );
                    lfdd_pci_list[ last_index ].devid = (unsigned short int)lfdd_pci_data.buf;


                    //
                    // Get Texts
                    //
                    GetVendorAndDeviceTexts( lfdd_pci_list[ last_index ].venid, lfdd_pci_list[ last_index ].devid
                                            ,lfdd_pci_list[ last_index ].ventxt, lfdd_pci_list[ last_index ].devtxt );

                    //
                    // Move to next record
                    //
                    last_index++;
                    if(last_index>=LFDK_MAX_PCIBUF)
                    {
                        PrintFixedWin( PCILScreen, error, 1, 50, 10, 15, WHITE_RED, "Too much device number (>%d)! Program will abort!", LFDK_MAX_PCIBUF );
                        update_panels();
                        doupdate();
                        sleep(5);
                        return 1;
                    }
                }
            }
        }
    }


}



//int pci_list_handle;
int pci_scan_handle;
static int pci_scan_start_win(st_cmd_info *p_cmd, void *data);
static int pci_scan_paint(st_cmd_info *p_cmd, void *data);
static int pci_scan_destroy_win(st_cmd_info *p_cmd, void *data);
static st_window_info  pci_scan_win_info  = {
        .name = "PCI-SCAN",
        .init = NULL,
        .start_win = pci_scan_start_win,
        .get_focus = NULL,
        .lost_focus = NULL,
        .paint = pci_scan_paint,
        .destroy_win = pci_scan_destroy_win
};
//static st_window_info  pci_list_win_info  = {
//        .name = "PCI-LIST",
//        .init = pci_list_init,
//        .start_win = pci_list_start_win
//};


static int pci_scan_destroy_win(st_cmd_info *p_cmd, void *data)
{
        DestroyWin( PCILScreen, scan );
        return 0;
}

static int pci_scan_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&pci_scan_win_info, pci_scan_handle) == WM_FOREGROUND) {
                ScanPCIDevice(p_cmd->fd_lfdd);

                request_destroy_windows(&pci_scan_win_info, pci_scan_handle);
        }
        return 0;
}

static int pci_scan_start_win(st_cmd_info *p_cmd, void *data)
{
        int ret;

        request_windows_focus(&pci_scan_win_info, pci_scan_handle);

        return ret;
}

//module_init(pci_list_win_info, pci_list_handle)
module_init(pci_scan_win_info, pci_scan_handle)

