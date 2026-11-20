#include "d3dmaterial_impl.h"
#include "miniwin.h"

#include <unordered_map>

static std::unordered_map<D3DMATERIALHANDLE, Direct3DMaterialImpl*>& MaterialHandles()
{
	static std::unordered_map<D3DMATERIALHANDLE, Direct3DMaterialImpl*> handles;
	return handles;
}

const D3DMATERIAL* Direct3DMaterial_FromHandle(D3DMATERIALHANDLE handle)
{
	auto it = MaterialHandles().find(handle);
	return it == MaterialHandles().end() ? nullptr : &it->second->m_material;
}

Direct3DMaterialImpl::~Direct3DMaterialImpl()
{
	if (m_handle) {
		MaterialHandles().erase(m_handle);
	}
}

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
	// A non-zero handle, unique per material; SetLightState(MATERIAL)
	// selects the material by it.
	static D3DMATERIALHANDLE nextHandle = 1;
	if (!m_handle) {
		m_handle = nextHandle++;
		MaterialHandles()[m_handle] = this;
	}
	*handle = m_handle;
	return DD_OK;
}
