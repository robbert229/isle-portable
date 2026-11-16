#include "d3dmaterial_impl.h"
#include "miniwin.h"

HRESULT Direct3DMaterialImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DMaterial3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DMaterial3*>(this);
		return DD_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT Direct3DMaterialImpl::SetMaterial(LPD3DMATERIAL data)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DMaterialImpl::GetHandle(IDirect3DDevice3* device, LPD3DMATERIALHANDLE handle)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}
