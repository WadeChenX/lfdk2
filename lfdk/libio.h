#ifndef __LIBIO_H__
#define __LIBIO_H__

typedef struct {

        PANEL *p_offset;
        PANEL *p_info;
        PANEL *p_value;
        PANEL *p_ascii;

        WINDOW *offset;
        WINDOW *info;
        WINDOW *value;
        WINDOW *ascii;

} IOPanel;

#endif //__LIBIO_H__
