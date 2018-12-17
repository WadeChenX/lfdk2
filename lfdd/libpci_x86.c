/*
 * LFDD - Linux Firmware Debug Driver
 * File: libpci_x86.c
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

#include "lfdd.h"


extern spinlock_t lfdd_lock;


uint32_t lfdd_cal_pci_addr( uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg ) 
{
    uint32_t addr = 0;

    addr |= (bus & 0xff);

    addr <<= 5;
    addr |= (dev & 0x1f);

    addr <<= 3;
    addr |= (fun & 0x07);

    addr <<= 8;
    addr |= 0x80000000;

    addr |= reg;

    return addr;
}


uint8_t lfdd_pci_read_byte( uint32_t addr ) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t value;

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Write new address
    outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

    // Read Value in byte
    value = inl( LFDD_PCI_DATA_PORT );

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );

    value >>= ((addr & 0x03) * 8);

    return (value & 0xff);
}


uint16_t lfdd_pci_read_word( uint32_t addr ) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t value;

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Write new address
    outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

    // Read Value in byte
    value = inl( LFDD_PCI_DATA_PORT );

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );

    value >>= ((addr & 0x03) * 8);

    return (value & 0xffff);
}


uint32_t lfdd_pci_read_dword( uint32_t addr ) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t value;

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Write new address
    outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

    // Read Value in byte
    value = inl( LFDD_PCI_DATA_PORT );

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );

    return value;
}


void lfdd_pci_write_byte( uint32_t value, uint32_t addr ) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t temp;

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Write new address
    outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

    temp = inl( LFDD_PCI_DATA_PORT );
    //pr_info("pci_wb: orig 0x%08X\n", temp);

    value = (value & 0xff) << ((addr & 0x03) * 8);
    temp &= ~(0x000000ff << ((addr & 0x03) * 8));
    temp |= value;

    // Write new Value
    outl( temp, LFDD_PCI_DATA_PORT );

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );
    //pr_info("pci_wb: 0x%08X\n", temp);
}


void lfdd_pci_write_word( uint32_t value, uint32_t addr ) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t temp;

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Write new address
    outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

    temp = inl( LFDD_PCI_DATA_PORT );
    //pr_info("pci_ww: orig 0x%08X\n", temp);

    value = (value & 0xffff) << ((addr & 0x02) * 8);
    temp &= ~(0x0000ffff << ((addr & 0x02) * 8));
    temp |= value;

    // Write new Value
    outl( temp, LFDD_PCI_DATA_PORT );

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );
    //pr_info("pci_ww: 0x%08X\n", temp);
}


void lfdd_pci_write_dword( uint32_t value, uint32_t addr ) 
{
    unsigned long flags;
    uint32_t orig_idx;

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Write new address
    outl( addr, LFDD_PCI_ADDR_PORT );

    // Write Value in byte
    outl( value, LFDD_PCI_DATA_PORT );

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


void lfdd_pci_read_256byte( struct lfdd_pci_t *ppci ) 
{ 
    unsigned long flags;
    uint32_t orig_idx;
    int i, value;
    uint32_t addr = lfdd_cal_pci_addr( ppci->bus, ppci->dev, ppci->fun, ppci->reg );

    spin_lock_irqsave( &lfdd_lock, flags );

    // Save original PCI address
    orig_idx = inl( LFDD_PCI_ADDR_PORT );

    // Read Values
    for( i = 0 ; i < LFDD_MASSBUF_SIZE ; i += 4 ) {

        outl( addr + i, LFDD_PCI_ADDR_PORT );
        value = inl( LFDD_PCI_DATA_PORT );

        ppci->mass_buf[ i ]     = (uint8_t) value        & 0xff;
        ppci->mass_buf[ i + 1 ] = (uint8_t)(value >> 8 ) & 0xff;
        ppci->mass_buf[ i + 2 ] = (uint8_t)(value >> 16) & 0xff;
        ppci->mass_buf[ i + 3 ] = (uint8_t)(value >> 24) & 0xff;
    }

    // Restore original PCI address
    outl( orig_idx, LFDD_PCI_ADDR_PORT );

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


