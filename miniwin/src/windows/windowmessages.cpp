#include "miniwin.h"
#include "miniwin/windows.h"

#include <SDL3/SDL.h>
#include <deque>
#include <string>
#include <unordered_map>

// Window classes, windows and the message queue over SDL3. A window is an
// SDL_Window, and its HWND is the SDL_Window pointer. SDL events become the
// matching window messages, which DispatchMessageA hands to the window
// procedure of the window's class.
//
// Screen coordinates are the client coordinates of the window under the
// mouse (or of the first window): miniwin's primary surface is the window's
// client area, so ClientToScreen leaves a point unchanged, and the cursor
// calls and a message's pt follow it.

namespace {

std::unordered_map<std::string, WNDCLASSA>& WindowClasses()
{
	static std::unordered_map<std::string, WNDCLASSA> classes;
	return classes;
}

std::unordered_map<SDL_Window*, WNDPROC>& WindowProcedures()
{
	static std::unordered_map<SDL_Window*, WNDPROC> procedures;
	return procedures;
}

std::deque<MSG>& PostedMessages()
{
	static std::deque<MSG> messages;
	return messages;
}

int g_cursorCount = 0;

SDL_Window* WindowOf(SDL_WindowID id)
{
	return SDL_GetWindowFromID(id);
}

// The window whose client area the screen coordinates are.
SDL_Window* CursorWindow()
{
	if (SDL_Window* window = SDL_GetMouseFocus()) {
		return window;
	}
	if (SDL_Window* window = SDL_GetKeyboardFocus()) {
		return window;
	}
	return WindowProcedures().empty() ? nullptr : WindowProcedures().begin()->first;
}

POINT CursorPosition()
{
	float x = 0.0f;
	float y = 0.0f;
	SDL_GetMouseState(&x, &y);
	return POINT{(LONG) x, (LONG) y};
}

void Post(SDL_Window* window, UINT message, WPARAM wParam, LPARAM lParam)
{
	MSG msg = {};
	msg.hwnd = reinterpret_cast<HWND>(window);
	msg.message = message;
	msg.wParam = wParam;
	msg.lParam = lParam;
	msg.time = (DWORD) SDL_GetTicks();
	msg.pt = CursorPosition();
	PostedMessages().push_back(msg);
}

LPARAM MousePosition(float x, float y)
{
	return MAKELPARAM((int) x, (int) y);
}

// Turns one SDL event into window messages on the queue.
void Translate(const SDL_Event& event)
{
	switch (event.type) {
	case SDL_EVENT_QUIT:
		for (auto& entry : WindowProcedures()) {
			Post(entry.first, WM_CLOSE, 0, 0);
		}
		break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		Post(WindowOf(event.window.windowID), WM_CLOSE, 0, 0);
		break;
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		Post(WindowOf(event.window.windowID), WM_ACTIVATEAPP, TRUE, 0);
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		Post(WindowOf(event.window.windowID), WM_ACTIVATEAPP, FALSE, 0);
		break;
	case SDL_EVENT_WINDOW_MOVED:
		Post(WindowOf(event.window.windowID), WM_MOVE, 0, MAKELPARAM(event.window.data1, event.window.data2));
		break;
	case SDL_EVENT_WINDOW_RESIZED:
		Post(WindowOf(event.window.windowID), WM_SIZE, 0, MAKELPARAM(event.window.data1, event.window.data2));
		break;
	case SDL_EVENT_MOUSE_MOTION:
		Post(WindowOf(event.motion.windowID), WM_MOUSEMOVE, 0, MousePosition(event.motion.x, event.motion.y));
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP: {
		bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
		bool doubleClick = down && event.button.clicks == 2;
		UINT message;
		if (event.button.button == SDL_BUTTON_LEFT) {
			message = doubleClick ? WM_LBUTTONDBLCLK : (down ? WM_LBUTTONDOWN : WM_LBUTTONUP);
		}
		else if (event.button.button == SDL_BUTTON_RIGHT) {
			message = doubleClick ? WM_RBUTTONDBLCLK : (down ? WM_RBUTTONDOWN : WM_RBUTTONUP);
		}
		else {
			break;
		}
		Post(WindowOf(event.button.windowID), message, 0, MousePosition(event.button.x, event.button.y));
		break;
	}
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
		Post(
			WindowOf(event.key.windowID),
			event.type == SDL_EVENT_KEY_DOWN ? WM_KEYDOWN : WM_KEYUP,
			(WPARAM) event.key.key,
			0
		);
		break;
	default:
		break;
	}
}

} // namespace

ATOM RegisterClassA(const WNDCLASSA* lpWndClass)
{
	if (!lpWndClass || !lpWndClass->lpszClassName || !lpWndClass->lpfnWndProc) {
		return 0;
	}
	WindowClasses()[lpWndClass->lpszClassName] = *lpWndClass;
	return (ATOM) WindowClasses().size();
}

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
)
{
	auto found = WindowClasses().find(lpClassName ? lpClassName : "");
	if (found == WindowClasses().end()) {
		return NULL;
	}
	if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		SDL_LogError(LOG_CATEGORY_MINIWIN, "SDL_InitSubSystem: %s", SDL_GetError());
		return NULL;
	}
	SDL_WindowFlags flags = SDL_WINDOW_HIDDEN;
	if (!(dwStyle & WS_CAPTION)) {
		flags |= SDL_WINDOW_BORDERLESS;
	}
	if (dwStyle & WS_THICKFRAME) {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	SDL_Window* window = SDL_CreateWindow(lpWindowName ? lpWindowName : "", nWidth, nHeight, flags);
	if (!window) {
		SDL_LogError(LOG_CATEGORY_MINIWIN, "SDL_CreateWindow: %s", SDL_GetError());
		return NULL;
	}
	SDL_SetWindowPosition(window, X, Y);
	WindowProcedures()[window] = found->second.lpfnWndProc;
	HWND hWnd = reinterpret_cast<HWND>(window);
	found->second.lpfnWndProc(hWnd, WM_CREATE, 0, 0);
	return hWnd;
}

BOOL DestroyWindow(HWND hWnd)
{
	SDL_Window* window = reinterpret_cast<SDL_Window*>(hWnd);
	auto found = WindowProcedures().find(window);
	if (found == WindowProcedures().end()) {
		return FALSE;
	}
	found->second(hWnd, WM_DESTROY, 0, 0);
	WindowProcedures().erase(found);
	auto& queue = PostedMessages();
	for (auto it = queue.begin(); it != queue.end();) {
		it = it->hwnd == hWnd ? queue.erase(it) : it + 1;
	}
	SDL_DestroyWindow(window);
	return TRUE;
}

LRESULT DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_CLOSE) {
		DestroyWindow(hWnd);
	}
	return 0;
}

BOOL PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		Translate(event);
	}
	auto& queue = PostedMessages();
	for (auto it = queue.begin(); it != queue.end(); ++it) {
		if (hWnd && it->hwnd != hWnd) {
			continue;
		}
		if ((wMsgFilterMin || wMsgFilterMax) && (it->message < wMsgFilterMin || it->message > wMsgFilterMax)) {
			continue;
		}
		*lpMsg = *it;
		if (wRemoveMsg & PM_REMOVE) {
			queue.erase(it);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL TranslateMessage(const MSG* lpMsg)
{
	return FALSE;
}

LRESULT DispatchMessageA(const MSG* lpMsg)
{
	auto found = WindowProcedures().find(reinterpret_cast<SDL_Window*>(lpMsg->hwnd));
	if (found == WindowProcedures().end()) {
		return 0;
	}
	return found->second(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
}

void PostQuitMessage(int nExitCode)
{
	MSG msg = {};
	msg.message = WM_QUIT;
	msg.wParam = (WPARAM) nExitCode;
	PostedMessages().push_back(msg);
}

BOOL ShowWindow(HWND hWnd, int nCmdShow)
{
	SDL_Window* window = reinterpret_cast<SDL_Window*>(hWnd);
	if (!window) {
		return FALSE;
	}
	bool wasVisible = !(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN);
	if (nCmdShow == SW_HIDE) {
		SDL_HideWindow(window);
	}
	else {
		SDL_ShowWindow(window);
	}
	return wasVisible;
}

BOOL UpdateWindow(HWND hWnd)
{
	return hWnd != NULL;
}

HWND SetActiveWindow(HWND hWnd)
{
	if (hWnd) {
		SDL_RaiseWindow(reinterpret_cast<SDL_Window*>(hWnd));
	}
	return NULL;
}

HWND SetFocus(HWND hWnd)
{
	if (hWnd) {
		SDL_RaiseWindow(reinterpret_cast<SDL_Window*>(hWnd));
	}
	return NULL;
}

HWND SetCapture(HWND hWnd)
{
	SDL_CaptureMouse(true);
	return NULL;
}

BOOL ReleaseCapture()
{
	return SDL_CaptureMouse(false);
}

BOOL SetWindowTextA(HWND hWnd, LPCSTR lpString)
{
	return SDL_SetWindowTitle(reinterpret_cast<SDL_Window*>(hWnd), lpString ? lpString : "");
}

BOOL GetWindowRect(HWND hWnd, LPRECT lpRect)
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	if (hWnd == GetDesktopWindow()) {
		const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
		if (!mode) {
			return FALSE;
		}
		w = mode->w;
		h = mode->h;
	}
	else {
		SDL_Window* window = reinterpret_cast<SDL_Window*>(hWnd);
		if (!SDL_GetWindowPosition(window, &x, &y) || !SDL_GetWindowSize(window, &w, &h)) {
			return FALSE;
		}
	}
	SetRect(lpRect, x, y, x + w, y + h);
	return TRUE;
}

BOOL GetClientRect(HWND hWnd, LPRECT lpRect)
{
	int w = 0;
	int h = 0;
	if (!SDL_GetWindowSize(reinterpret_cast<SDL_Window*>(hWnd), &w, &h)) {
		return FALSE;
	}
	SetRect(lpRect, 0, 0, w, h);
	return TRUE;
}

HWND GetDesktopWindow()
{
	// Never an SDL_Window pointer: GetWindowRect reads the desktop's mode.
	return reinterpret_cast<HWND>(static_cast<uintptr_t>(1));
}

BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
	return AdjustWindowRectEx(lpRect, dwStyle, bMenu, 0);
}

LONG SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
	return SetWindowLong(hWnd, nIndex, dwNewLong);
}

int ShowCursor(BOOL bShow)
{
	g_cursorCount += bShow ? 1 : -1;
	if (g_cursorCount >= 0) {
		SDL_ShowCursor();
	}
	else {
		SDL_HideCursor();
	}
	return g_cursorCount;
}

HCURSOR SetCursor(HCURSOR hCursor)
{
	if (!hCursor) {
		SDL_HideCursor();
	}
	return NULL;
}

BOOL GetCursorPos(LPPOINT lpPoint)
{
	*lpPoint = CursorPosition();
	return TRUE;
}

BOOL SetCursorPos(int X, int Y)
{
	SDL_Window* window = CursorWindow();
	if (!window) {
		return FALSE;
	}
	SDL_WarpMouseInWindow(window, (float) X, (float) Y);
	return TRUE;
}

BOOL ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
	// The inverse of ClientToScreen: screen coordinates are client coordinates.
	return hWnd != NULL;
}

int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	SDL_LogError(LOG_CATEGORY_MINIWIN, "%s: %s", lpCaption ? lpCaption : "", lpText ? lpText : "");
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR,
		lpCaption ? lpCaption : "",
		lpText ? lpText : "",
		reinterpret_cast<SDL_Window*>(hWnd)
	);
	return 1;
}

LRESULT SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	auto found = WindowProcedures().find(reinterpret_cast<SDL_Window*>(hWnd));
	if (found == WindowProcedures().end()) {
		MINIWIN_NOT_IMPLEMENTED();
		return 0;
	}
	return found->second(hWnd, Msg, wParam, lParam);
}

BOOL MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	return SetWindowPos(hWnd, NULL, X, Y, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL EnableWindow(HWND hWnd, BOOL bEnable)
{
	MINIWIN_NOT_IMPLEMENTED();
	return FALSE;
}

INT_PTR DialogBoxParamA(
	HINSTANCE hInstance,
	LPCSTR lpTemplateName,
	HWND hWndParent,
	DLGPROC lpDialogFunc,
	LPARAM dwInitParam
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return -1;
}

BOOL EndDialog(HWND hDlg, INT_PTR nResult)
{
	MINIWIN_NOT_IMPLEMENTED();
	return FALSE;
}

HWND GetDlgItem(HWND hDlg, int nIDDlgItem)
{
	MINIWIN_NOT_IMPLEMENTED();
	return NULL;
}
