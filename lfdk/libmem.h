#ifndef __LIBMEM_H__
#define __LIBMEM_H__

typedef struct {

        PANEL *p_offset;
        PANEL *p_info;
        PANEL *p_value;
        PANEL *p_ascii;

        WINDOW *offset;
        WINDOW *info;
        WINDOW *value;
        WINDOW *ascii;

} MemPanel;

#endif //__LIBMEM_H__
