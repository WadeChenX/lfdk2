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

#define TAG "PCI-SCAN"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <error.h>


#include <ncurses.h>
#include <panel.h>

#include "debug.h"
#include "../lfdd/lfdd.h"
#include "lfdk.h"
#include "libpci.h"


extern uint64_t pcix_mmio_base;
extern PCIData lfdd_pci_list[ LFDK_MAX_PCIBUF ];
char read_buffer[ LFDK_MAX_READBUF ];
PCILPanel PCIScanScreen;
struct lfdd_pcix_t lfdd_pci_data;


extern int pci_list_len;
extern int maxpcibus;
extern char pciname[ LFDK_MAX_PATH ];

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

static void AssignMMIOBase(int fd_lfdd, struct lfdd_pcix_t *p_target_dev,  PCIData *p_pci_target_data)
{
#define CAP_REG 0x34
#define CAP_ID_PCIE 0x10

        int walk_limit = 64;
        uint16_t cap_list = 0;
        uint8_t  cap_id = 0;
        uint8_t  cap_next = 0;

        // No mmio base, then return directly ...
        if (!pcix_mmio_base) 
                return;

        p_pci_target_data->phy_base = 0;

        p_target_dev->reg = CAP_REG;
        if( ioctl(fd_lfdd, LFDD_PCI_READ_WORD, p_target_dev) ) {
                log_e("%s, Cannot execute command\n", __func__ );
                return;
        }
        //first cap list
        p_target_dev->reg = p_target_dev->buf & 0x0ff;
        if( ioctl(fd_lfdd, LFDD_PCI_READ_WORD, p_target_dev) ) {
                log_e("%s, Cannot execute command\n", __func__ );
                return;
        }
        cap_id = p_target_dev->buf & 0x0ff;
        cap_next = (p_target_dev->buf & 0x0ff00)>>8;

        log_v("bus: 0x%02X, dev: 0x%02X, func: 0x%02X, cap_id: 0x%02X, cap_next: 0x%02X\n",
                p_target_dev->bus,
                p_target_dev->dev,
                p_target_dev->fun,
                cap_id,
                cap_next
        );

        while(walk_limit-- && cap_next && cap_id != CAP_ID_PCIE) {
                p_target_dev->reg = cap_next;
                if( ioctl(fd_lfdd, LFDD_PCI_READ_WORD, p_target_dev) ) {
                        log_e("%s, Cannot execute command\n", __func__ );
                        return;
                }
                cap_id = p_target_dev->buf & 0x0ff;
                cap_next = (p_target_dev->buf & 0x0ff00)>>8;
                log_v("bus: 0x%02X, dev: 0x%02X, func: 0x%02X, cap_id: 0x%02X, cap_next: 0x%02X\n",
                                p_target_dev->bus,
                                p_target_dev->dev,
                                p_target_dev->fun,
                                cap_id,
                                cap_next
                     );
        }

        if (cap_id == CAP_ID_PCIE) {
                uint32_t tmp_buf;
                //
                // Simple validation, read first 4Byte of PCI config space through IO Port/MMIO
                // and compare them.
                //
                p_target_dev->reg = 0x00;
                p_target_dev->phy_base = 0;
                p_target_dev->buf = 0;
                if( ioctl(fd_lfdd, LFDD_PCI_READ_DWORD, p_target_dev) ) {
                        log_e("%s, Cannot execute command\n", __func__ );
                        return;
                }
                tmp_buf = p_target_dev->buf;
                if (!tmp_buf) {
                        log_e("bus: 0x%02X, dev: 0x%02X, func: 0x%02X  reg: 0x00 is zero !!\n", 
                                p_target_dev->bus,
                                p_target_dev->dev,
                                p_target_dev->fun
                        );
                        return;
                }

                p_target_dev->reg = 0x00;
                p_target_dev->phy_base = pcix_mmio_base;
                p_target_dev->buf = 0;
                if( ioctl(fd_lfdd, LFDD_PCI_READ_DWORD, p_target_dev) ) {
                        log_e("%s, Cannot execute command\n", __func__ );
                        return;
                }

                if (tmp_buf == p_target_dev->buf) {
                        log_v("Assign bus: 0x%02X, dev: 0x%02X, func: 0x%02X  0x%08X 0x%08X\n", 
                                        p_target_dev->bus,
                                        p_target_dev->dev,
                                        p_target_dev->fun,
                                        tmp_buf, p_target_dev->buf
                             );
                        p_pci_target_data->phy_base = pcix_mmio_base;
                }
        } //end if 
}

int ScanPCIDevice( int fd ) {

    int i;
    unsigned char bus = 0, dev = 0, fun = 0;


    //
    // Scan PCI memory space
    //
    pci_list_len = 0;
    for( bus = 0 ; bus < maxpcibus ; bus++ ) {
        for( dev = 0 ; dev <= 0x1f ; dev++ ) {
            for( fun = 0 ; fun <= 0x07 ; fun++ ) {

                PrintFixedWin( PCIScanScreen, scan, 1, 36, 10, 25, WHITE_BLUE, "Bus = %2.2X, Dev = %2.2X, Fun = %2.2X", bus, dev, fun );
                update_panels();
                doupdate();

                lfdd_pci_data.bus = bus;
                lfdd_pci_data.dev = dev;
                lfdd_pci_data.fun = fun;
                lfdd_pci_data.reg = 0;
                lfdd_pci_data.phy_base = 0;
                LFDD_IOCTL( fd, LFDD_PCI_READ_WORD, lfdd_pci_data );

                if( (lfdd_pci_data.buf & 0xffff) != 0xffff ) {
                    //
                    // Yes, it's a PCI device
                    //
                    lfdd_pci_list[ pci_list_len ].bus   = bus;
                    lfdd_pci_list[ pci_list_len ].dev   = dev;
                    lfdd_pci_list[ pci_list_len ].fun   = fun;
                    lfdd_pci_list[ pci_list_len ].venid = (uint16_t)lfdd_pci_data.buf;
                    //
                    // Read and record Device ID
                    //
                    lfdd_pci_data.reg += 0x02;
                    LFDD_IOCTL( fd, LFDD_PCI_READ_WORD, lfdd_pci_data );
                    lfdd_pci_list[ pci_list_len ].devid = (uint16_t)lfdd_pci_data.buf;
                    //
                    // Get Texts
                    //
                    GetVendorAndDeviceTexts( lfdd_pci_list[ pci_list_len ].venid, lfdd_pci_list[ pci_list_len ].devid
                                            ,lfdd_pci_list[ pci_list_len ].ventxt, lfdd_pci_list[ pci_list_len ].devtxt );
                    //
                    // if we have mmio base, then
                    //   assign MMIO base if it's PCIE
                    //
                    if (pcix_mmio_base) {
                            AssignMMIOBase(fd, &lfdd_pci_data, &lfdd_pci_list[ pci_list_len ]);
                    }
                    //
                    // Move to next record
                    //
                    pci_list_len++;
                    if(pci_list_len>=LFDK_MAX_PCIBUF)
                    {
                        PrintFixedWin( PCIScanScreen, error, 1, 50, 10, 15, WHITE_RED, "Too much device number (>%d)! Program will abort!", LFDK_MAX_PCIBUF );
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
static int pci_scan_init(st_cmd_info *p_cmd, void *data);
static int pci_scan_start_win(st_cmd_info *p_cmd, void *data);
static int pci_scan_paint(st_cmd_info *p_cmd, void *data);
static int pci_scan_destroy_win(st_cmd_info *p_cmd, void *data);
static st_window_info  pci_scan_win_info  = {
        .name = "PCI-SCAN",
        .init = pci_scan_init,
        .start_win = pci_scan_start_win,
        .get_focus = NULL,
        .lost_focus = NULL,
        .paint = pci_scan_paint,
        .destroy_win = pci_scan_destroy_win
};

static uint64_t find_mmio_in_MCFG()
{
        FILE *fp = NULL;
        size_t byte_count;
        uint8_t buf[512];
        uint32_t *p_sig_off = NULL;
        uint32_t *p_table_len_off = NULL;
        uint8_t  *p_check_sum_off = NULL;
        int i;
        uint8_t sum;
        uint64_t pcie_mmio_base = 0;


        fp = fopen(ACPI_MCFG_PATH, "rb");
        if (!fp) {
                log_w("Can't find " ACPI_MCFG_PATH "\n");
                return 0;
        }

        byte_count = fread(buf, 1, sizeof(buf), fp);
        // MUST exist field size
        if (byte_count < 44) {
                log_w("table size is incorrect %d\n", byte_count);
                goto err_out;
        }

        p_sig_off = (uint32_t *) &buf[0];
        p_table_len_off = (uint32_t *) &buf[4];
        p_check_sum_off = &buf[9]; 

        //check whole table size
        if (*p_table_len_off != byte_count) {
                log_w("table size is incorrect. %d != %d\n", *p_table_len_off, byte_count);
                goto err_out;
        }

        //validate whole table
        sum = 0;
        for (i=0; i<byte_count; i++) {
                sum += buf[i];
        }
        if (sum) {
                log_w("Validate table by Checksum is invalid\n");
                goto err_out;
        }


        //now find mmio 
        for (i=44; i<byte_count; i+=16) {
                uint64_t *p_base_off = NULL;
                uint16_t *p_segg_num_off = NULL;
                uint8_t *p_start_bus_off = NULL;
                uint8_t *p_end_bus_off = NULL;

                p_base_off = (uint64_t *)&buf[i];
                p_segg_num_off = (uint16_t *)&buf[i+8];
                p_start_bus_off = (uint8_t *)&buf[i+10];
                p_end_bus_off = (uint8_t *)&buf[i+11];

                log_i("PCIE MMIO Base: 0x%" PRIx64 "\n"
                        "Segment: %d\n"
                        "Start bus: 0x%02X\n"
                        "End bus: 0x%02X\n",
                        *p_base_off,
                        *p_segg_num_off,
                        *p_start_bus_off,
                        *p_end_bus_off
                );
                //since only support one, just break ...
                pcie_mmio_base = *p_base_off;
                break;
        }//for i

err_out:
        fclose(fp);
        return pcie_mmio_base;
}

static int pci_scan_init(st_cmd_info *p_cmd, void *data)
{
        if (p_cmd->phy_pcix_mmio_base) {
                pcix_mmio_base = p_cmd->phy_pcix_mmio_base;
                log_i("Assign PCIX MMIO Base: 0x%" PRIx64 "\n", pcix_mmio_base);
        } else {
                pcix_mmio_base = find_mmio_in_MCFG();
                if (pcix_mmio_base >= (1ULL<<32)) {
                        log_e("Error!! Find MMIO space above 4GB !! 0x%" PRIx64 "\n", pcix_mmio_base);
                        pcix_mmio_base = 0;
                }

        }
        log_i("Final PCIX MMIO Base: 0x%" PRIx64 "\n", pcix_mmio_base);

        return 0;
}

static int pci_scan_destroy_win(st_cmd_info *p_cmd, void *data)
{
        DestroyWin( PCIScanScreen, scan );
        return 0;
}

int validate_pcie_r_single(int lfdd_fd, PCIData *p_pcie_data, int mode)
{
        int i;
        uint32_t tmp_buf;
        uint16_t validate_reg_map[] = {
                0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0A, 0x0B
        };
        char *mode_str[3] = { "byte", "word", "dword" };
        char *p_mode_ptr = NULL;

        if (!p_pcie_data->phy_base) {
                log_e("%s: fail %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }

        p_mode_ptr = mode_str[2];
        if (mode == LFDD_PCI_READ_BYTE) {
                p_mode_ptr = mode_str[0];
        } else if (mode == LFDD_PCI_READ_WORD) {
                p_mode_ptr = mode_str[1];
        }

        lfdd_pci_data.bus = p_pcie_data->bus;
        lfdd_pci_data.dev = p_pcie_data->dev;
        lfdd_pci_data.fun = p_pcie_data->fun;

        for (i=0; i<sizeof(validate_reg_map)/sizeof(uint16_t); i++) {
                //first, mmio read
                lfdd_pci_data.phy_base = p_pcie_data->phy_base;
                lfdd_pci_data.reg = validate_reg_map[i];
                if( ioctl(lfdd_fd, mode, &lfdd_pci_data) ) {
                        log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                        return ERR_TEST_FAIL;
                }
                tmp_buf = lfdd_pci_data.buf;
                if (i == 0 && !tmp_buf) {
                        log_e("%s, Read value error %d\n", __func__, __LINE__);
                }

                //second, io port read
                lfdd_pci_data.phy_base = 0;
                lfdd_pci_data.reg = validate_reg_map[i];
                if( ioctl(lfdd_fd, mode, &lfdd_pci_data) ) {
                        log_e("%s, Cannot execute command %d\n", __func__, __LINE__ );
                        return ERR_TEST_FAIL;
                }

                if (tmp_buf != lfdd_pci_data.buf) {
                        log_e("%s, Compare error[%s]. bus: 0x%02X, dev: 0x%02X, func: 0x%02X, Reg: 0x%02X, 0x%02X != 0x%02X\n", 
                                __func__, p_mode_ptr, 
                                lfdd_pci_data.bus, lfdd_pci_data.dev, lfdd_pci_data.fun,
                                validate_reg_map[i], tmp_buf, lfdd_pci_data.buf);
                        return ERR_TEST_FAIL;
                }
        }
        return 0;
}

int validate_pcie_r_256B(int lfdd_fd, PCIData *p_pcie_data)
{
        int i;
        uint8_t tmp_buf[LFDD_MASSBUF_SIZE];
        uint16_t validate_reg_map[] = {
                0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0A, 0x0B
        };

        //first, mmio read
        lfdd_pci_data.bus = p_pcie_data->bus;
        lfdd_pci_data.dev = p_pcie_data->dev;
        lfdd_pci_data.fun = p_pcie_data->fun;
        lfdd_pci_data.reg = 0;
        lfdd_pci_data.phy_base = p_pcie_data->phy_base;
        LFDD_IOCTL( lfdd_fd, LFDD_PCI_READ_256BYTE, lfdd_pci_data );

        //copy to tmp buf
        memcpy(tmp_buf, lfdd_pci_data.mass_buf, LFDD_MASSBUF_SIZE);

        //second, io port read
        lfdd_pci_data.reg = 0;
        lfdd_pci_data.phy_base = 0;
        LFDD_IOCTL( lfdd_fd, LFDD_PCI_READ_256BYTE, lfdd_pci_data );

        for (i=0; i<sizeof(validate_reg_map)/sizeof(uint16_t); i++) {
                if (tmp_buf[ validate_reg_map[i] ] != lfdd_pci_data.mass_buf[ validate_reg_map[i] ]) {
                        log_e("%s, Compare error. bus: 0x%02X, dev: 0x%02X, func: 0x%02X, Reg: 0x%02X, 0x%02X != 0x%02X\n", 
                                __func__, 
                                lfdd_pci_data.bus, lfdd_pci_data.dev, lfdd_pci_data.fun,
                                validate_reg_map[i], tmp_buf, lfdd_pci_data.buf);
                        return ERR_TEST_FAIL;
                }
        }

        return 0;
}

int validate_pcie_w_single(int lfdd_fd, PCIData *p_pcie_data, int r_mode, int w_mode)
{
        int i;
        uint32_t tmp_buf;
        char *mode_str[3] = { "byte", "word", "dword" };
        char *p_mode_ptr = NULL;
        uint32_t pattern = 0x12345678;
        uint32_t mask = 0;
        const uint16_t REG_TEST = 0x2C; //assume this reg. could be r/w. 4Byte

        if (!p_pcie_data->phy_base) {
                log_e("%s: fail %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }

        p_mode_ptr = mode_str[2];
        mask = ~0x0;
        if (w_mode == LFDD_PCI_WRITE_BYTE) {
                p_mode_ptr = mode_str[0];
                mask = 0x0ff;
        } else if (w_mode == LFDD_PCI_WRITE_WORD) {
                p_mode_ptr = mode_str[1];
                mask = 0x0ffff;
        }

        lfdd_pci_data.bus = p_pcie_data->bus;
        lfdd_pci_data.dev = p_pcie_data->dev;
        lfdd_pci_data.fun = p_pcie_data->fun;
        lfdd_pci_data.phy_base = p_pcie_data->phy_base;
        lfdd_pci_data.reg = REG_TEST;

        //first, save orignal value
        if( ioctl(lfdd_fd, r_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        tmp_buf = lfdd_pci_data.buf;


        //second, write zero's and validate
        lfdd_pci_data.buf = 0x00;
        if( ioctl(lfdd_fd, w_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        if( ioctl(lfdd_fd, r_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        if (lfdd_pci_data.buf != 0) {
                log_e("%s, Compare error[%s]. bus: 0x%02X, dev: 0x%02X, func: 0x%02X, Reg: 0x%02X, 0x%08X != 0\n", 
                        __func__, p_mode_ptr, 
                       lfdd_pci_data.bus, lfdd_pci_data.dev, lfdd_pci_data.fun,
                       REG_TEST, lfdd_pci_data.buf);
                return ERR_TEST_FAIL;
        }

        //third, write pattern and validate
        lfdd_pci_data.buf = pattern & mask;
        if( ioctl(lfdd_fd, w_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        if( ioctl(lfdd_fd, r_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        if (lfdd_pci_data.buf != (pattern & mask)) {
                log_e("%s, Compare error[%s]. bus: 0x%02X, dev: 0x%02X, func: 0x%02X, Reg: 0x%02X, 0x%08X != 0x%08X\n", 
                        __func__, p_mode_ptr, 
                       lfdd_pci_data.bus, lfdd_pci_data.dev, lfdd_pci_data.fun,
                       REG_TEST, lfdd_pci_data.buf, (pattern & mask));
                return ERR_TEST_FAIL;
        }

        //fourth, restore the value
        lfdd_pci_data.buf = tmp_buf;
        if( ioctl(lfdd_fd, w_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        if( ioctl(lfdd_fd, r_mode, &lfdd_pci_data) ) {
                log_e("%s, Cannot execute command %d\n", __func__, __LINE__);
                return ERR_TEST_FAIL;
        }
        if (lfdd_pci_data.buf != tmp_buf) {
                log_e("%s, Restore error[%s]. bus: 0x%02X, dev: 0x%02X, func: 0x%02X, Reg: 0x%02X, 0x%08X != 0x%08X\n", 
                        __func__, p_mode_ptr, 
                       lfdd_pci_data.bus, lfdd_pci_data.dev, lfdd_pci_data.fun,
                       REG_TEST, lfdd_pci_data.buf, tmp_buf);
                return ERR_TEST_FAIL;
        }

        return 0;
}
static void ValidationPCIE(int lfdd_fd)
{
        int i;
        int ret;

        for (i=0; i<pci_list_len; i++) {
                if (!lfdd_pci_list[i].phy_base)
                        continue;

                //Read testing
                ret = validate_pcie_r_single(lfdd_fd, &lfdd_pci_list[i], LFDD_PCI_READ_BYTE);
                if (ret) return;
                ret = validate_pcie_r_single(lfdd_fd, &lfdd_pci_list[i], LFDD_PCI_READ_WORD);
                if (ret) return;
                ret = validate_pcie_r_single(lfdd_fd, &lfdd_pci_list[i], LFDD_PCI_READ_DWORD);
                if (ret) return;
                ret = validate_pcie_r_256B(lfdd_fd, &lfdd_pci_list[i]);
                if (ret) return;

                //Write testing
                ret = validate_pcie_w_single(lfdd_fd, &lfdd_pci_list[i], LFDD_PCI_READ_BYTE, LFDD_PCI_WRITE_BYTE);
                if (ret) return;
                ret = validate_pcie_w_single(lfdd_fd, &lfdd_pci_list[i], LFDD_PCI_READ_WORD, LFDD_PCI_WRITE_WORD);
                if (ret) return;
                ret = validate_pcie_w_single(lfdd_fd, &lfdd_pci_list[i], LFDD_PCI_READ_DWORD, LFDD_PCI_WRITE_DWORD);
                if (ret) return;
        }

        log_i("%s: Success\n", __func__);

}

static int pci_scan_paint(st_cmd_info *p_cmd, void *data)
{
        if (request_window_state(&pci_scan_win_info, pci_scan_handle) == WM_FOREGROUND) {
                ScanPCIDevice(p_cmd->fd_lfdd);
                //ValidationPCIE(p_cmd->fd_lfdd);

                request_xfer_control(&pci_scan_win_info, pci_scan_handle, "PCI-LIST");
        }
        return 0;
}

static int pci_scan_start_win(st_cmd_info *p_cmd, void *data)
{
        int ret;

        request_windows_focus(&pci_scan_win_info, pci_scan_handle);

        return ret;
}

module_init(pci_scan_win_info, pci_scan_handle)

