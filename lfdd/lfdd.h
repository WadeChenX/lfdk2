/*
 * LFDD - Linux Firmware Debug Driver
 * File: lfdd.h
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
#define LFDD_VERSION            "2.0.1"


#define LFDD_DEFAULT_PATH       "/dev/lfdd"
#define LFDD_MASSBUF_SIZE       256
#define LFDD_PCI_ADDR_PORT      0xcf8
#define LFDD_PCI_DATA_PORT      0xcfc


/*
 * IOCTL commands
 */
enum {

	LFDD_PCI_READ_BYTE = 0,
	LFDD_PCI_READ_WORD,
	LFDD_PCI_READ_DWORD,
	LFDD_PCI_WRITE_BYTE,
	LFDD_PCI_WRITE_WORD,
	LFDD_PCI_WRITE_DWORD,
	LFDD_PCI_READ_256BYTE,

	LFDD_PCIE_READ_BYTE,
	LFDD_PCIE_READ_WORD,
	LFDD_PCIE_READ_DWORD,
	LFDD_PCIE_WRITE_BYTE,
	LFDD_PCIE_WRITE_WORD,
	LFDD_PCIE_WRITE_DWORD,
	LFDD_PCIE_READ_256BYTE,

	LFDD_MEM_READ_BYTE,
	LFDD_MEM_READ_WORD,
	LFDD_MEM_READ_DWORD,
	LFDD_MEM_WRITE_BYTE,
	LFDD_MEM_WRITE_WORD,
	LFDD_MEM_WRITE_DWORD,
	LFDD_MEM_READ_256BYTE,

	LFDD_IO_READ_BYTE,
	LFDD_IO_READ_WORD,
	LFDD_IO_READ_DWORD,
	LFDD_IO_WRITE_BYTE,
	LFDD_IO_WRITE_WORD,
	LFDD_IO_WRITE_DWORD,
	LFDD_IO_READ_256BYTE,

	LFDD_I2C_READ_BYTE,
	LFDD_I2C_WRITE_BYTE,

	LFDD_NVRAM_READ_BYTE,
	LFDD_NVRAM_WRITE_BYTE
};


struct lfdd_pci_t {
        uint8_t   bus;
        uint8_t   dev;
        uint8_t   fun;
        uint8_t   reg;

        uint32_t  buf;
        uint8_t   mass_buf[ LFDD_MASSBUF_SIZE ];
};


struct lfdd_pcie_t {
        uint8_t   bus;
        uint8_t   dev;
        uint8_t   fun;
        uint8_t   reg;

        uint32_t  buf;
        uint8_t   mass_buf[ LFDD_MASSBUF_SIZE ];
};


struct lfdd_mem_t {
        uint32_t    addr;
        uint32_t    buf;
        uint8_t     mass_buf[ LFDD_MASSBUF_SIZE ];
};


struct lfdd_io_t {
        uint32_t    addr;
        uint32_t    buf;
        uint8_t     mass_buf[ LFDD_MASSBUF_SIZE ];
};


struct lfdd_i2c_t {
        uint8_t	addr;
        uint8_t	off;
        uint8_t	buf;
        uint8_t	mass_buf[ LFDD_MASSBUF_SIZE ];
};


#ifdef LFDD_DEBUG
#define DBGPRINT( msg, args... )	printk( KERN_INFO "lfdd %s: " msg, __func__, ##args );
#else
#define DBGPRINT( msg, args... )	do{ }while( 0 ); 
#endif


#ifdef __KERNEL__
uint8_t lfdd_io_read_byte( uint32_t addr );
void lfdd_io_write_byte( uint8_t value, uint32_t addr );
uint16_t lfdd_io_read_word( uint32_t addr );
void lfdd_io_write_word( unsigned short value, uint32_t addr );
uint32_t lfdd_io_read_dword( uint32_t addr );
void lfdd_io_write_dword( uint32_t value, uint32_t addr );
void lfdd_io_read_256byte( struct lfdd_io_t *pio );

uint8_t lfdd_mem_read_byte( uint32_t addr );
uint16_t lfdd_mem_read_word( uint32_t addr );
uint32_t lfdd_mem_read_dword( uint32_t addr );
void lfdd_mem_write_byte( uint32_t value, uint32_t addr );
void lfdd_mem_write_word( uint32_t value, uint32_t addr );
void lfdd_mem_write_dword( uint32_t value, uint32_t addr );
void lfdd_mem_read_256byte( struct lfdd_mem_t *pmem );

uint32_t lfdd_cal_pci_addr( uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg );
uint8_t lfdd_pci_read_byte( uint32_t addr );
uint16_t lfdd_pci_read_word( uint32_t addr );
uint32_t lfdd_pci_read_dword( uint32_t addr );
void lfdd_pci_write_byte( uint32_t value, uint32_t addr );
void lfdd_pci_write_word( uint32_t value, uint32_t addr );
void lfdd_pci_write_dword( uint32_t value, uint32_t addr );
void lfdd_pci_read_256byte( struct lfdd_pci_t *ppci );
#endif


