// Including SDKDDKVer.h defines the highest available Windows platform.
#include <SDKDDKVer.h>

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// Windows header files
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <atlstr.h>

// COM header files
#include <ole2.h>

// Generic C++ stuff
#include <filesystem>
#include <fstream>
#include <vector>

// for subclassing
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include <Uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <vsstyle.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// inipp from https://github.com/mcmtroffaes/inipp
#include "inipp.h"

// window messages related to menu bar drawing
enum
{
    WM_UAHDESTROYWINDOW = 0x0090,
    WM_UAHDRAWMENU = 0x0091,
    WM_UAHDRAWMENUITEM = 0x0092,
    WM_UAHINITMENU = 0x0093,
    WM_UAHMEASUREMENUITEM = 0x0094,
    WM_UAHNCPAINTMENUPOPUP = 0x0095
};

// undocumented app mode enum for the private SetPreferredAppMode API
enum class PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

// describes the sizes of the menu bar or menu item
typedef union tagUAHMENUITEMMETRICS
{
    // cx appears to be 14 / 0xE less than rcItem's width!
    // cy 0x14 seems stable, i wonder if it is 4 less than rcItem's height which is always 24 atm
    struct {
        DWORD cx;
        DWORD cy;
    } rgsizeBar[2];
    struct {
        DWORD cx;
        DWORD cy;
    } rgsizePopup[4];
} UAHMENUITEMMETRICS;

// not really used in our case but part of the other structures
typedef struct tagUAHMENUPOPUPMETRICS
{
    DWORD rgcx[4];
    DWORD fUpdateMaxWidths : 2; // from kernel symbols, padded to full dword
} UAHMENUPOPUPMETRICS;

// hmenu is the main window menu; hdc is the context to draw in
typedef struct tagUAHMENU
{
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags; // no idea what these mean, in my testing it's either 0x00000a00 or sometimes 0x00000a10
} UAHMENU;

// menu items are always referred to by iPosition here
typedef struct tagUAHMENUITEM
{
    int iPosition; // 0-based position of menu item in menubar
    UAHMENUITEMMETRICS umim;
    UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

// the DRAWITEMSTRUCT contains the states of the menu items, as well as
// the position index of the item in the menu, which is duplicated in
// the UAHMENUITEM's iPosition as well
typedef struct UAHDRAWMENUITEM
{
    DRAWITEMSTRUCT dis; // itemID looks uninitialized
    UAHMENU um;
    UAHMENUITEM umi;
} UAHDRAWMENUITEM;

// the MEASUREITEMSTRUCT is intended to be filled with the size of the item
// height appears to be ignored, but width can be modified
typedef struct tagUAHMEASUREMENUITEM
{
    MEASUREITEMSTRUCT mis;
    UAHMENU um;
    UAHMENUITEM umi;
} UAHMEASUREMENUITEM;

// theme config struct
typedef struct {
    COLORREF menubar_textcolor;
    COLORREF menubar_textcolor_disabled;
    COLORREF menubar_bgcolor;
    COLORREF menubaritem_bgcolor;
    COLORREF menubaritem_bgcolor_hot;
    COLORREF menubaritem_bgcolor_selected;

    HBRUSH menubar_bgbrush;
    HBRUSH menubaritem_bgbrush;
    HBRUSH menubaritem_bgbrush_hot;
    HBRUSH menubaritem_bgbrush_selected;
} theme_cfg;

// global variables
static HTHEME g_menuTheme = nullptr;
HHOOK g_hook = nullptr;

bool IsWndClass(HWND hWnd, const TCHAR* classname) {
    TCHAR buf[512];
    GetClassName(hWnd, buf, 512);
    return _wcsicmp(classname, buf) == 0;
}

bool IsUnityWndClass(HWND hWnd) {
    return IsWndClass(hWnd, L"UnityContainerWndClass");
}

void GetAllWindowsByProcessID(DWORD dwProcessID, std::vector<HWND>& vhWnds) {
    HWND hCurWnd = nullptr;
    do
    {
        hCurWnd = FindWindowEx(nullptr, hCurWnd, nullptr, nullptr);
        if (hCurWnd != nullptr)
        {
            DWORD processID = 0;
            GetWindowThreadProcessId(hCurWnd, &processID);
            if (processID == dwProcessID)
            {
                vhWnds.push_back(hCurWnd);
            }
        }
    } while (hCurWnd != nullptr);
}

const theme_cfg* LoadThemeConfig() {
    static BOOL config_loaded = FALSE;
    static theme_cfg _cfg;

    if (config_loaded) return &_cfg;
    config_loaded = TRUE;

    HMODULE hm = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)LoadThemeConfig, &hm);
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(hm, path, MAX_PATH);
    CStringW inifn(path);
    inifn.Append(L".ini");

    if (!std::filesystem::exists(inifn.GetString())) {
        std::ofstream configFile;
        configFile.open(inifn.GetString());
        configFile << "menubar_textcolor = 200,200,200" << std::endl;
        configFile << "menubar_textcolor_disabled = 160,160,160" << std::endl;
        configFile << "menubar_bgcolor = 48,48,48" << std::endl;
        configFile << "menubaritem_bgcolor = 48,48,48" << std::endl;
        configFile << "menubaritem_bgcolor_hot = 62,62,62" << std::endl;
        configFile << "menubaritem_bgcolor_selected = 62,62,62" << std::endl;
        configFile.close();
    }

    inipp::Ini<char> ini;
    std::ifstream is(inifn.GetString());
    ini.parse(is);
    is.close();
    std::ostringstream oss;
    ini.generate(oss);

#define _PARSE_COLOR(x) \
    { \
        int r, g, b; \
        char comma; \
        std::stringstream ss(ini.sections[""][#x]); \
        ss >> r >> comma >> g >> comma >> b; \
        _cfg.x = RGB(r, g, b); \
    }

    _PARSE_COLOR(menubar_textcolor)
    _PARSE_COLOR(menubar_textcolor_disabled)
    _PARSE_COLOR(menubar_bgcolor)
    _PARSE_COLOR(menubaritem_bgcolor)
    _PARSE_COLOR(menubaritem_bgcolor_hot)
    _PARSE_COLOR(menubaritem_bgcolor_selected)

    _cfg.menubar_bgbrush = CreateSolidBrush(_cfg.menubar_bgcolor);
    _cfg.menubaritem_bgbrush = CreateSolidBrush(_cfg.menubaritem_bgcolor);
    _cfg.menubaritem_bgbrush_hot = CreateSolidBrush(_cfg.menubaritem_bgcolor_hot);
    _cfg.menubaritem_bgbrush_selected = CreateSolidBrush(_cfg.menubaritem_bgcolor_selected);

    return &_cfg;
}

// https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
// https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
// https://github.com/microsoft/WindowsAppSDK/issues/41
// https://gist.github.com/ericoporto/1745f4b912e22f9eabfce2c7166d979b
void EnableDarkMode(HWND hWnd) {

    // apply dark mode to the window
    {
        const BOOL USE_DARK_MODE = true;

        DwmSetWindowAttribute(hWnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE,
            &USE_DARK_MODE,
            sizeof(USE_DARK_MODE));
    }

    // apply dark mode to the context menus
    {
        using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
        fnSetPreferredAppMode SetPreferredAppMode;

        static HMODULE hUxtheme = nullptr;

        if (!hUxtheme) {
            hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        }

        if (hUxtheme) {
            // #135 is the ordinal for SetPreferredAppMode private API
            // which is available in uxtheme.dll since Windows 10 1903+
            SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
            SetPreferredAppMode(PreferredAppMode::ForceDark);
		}
    }
}

LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
    switch (nCode) {
        case HCBT_CREATEWND:
        {
            HWND hWnd = (HWND)wParam;
            if (IsUnityWndClass(hWnd) ||
                IsWndClass(hWnd, L"#32770") ||
                IsWndClass(hWnd, L"Button") ||
                IsWndClass(hWnd, L"tooltips_class32") ||
                IsWndClass(hWnd, L"ComboBox") ||
                IsWndClass(hWnd, L"SysListView32") ||
                IsWndClass(hWnd, L"SysTreeView32")) {

                EnableDarkMode(hWnd);
                SetWindowSubclass(hWnd, CallWndSubClassProc, 0, 0);
            }
            break;
        }
        case HCBT_DESTROYWND:
        {
            HWND hWnd = (HWND)wParam;
            if (IsUnityWndClass(hWnd) ||
                IsWndClass(hWnd, L"#32770") ||
                IsWndClass(hWnd, L"Button") ||
                IsWndClass(hWnd, L"tooltips_class32") ||
                IsWndClass(hWnd, L"ComboBox") ||
                IsWndClass(hWnd, L"SysListView32") ||
                IsWndClass(hWnd, L"SysTreeView32")) {

                RemoveWindowSubclass(hWnd, CallWndSubClassProc, 0);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

void UAHDrawMenuNCBottomLine(HWND hWnd) {
    MENUBARINFO mbi = { sizeof(mbi) };

    if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
    {
        return;
    }

    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
    // the rcBar is offset by the window rect
    RECT rcAnnoyingLine = rcClient;
    rcAnnoyingLine.bottom = rcAnnoyingLine.top;
    rcAnnoyingLine.top--;

    HDC hdc = GetWindowDC(hWnd);
    FillRect(hdc, &rcAnnoyingLine, LoadThemeConfig()->menubar_bgbrush);
    ReleaseDC(hWnd, hdc);
}

// https://stackoverflow.com/questions/16313333/drawing-rounded-and-colored-owner-draw-buttons
void PaintODTBUTTON(const DRAWITEMSTRUCT& dis) {
    HWND hwnd = dis.hwndItem;

    RECT rc;
    GetClientRect(hwnd, &rc);

    auto hdc = dis.hDC;
    auto bkcolor = LoadThemeConfig()->menubar_bgcolor;
    auto brush = CreateSolidBrush(bkcolor);
    auto pen = CreatePen(PS_SOLID, 1, LoadThemeConfig()->menubar_textcolor);
    auto oldbrush = SelectObject(hdc, brush);
    auto oldpen = SelectObject(hdc, pen);

    SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
    SetBkColor(hdc, bkcolor);
    SetTextColor(hdc, LoadThemeConfig()->menubar_textcolor);

    HBRUSH tmp = CreateSolidBrush(RGB(127, 192, 127));
    FillRect(hdc, &rc, tmp);

    Rectangle(hdc, 0, 0, rc.right, rc.bottom);

    if (GetFocus() == hwnd)
    {
        RECT temp = rc;
        InflateRect(&temp, -2, -2);
        DrawFocusRect(hdc, &temp);
    }

    TCHAR buf[128];
    GetWindowText(hwnd, buf, 128);
    DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldpen);
    SelectObject(hdc, oldbrush);
    DeleteObject(brush);
    DeleteObject(pen);
}

LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_CTLCOLORDLG:
        {
            return (INT_PTR)LoadThemeConfig()->menubar_bgcolor;
        }
        case WM_CTLCOLOREDIT:
        {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, LoadThemeConfig()->menubar_textcolor);
            SetBkColor(hdcStatic, LoadThemeConfig()->menubar_bgcolor);
            return (INT_PTR)LoadThemeConfig()->menubar_bgbrush;
        }
        case WM_CTLCOLORLISTBOX:
        {
            if (IsWndClass(hWnd, L"ComboBox")) {
                COMBOBOXINFO info;
                info.cbSize = sizeof(info);
                SendMessage(hWnd, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);

                if ((HWND)lParam == info.hwndList)
                {
                    HDC dc = (HDC)wParam;
                    SetBkMode(dc, OPAQUE);
                    SetTextColor(dc, LoadThemeConfig()->menubar_textcolor);
                    SetBkColor(dc, LoadThemeConfig()->menubar_bgcolor);
                    return (LRESULT)LoadThemeConfig()->menubar_bgbrush;
                }
            }
            break;
        }
        case WM_CTLCOLORSCROLLBAR:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, LoadThemeConfig()->menubar_textcolor);
            SetBkColor(hdc, LoadThemeConfig()->menubar_bgcolor);
            return reinterpret_cast<LRESULT>(LoadThemeConfig()->menubar_bgbrush);
        }
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, LoadThemeConfig()->menubar_textcolor);
            SetBkColor(hdc, LoadThemeConfig()->menubar_bgcolor);
            return reinterpret_cast<LRESULT>(LoadThemeConfig()->menubar_bgbrush);
        }
        case WM_DRAWITEM:
        {
            const DRAWITEMSTRUCT& dis = *(DRAWITEMSTRUCT*)lParam;
            if (dis.CtlType == ODT_BUTTON) {
                if (dis.itemAction) {
                    PaintODTBUTTON(dis);
                    if (dis.itemState & ODS_FOCUS) {
                        DrawFocusRect(dis.hDC, &dis.rcItem);
                    }
                }
                return TRUE;
            }
            break;
        }
        case WM_ERASEBKGND:
        {
            DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
            switch (LOWORD(style)) {
                case BS_GROUPBOX: // 0x7
                    break;
                default:
                {
                    RECT rc;
                    GetClientRect(hWnd, &rc);
                    FillRect(reinterpret_cast<HDC>(wParam), &rc, LoadThemeConfig()->menubar_bgbrush);
                    return TRUE;
                }
            }
            break;
        }
        case WM_NCACTIVATE:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                UAHDrawMenuNCBottomLine(hWnd);
                return lr;
            }
            break;
        }
        case WM_NCCREATE:
        {
            EnableDarkMode(hWnd);
            if (IsWndClass(hWnd, L"tooltips_class32")) {
                SetWindowTheme(hWnd, L"wstr", L"wstr");
            }
            else if (IsWndClass(hWnd, L"ComboBox")) {
                SetWindowTheme(hWnd, L"wstr", L"wstr");
            }
            else if (IsWndClass(hWnd, L"Button")) {
                DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
                switch(LOWORD(style)) {
                    case BS_AUTOCHECKBOX:    // 0x3
                    case BS_AUTORADIOBUTTON: // 0x9
                    case BS_CHECKBOX:        // 0x2
                    case BS_GROUPBOX:        // 0x7
                        SetWindowTheme(hWnd, L"wstr", L"wstr");
                        break;
                    case BS_LEFT: // 0x100
                    case BS_PUSHBUTTON: // 0x0
                        style = style | BS_OWNERDRAW;
                        //style = WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
                        SetWindowLongPtr(hWnd, GWL_STYLE, style);
                        break;
                }
            }
            break;
        }
        case WM_NCPAINT:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                UAHDrawMenuNCBottomLine(hWnd);
                return lr;
            }
            break;
        }
        case WM_PAINT:
        {
            if (IsWndClass(hWnd, L"tooltips_class32")) {
                SendMessage(hWnd, TTM_SETTIPBKCOLOR, LoadThemeConfig()->menubar_bgcolor, 0);
                SendMessage(hWnd, TTM_SETTIPTEXTCOLOR, LoadThemeConfig()->menubar_textcolor, 0);
            }
            else if (IsWndClass(hWnd, L"SysTreeView32")) { // left pane of config dialog
                TreeView_SetBkColor(hWnd, LoadThemeConfig()->menubar_bgcolor);
                TreeView_SetTextColor(hWnd, LoadThemeConfig()->menubar_textcolor);
            }
            else if (IsWndClass(hWnd, L"SysListView32")) { // left pane of FX dialog
                ListView_SetBkColor(hWnd, LoadThemeConfig()->menubar_bgcolor);
                ListView_SetTextBkColor(hWnd, LoadThemeConfig()->menubar_bgcolor);
                ListView_SetTextColor(hWnd, LoadThemeConfig()->menubar_textcolor);
            }
            break;
        }
        case WM_STYLECHANGING:
        case WM_STYLECHANGED:
        {
            if (IsUnityWndClass(hWnd)) {
                // prevent propagation to prevent menu bar from going back to the standard one...?!? FIXME
                return true;
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                if (g_menuTheme) {
                    CloseThemeData(g_menuTheme);
                    g_menuTheme = nullptr;
                }
            }
            break;
        }
        // https://stackoverflow.com/questions/77985210/how-to-set-menu-bar-color-in-win32
        case WM_UAHDRAWMENU:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                UAHMENU* pUDM = (UAHMENU*)lParam;
                RECT rc = { 0 };
                {
                    MENUBARINFO mbi = { sizeof(mbi) };
                    GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

                    RECT rcWindow;
                    GetWindowRect(hWnd, &rcWindow);
                    // the rcBar is offset by the window rect
                    rc = mbi.rcBar;
                    OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
                }
                FillRect(pUDM->hdc, &rc, LoadThemeConfig()->menubar_bgbrush);
                return true;
            }
            break;
        }
        case WM_UAHDRAWMENUITEM:
        {
            if (IsUnityWndClass(hWnd) || IsWndClass(hWnd, L"#32770")) {
                UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

                const HBRUSH* pbrBackground = &LoadThemeConfig()->menubaritem_bgbrush;
                // get the menu item string
                wchar_t menuString[256] = { 0 };
                MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
                {
                    mii.dwTypeData = menuString;
                    mii.cch = (sizeof(menuString) / 2) - 1;

                    GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
                }
                // get the item state for drawing
                DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
                int iTextStateID = 0;

                if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT)) {
                    // normal display
                    iTextStateID = MPI_NORMAL;
                }
                if (pUDMI->dis.itemState & ODS_HOTLIGHT) {
                    // hot tracking
                    iTextStateID = MPI_HOT;
                    pbrBackground = &LoadThemeConfig()->menubaritem_bgbrush_hot;
                }
                if (pUDMI->dis.itemState & ODS_SELECTED) {
                    // clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
                    iTextStateID = MPI_HOT;
                    pbrBackground = &LoadThemeConfig()->menubaritem_bgbrush_selected;
                }
                if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED)) {
                    // disabled / grey text
                    iTextStateID = MPI_DISABLED;
                }
                if (pUDMI->dis.itemState & ODS_NOACCEL) {
                    dwFlags |= DT_HIDEPREFIX;
                }

                if (!g_menuTheme) {
                    g_menuTheme = OpenThemeData(hWnd, L"Menu");
                }

                DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, iTextStateID != MPI_DISABLED ? LoadThemeConfig()->menubar_textcolor : LoadThemeConfig()->menubar_textcolor_disabled };
                FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBackground);
                DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, MBI_NORMAL, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts);
                return true;
            }
            break;
        }
        case WM_UAHMEASUREMENUITEM:
        {
            if (!IsUnityWndClass(hWnd) && !IsWndClass(hWnd, L"#32770")) {

                //UAHMEASUREMENUITEM* pMmi = (UAHMEASUREMENUITEM*)lParam;
                // allow the default window procedure to handle the message
                // since we don't really care about changing the width
                //*lr = DefWindowProc(hWnd, message, wParam, lParam);
                LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
                // but we can modify it here to make it 1/3rd wider for example
                //pMmi->mis.itemWidth = (pMmi->mis.itemWidth * 4) / 3;
                return lr;
            }
            break;
        }
        default:
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// DLL entry
bool APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpRes) {
    switch (reason)
    {
        case DLL_PROCESS_ATTACH: {
            // enable dark mode for the current process window
            EnableDarkMode(nullptr);

            // catch windows that have been created before we set up the CBTProc hook
            std::vector<HWND> windowHandles;
            GetAllWindowsByProcessID(GetCurrentProcessId(), windowHandles);

            for (const HWND& hWnd : windowHandles) {
                SetWindowSubclass(hWnd, CallWndSubClassProc, 0, 0);
                EnableDarkMode(hWnd);
            }

            // set up the CBTProc hook to catch new windows
            g_hook = SetWindowsHookEx(WH_CBT, CBTProc, nullptr, GetCurrentThreadId());
            break;
        }
        case DLL_PROCESS_DETACH: {
            if (g_hook) {
                UnhookWindowsHookEx(g_hook);
                g_hook = nullptr;
            }
            break;
        }
        default: break;
    }

    return true;
}