#include "d3dviewport_impl.h"
#include "miniwin.h"

HRESULT Direct3DViewportImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DViewport, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DViewport*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DViewport3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DViewport3*>(this);
		return DD_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT Direct3DViewportImpl::Clear2(DWORD count, LPD3DRECT rects, DWORD flags, DWORD color, float z, DWORD stencil)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}
