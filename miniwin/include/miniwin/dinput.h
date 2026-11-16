#pragma once

#include "miniwin/windows.h"

// --- Defines and Macros ---
#define DI_OK S_OK
#define DIERR_INVALIDPARAM ((HRESULT) 0x80070057)
#define DIERR_UNSUPPORTED E_NOTIMPL
#define DIERR_DEVICENOTREG ((HRESULT) 0x80040154)

#define DISCL_EXCLUSIVE 0x00000001
#define DISCL_NONEXCLUSIVE 0x00000002
#define DISCL_FOREGROUND 0x00000004
#define DISCL_BACKGROUND 0x00000008

#define DIDF_ABSAXIS 0x00000001
#define DIDF_RELAXIS 0x00000002

// --- GUIDs ---
DEFINE_GUID(IID_IDirectInputA, 0x89521360, 0xaa8a, 0x11cf, 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(IID_IDirectInputDeviceA, 0x5944e680, 0xc92e, 0x11cf, 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(IID_IDirectInputDevice2A, 0x5944e682, 0xc92e, 0x11cf, 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(GUID_SysMouse, 0x6f1d2b60, 0xd5a0, 0x11cf, 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);
DEFINE_GUID(GUID_SysKeyboard, 0x6f1d2b61, 0xd5a0, 0x11cf, 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);

// --- Structs ---
struct DIOBJECTDATAFORMAT {
	const GUID* pguid;
	DWORD dwOfs;
	DWORD dwType;
	DWORD dwFlags;
};
typedef DIOBJECTDATAFORMAT* LPDIOBJECTDATAFORMAT;

struct DIDATAFORMAT {
	DWORD dwSize;
	DWORD dwObjSize;
	DWORD dwFlags;
	DWORD dwDataSize;
	DWORD dwNumObjs;
	LPDIOBJECTDATAFORMAT rgodf;
};
typedef DIDATAFORMAT* LPDIDATAFORMAT;
typedef const DIDATAFORMAT* LPCDIDATAFORMAT;

// --- Interfaces ---
struct IDirectInputDeviceA : virtual public IUnknown {
	virtual HRESULT Acquire() = 0;
	virtual HRESULT Unacquire() = 0;
	virtual HRESULT GetDeviceState(DWORD cbData, LPVOID lpvData) = 0;
	virtual HRESULT SetDataFormat(LPCDIDATAFORMAT lpdf) = 0;
	virtual HRESULT SetCooperativeLevel(HWND hwnd, DWORD dwFlags) = 0;
};
typedef IDirectInputDeviceA* LPDIRECTINPUTDEVICEA;

struct IDirectInputDevice2A : public IDirectInputDeviceA {
	virtual HRESULT Poll() = 0;
};
typedef IDirectInputDevice2A* LPDIRECTINPUTDEVICE2A;

struct IDirectInputA : virtual public IUnknown {
	virtual HRESULT CreateDevice(
		const GUID& rguid,
		LPDIRECTINPUTDEVICEA* lplpDirectInputDevice,
		IUnknown* pUnkOuter
	) = 0;
};
typedef IDirectInputA* LPDIRECTINPUTA;

// --- Data ---
extern const DIDATAFORMAT c_dfDIKeyboard;

// --- Functions ---
HRESULT DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA* lplpDirectInput, IUnknown* punkOuter);
