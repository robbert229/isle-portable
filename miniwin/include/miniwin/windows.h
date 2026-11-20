#pragma once

#include <SDL3/SDL_video.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

// --- Defines and Macros ---
#define MAKE_HRESULT(sev, fac, code)                                                                                   \
	((HRESULT) (((uint32_t) (sev) << 31) | ((uint32_t) (fac) << 16) | ((uint32_t) (code))))

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)                                                   \
	const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

// Remove WinAPI stuff
#define CALLBACK
#define FAR
#define WINAPI
#define HWND_NOTOPMOST ((HWND) (-2))
#define RGB(r, g, b) (((BYTE) (r) | ((BYTE) (g) << 8) | ((BYTE) (b) << 16)))
#define S_OK ((HRESULT) 0)
#define E_NOTIMPL ((HRESULT) 0x80004001)
#define E_NOINTERFACE (0x80004002)
#define VOID void
#define TRUE 1
#define FALSE 0

#define SWP_NOACTIVATE 0x0010
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0004
#define SWP_NOZORDER 0x0001

#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_ACTIVATEAPP 0x001C
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_NCACTIVATE 0x0086
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_TIMER 0x0113
#define WM_ENTERMENULOOP 0x0211
#define WM_EXITMENULOOP 0x0212
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206

// Virtual-key codes: WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN and WM_SYSKEYUP
// carry these in wParam, as on Windows.
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_OEM_1 0xBA
#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE
#define VK_OEM_102 0xE2

// Key message lParam flags (the high word): extended key, Alt held,
// previous key state and transition state.
#define KF_EXTENDED 0x0100
#define KF_ALTDOWN 0x2000
#define KF_REPEAT 0x4000
#define KF_UP 0x8000

#define WM_SETTEXT 0x000C
#define WM_INITDIALOG 0x0110
#define WM_COMMAND 0x0111

#define BM_SETCHECK 0x00F1
#define BN_CLICKED 0
#define LB_ADDSTRING 0x0180
#define LB_DELETESTRING 0x0182
#define LB_SETCURSEL 0x0186
#define LB_GETCURSEL 0x0188
#define LB_GETTEXT 0x0189
#define LB_ERR (-1)
#define LBN_SELCHANGE 1

#define MAKEINTRESOURCEA(i) ((LPSTR) ((uintptr_t) ((WORD) (i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001

#define CS_DBLCLKS 0x0008

#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOW 5

#define SWP_NOOWNERZORDER 0x0200

#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_ICONERROR 0x00000010L

#define IDOK 1
#define IDCANCEL 2

#define BITSPIXEL 12
#define PLANES 14

#define ERROR_SUCCESS 0L
#define ERROR_ALREADY_EXISTS 183L

#define MAKELPARAM(l, h) ((LPARAM) (DWORD) (((WORD) (l)) | (((DWORD) (WORD) (h)) << 16)))
#define LOWORD(l) ((WORD) (((uintptr_t) (l)) & 0xffff))
#define HIWORD(l) ((WORD) ((((uintptr_t) (l)) >> 16) & 0xffff))

#define WS_CAPTION 0x00C00000L
#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_EX_APPWINDOW 0x00040000L

#define GWL_STYLE (-16)
#define GWL_EXSTYLE -20

#define ANSI_CHARSET 0
#define BI_RGB 0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define ETO_OPAQUE 0x0002
#define RASTERCAPS 0x00000000
#define RC_PALETTE 0x0100
#define SIZEPALETTE 104
#define FW_NORMAL 400
#define OPAQUE 2
#define OUT_DEFAULT_PRECIS 0
#define RDW_FRAME 0x0400
#define SRCCOPY 0x00CC0020
#define VARIABLE_PITCH 2

// --- Typedefs ---
typedef uint8_t BYTE, byte;
typedef int32_t LONG;
typedef uint32_t ULONG, DWORD;
typedef long HRESULT;
typedef DWORD* LPDWORD;
typedef int BOOL, WINBOOL, INT;
typedef unsigned int UINT;
typedef unsigned short WORD;
typedef int* LPLONG;
typedef void* LPVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef void* HANDLE;
typedef struct HICON__* HICON;
typedef struct HFONT__* HFONT;
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HMENU__* HMENU;
typedef struct HWND__* HWND;
typedef HINSTANCE HMODULE;
typedef struct HDC__* HDC;
typedef struct HPALETTE__* HPALETTE;
typedef HICON HCURSOR;
typedef HANDLE HFILE;
typedef int LSTATUS, HKEY, REGSAM;
typedef struct HBRUSH__* HBRUSH;
typedef struct HRGN__* HRGN;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM, LRESULT;
typedef WORD ATOM;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef intptr_t INT_PTR;
typedef INT_PTR (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);

// --- Structs ---
struct tagPOINT {
	LONG x;
	LONG y;
};
typedef tagPOINT POINT, *LPPOINT;

struct SIZE {
	LONG cx;
	LONG cy;
};

struct RECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};
typedef RECT* LPRECT;

struct WNDCLASSA {
	UINT style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HICON hIcon;
	HCURSOR hCursor;
	HBRUSH hbrBackground;
	LPCSTR lpszMenuName;
	LPCSTR lpszClassName;
};

struct MSG {
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	tagPOINT pt;
};
typedef MSG* LPMSG;

struct WINDOWPOS {
	HWND hwnd;
	HWND hwndInsertAfter;
	int x;
	int y;
	int cx;
	int cy;
	UINT flags;
};

struct RGNDATAHEADER {
	DWORD dwSize;
	DWORD iType;
	DWORD nCount;
	DWORD nRgnSize;
	RECT rcBound;
};

struct RGNDATA {
	RGNDATAHEADER rdh;
	char Buffer[1];
};
typedef RGNDATA* LPRGNDATA;

struct BITMAPINFOHEADER {
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
};
static_assert(sizeof(BITMAPINFOHEADER) == 40, "Incorrect size");
typedef BITMAPINFOHEADER* LPBITMAPINFOHEADER;

struct RGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
};

#pragma pack(push, 1)
struct BITMAPFILEHEADER {
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
};
#pragma pack(pop)

struct BITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
};

struct GUID {
	uint32_t m_data1;
	uint16_t m_data2;
	uint16_t m_data3;
	uint8_t m_data4[8];
};
typedef GUID* LPGUID;

struct IUnknown {
	IUnknown() : m_refCount(1) {}
	virtual ULONG AddRef();
	virtual ULONG Release();
	virtual HRESULT QueryInterface(const GUID& riid, void** ppvObject);
	virtual ~IUnknown() = default;

protected:
	int m_refCount;
};

BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);

HDC WINAPI GetDC(HWND hWnd);

int WINAPI ReleaseDC(HWND hWnd, HDC hDC);

int WINAPI GetDeviceCaps(HDC hdc, int index);

BOOL RedrawWindow(void* hWnd, const void* lprcUpdate, void* hrgnUpdate, unsigned int flags);

HMENU GetMenu(HWND hWnd);

int DrawMenuBar(void* hWnd);

int StretchDIBits(
	void* hdc,
	int xDest,
	int yDest,
	int DestWidth,
	int DestHeight,
	int xSrc,
	int ySrc,
	int SrcWidth,
	int SrcHeight,
	const void* lpBits,
	const void* lpbmi,
	unsigned int iUsage,
	uint32_t rop
);

LONG GetWindowLong(HWND hWnd, int nIndex);

LONG SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong);

int DeleteObject(void*);

BOOL AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);

int SetRect(RECT* rc, int left, int top, int right, int bottom);

VOID WINAPI Sleep(DWORD dwMilliseconds);

BOOL ClientToScreen(HWND hWnd, LPPOINT lpPoint);

BOOL OffsetRect(LPRECT lprc, int dx, int dy);

// A region is one rectangle.
HRGN CreateRectRgn(int x1, int y1, int x2, int y2);

DWORD GetRegionData(HRGN hrgn, DWORD nCount, LPRGNDATA lpRgnData);

// --- Window classes, windows and the message queue (over SDL3) ---
// A window is an SDL_Window; its HWND is the SDL_Window pointer, as for
// SetWindowPos and DirectDraw's SetCooperativeLevel.

ATOM RegisterClassA(const WNDCLASSA* lpWndClass);

HWND CreateWindowExA(
	DWORD dwExStyle,
	LPCSTR lpClassName,
	LPCSTR lpWindowName,
	DWORD dwStyle,
	int X,
	int Y,
	int nWidth,
	int nHeight,
	HWND hWndParent,
	HMENU hMenu,
	HINSTANCE hInstance,
	LPVOID lpParam
);

BOOL DestroyWindow(HWND hWnd);

LRESULT DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

BOOL PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);

BOOL TranslateMessage(const MSG* lpMsg);

LRESULT DispatchMessageA(const MSG* lpMsg);

void PostQuitMessage(int nExitCode);

BOOL ShowWindow(HWND hWnd, int nCmdShow);

BOOL UpdateWindow(HWND hWnd);

HWND SetActiveWindow(HWND hWnd);

HWND SetFocus(HWND hWnd);

HWND SetCapture(HWND hWnd);

BOOL ReleaseCapture();

BOOL SetWindowTextA(HWND hWnd, LPCSTR lpString);

BOOL GetWindowRect(HWND hWnd, LPRECT lpRect);

BOOL GetClientRect(HWND hWnd, LPRECT lpRect);

HWND GetDesktopWindow();

BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);

LONG SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);

int ShowCursor(BOOL bShow);

HCURSOR SetCursor(HCURSOR hCursor);

BOOL GetCursorPos(LPPOINT lpPoint);

BOOL SetCursorPos(int X, int Y);

BOOL ScreenToClient(HWND hWnd, LPPOINT lpPoint);

int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

LRESULT SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

BOOL MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);

BOOL EnableWindow(HWND hWnd, BOOL bEnable);

// Dialog boxes need resource templates, which miniwin has none of: these fail.
INT_PTR DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

BOOL EndDialog(HWND hDlg, INT_PTR nResult);

HWND GetDlgItem(HWND hDlg, int nIDDlgItem);

