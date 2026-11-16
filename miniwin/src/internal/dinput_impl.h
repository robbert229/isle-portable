#pragma once

#include "miniwin/dinput.h"

struct DirectInputDeviceImpl : public IDirectInputDevice2A {
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirectInputDeviceA interface
	HRESULT Acquire() override;
	HRESULT Unacquire() override;
	HRESULT GetDeviceState(DWORD cbData, LPVOID lpvData) override;
	HRESULT SetDataFormat(LPCDIDATAFORMAT lpdf) override;
	HRESULT SetCooperativeLevel(HWND hwnd, DWORD dwFlags) override;
	// IDirectInputDevice2A interface
	HRESULT Poll() override;
};

struct DirectInputImpl : public IDirectInputA {
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirectInputA interface
	HRESULT CreateDevice(const GUID& rguid, LPDIRECTINPUTDEVICEA* lplpDirectInputDevice, IUnknown* pUnkOuter) override;
};
