#include "dinput_impl.h"
#include "miniwin.h"

// The keyboard data format's header. Its per-key object table is not provided until
// IDirectInputDeviceA::SetDataFormat is implemented.
const DIDATAFORMAT c_dfDIKeyboard = {sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), DIDF_RELAXIS, 256, 0, nullptr};

HRESULT DirectInputDeviceImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirectInputDeviceA, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectInputDeviceA*>(this);
		return DI_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirectInputDevice2A, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectInputDevice2A*>(this);
		return DI_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT DirectInputDeviceImpl::Acquire()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DIERR_UNSUPPORTED;
}

HRESULT DirectInputDeviceImpl::Unacquire()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DIERR_UNSUPPORTED;
}

HRESULT DirectInputDeviceImpl::GetDeviceState(DWORD cbData, LPVOID lpvData)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DIERR_UNSUPPORTED;
}

HRESULT DirectInputDeviceImpl::SetDataFormat(LPCDIDATAFORMAT lpdf)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DIERR_UNSUPPORTED;
}

HRESULT DirectInputDeviceImpl::SetCooperativeLevel(HWND hwnd, DWORD dwFlags)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DIERR_UNSUPPORTED;
}

HRESULT DirectInputDeviceImpl::Poll()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DIERR_UNSUPPORTED;
}

HRESULT DirectInputImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirectInputA, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectInputA*>(this);
		return DI_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT DirectInputImpl::CreateDevice(
	const GUID& rguid,
	LPDIRECTINPUTDEVICEA* lplpDirectInputDevice,
	IUnknown* pUnkOuter
)
{
	if (!lplpDirectInputDevice) {
		return DIERR_INVALIDPARAM;
	}
	if (SDL_memcmp(&rguid, &GUID_SysKeyboard, sizeof(GUID)) != 0 &&
		SDL_memcmp(&rguid, &GUID_SysMouse, sizeof(GUID)) != 0) {
		return DIERR_DEVICENOTREG;
	}
	*lplpDirectInputDevice = static_cast<IDirectInputDeviceA*>(new DirectInputDeviceImpl);
	return DI_OK;
}

HRESULT DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA* lplpDirectInput, IUnknown* punkOuter)
{
	if (!lplpDirectInput) {
		return DIERR_INVALIDPARAM;
	}
	*lplpDirectInput = new DirectInputImpl;
	return DI_OK;
}
