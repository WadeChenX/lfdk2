
#ifndef __LIBPCI_H__
#define __LIBPCI_H__

#define LFDK_MAX_PCIBUF         200
#define LFDK_MAX_PCIBUS			256
#define LFDK_DEFAULT_PCINAME    "/usr/share/misc/pci.ids"
#define LFDK_MAX_PCINAME        75

typedef struct {

        PANEL *p_ven;
        PANEL *p_dev;
        PANEL *p_offset;
        PANEL *p_info;
        PANEL *p_rtitle;
        PANEL *p_value;

        WINDOW *ven;
        WINDOW *dev;
        WINDOW *offset;
        WINDOW *info;
        WINDOW *rtitle;
        WINDOW *value;

} PCIPanel;

typedef struct {
        PANEL *p_title;
        PANEL *p_devname;
        PANEL *p_vendev;
        PANEL *p_scan;
        PANEL *p_error;

        WINDOW *title;
        WINDOW *devname;
        WINDOW *vendev;
        WINDOW *scan;
        WINDOW *error;

} PCILPanel;

typedef struct {

        unsigned short int      venid;
        unsigned short int      devid;
        unsigned char           bus;
        unsigned char           dev;
        unsigned char           fun;
        unsigned char           ventxt[ LFDK_MAX_PCINAME + 1 ];
        unsigned char           devtxt[ LFDK_MAX_PCINAME + 1 ];

} PCIData;



#endif //__LIBPCI_H__
