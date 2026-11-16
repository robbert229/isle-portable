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
	if (!data) {
		return DDERR_INVALIDPARAMS;
	}
	m_material = *data;
	return DD_OK;
}

HRESULT Direct3DMaterialImpl::GetHandle(IDirect3DDevice3* device, LPD3DMATERIALHANDLE handle)
{
	if (!handle) {
		return DDERR_INVALIDPARAMS;
	}
	// A non-zero handle, unique per material; no renderer reads it yet.
	static D3DMATERIALHANDLE nextHandle = 1;
	if (!m_handle) {
		m_handle = nextHandle++;
	}
	*handle = m_handle;
	return DD_OK;
}
