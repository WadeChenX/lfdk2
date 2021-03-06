/*
 * LFDD - Linux Firmware Debug Driver
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/proc_fs.h>

#include <linux/delay.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
#include <asm/system.h>
#endif

#include <linux/highmem.h>

#include "lfdd.h"


extern spinlock_t lfdd_lock;


uint8_t lfdd_mem_read_byte( uint32_t addr ) 
{
    uint8_t __iomem *phymem;
    uint32_t value;

    // Check the range of physical address
    if( ((0xffffffff - addr) <= LFDD_MASSBUF_SIZE) 
        || ((addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        return 0xff;
    }

    // Map physical memory address
    phymem = phys_to_virt( addr );
    value = *phymem;
    phymem = NULL;

    return (value & 0xff);
}


uint16_t lfdd_mem_read_word( uint32_t addr ) 
{
    uint16_t __iomem *phymem;
    uint32_t value;

    // Check the range of physical address
    if( ((0xffffffff - addr) <= LFDD_MASSBUF_SIZE) 
        || ((addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        return 0xff;
    }

    // Map physical memory address
    phymem = phys_to_virt( addr );
    value = *phymem;
    phymem = NULL;

    return (value & 0xffff);
}


uint32_t lfdd_mem_read_dword( uint32_t addr ) 
{
    uint32_t __iomem *phymem;
    uint32_t value;

    // Check the range of physical address
    if( ((0xffffffff - addr) <= LFDD_MASSBUF_SIZE) 
        || ((addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        return 0xff;
    }

    // Map physical memory address
    phymem = phys_to_virt( addr );
    value = *phymem;
    phymem = NULL;

    return value;
}


void lfdd_mem_write_byte( uint32_t value, uint32_t addr ) 
{
    unsigned long flags;
    uint8_t __iomem *phymem;
    uint32_t temp;

    // Check the range of physical address
    if( ((0xffffffff - addr) <= LFDD_MASSBUF_SIZE) 
        || ((addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        return;
    }

    // Map physical memory address
    phymem = phys_to_virt( addr );

    spin_lock_irqsave( &lfdd_lock, flags );
    temp = *phymem;
    temp &= ~0xff;
    temp |= (uint8_t)(value & 0xff);
    *phymem = (uint32_t)temp;
    spin_unlock_irqrestore( &lfdd_lock, flags );

    phymem = NULL;
}


void lfdd_mem_write_word( uint32_t value, uint32_t addr ) 
{
    unsigned long flags;
    uint16_t __iomem *phymem;
    uint32_t temp;

    // Check the range of physical address
    if( ((0xffffffff - addr) <= LFDD_MASSBUF_SIZE) 
        || ((addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        return;
    }

    // Map physical memory address
    phymem = phys_to_virt( addr );

    spin_lock_irqsave( &lfdd_lock, flags );
    temp = *phymem;
    temp &= ~0xffff;
    temp |= (uint16_t)(value & 0xffff);
    *phymem = temp;
    spin_unlock_irqrestore( &lfdd_lock, flags );

    phymem = NULL;
}


void lfdd_mem_write_dword( uint32_t value, uint32_t addr ) 
{
    unsigned long flags;
    uint32_t __iomem *phymem;

    // Check the range of physical address
    if( ((0xffffffff - addr) <= LFDD_MASSBUF_SIZE) 
        || ((addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        return;
    }

    // Map physical memory address
    phymem = phys_to_virt( addr );

    spin_lock_irqsave( &lfdd_lock, flags );
    *phymem = value;
    spin_unlock_irqrestore( &lfdd_lock, flags );

    phymem = NULL;
}


void lfdd_mem_read_256byte( struct lfdd_mem_t *pmem )
{
    uint8_t __iomem *phymem;
    void __iomem *virtmem, *p;
    int i;

    // Check the range of physical address
    if( ((0xffffffff - pmem->addr) <= LFDD_MASSBUF_SIZE) 
        || ((pmem->addr + LFDD_MASSBUF_SIZE) >= virt_to_phys( high_memory )) ) {

        virtmem = ioremap( pmem->addr, LFDD_MASSBUF_SIZE );

        if( virtmem ) {

            p = virtmem;
            for( i = 0 ; i < LFDD_MASSBUF_SIZE ; i++, p++ ) {

                pmem->mass_buf[ i ] = readb( p );
            }
            iounmap( virtmem );
        }
        else {

            memset( pmem->mass_buf, 0xff, LFDD_MASSBUF_SIZE );
        }
    }
    else {

        // Map physical memory address
        phymem = phys_to_virt( pmem->addr );

        // Read LFDD_MASSBUF_SIZE bytes
        for( i = 0 ; i < LFDD_MASSBUF_SIZE ; i++ ) {

            pmem->mass_buf[ i ] = *(phymem + i);
        }

        phymem = NULL;
    }
}


