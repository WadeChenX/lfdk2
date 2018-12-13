/*
 * LFDD - Linux Firmware Debug Driver
 * File: lfdd.c
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/proc_fs.h>

#include <linux/delay.h>

#include <asm/io.h>
#include <asm/irq.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
#include <asm/uaccess.h>
#else
#include <linux/uaccess.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include <asm/system.h>
#endif

#include "lfdd.h"


#define LFDD_PCI_READ( RFUNC, DATA ) {                                                  \
                                                                                        \
    if( copy_from_user( &DATA, argp, sizeof( DATA ) ) ) {                               \
                                                                                        \
        return -EFAULT;                                                                 \
    }                                                                                   \
                                                                                        \
    DATA.buf = RFUNC( lfdd_cal_pci_addr( DATA.bus, DATA.dev, DATA.fun, DATA.reg ) );    \
                                                                                        \
    return copy_to_user( argp, &DATA, sizeof( DATA ) );                                 \
}


#define LFDD_PCI_WRITE( WFUNC, DATA ) {                                                 \
                                                                                        \
    if( copy_from_user( &DATA, argp, sizeof( DATA ) ) ) {                               \
                                                                                        \
        return -EFAULT;                                                                 \
    }                                                                                   \
                                                                                        \
    WFUNC( DATA.buf, lfdd_cal_pci_addr( DATA.bus, DATA.dev, DATA.fun, DATA.reg ) );     \
    return 0;                                                                           \
}


#define LFDD_MEM_READ( RFUNC, DATA ) {                      \
                                                            \
    if( copy_from_user( &DATA, argp, sizeof( DATA ) ) ) {   \
                                                            \
        return -EFAULT;                                     \
    }                                                       \
                                                            \
    DATA.buf = RFUNC( DATA.addr );                          \
                                                            \
    return copy_to_user( argp, &DATA, sizeof( DATA ) );     \
}


#define LFDD_MEM_WRITE( WFUNC, DATA ) {                     \
                                                            \
    if( copy_from_user( &DATA, argp, sizeof( DATA ) ) ) {   \
                                                            \
        return -EFAULT;                                     \
    }                                                       \
                                                            \
    WFUNC( DATA.buf, DATA.addr );                           \
    return 0;                                               \
}


spinlock_t lfdd_lock;


static int lfdd_open( struct inode *inode, struct file *file ) 
{
        return 0;
}


static int lfdd_release( struct inode *inode, struct file *file ) 
{
        return 0;
}

char cmd_str_table[][64] = {
	"PCI_READ_BYTE",
	"PCI_READ_WORD",
	"PCI_READ_DWORD",
	"PCI_WRITE_BYTE",
	"PCI_WRITE_WORD",
	"PCI_WRITE_DWORD",
	"PCI_READ_256BYTE",
	"PCIE_READ_BYTE",
	"PCIE_READ_WORD",
	"PCIE_READ_DWORD",
	"PCIE_WRITE_BYTE",
	"PCIE_WRITE_WORD",
	"PCIE_WRITE_DWORD",
	"PCIE_READ_256BYTE",
	"MEM_READ_BYTE",
	"MEM_READ_WORD",
	"MEM_READ_DWORD",
	"MEM_WRITE_BYTE",
	"MEM_WRITE_WORD",
	"MEM_WRITE_DWORD",
	"MEM_READ_256BYTE",
	"IO_READ_BYTE",
	"IO_READ_WORD",
	"IO_READ_DWORD",
	"IO_WRITE_BYTE",
	"IO_WRITE_WORD",
	"IO_WRITE_DWORD",
	"IO_READ_256BYTE",
        NULL
};

static long lfdd_ioctl( struct file *file, unsigned int cmd, unsigned long arg ) 
{
        struct lfdd_pci_t lfdd_pci_data;
        struct lfdd_mem_t lfdd_mem_data;
        struct lfdd_io_t lfdd_io_data;
        void __user *argp = (void __user *)arg;

        if (cmd < LFDD_IOCTL_CMD_MAX &&
                cmd != LFDD_PCI_READ_256BYTE &&
                cmd != LFDD_MEM_READ_256BYTE &&
                cmd != LFDD_IO_READ_256BYTE) { 
                pr_info("%s\n", cmd_str_table[cmd]);
        }

        switch( cmd ) {
                //
                // PCI Functions
                //
                case LFDD_PCI_READ_256BYTE:
                        if( copy_from_user( &lfdd_pci_data, argp, sizeof( struct lfdd_pci_t ) ) ) {
                                pr_info("Copy pci cmd fail\n");
                                return -EFAULT;
                        }

                        memset( lfdd_pci_data.mass_buf, 0, LFDD_MASSBUF_SIZE );
                        lfdd_pci_read_256byte( &lfdd_pci_data );

                        return copy_to_user( argp, &lfdd_pci_data, sizeof( struct lfdd_pci_t ) );

                case LFDD_PCI_READ_BYTE:
                        LFDD_PCI_READ( lfdd_pci_read_byte, lfdd_pci_data );

                case LFDD_PCI_READ_WORD:
                        LFDD_PCI_READ( lfdd_pci_read_word, lfdd_pci_data );

                case LFDD_PCI_READ_DWORD:
                        LFDD_PCI_READ( lfdd_pci_read_dword, lfdd_pci_data );

                case LFDD_PCI_WRITE_BYTE:
                        LFDD_PCI_WRITE( lfdd_pci_write_byte, lfdd_pci_data );

                case LFDD_PCI_WRITE_WORD:
                        LFDD_PCI_WRITE( lfdd_pci_write_word, lfdd_pci_data );

                case LFDD_PCI_WRITE_DWORD:
                        LFDD_PCI_WRITE( lfdd_pci_write_dword, lfdd_pci_data );

                case LFDD_MEM_READ_256BYTE:
                        if( copy_from_user( &lfdd_mem_data, argp, sizeof( struct lfdd_mem_t ) ) ) {
                                pr_info("Copy mem cmd fail\n");
                                return -EFAULT;
                        }

                        memset( lfdd_mem_data.mass_buf, 0, LFDD_MASSBUF_SIZE );
                        lfdd_mem_read_256byte( &lfdd_mem_data );

                        return copy_to_user( argp, &lfdd_mem_data, sizeof( struct lfdd_mem_t ) );

                case LFDD_MEM_READ_BYTE:
                        LFDD_MEM_READ( lfdd_mem_read_byte, lfdd_mem_data );

                case LFDD_MEM_READ_WORD:
                        LFDD_MEM_READ( lfdd_mem_read_word, lfdd_mem_data );

                case LFDD_MEM_READ_DWORD:
                        LFDD_MEM_READ( lfdd_mem_read_dword, lfdd_mem_data );

                case LFDD_MEM_WRITE_BYTE:
                        LFDD_MEM_WRITE( lfdd_mem_write_byte, lfdd_mem_data );

                case LFDD_MEM_WRITE_WORD:
                        LFDD_MEM_WRITE( lfdd_mem_write_word, lfdd_mem_data );

                case LFDD_MEM_WRITE_DWORD:
                        LFDD_MEM_WRITE( lfdd_mem_write_dword, lfdd_mem_data );        

                case LFDD_IO_READ_256BYTE:
                        if( copy_from_user( &lfdd_io_data, argp, sizeof( struct lfdd_io_t ) ) ) {
                                pr_info("Copy io cmd fail\n");
                                return -EFAULT;
                        }

                        memset( lfdd_io_data.mass_buf, 0, LFDD_MASSBUF_SIZE );
                        lfdd_io_read_256byte( &lfdd_io_data );

                        return copy_to_user( argp, &lfdd_io_data, sizeof( struct lfdd_io_t ) );

                case LFDD_IO_WRITE_BYTE:
                        LFDD_MEM_WRITE( lfdd_io_write_byte, lfdd_io_data );

                case LFDD_IO_READ_BYTE:
                        LFDD_MEM_READ( lfdd_io_read_byte, lfdd_io_data );

        }

        return -EINVAL;
}


static struct file_operations lfdd_fops = {

        .owner      =   THIS_MODULE,
        .unlocked_ioctl = lfdd_ioctl,
        .compat_ioctl =   lfdd_ioctl,
        .open       =   lfdd_open,
        .release    =   lfdd_release,
};


static struct miscdevice lfdd_dev = {

        .minor      =   100,
        .name       =   "lfdd",
        .fops       =   &lfdd_fops,
};


static int __init lfdd_init( void ) 
{
        int ret;


        printk( KERN_INFO "lfdd: Linux Firmware Debug Driver Version %s\n", LFDD_VERSION );


        // Register character device
        ret = misc_register( &lfdd_dev );
        if( ret < 0 ) {

                DBGPRINT( "register lfdd driver failed.\n" );
                return ret;
        }


        // Initialize spin lock
        spin_lock_init( &lfdd_lock );


        return 0;
}


static void __exit lfdd_exit( void ) 
{
        misc_deregister( &lfdd_dev );
        printk( KERN_INFO "lfdd: driver unloaded.\n" );
}


module_init( lfdd_init );
module_exit( lfdd_exit );

MODULE_AUTHOR( "Merck Hung <merckhung@gmail.com>" );
MODULE_DESCRIPTION( "Linux Firmware Debug I/O Control Driver" );
MODULE_LICENSE( "GPL" );


