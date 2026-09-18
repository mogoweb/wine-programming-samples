/*
 * dsoframer.h - Hand-declared COM interfaces for the DSO Framer Control
 * (KB 311765 sample OCX). No MIDL/typelib needed: the control's dual
 * interface is marked "oleautomation" and marshals with the universal
 * marshaler, so in-proc callers can use the raw vtable.
 *
 * Vtable order strictly follows Lib/dsoframer.idl declaration order.
 * For dual interfaces every dispinterface property expands to two slots:
 * propput then propget, in IDL declaration order.
 */
#ifndef DS_DSOFRAMER_TEST_H
#define DS_DSOFRAMER_TEST_H

#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <olectl.h>

/* Instantiate DEFINE_GUID declarations below in every translation unit
   that includes this header (no separate guid source file needed). */
#include <initguid.h>
#include <guiddef.h>

/* ===== GUIDs (version.h of the DsoFramer source) ===== */

/* {00460182-9E5E-11d5-B7C8-B8269041DD57} */
DEFINE_GUID(CLSID_FramerControl,
    0x00460182, 0x9E5E, 0x11d5, 0xB7, 0xC8, 0xB8, 0x26, 0x90, 0x41, 0xDD, 0x57);

/* {00460181-9E5E-11d5-B7C8-B8269041DD57} - _FramerControl (dual) */
DEFINE_GUID(IID__FramerControl,
    0x00460181, 0x9E5E, 0x11d5, 0xB7, 0xC8, 0xB8, 0x26, 0x90, 0x41, 0xDD, 0x57);

/* {00460185-9E5E-11d5-B7C8-B8269041DD57} - _DFramerCtlEvents (dispinterface) */
DEFINE_GUID(DIID__DFramerCtlEvents,
    0x00460185, 0x9E5E, 0x11d5, 0xB7, 0xC8, 0xB8, 0x26, 0x90, 0x41, 0xDD, 0x57);

/* ===== Enums from Lib/dsoframer.idl ===== */

typedef enum dsoBorderStyle {
    dsoBorderNone = 0,
    dsoBorderFlat = 1,
    dsoBorder3D   = 2,
    dsoBorder3DThin = 3
} dsoBorderStyle;

typedef enum dsoShowDialogType {
    dsoDialogNew = 0,
    dsoDialogOpen = 1,
    dsoDialogSave = 2,
    dsoDialogSaveCopy = 3,
    dsoDialogPrint = 4,
    dsoDialogPageSetup = 5,
    dsoDialogProperties = 6
} dsoShowDialogType;

typedef enum dsoFileCommandType {
    dsoFileNew = 0,
    dsoFileOpen = 1,
    dsoFileClose = 2,
    dsoFileSave = 3,
    dsoFileSaveAs = 4,
    dsoFilePrint = 5,
    dsoFilePageSetup = 6,
    dsoFileProperties = 7,
    dsoFilePrintPreview = 8
} dsoFileCommandType;

/* Event DISPIDs of _DFramerCtlEvents */
#define DSOF_DISPID_FILECMD       1
#define DSOF_DISPID_DOCOPEN       2
#define DSOF_DISPID_DOCCLOSE      3
#define DSOF_DISPID_ACTIVATE      4
#define DSOF_DISPID_BDOCCLOSE     5
#define DSOF_DISPID_BDOCSAVE      6
#define DSOF_DISPID_ENDPREVIEW    7
#define DSOF_DISPID_SAVECOMPLETE  8

/* ===== _FramerControl dual interface =====
 *
 * Slot layout (IUnknown 3 + IDispatch 4, then IDL order; each
 * [propget]/[propput] pair on the same dispid takes two slots,
 * propget first):
 *
 *  7  Activate()
 *  8  get_ActiveDocument(IDispatch**)
 *  9  CreateNew(BSTR)
 * 10  Open(VARIANT,VARIANT,VARIANT,VARIANT,VARIANT)
 * 11  Save(VARIANT,VARIANT,VARIANT,VARIANT)
 * 12  _PrintOutOld(VARIANT)
 * 13  Close()
 * 14  put_Caption(BSTR)      15 get_Caption(BSTR*)
 * 16  put_Titlebar(VARIANT_BOOL)  17 get_Titlebar(VARIANT_BOOL*)
 * 18  put_Toolbars(VARIANT_BOOL)  19 get_Toolbars(VARIANT_BOOL*)
 * 20  put_ModalState(VARIANT_BOOL) 21 get_ModalState(VARIANT_BOOL*)
 * 22  ShowDialog(dsoShowDialogType)
 * 23  put_EnableFileCommand(Item, VARIANT_BOOL) 24 get_EnableFileCommand(Item, VARIANT_BOOL*)
 * 25  put_BorderStyle(dsoBorderStyle) 26 get_BorderStyle(dsoBorderStyle*)
 * 27  put_BorderColor(OLE_COLOR) 28 get_BorderColor(OLE_COLOR*)
 * 29  put_BackColor(OLE_COLOR)   30 get_BackColor(OLE_COLOR*)
 * 31  put_ForeColor(OLE_COLOR)   32 get_ForeColor(OLE_COLOR*)
 * 33  put_TitlebarColor(OLE_COLOR) 34 get_TitlebarColor(OLE_COLOR*)
 * 35  put_TitlebarTextColor(OLE_COLOR) 36 get_TitlebarTextColor(OLE_COLOR*)
 * 37  ExecOleCommand(LONG, VARIANT, VARIANT*, VARIANT*)
 * 38  put_Menubar(VARIANT_BOOL)  39 get_Menubar(VARIANT_BOOL*)
 * 40  put_HostName(BSTR)         41 get_HostName(BSTR*)
 * 42  get_DocumentFullName(BSTR*)
 * 43  PrintOut(VARIANT,VARIANT,VARIANT,VARIANT,VARIANT,VARIANT)
 * 44  PrintPreview()
 * 45  PrintPreviewExit()
 * 46  get_IsReadOnly(VARIANT_BOOL*)
 * 47  get_IsDirty(VARIANT_BOOL*)
 * 48  put_LockServer(VARIANT_BOOL) 49 get_LockServer(VARIANT_BOOL*)
 * 50  GetDataObjectContent(VARIANT, VARIANT*)
 * 51  SetDataObjectContent(VARIANT, VARIANT)
 * 52  put_ActivationPolicy(LONG) 53 get_ActivationPolicy(LONG*)
 * 54  put_FrameHookPolicy(LONG)  55 get_FrameHookPolicy(LONG*)
 * 56  put_MenuAccelerators(VARIANT_BOOL) 57 get_MenuAccelerators(VARIANT_BOOL*)
 * 58  put_EventsEnabled(VARIANT_BOOL)    59 get_EventsEnabled(VARIANT_BOOL*)
 * 60  get_DocumentName(BSTR*)
 */
#undef INTERFACE
#define INTERFACE __FramerControl
DECLARE_INTERFACE_(__FramerControl, IDispatch)
{
    /* IUnknown */
    STDMETHOD(QueryInterface)(THIS_ REFIID, void**) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /* IDispatch */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT*) PURE;
    STDMETHOD(GetTypeInfo)(THIS_ UINT, LCID, ITypeInfo**) PURE;
    STDMETHOD(GetIDsOfNames)(THIS_ REFIID, LPOLESTR*, UINT, LCID, DISPID*) PURE;
    STDMETHOD(Invoke)(THIS_ DISPID, REFIID, LCID, WORD, DISPPARAMS*,
                      VARIANT*, EXCEPINFO*, UINT*) PURE;
    /* _FramerControl */
    STDMETHOD(Activate)(THIS) PURE;
    STDMETHOD(get_ActiveDocument)(THIS_ IDispatch**) PURE;
    STDMETHOD(CreateNew)(THIS_ BSTR) PURE;
    STDMETHOD(Open)(THIS_ VARIANT, VARIANT, VARIANT, VARIANT, VARIANT) PURE;
    STDMETHOD(Save)(THIS_ VARIANT, VARIANT, VARIANT, VARIANT) PURE;
    STDMETHOD(_PrintOutOld)(THIS_ VARIANT) PURE;
    STDMETHOD(Close)(THIS) PURE;
    STDMETHOD(put_Caption)(THIS_ BSTR) PURE;
    STDMETHOD(get_Caption)(THIS_ BSTR*) PURE;
    STDMETHOD(put_Titlebar)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_Titlebar)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(put_Toolbars)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_Toolbars)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(put_ModalState)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_ModalState)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(ShowDialog)(THIS_ dsoShowDialogType) PURE;
    STDMETHOD(put_EnableFileCommand)(THIS_ dsoFileCommandType, VARIANT_BOOL) PURE;
    STDMETHOD(get_EnableFileCommand)(THIS_ dsoFileCommandType, VARIANT_BOOL*) PURE;
    STDMETHOD(put_BorderStyle)(THIS_ dsoBorderStyle) PURE;
    STDMETHOD(get_BorderStyle)(THIS_ dsoBorderStyle*) PURE;
    STDMETHOD(put_BorderColor)(THIS_ OLE_COLOR) PURE;
    STDMETHOD(get_BorderColor)(THIS_ OLE_COLOR*) PURE;
    STDMETHOD(put_BackColor)(THIS_ OLE_COLOR) PURE;
    STDMETHOD(get_BackColor)(THIS_ OLE_COLOR*) PURE;
    STDMETHOD(put_ForeColor)(THIS_ OLE_COLOR) PURE;
    STDMETHOD(get_ForeColor)(THIS_ OLE_COLOR*) PURE;
    STDMETHOD(put_TitlebarColor)(THIS_ OLE_COLOR) PURE;
    STDMETHOD(get_TitlebarColor)(THIS_ OLE_COLOR*) PURE;
    STDMETHOD(put_TitlebarTextColor)(THIS_ OLE_COLOR) PURE;
    STDMETHOD(get_TitlebarTextColor)(THIS_ OLE_COLOR*) PURE;
    STDMETHOD(ExecOleCommand)(THIS_ LONG, VARIANT, VARIANT*, VARIANT*) PURE;
    STDMETHOD(put_Menubar)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_Menubar)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(put_HostName)(THIS_ BSTR) PURE;
    STDMETHOD(get_HostName)(THIS_ BSTR*) PURE;
    STDMETHOD(get_DocumentFullName)(THIS_ BSTR*) PURE;
    STDMETHOD(PrintOut)(THIS_ VARIANT, VARIANT, VARIANT, VARIANT, VARIANT, VARIANT) PURE;
    STDMETHOD(PrintPreview)(THIS) PURE;
    STDMETHOD(PrintPreviewExit)(THIS) PURE;
    STDMETHOD(get_IsReadOnly)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(get_IsDirty)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(put_LockServer)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_LockServer)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(GetDataObjectContent)(THIS_ VARIANT, VARIANT*) PURE;
    STDMETHOD(SetDataObjectContent)(THIS_ VARIANT, VARIANT) PURE;
    STDMETHOD(put_ActivationPolicy)(THIS_ LONG) PURE;
    STDMETHOD(get_ActivationPolicy)(THIS_ LONG*) PURE;
    STDMETHOD(put_FrameHookPolicy)(THIS_ LONG) PURE;
    STDMETHOD(get_FrameHookPolicy)(THIS_ LONG*) PURE;
    STDMETHOD(put_MenuAccelerators)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_MenuAccelerators)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(put_EventsEnabled)(THIS_ VARIANT_BOOL) PURE;
    STDMETHOD(get_EventsEnabled)(THIS_ VARIANT_BOOL*) PURE;
    STDMETHOD(get_DocumentName)(THIS_ BSTR*) PURE;
};

#endif /* DS_DSOFRAMER_TEST_H */
