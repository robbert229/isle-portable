#include "d3drm_impl.h"
#include "d3drmviewport_impl.h"
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

// Clear rectangles of the target and z-buffer. Rectangles are in device
// coordinates and clipped to the viewport; there is no stencil buffer.
HRESULT Direct3DViewportImpl::Clear2(DWORD count, LPD3DRECT rects, DWORD flags, DWORD color, float z, DWORD stencil)
{
	if (!m_owner) {
		return DDERR_GENERIC;
	}
	if (count && !rects) {
		return DDERR_INVALIDPARAMS;
	}
	flags &= D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	for (DWORD i = 0; i < count; ++i) {
		HRESULT result = m_owner->ClearRect(&rects[i], flags, color, z);
		if (result != DD_OK) {
			return result;
		}
	}
	return DD_OK;
}
