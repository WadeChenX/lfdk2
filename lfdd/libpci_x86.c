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
#include <linux/highmem.h>

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

void __iomem *phy2virt_mem = NULL;
uint64_t  phy_map_start_addr = 0;
const size_t MAP_SIZE = LFDD_MASSBUF_SIZE; //assume power of 2

static inline int out_range(uint64_t addr, uint64_t start, size_t size)
{
        if (addr < start || addr >= start+size)
                return 1;
        return 0;
}


uint64_t lfdd_cal_pcix_addr( struct lfdd_pcix_t *ppci)
{
    uint64_t addr = 0;

    addr |= (ppci->bus & 0xff);

    addr <<= 5;
    addr |= (ppci->dev & 0x1f);

    addr <<= 3;
    addr |= (ppci->fun & 0x07);

    if (!ppci->phy_base) {
            addr <<= 8;
            addr |= 0x80000000;

            addr |= ppci->reg;
    } else {
            addr <<= 12;
            addr |= (ppci->reg & 0x0fff);

            addr += ppci->phy_base;
    }

    return addr;
}


uint8_t lfdd_pcix_read_byte(struct lfdd_pcix_t *ppci) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t value;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t addr_aligh_buf = addr & ~(MAP_SIZE-1);
    uint32_t off = 0;

    if (ppci->phy_base && (addr_aligh_buf + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return 0xFF;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;
            }
            off = addr - addr_aligh_buf;
    }

    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );

            // Write new address
            outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

            // Read Value in byte
            value = inl( LFDD_PCI_DATA_PORT );

            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );

            pr_info("ReadIO: 0x%08X\n", value);
    } else {
            value = readl(phy2virt_mem + (off & ~0x3));
            pr_info("ReadMMIO: 0x%08X\n", value);
    }

    spin_unlock_irqrestore( &lfdd_lock, flags );

    value >>= ((addr & 0x03) * 8);
    pr_info("value: 0x%02X\n", value & 0xff);

    return (value & 0xff);
}


uint16_t lfdd_pcix_read_word(struct lfdd_pcix_t *ppci) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t value;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t addr_aligh_buf = addr & ~(MAP_SIZE-1);
    uint32_t off = 0;

    if (ppci->phy_base && (addr_aligh_buf + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return 0xFFFF;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;
            } 
            off = addr - addr_aligh_buf;
    }

    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );

            // Write new address
            outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

            // Read Value in byte
            value = inl( LFDD_PCI_DATA_PORT );

            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );
    } else {
            value = readl(phy2virt_mem + (off & ~0x3));
    }
    value >>= ((addr & 0x03) * 8);

    spin_unlock_irqrestore( &lfdd_lock, flags );

    return (value & 0xffff);
}


uint32_t lfdd_pcix_read_dword(struct lfdd_pcix_t *ppci ) 
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t value;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t addr_aligh_buf = addr & ~(MAP_SIZE-1);
    uint32_t off = 0;

    //pr_info("Device: %02X %02X %02X.%04X addr: 0x%08X\n", 
    //    ppci->buf, ppci->dev, ppci->fun, ppci->reg,
    //    addr
    //    );
    if (ppci->phy_base && (addr_aligh_buf + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return ~0;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;
            }
            off = addr - addr_aligh_buf;
    }

    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );

            // Write new address
            outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

            // Read Value in byte
            value = inl( LFDD_PCI_DATA_PORT );

            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );
    } else {
            value = readl(phy2virt_mem + (off & ~0x3));
    }

    spin_unlock_irqrestore( &lfdd_lock, flags );

    return value;
}


void lfdd_pcix_write_byte(struct lfdd_pcix_t *ppci)
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t temp;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t value;
    uint32_t addr_aligh_buf = addr & ~(MAP_SIZE-1);
    uint32_t off = 0;

    if (ppci->phy_base && (addr_aligh_buf + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return ;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;
            } 
            off = addr - addr_aligh_buf;
    }

    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );

            // Write new address
            outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

            temp = inl( LFDD_PCI_DATA_PORT );
            //pr_info("pci_wb: orig 0x%08X\n", temp);

            value = (ppci->buf & 0xff) << ((addr & 0x03) * 8);
            temp &= ~(0x000000ff << ((addr & 0x03) * 8));
            temp |= value;

            // Write new Value
            outl( temp, LFDD_PCI_DATA_PORT );

            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );
            //pr_info("pci_wb: 0x%08X\n", temp);
    } else {
            temp = readl(phy2virt_mem + (off & ~0x3));
            //pr_info("pcix_wb: orig 0x%08X\n", temp);
            value = (ppci->buf & 0x0ff) << ((addr & 0x03) * 8);
            temp &= ~(0x000000ff << ((addr & 0x03) * 8));
            temp |= value;
            writel(temp, phy2virt_mem + (off & ~0x3));
            //pr_info("pcix_wb: 0x%08X\n", temp);
    }

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


void lfdd_pcix_write_word( struct lfdd_pcix_t *ppci)
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t temp;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t value;
    uint32_t addr_aligh_buf = addr & ~(MAP_SIZE-1);
    uint32_t off = 0;

    if (ppci->phy_base && (addr_aligh_buf + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return ;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;
            } 
            off = addr - addr_aligh_buf;
    }

    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );

            // Write new address
            outl( addr & 0xfffffffc, LFDD_PCI_ADDR_PORT );

            temp = inl( LFDD_PCI_DATA_PORT );
            //pr_info("pci_ww: orig 0x%08X\n", temp);

            value = (ppci->buf & 0x0ffff) << ((addr & 0x02) * 8);
            temp &= ~(0x0000ffff << ((addr & 0x02) * 8));
            temp |= value;

            // Write new Value
            outl( temp, LFDD_PCI_DATA_PORT );

            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );
    } else {
            temp = readl(phy2virt_mem + (off & ~0x3));
            value = (ppci->buf & 0x0ffff) << ((addr & 0x02) * 8);
            temp &= ~(0x0000ffff << ((addr & 0x02) * 8));
            temp |= value;
            writel(temp, phy2virt_mem + (off & ~0x3));
    }
    spin_unlock_irqrestore( &lfdd_lock, flags );
    //pr_info("pci_ww: 0x%08X\n", temp);
}


void lfdd_pcix_write_dword( struct lfdd_pcix_t *ppci)
{
    unsigned long flags;
    uint32_t orig_idx;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t addr_aligh_buf = addr & ~(MAP_SIZE-1);
    uint32_t off = 0;

    if (addr & 0x3 || 
               ( ppci->phy_base && (addr_aligh_buf + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory)))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return ;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr_aligh_buf, MAP_SIZE);
                    phy_map_start_addr = addr_aligh_buf;
            } 
            off = addr - addr_aligh_buf;
    }

    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );

            // Write new address
            outl( addr, LFDD_PCI_ADDR_PORT );

            // Write Value in byte
            outl( ppci->buf, LFDD_PCI_DATA_PORT );

            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );
    } else {
            writel(ppci->buf, phy2virt_mem + (off & ~0x3));
    }

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


void lfdd_pcix_read_256byte( struct lfdd_pcix_t *ppci ) 
{ 
    unsigned long flags;
    uint32_t orig_idx = 0;
    int i;
    uint32_t addr = (uint32_t) lfdd_cal_pcix_addr( ppci);
    uint32_t value;
    //uint64_t tmp_base = 0;

    if (ppci->phy_base && (addr + LFDD_MASSBUF_SIZE <= virt_to_phys(high_memory))) {
            pr_warn("Invalid mmio address: 0x%08X\n", addr);
            return ;
    }

    if (ppci->phy_base) {
            if (!phy2virt_mem && ppci->phy_base) {
                    phy2virt_mem = ioremap( addr, MAP_SIZE);
                    phy_map_start_addr = addr;

            } else if (phy2virt_mem && out_range(addr, phy_map_start_addr, MAP_SIZE)) {
                    iounmap( phy2virt_mem);
                    //remap mmio
                    phy2virt_mem = ioremap( addr, MAP_SIZE);
                    phy_map_start_addr = addr;
            }
    }

    //pr_info("base addr: 0x%08X\n", ppci->phy_base);
    //tmp_base = ppci->phy_base;
    //ppci->phy_base = 0;
    //pr_info("calc addr: 0x%08X\n", addr);
    //addr = (uint32_t) lfdd_cal_pcix_addr(ppci);
    //pr_info("compat addr: 0x%08X\n", addr);


    spin_lock_irqsave( &lfdd_lock, flags );

    if (!ppci->phy_base) {
            // Save original PCI address
            orig_idx = inl( LFDD_PCI_ADDR_PORT );
    }

    // Read Values
    for( i = 0 ; i < LFDD_MASSBUF_SIZE ; i += 4 ) {
            if (!ppci->phy_base) {
                    outl( addr + i, LFDD_PCI_ADDR_PORT );
                    value = inl( LFDD_PCI_DATA_PORT );
            } else {
                    value = readl(phy2virt_mem+i);
            }

            ppci->mass_buf[ i ]     = (uint8_t) value        & 0xff;
            ppci->mass_buf[ i + 1 ] = (uint8_t)(value >> 8 ) & 0xff;
            ppci->mass_buf[ i + 2 ] = (uint8_t)(value >> 16) & 0xff;
            ppci->mass_buf[ i + 3 ] = (uint8_t)(value >> 24) & 0xff;
    }

    if (!ppci->phy_base) {
            // Restore original PCI address
            outl( orig_idx, LFDD_PCI_ADDR_PORT );
    }

    spin_unlock_irqrestore( &lfdd_lock, flags );
}


