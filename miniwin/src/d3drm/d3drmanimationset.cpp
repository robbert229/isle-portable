#include "d3drmanimationset_impl.h"
#include "miniwin.h"

HRESULT Direct3DRMAnimationSetImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DRMAnimationSet2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMAnimationSet2*>(this);
		return DD_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT Direct3DRMAnimationSetImpl::Load(
	void* filename,
	void* name,
	D3DRMLOADOPTIONS flags,
	D3DRMLOADTEXTURE3CALLBACK callback,
	void* arg,
	IDirect3DRMFrame3* parent
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMAnimationSetImpl::SetTime(D3DVALUE time)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}
