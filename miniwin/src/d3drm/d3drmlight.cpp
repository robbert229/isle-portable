#include "d3drmlight_impl.h"
#include "miniwin.h"

Direct3DRMLightImpl::Direct3DRMLightImpl(D3DRMLIGHTTYPE type, float r, float g, float b) : m_type(type)
{
	SetColorRGB(r, g, b);
}

HRESULT Direct3DRMLightImpl::SetColorRGB(float r, float g, float b)
{
	m_color = (0xFF << 24) | (static_cast<BYTE>(r * 255.0f) << 16) | (static_cast<BYTE>(g * 255.0f) << 8) |
			  (static_cast<BYTE>(b * 255.0f));
	return DD_OK;
}

D3DRMLIGHTTYPE Direct3DRMLightImpl::GetType()
{
	return m_type;
}

D3DCOLOR Direct3DRMLightImpl::GetColor()
{
	return m_color;
}

HRESULT Direct3DRMLightImpl::SetColor(D3DCOLOR color)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMLightImpl::SetRange(D3DVALUE range)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMLightImpl::SetUmbra(D3DVALUE angle)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMLightImpl::SetPenumbra(D3DVALUE angle)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMLightImpl::SetEnableFrame(IDirect3DRMFrame* frame)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}
