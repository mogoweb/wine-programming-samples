/*
 * resource.h - resource IDs for the DsoFramer C++ test client
 * (equivalent of Samples/Vb6Test/Src/FTestApp.frm menu tree,
 * minimal core subset).
 */
#ifndef FRAMERAPP_RESOURCE_H
#define FRAMERAPP_RESOURCE_H

/* Main menu */
#define IDR_MAINMENU            100

/* File menu */
#define IDM_FILE_NEW            2001
#define IDM_FILE_OPEN           2002
#define IDM_FILE_CLOSE          2003
#define IDM_FILE_SAVE           2004
#define IDM_FILE_SAVEAS         2005
#define IDM_FILE_PAGESETUP      2006
#define IDM_FILE_PRINTPREVIEW   2007
#define IDM_FILE_PRINT          2008
#define IDM_FILE_PROPERTIES     2009
#define IDM_FILE_QUIT           2010

/* Show menu */
#define IDM_SHOW_CAPTION        2101
#define IDM_SHOW_MENUBAR        2102
#define IDM_SHOW_TOOLBAR        2103
#define IDM_SHOW_BORDER_FIRST   2110   /* +0..+3 border styles */
#define IDM_SHOW_BORDER_NONE    2110
#define IDM_SHOW_BORDER_OUTLINE 2111
#define IDM_SHOW_BORDER_3D      2112
#define IDM_SHOW_BORDER_3DTHIN  2113
#define IDM_SHOW_DISABLE_FIRST  2120   /* +0..+8 file commands */
#define IDM_SHOW_CUSTOMCAPTION  2130

/* Status label (drawn manually, no dialog template) */
#define IDC_STATUS_FILE         3001

#endif /* FRAMERAPP_RESOURCE_H */
