#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRAY_ICON_SIZE 22

static Display *dpy;
static int screen;

static Window main_win;
static Window tray_icon;
static Window menu_win;

static Atom WM_DELETE_WINDOW;

static int running = 1;
static int menu_visible = 0;

/* ---------------------------------- */
/* 创建简单菜单 */
/* ---------------------------------- */

void create_menu()
{
    menu_win = XCreateSimpleWindow(
        dpy,
        RootWindow(dpy, screen),
        0, 0,
        140, 60,
        1,
        BlackPixel(dpy, screen),
        WhitePixel(dpy, screen));

    XSelectInput(dpy, menu_win,
                 ExposureMask |
                 ButtonPressMask);

    XMapRaised(dpy, menu_win);
    XUnmapWindow(dpy, menu_win);
}

void draw_menu()
{
    GC gc = DefaultGC(dpy, screen);

    XClearWindow(dpy, menu_win);

    XDrawString(dpy, menu_win, gc, 10, 20,
                "Show Window", 11);

    XDrawString(dpy, menu_win, gc, 10, 45,
                "Exit", 4);

    XDrawLine(dpy, menu_win, gc,
              0, 30, 140, 30);
}

void show_menu(int x, int y)
{
    XMoveWindow(dpy, menu_win, x, y);
    XMapRaised(dpy, menu_win);
    menu_visible = 1;
}

void hide_menu()
{
    XUnmapWindow(dpy, menu_win);
    menu_visible = 0;
}

/* ---------------------------------- */
/* tray */
/* ---------------------------------- */

Window get_tray_manager()
{
    char buf[64];

    snprintf(buf, sizeof(buf),
             "_NET_SYSTEM_TRAY_S%d",
             screen);

    Atom tray_atom = XInternAtom(dpy, buf, False);

    return XGetSelectionOwner(dpy, tray_atom);
}

void dock_tray_icon()
{
    Window tray = get_tray_manager();

    if (!tray)
    {
        printf("No system tray found\n");
        return;
    }

    tray_icon = XCreateSimpleWindow(
        dpy,
        RootWindow(dpy, screen),
        0, 0,
        TRAY_ICON_SIZE,
        TRAY_ICON_SIZE,
        0,
        BlackPixel(dpy, screen),
        WhitePixel(dpy, screen));

    XSelectInput(dpy, tray_icon,
                 ExposureMask |
                 ButtonPressMask);

    XMapRaised(dpy, tray_icon);

    Atom opcode = XInternAtom(
        dpy,
        "_NET_SYSTEM_TRAY_OPCODE",
        False);

    XEvent ev;
    memset(&ev, 0, sizeof(ev));

    ev.xclient.type = ClientMessage;
    ev.xclient.window = tray;
    ev.xclient.message_type = opcode;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = 0; /* SYSTEM_TRAY_REQUEST_DOCK */
    ev.xclient.data.l[2] = tray_icon;

    XSendEvent(dpy,
               tray,
               False,
               NoEventMask,
               &ev);

    XSync(dpy, False);
}

void draw_tray_icon()
{
    GC gc = DefaultGC(dpy, screen);

    XSetForeground(dpy, gc,
                   BlackPixel(dpy, screen));

    XFillRectangle(dpy,
                   tray_icon,
                   gc,
                   4, 4,
                   14, 14);

    XSetForeground(dpy, gc,
                   WhitePixel(dpy, screen));

    XDrawString(dpy,
                tray_icon,
                gc,
                7, 15,
                "T", 1);
}

/* ---------------------------------- */
/* main window */
/* ---------------------------------- */

void create_main_window()
{
    main_win = XCreateSimpleWindow(
        dpy,
        RootWindow(dpy, screen),
        200, 200,
        500, 300,
        1,
        BlackPixel(dpy, screen),
        WhitePixel(dpy, screen));

    XStoreName(dpy, main_win,
               "Pure X11 Tray Demo");

    XSelectInput(dpy, main_win,
                 ExposureMask |
                 StructureNotifyMask);

    WM_DELETE_WINDOW =
        XInternAtom(dpy,
                    "WM_DELETE_WINDOW",
                    False);

    XSetWMProtocols(dpy,
                    main_win,
                    &WM_DELETE_WINDOW,
                    1);

    XMapWindow(dpy, main_win);
}

void draw_main_window()
{
    GC gc = DefaultGC(dpy, screen);

    XClearWindow(dpy, main_win);

    const char *msg =
        "Close button => minimize to tray";

    XDrawString(dpy,
                main_win,
                gc,
                20, 40,
                msg,
                strlen(msg));
}

/* ---------------------------------- */
/* actions */
/* ---------------------------------- */

void hide_to_tray()
{
    XUnmapWindow(dpy, main_win);
}

void show_main_window()
{
    XMapRaised(dpy, main_win);
}

/* ---------------------------------- */
/* main */
/* ---------------------------------- */

int main()
{
    dpy = XOpenDisplay(NULL);

    if (!dpy)
    {
        fprintf(stderr,
                "Cannot open display\n");
        return 1;
    }

    screen = DefaultScreen(dpy);

    create_main_window();
    /* create_menu(); */

    dock_tray_icon();

    while (running)
    {
        XEvent ev;
        XNextEvent(dpy, &ev);

        /* main window */
        if (ev.xany.window == main_win)
        {
            switch (ev.type)
            {
                case Expose:
                    draw_main_window();
                    break;

                case ClientMessage:
                {
                    if ((Atom)ev.xclient.data.l[0]
                        == WM_DELETE_WINDOW)
                    {
                        hide_to_tray();
                    }
                    break;
                }
            }
        }

        /* tray icon */
        else if (ev.xany.window == tray_icon)
        {
            switch (ev.type)
            {
                case Expose:
                    draw_tray_icon();
                    break;

                case ButtonPress:
                {
                    XButtonEvent *bev =
                        (XButtonEvent *)&ev;

                    /* left click */
                    if (bev->button == Button1)
                    {
                        show_main_window();
                    }

                    /* right click */
                    else if (bev->button == Button3)
                    {
                        printf("Tray right click: "
                            "root=(%d,%d), "
                            "local=(%d,%d)\n",
                            bev->x_root,
                            bev->y_root,
                            bev->x,
                            bev->y);

                        fflush(stdout);

                        /*show_menu(bev->x_root,
                                  bev->y_root); */
                    }

                    break;
                }
            }
        }

        /* menu */
        else if (ev.xany.window == menu_win)
        {
            switch (ev.type)
            {
                case Expose:
                    draw_menu();
                    break;

                case ButtonPress:
                {
                    XButtonEvent *bev =
                        (XButtonEvent *)&ev;

                    if (bev->y < 30)
                    {
                        show_main_window();
                    }
                    else
                    {
                        running = 0;
                    }

                    hide_menu();
                    break;
                }
            }
        }
    }

    XDestroyWindow(dpy, tray_icon);
    XDestroyWindow(dpy, menu_win);
    XDestroyWindow(dpy, main_win);

    XCloseDisplay(dpy);

    return 0;
}
