#ifndef __LIBBASE_H__
#define __LIBBASE_H__

#define PrintWin( RESRC, NAME, LINE, COLUMN, X, Y, COLORPAIR, FORMAT, ARGS... ) {   \
                                                                                    \
    RESRC.NAME = newwin( LINE, COLUMN, X, Y );                                      \
    RESRC.p_##NAME = new_panel( RESRC.NAME );                                       \
    wbkgd( RESRC.NAME, COLOR_PAIR( COLORPAIR ) );                                   \
    wattrset( RESRC.NAME, COLOR_PAIR( COLORPAIR ) | A_BOLD );                       \
    wprintw( RESRC.NAME, FORMAT, ##ARGS );                                          \
    wattrset( RESRC.NAME, A_NORMAL );                                               \
}


#define PrintFixedWin( RESRC, NAME, LINE, COLUMN, X, Y, COLORPAIR, FORMAT, ARGS... ) {  \
                                                                                        \
    if( !RESRC.NAME ) {                                                                 \
                                                                                        \
        RESRC.NAME = newwin( LINE, COLUMN, X, Y );                                      \
        RESRC.p_##NAME = new_panel( RESRC.NAME );                                       \
    }                                                                                   \
    wbkgd( RESRC.NAME, COLOR_PAIR( COLORPAIR ) );                                       \
    wattrset( RESRC.NAME, COLOR_PAIR( COLORPAIR ) | A_BOLD );                           \
    mvwprintw( RESRC.NAME, 0, 0, FORMAT, ##ARGS );                                      \
    wattrset( RESRC.NAME, A_NORMAL );                                                   \
}


#define DestroyWin( RESRC, NAME ) {     \
                                        \
    if( RESRC.p_##NAME ) {              \
                                        \
        del_panel( RESRC.p_##NAME );    \
        RESRC.p_##NAME = NULL;          \
    }                                   \
                                        \
    if( RESRC.NAME ) {                  \
                                        \
        delwin( RESRC.NAME );           \
        RESRC.NAME = NULL;              \
    }                                   \
}

typedef struct {

    PANEL   *p_bg;
    PANEL   *p_logo;
    PANEL   *p_copyright;
    PANEL   *p_help;
    PANEL   *p_time;

    WINDOW  *bg;
    WINDOW  *logo;
    WINDOW  *copyright;
    WINDOW  *help;
    WINDOW  *time;

} BasePanel;


#endif //__LIBBASE_H__
