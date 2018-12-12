#ifndef __LIBCMOS_H__
#define __LIBCMOS_H__

#define LFDK_CMOS_RANGE_BYTES   256
#define LFDK_CMOS_IO_START              0x70
#define LFDK_CMOS_IO_END                0x72
#define LFDK_CMOS_ADDR_PORT             0x70
#define LFDK_CMOS_DATA_PORT             0x71

typedef struct {

        PANEL *p_offset;
        PANEL *p_info;
        PANEL *p_value;
        PANEL *p_ascii;

        WINDOW *offset;
        WINDOW *info;
        WINDOW *value;
        WINDOW *ascii;

} CmosPanel;

#endif //__LIBIO_H__
