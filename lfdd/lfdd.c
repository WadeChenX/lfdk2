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
    DATA.buf = RFUNC(&DATA);                                                             \
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
    WFUNC(&DATA);                                                                        \
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

//
// implement kernel log function
//
static char k_log_buf[512];
static ssize_t lfdd_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        int copy_len = len;

        if (copy_len >= sizeof(k_log_buf))
                copy_len = sizeof(k_log_buf);

        if( copy_from_user(k_log_buf, buf, copy_len) ) {
                return -EFAULT;
        }
        pr_info("%s", k_log_buf);

        return len;
}


static int lfdd_release( struct inode *inode, struct file *file ) 
{
        return 0;
}

static long lfdd_ioctl( struct file *file, unsigned int cmd, unsigned long arg ) 
{
        struct lfdd_pcix_t lfdd_pcix_data;
        struct lfdd_mem_t lfdd_mem_data;
        struct lfdd_io_t lfdd_io_data;
        void __user *argp = (void __user *)arg;

        if (    cmd != LFDD_PCI_READ_256BYTE &&
                cmd != LFDD_MEM_READ_256BYTE &&
                cmd != LFDD_IO_READ_256BYTE) { 
              pr_debug("[0x%08X]\n", cmd);
        }

        switch( cmd ) {
                //
                // PCI Functions
                //
                case LFDD_PCI_READ_256BYTE:
                        if( copy_from_user( &lfdd_pcix_data, argp, sizeof( struct lfdd_pcix_t ) ) ) {
                                pr_info("Copy pci cmd fail\n");
                                return -EFAULT;
                        }

                        memset( lfdd_pcix_data.mass_buf, 0, LFDD_MASSBUF_SIZE );
                        lfdd_pcix_read_256byte( &lfdd_pcix_data );

                        return copy_to_user( argp, &lfdd_pcix_data, sizeof( struct lfdd_pcix_t ) );

                case LFDD_PCI_READ_BYTE:
                        LFDD_PCI_READ( lfdd_pcix_read_byte, lfdd_pcix_data );

                case LFDD_PCI_READ_WORD:
                        LFDD_PCI_READ( lfdd_pcix_read_word, lfdd_pcix_data );

                case LFDD_PCI_READ_DWORD:
                        LFDD_PCI_READ( lfdd_pcix_read_dword, lfdd_pcix_data );

                case LFDD_PCI_WRITE_BYTE:
                        LFDD_PCI_WRITE( lfdd_pcix_write_byte, lfdd_pcix_data );

                case LFDD_PCI_WRITE_WORD:
                        LFDD_PCI_WRITE( lfdd_pcix_write_word, lfdd_pcix_data );

                case LFDD_PCI_WRITE_DWORD:
                        LFDD_PCI_WRITE( lfdd_pcix_write_dword, lfdd_pcix_data );

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
        .write      =   lfdd_write,
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


