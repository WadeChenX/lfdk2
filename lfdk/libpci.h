#ifndef __LIBPCI_H__
#define __LIBPCI_H__

#define LFDK_MAX_PCIBUF         200
#define LFDK_MAX_PCIBUS			256
#define LFDK_DEFAULT_PCINAME    "/usr/share/misc/pci.ids"
#define LFDK_MAX_PCINAME        75
#define ACPI_MCFG_PATH          "/sys/firmware/acpi/tables/MCFG"

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
        uint16_t      venid;
        uint16_t      devid;
        uint8_t       bus;
        uint8_t       dev;
        uint8_t       fun;
        uint8_t       ventxt[ LFDK_MAX_PCINAME + 1 ];
        uint8_t       devtxt[ LFDK_MAX_PCINAME + 1 ];
        uint64_t      phy_base;
} PCIData;



#endif //__LIBPCI_H__
