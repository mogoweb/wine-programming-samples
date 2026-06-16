/* tray_demo.c  
 * 编译: gcc tray_demo.c -o tray_demo -lX11  
 * 运行: ./tray_demo  
 */  
#include <X11/Xlib.h>  
#include <X11/Xatom.h>  
#include <X11/Xutil.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
  
#define SYSTEM_TRAY_REQUEST_DOCK  0  
  
/* 找到当前屏幕的托盘宿主窗口 (_NET_SYSTEM_TRAY_S<n>) */  
static Window get_systray_owner(Display *dpy)  
{  
    char name[32];  
    snprintf(name, sizeof(name), "_NET_SYSTEM_TRAY_S%d", DefaultScreen(dpy));  
    Atom sel = XInternAtom(dpy, name, False);  
    return XGetSelectionOwner(dpy, sel);  
}  
  
/* 查找 32-bit TrueColor (ARGB) Visual */  
static int find_argb_visual(Display *dpy, XVisualInfo *out)  
{  
    XVisualInfo templ;  
    templ.screen = DefaultScreen(dpy);  
    templ.depth  = 32;  
    templ.class  = TrueColor;  
    int n = 0;  
    XVisualInfo *list = XGetVisualInfo(dpy,  
        VisualScreenMask | VisualDepthMask | VisualClassMask, &templ, &n);  
    if (!list || n == 0) return 0;  
    *out = list[0];  
    XFree(list);  
    return 1;  
}  
  
/* 绘制一个半透明圆形图标到 ARGB 窗口 */  
static void draw_icon(Display *dpy, Window win, Visual *vis, int w, int h)  
{  
    /* 分配 ARGB 像素缓冲区 */  
    unsigned int *buf = calloc(w * h, sizeof(unsigned int));  
    if (!buf) return;  
  
    int cx = w / 2, cy = h / 2, r = w / 2 - 2;  
  
    for (int y = 0; y < h; y++) {  
        for (int x = 0; x < w; x++) {  
            int dx = x - cx, dy = y - cy;  
            if (dx*dx + dy*dy <= r*r) {  
                /*  
                 * 预乘 Alpha (pre-multiplied alpha):  
                 *   A=0xC0 (75% 不透明), R=0xFF, G=0x40, B=0x00  
                 *   存储格式: 0xAARRGGBB (little-endian 下即内存顺序 BB GG RR AA)  
                 *  
                 * 注意: X11 ARGB Visual 要求像素值为预乘 alpha，  
                 *       即 R_stored = R * A / 255，否则颜色会偏暗。  
                 */  
                unsigned char a = 0xC0;  
                unsigned char rv = (unsigned char)(0xFF * a / 255);  
                unsigned char gv = (unsigned char)(0x40 * a / 255);  
                unsigned char bv = 0;  
                buf[y * w + x] = ((unsigned int)a  << 24)  
                                | ((unsigned int)rv << 16)  
                                | ((unsigned int)gv <<  8)  
                                |  (unsigned int)bv;  
            }  
            /* else: alpha=0, 完全透明，保持 calloc 的 0 值 */  
        }  
    }  
  
    /* 用 XCreateImage 包装缓冲区，再 XPutImage 写入窗口 */  
    XImage *img = XCreateImage(dpy, vis, 32, ZPixmap, 0,  
                                (char *)buf, w, h, 32, w * 4);  
    if (img) {  
        GC gc = XCreateGC(dpy, win, 0, NULL);  
        XPutImage(dpy, win, gc, img, 0, 0, 0, 0, w, h);  
        XFreeGC(dpy, gc);  
        img->data = NULL;   /* 阻止 XDestroyImage 释放我们的 buf */  
        XDestroyImage(img);  
    }  
    free(buf);  
}  
  
int main(void)  
{  
    Display *dpy = XOpenDisplay(NULL);  
    if (!dpy) { fprintf(stderr, "Cannot open display\n"); return 1; }  
  
    int screen = DefaultScreen(dpy);  
    Window root = RootWindow(dpy, screen);  
  
    /* 1. 找托盘宿主 */  
    Window tray = get_systray_owner(dpy);  
    if (!tray) { fprintf(stderr, "No system tray found\n"); return 1; }  
    printf("Systray owner: 0x%lx\n", tray);  
  
    /* 2. 获取 ARGB Visual（透明的关键） */  
    XVisualInfo vinfo;  
    if (!find_argb_visual(dpy, &vinfo)) {  
        fprintf(stderr, "No 32-bit ARGB visual available\n");  
        return 1;  
    }  
    printf("ARGB visual id: 0x%lx\n", vinfo.visualid);  
  
    /* 3. 为 ARGB Visual 创建专用 Colormap  
     *    （深度与 root 不同，不能用 DefaultColormap） */  
    Colormap cmap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);  
  
    /* 4. 创建图标窗口  
     *    - background_pixel=0: 初始全透明  
     *    - border_pixel=0:     必须显式设置，否则 X 会用默认深度的颜色  
     *    - CWColormap:         绑定上面创建的 colormap */  
    int icon_w = 22, icon_h = 22;  
    XSetWindowAttributes wa;  
    wa.colormap      = cmap;  
    wa.border_pixel  = 0;  
    wa.background_pixel = 0;  
    wa.event_mask    = ExposureMask | ButtonPressMask | StructureNotifyMask;  
  
    Window icon_win = XCreateWindow(dpy, root,  
                                     0, 0, icon_w, icon_h, 0,  
                                     vinfo.depth, InputOutput, vinfo.visual,  
                                     CWColormap | CWBorderPixel |  
                                     CWBackPixel | CWEventMask,  
                                     &wa);  
  
    XStoreName(dpy, icon_win, "TrayDemo");  
  
    /* 5. 监听托盘宿主的销毁事件 */  
    XSelectInput(dpy, tray, StructureNotifyMask);  
  
    /* 6. 发送 SYSTEM_TRAY_REQUEST_DOCK 请求嵌入  
     *    协议与 Wine 的 X11DRV_SystrayDockInsert 完全一致 */  
    Atom opcode = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);  
    XEvent ev;  
    memset(&ev, 0, sizeof(ev));  
    ev.xclient.type         = ClientMessage;  
    ev.xclient.window       = tray;  
    ev.xclient.message_type = opcode;  
    ev.xclient.format       = 32;  
    ev.xclient.data.l[0]    = CurrentTime;  
    ev.xclient.data.l[1]    = SYSTEM_TRAY_REQUEST_DOCK;  
    ev.xclient.data.l[2]    = icon_win;  
    XSendEvent(dpy, tray, False, NoEventMask, &ev);  
    XFlush(dpy);  
    printf("Docked, icon window: 0x%lx\n", icon_win);  
  
    /* 7. 事件循环 */  
    while (1) {  
        XNextEvent(dpy, &ev);  
  
        switch (ev.type) {  
        case Expose:  
            if (ev.xexpose.count == 0)  
                draw_icon(dpy, icon_win, vinfo.visual, icon_w, icon_h);  
            break;  
  
        case ButtonPress:  
            printf("Tray icon clicked (button %d)\n", ev.xbutton.button);  
            break;  
  
        case DestroyNotify:  
            if (ev.xdestroywindow.window == tray) {  
                printf("Systray gone, exiting\n");  
                goto done;  
            }  
            break;  
        }  
    }  
  
done:  
    XDestroyWindow(dpy, icon_win);  
    XFreeColormap(dpy, cmap);  
    XCloseDisplay(dpy);  
    return 0;  
}

