#include "ddraw_impl.h"
#include "miniwin.h"
#include "miniwin/ddraw.h"

#include <SDL3/SDL.h>
#include <cassert>
#include <vector>

ULONG IUnknown::AddRef()
{
	assert(m_refCount > 0);
	m_refCount += 1;
	return m_refCount;
}

ULONG IUnknown::Release()
{
	assert(m_refCount > 0);
	m_refCount -= 1;
	if (m_refCount == 0) {
		delete this;
		return 0;
	}
	return m_refCount;
}

HRESULT IUnknown::QueryInterface(const GUID& riid, void** ppvObject)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT IDirectDrawClipper::SetClipList(LPRGNDATA lpClipList, DWORD dwFlags)
{
	// Blits are clipped to the destination surface; a clip list that covers
	// the surface, the common case, needs nothing more.
	return DD_OK;
}

HRESULT IDirectDrawClipper::SetHWnd(DWORD unnamedParam1, HWND hWnd)
{
	return DD_OK;
}

BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	if (!hWnd) {
		return FALSE;
	}
	SDL_Window* sdlWindow = reinterpret_cast<SDL_Window*>(hWnd);

	if (!(uFlags & SWP_NOACTIVATE)) {
		SDL_RaiseWindow(sdlWindow);
	}

	if (!(uFlags & SWP_NOSIZE)) {
		SDL_SetWindowSize(sdlWindow, cx, cy);
	}

	if (!(uFlags & SWP_NOMOVE)) {
		SDL_SetWindowPosition(sdlWindow, X, Y);
	}

	return TRUE;
}

HDC WINAPI GetDC(HWND hWnd)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

int WINAPI ReleaseDC(HWND hWnd, HDC hDC)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

int WINAPI GetDeviceCaps(HDC hdc, int index)
{

	if (index == RASTERCAPS) {
		return 0;
	}
	if (index == SIZEPALETTE) {
		return 256;
	}
	if (index == BITSPIXEL) {
		const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(MiniwinDisplayPixelFormat());
		return details ? details->bits_per_pixel : 0;
	}
	if (index == PLANES) {
		return 1;
	}
	return 0;
}

BOOL RedrawWindow(void* hWnd, const void* lprcUpdate, void* hrgnUpdate, unsigned int flags)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 1;
}

HMENU GetMenu(HWND hWnd)
{
	return NULL;
}

int DrawMenuBar(void* hWnd)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 1;
}

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
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

LONG GetWindowLong(HWND hWnd, int nIndex)
{
	SDL_Window* sdlWindow = reinterpret_cast<SDL_Window*>(hWnd);
	if (nIndex == GWL_STYLE) {
		Uint32 flags = SDL_GetWindowFlags(sdlWindow);
		LONG style = WS_POPUP;
		if ((flags & SDL_WINDOW_BORDERLESS) == 0) {
			style = WS_OVERLAPPED | WS_CAPTION;
			if (flags & SDL_WINDOW_RESIZABLE) {
				style |= WS_THICKFRAME;
			}
		}
		return style;
	}
	else if (nIndex == GWL_EXSTYLE) {
		return 0;
	}

	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

LONG SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
	SDL_Window* sdlWindow = reinterpret_cast<SDL_Window*>(hWnd);
	if (nIndex == GWL_STYLE) {
		SDL_SetWindowBordered(sdlWindow, (dwNewLong & WS_CAPTION) != 0);
#ifndef __EMSCRIPTEN__
		SDL_SetWindowResizable(sdlWindow, (dwNewLong & WS_THICKFRAME) != 0);
#endif

		return dwNewLong;
	}

	return 0;
}

namespace {
std::vector<RECT*>& Regions()
{
	static std::vector<RECT*> regions;
	return regions;
}
} // namespace

int DeleteObject(void* object)
{
	auto& regions = Regions();
	for (auto it = regions.begin(); it != regions.end(); ++it) {
		if (*it == object) {
			delete *it;
			regions.erase(it);
			return 1;
		}
	}
	MINIWIN_NOT_IMPLEMENTED();
	return 1;
}

BOOL OffsetRect(LPRECT lprc, int dx, int dy)
{
	if (!lprc) {
		return FALSE;
	}
	lprc->left += dx;
	lprc->right += dx;
	lprc->top += dy;
	lprc->bottom += dy;
	return TRUE;
}

HRGN CreateRectRgn(int x1, int y1, int x2, int y2)
{
	RECT* rect = new RECT{x1, y1, x2, y2};
	Regions().push_back(rect);
	return reinterpret_cast<HRGN>(rect);
}

DWORD GetRegionData(HRGN hrgn, DWORD nCount, LPRGNDATA lpRgnData)
{
	const DWORD size = sizeof(RGNDATAHEADER) + sizeof(RECT);
	if (!hrgn) {
		return 0;
	}
	if (!lpRgnData) {
		return size;
	}
	if (nCount < size) {
		return 0;
	}
	const RECT* rect = reinterpret_cast<const RECT*>(hrgn);
	lpRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
	lpRgnData->rdh.iType = 1; // RDH_RECTANGLES
	lpRgnData->rdh.nCount = 1;
	lpRgnData->rdh.nRgnSize = sizeof(RECT);
	lpRgnData->rdh.rcBound = *rect;
	memcpy(lpRgnData->Buffer, rect, sizeof(RECT));
	return size;
}

BOOL AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	return TRUE;
}

int SetRect(RECT* rc, int left, int top, int right, int bottom)
{
	rc->left = left;
	rc->top = top;
	rc->right = right;
	rc->bottom = bottom;
	return 1;
}

VOID WINAPI Sleep(DWORD dwMilliseconds)
{
	SDL_Delay(dwMilliseconds);
}

BOOL ClientToScreen(HWND hWnd, LPPOINT lpPoint)
{
	return TRUE;
}

int _chdir(char* path)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

intptr_t _spawnl(
	int mode,
	const char* cmdname,
	const char* arg0,
	const char* arg1,
	const char* arg2,
	const char* arg3,
	...
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

UINT WINAPI GetSystemPaletteEntries(HDC hdc, UINT iStart, UINT cEntries, LPPALETTEENTRY pPalEntries)
{
	MINIWIN_NOT_IMPLEMENTED();
	for (UINT i = 0; i < cEntries; i++) {
		UINT val = iStart + i;
		pPalEntries[i].peRed = val;
		pPalEntries[i].peGreen = val;
		pPalEntries[i].peBlue = val;
		pPalEntries[i].peFlags = PC_NONE;
	}
	return cEntries;
}

HPALETTE CreatePalette(LPLOGPALETTE lpLogPalette)
{
	MINIWIN_NOT_IMPLEMENTED();
	return nullptr;
}

int SelectPalette(HDC hdc, HPALETTE hpal, BOOL bForceBackground)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

int RealizePalette(HDC hdc)
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}
