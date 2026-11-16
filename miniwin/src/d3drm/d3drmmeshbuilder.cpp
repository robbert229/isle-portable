#include "d3drmmeshbuilder_impl.h"
#include "miniwin.h"

HRESULT Direct3DRMMeshBuilderImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DRMMeshBuilder, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMMeshBuilder*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMMeshBuilder2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMMeshBuilder2*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMMeshBuilder3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMMeshBuilder3*>(this);
		return DD_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT Direct3DRMMeshBuilderImpl::SetPerspective(BOOL perspective)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMMeshBuilderImpl::CreateMesh(IDirect3DRMMesh** mesh)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMMeshBuilderImpl::Load(
	void* filename,
	void* name,
	D3DRMLOADOPTIONS flags,
	D3DRMLOADTEXTURE3CALLBACK callback,
	void* arg
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}
