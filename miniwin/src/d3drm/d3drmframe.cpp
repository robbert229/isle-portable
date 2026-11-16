#include "d3drm_impl.h"
#include "d3drmframe_impl.h"
#include "d3drmlight_impl.h"
#include "d3drmtexture_impl.h"
#include "d3drmvisual_impl.h"
#include "miniwin.h"

#include <cstring>

Direct3DRMFrameImpl::Direct3DRMFrameImpl(Direct3DRMFrameImpl* parent)
{
	m_children = new Direct3DRMFrameArrayImpl;
	m_lights = new Direct3DRMLightArrayImpl;
	m_visuals = new Direct3DRMVisualArrayImpl;
	if (parent) {
		parent->AddChild(this);
	}
}

Direct3DRMFrameImpl::~Direct3DRMFrameImpl()
{
	m_children->Release();
	m_lights->Release();
	m_visuals->Release();
	if (m_texture) {
		m_texture->Release();
	}
}

HRESULT Direct3DRMFrameImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DRMFrame, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMFrame*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMFrame2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMFrame2*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMFrame3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMFrame3*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMMesh, sizeof(GUID)) == 0) {
		return E_NOINTERFACE;
	}
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Direct3DRMFrameImpl does not implement guid");
	return E_NOINTERFACE;
}

HRESULT Direct3DRMFrameImpl::AddChild(IDirect3DRMFrame* child)
{
	Direct3DRMFrameImpl* childImpl = static_cast<Direct3DRMFrameImpl*>(child);
	if (childImpl->m_parent) {
		if (childImpl->m_parent == this) {
			return DD_OK;
		}
		auto result = childImpl->m_parent->m_children->DeleteElement(childImpl);
		SDL_assert(result == DD_OK);
	}
	childImpl->m_parent = this;
	return m_children->AddElement(child);
}

HRESULT Direct3DRMFrameImpl::DeleteChild(IDirect3DRMFrame* child)
{
	Direct3DRMFrameImpl* childImpl = static_cast<Direct3DRMFrameImpl*>(child);
	HRESULT result = m_children->DeleteElement(childImpl);
	if (result == DD_OK) {
		childImpl->m_parent = nullptr;
	}
	return result;
}

HRESULT Direct3DRMFrameImpl::SetSceneBackgroundRGB(float r, float g, float b)
{
	m_backgroundColor = (0xFF << 24) | (static_cast<BYTE>(r * 255.0f) << 16) | (static_cast<BYTE>(g * 255.0f) << 8) |
						(static_cast<BYTE>(b * 255.0f));
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddLight(IDirect3DRMLight* light)
{
	return m_lights->AddElement(light);
}

HRESULT Direct3DRMFrameImpl::GetLights(IDirect3DRMLightArray** lightArray)
{
	*lightArray = m_lights;
	m_lights->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddTransform(D3DRMCOMBINETYPE combine, D3DRMMATRIX4D matrix)
{
	switch (combine) {
	case D3DRMCOMBINETYPE::REPLACE:
		std::memcpy(m_transform, matrix, sizeof(m_transform));
		return DD_OK;
	default:
		MINIWIN_NOT_IMPLEMENTED();
		return DDERR_GENERIC;
	}
}

HRESULT Direct3DRMFrameImpl::GetPosition(IDirect3DRMFrame* reference, D3DVECTOR* position)
{
	if (reference) {
		MINIWIN_NOT_IMPLEMENTED();
		return DDERR_GENERIC;
	}
	position->x = m_transform[3][0] / m_transform[3][3];
	position->y = m_transform[3][1] / m_transform[3][3];
	position->z = m_transform[3][2] / m_transform[3][3];
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddVisual(IDirect3DRMVisual* visual)
{
	return m_visuals->AddElement(visual);
}

HRESULT Direct3DRMFrameImpl::DeleteVisual(IDirect3DRMVisual* visual)
{
	return m_visuals->DeleteElement(visual);
}

HRESULT Direct3DRMFrameImpl::GetVisuals(IDirect3DRMVisualArray** visuals)
{
	*visuals = m_visuals;
	m_visuals->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetTexture(IDirect3DRMTexture* texture)
{
	auto textureImpl = static_cast<Direct3DRMTextureImpl*>(texture);
	if (m_texture) {
		m_texture->Release();
	}

	m_texture = textureImpl;
	m_texture->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetTexture(IDirect3DRMTexture** texture)
{
	if (!m_texture) {
		return DDERR_GENERIC;
	}

	*texture = m_texture;
	if (m_texture) {
		m_texture->AddRef();
	}
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetColor(float r, float g, float b, float a)
{
	m_color = (static_cast<BYTE>(a * 255.0f) << 24) | (static_cast<BYTE>(r * 255.0f) << 16) |
			  (static_cast<BYTE>(g * 255.0f) << 8) | (static_cast<BYTE>(b * 255.0f));
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetColor(D3DCOLOR c)
{
	m_color = c;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetColorRGB(float r, float g, float b)
{
	return SetColor(r, g, b, 1.f);
}

HRESULT Direct3DRMFrameImpl::SetMaterialMode(D3DRMMATERIALMODE mode)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetChildren(IDirect3DRMFrameArray** children)
{
	*children = m_children;
	m_children->AddRef();
	return DD_OK;
}

// IDirect3DRMFrame3 interface
HRESULT Direct3DRMFrameImpl::AddChild(IDirect3DRMFrame3* child)
{
	return AddChild(static_cast<IDirect3DRMFrame*>(child));
}

HRESULT Direct3DRMFrameImpl::DeleteChild(IDirect3DRMFrame3* child)
{
	return DeleteChild(static_cast<IDirect3DRMFrame*>(child));
}

HRESULT Direct3DRMFrameImpl::GetPosition(IDirect3DRMFrame3* reference, D3DVECTOR* position)
{
	return GetPosition(static_cast<IDirect3DRMFrame*>(reference), position);
}

HRESULT Direct3DRMFrameImpl::AddTranslation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::AddScale(D3DRMCOMBINETYPE combine, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::AddRotation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::AddVisual(IUnknown* visual)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::GetParent(IDirect3DRMFrame3** parent)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::GetScene(IDirect3DRMFrame3** scene)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::GetTransform(IDirect3DRMFrame3* reference, D3DRMMATRIX4D matrix)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::GetOrientation(IDirect3DRMFrame3* reference, D3DVECTOR* dir, D3DVECTOR* up)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::GetVisuals(DWORD* count, IUnknown** visuals)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::InverseTransform(D3DVECTOR* d, D3DVECTOR* s)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::Load(
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

HRESULT Direct3DRMFrameImpl::LookAt(
	IDirect3DRMFrame3* target,
	IDirect3DRMFrame3* reference,
	D3DRMFRAMECONSTRAINT constraint
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::DeleteVisual(IUnknown* visual)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

D3DCOLOR Direct3DRMFrameImpl::GetSceneBackground()
{
	MINIWIN_NOT_IMPLEMENTED();
	return 0;
}

HRESULT Direct3DRMFrameImpl::SetSceneBackground(D3DCOLOR color)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogEnable(BOOL enable)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogColor(D3DCOLOR color)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogMode(D3DRMFOGMODE mode)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogParams(D3DVALUE start, D3DVALUE end, D3DVALUE density)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetOrientation(
	IDirect3DRMFrame3* reference,
	D3DVALUE dx,
	D3DVALUE dy,
	D3DVALUE dz,
	D3DVALUE ux,
	D3DVALUE uy,
	D3DVALUE uz
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetPosition(IDirect3DRMFrame3* reference, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetSortMode(D3DRMSORTMODE mode)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::Transform(D3DVECTOR* d, D3DVECTOR* s)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogMethod(DWORD flags)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}
