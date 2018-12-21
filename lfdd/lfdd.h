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

//include pci/pcie
struct lfdd_pcix_t {
        uint8_t   bus;
        uint8_t   dev;
        uint8_t   fun;
        uint16_t  reg;

        // cmd that phy_base != 0, then use mmio method
        uint64_t  phy_base;

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

/*
 * IOCTL commands
 */

#define IOC_MAGIC 'L'
enum {

	PCI_READ_BYTE = 0,
	PCI_READ_WORD,
	PCI_READ_DWORD,
	PCI_WRITE_BYTE,
	PCI_WRITE_WORD,
	PCI_WRITE_DWORD,
	PCI_READ_256BYTE,

	MEM_READ_BYTE,
	MEM_READ_WORD,
	MEM_READ_DWORD,
	MEM_WRITE_BYTE,
	MEM_WRITE_WORD,
	MEM_WRITE_DWORD,
	MEM_READ_256BYTE,

	IO_READ_BYTE,
	IO_READ_WORD,
	IO_READ_DWORD,
	IO_WRITE_BYTE,
	IO_WRITE_WORD,
	IO_WRITE_DWORD,
	IO_READ_256BYTE,

        IOCTL_CMD_MAX
};

#define LFDD_PCI_READ_BYTE      _IOR(IOC_MAGIC, PCI_READ_BYTE, struct lfdd_pcix_t)
#define LFDD_PCI_READ_WORD      _IOR(IOC_MAGIC, PCI_READ_WORD, struct lfdd_pcix_t)
#define LFDD_PCI_READ_DWORD     _IOR(IOC_MAGIC, PCI_READ_DWORD, struct lfdd_pcix_t)
#define LFDD_PCI_WRITE_BYTE     _IOW(IOC_MAGIC, PCI_WRITE_BYTE, struct lfdd_pcix_t)
#define LFDD_PCI_WRITE_WORD     _IOW(IOC_MAGIC, PCI_WRITE_WORD, struct lfdd_pcix_t)
#define LFDD_PCI_WRITE_DWORD    _IOW(IOC_MAGIC, PCI_WRITE_DWORD, struct lfdd_pcix_t)
#define LFDD_PCI_READ_256BYTE   _IOR(IOC_MAGIC, PCI_READ_256BYTE, struct lfdd_pcix_t)

#define LFDD_MEM_READ_BYTE      _IOR(IOC_MAGIC, MEM_READ_BYTE, struct lfdd_mem_t)
#define LFDD_MEM_READ_WORD      _IOR(IOC_MAGIC, MEM_READ_WORD, struct lfdd_mem_t)
#define LFDD_MEM_READ_DWORD     _IOR(IOC_MAGIC, MEM_READ_DWORD, struct lfdd_mem_t)
#define LFDD_MEM_WRITE_BYTE     _IOW(IOC_MAGIC, MEM_WRITE_BYTE, struct lfdd_mem_t)
#define LFDD_MEM_WRITE_WORD     _IOW(IOC_MAGIC, MEM_WRITE_WORD, struct lfdd_mem_t)
#define LFDD_MEM_WRITE_DWORD    _IOW(IOC_MAGIC, MEM_WRITE_DWORD, struct lfdd_mem_t)
#define LFDD_MEM_READ_256BYTE   _IOR(IOC_MAGIC, MEM_READ_256BYTE, struct lfdd_mem_t)

#define LFDD_IO_READ_BYTE  _IOR(IOC_MAGIC, IO_READ_BYTE, struct lfdd_io_t)
#define LFDD_IO_READ_WORD  _IOR(IOC_MAGIC, IO_READ_WORD, struct lfdd_io_t)
#define LFDD_IO_READ_DWORD  _IOR(IOC_MAGIC, IO_READ_DWORD, struct lfdd_io_t)
#define LFDD_IO_WRITE_BYTE  _IOW(IOC_MAGIC, IO_WRITE_BYTE, struct lfdd_io_t)
#define LFDD_IO_WRITE_WORD  _IOW(IOC_MAGIC, IO_WRITE_WORD, struct lfdd_io_t)
#define LFDD_IO_WRITE_DWORD  _IOW(IOC_MAGIC, IO_WRITE_DWORD, struct lfdd_io_t)
#define LFDD_IO_READ_256BYTE  _IOR(IOC_MAGIC, IO_READ_256BYTE, struct lfdd_io_t)



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

//uint64_t lfdd_cal_pcix_addr( struct lfdd_pcix_t *ppci);
uint8_t lfdd_pcix_read_byte( struct lfdd_pcix_t *ppci);
uint16_t lfdd_pcix_read_word( struct lfdd_pcix_t *ppci);
uint32_t lfdd_pcix_read_dword( struct lfdd_pcix_t *ppci);
void lfdd_pcix_write_byte( struct lfdd_pcix_t *ppci);
void lfdd_pcix_write_word( struct lfdd_pcix_t *ppci);
void lfdd_pcix_write_dword(struct lfdd_pcix_t *ppci);
void lfdd_pcix_read_256byte( struct lfdd_pcix_t *ppci );
#endif


