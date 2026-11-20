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

// The virtual-key code Windows reports for a key: a letter or digit by the
// character the layout gives it (as Windows' VK_A..VK_Z follow the layout),
// any other key by its SDL keycode, and a key the layout gives no Latin
// character by its physical position.
WPARAM VirtualKey(SDL_Keycode key, SDL_Scancode scancode)
{
	if (key >= SDLK_A && key <= SDLK_Z) {
		return 'A' + (key - SDLK_A);
	}
	if (key >= SDLK_0 && key <= SDLK_9) {
		return '0' + (key - SDLK_0);
	}
	if (key >= SDLK_F1 && key <= SDLK_F12) {
		return VK_F1 + (key - SDLK_F1);
	}
	switch (key) {
	case SDLK_BACKSPACE:
		return VK_BACK;
	case SDLK_TAB:
		return VK_TAB;
	case SDLK_CLEAR:
		return VK_CLEAR;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		return VK_RETURN;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		return VK_SHIFT;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		return VK_CONTROL;
	case SDLK_LALT:
	case SDLK_RALT:
		return VK_MENU;
	case SDLK_PAUSE:
		return VK_PAUSE;
	case SDLK_CAPSLOCK:
		return VK_CAPITAL;
	case SDLK_ESCAPE:
		return VK_ESCAPE;
	case SDLK_SPACE:
		return VK_SPACE;
	case SDLK_PAGEUP:
		return VK_PRIOR;
	case SDLK_PAGEDOWN:
		return VK_NEXT;
	case SDLK_END:
		return VK_END;
	case SDLK_HOME:
		return VK_HOME;
	case SDLK_LEFT:
		return VK_LEFT;
	case SDLK_UP:
		return VK_UP;
	case SDLK_RIGHT:
		return VK_RIGHT;
	case SDLK_DOWN:
		return VK_DOWN;
	case SDLK_PRINTSCREEN:
		return VK_SNAPSHOT;
	case SDLK_INSERT:
		return VK_INSERT;
	case SDLK_DELETE:
		return VK_DELETE;
	case SDLK_LGUI:
		return VK_LWIN;
	case SDLK_RGUI:
		return VK_RWIN;
	case SDLK_APPLICATION:
		return VK_APPS;
	case SDLK_KP_0:
		return VK_NUMPAD0;
	case SDLK_KP_1:
		return VK_NUMPAD1;
	case SDLK_KP_2:
		return VK_NUMPAD2;
	case SDLK_KP_3:
		return VK_NUMPAD3;
	case SDLK_KP_4:
		return VK_NUMPAD4;
	case SDLK_KP_5:
		return VK_NUMPAD5;
	case SDLK_KP_6:
		return VK_NUMPAD6;
	case SDLK_KP_7:
		return VK_NUMPAD7;
	case SDLK_KP_8:
		return VK_NUMPAD8;
	case SDLK_KP_9:
		return VK_NUMPAD9;
	case SDLK_KP_MULTIPLY:
		return VK_MULTIPLY;
	case SDLK_KP_PLUS:
		return VK_ADD;
	case SDLK_KP_MINUS:
		return VK_SUBTRACT;
	case SDLK_KP_PERIOD:
		return VK_DECIMAL;
	case SDLK_KP_DIVIDE:
		return VK_DIVIDE;
	case SDLK_NUMLOCKCLEAR:
		return VK_NUMLOCK;
	case SDLK_SCROLLLOCK:
		return VK_SCROLL;
	case SDLK_SEMICOLON:
		return VK_OEM_1;
	case SDLK_EQUALS:
		return VK_OEM_PLUS;
	case SDLK_COMMA:
		return VK_OEM_COMMA;
	case SDLK_MINUS:
		return VK_OEM_MINUS;
	case SDLK_PERIOD:
		return VK_OEM_PERIOD;
	case SDLK_SLASH:
		return VK_OEM_2;
	case SDLK_GRAVE:
		return VK_OEM_3;
	case SDLK_LEFTBRACKET:
		return VK_OEM_4;
	case SDLK_BACKSLASH:
		return VK_OEM_5;
	case SDLK_RIGHTBRACKET:
		return VK_OEM_6;
	case SDLK_APOSTROPHE:
		return VK_OEM_7;
	default:
		break;
	}
	if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
		return 'A' + (scancode - SDL_SCANCODE_A);
	}
	if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
		return '1' + (scancode - SDL_SCANCODE_1);
	}
	if (scancode == SDL_SCANCODE_0) {
		return '0';
	}
	if (scancode == SDL_SCANCODE_NONUSBACKSLASH) {
		return VK_OEM_102;
	}
	return 0;
}

// Windows' extended-key flag: the right-hand Ctrl and Alt, the navigation
// keys outside the numeric keypad, and the keypad's Enter and divide.
bool ExtendedKey(SDL_Scancode scancode)
{
	switch (scancode) {
	case SDL_SCANCODE_RCTRL:
	case SDL_SCANCODE_RALT:
	case SDL_SCANCODE_INSERT:
	case SDL_SCANCODE_DELETE:
	case SDL_SCANCODE_HOME:
	case SDL_SCANCODE_END:
	case SDL_SCANCODE_PAGEUP:
	case SDL_SCANCODE_PAGEDOWN:
	case SDL_SCANCODE_LEFT:
	case SDL_SCANCODE_RIGHT:
	case SDL_SCANCODE_UP:
	case SDL_SCANCODE_DOWN:
	case SDL_SCANCODE_KP_ENTER:
	case SDL_SCANCODE_KP_DIVIDE:
	case SDL_SCANCODE_NUMLOCKCLEAR:
	case SDL_SCANCODE_PRINTSCREEN:
	case SDL_SCANCODE_LGUI:
	case SDL_SCANCODE_RGUI:
	case SDL_SCANCODE_APPLICATION:
		return true;
	default:
		return false;
	}
}

// A key event as Windows sends it: WM_SYSKEYDOWN/WM_SYSKEYUP for F10 and
// for keys pressed with Alt but not Ctrl (Alt itself included), otherwise
// WM_KEYDOWN/WM_KEYUP. wParam is the virtual-key code; lParam has a repeat
// count of 1, the extended-key, Alt-held, previous-state and transition
// flags in its high word, and no OEM scan code.
void TranslateKey(const SDL_KeyboardEvent& key)
{
	WPARAM vk = VirtualKey(key.key, key.scancode);
	bool alt = (key.mod & SDL_KMOD_ALT) || key.key == SDLK_LALT || key.key == SDLK_RALT;
	bool sys = (alt && !(key.mod & SDL_KMOD_CTRL)) || key.key == SDLK_F10;
	UINT message = key.down ? (sys ? WM_SYSKEYDOWN : WM_KEYDOWN) : (sys ? WM_SYSKEYUP : WM_KEYUP);
	WORD flags = 0;
	if (ExtendedKey(key.scancode)) {
		flags |= KF_EXTENDED;
	}
	if (alt && sys) {
		flags |= KF_ALTDOWN;
	}
	if (!key.down || key.repeat) {
		flags |= KF_REPEAT;
	}
	if (!key.down) {
		flags |= KF_UP;
	}
	Post(WindowOf(key.windowID), message, vk, MAKELPARAM(1, flags));
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
		TranslateKey(event.key);
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
	// Alt+F4 closes the window: Windows' default handling turns it into
	// SC_CLOSE, which sends WM_CLOSE.
	else if (Msg == WM_SYSKEYDOWN && wParam == VK_F4 && (HIWORD(lParam) & KF_ALTDOWN)) {
		SendMessageA(hWnd, WM_CLOSE, 0, 0);
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
