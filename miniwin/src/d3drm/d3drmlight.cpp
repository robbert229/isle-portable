#include "d3drmlight_impl.h"
#include "miniwin.h"

#include <algorithm>

Direct3DRMLightImpl::Direct3DRMLightImpl(D3DRMLIGHTTYPE type, float r, float g, float b) : m_type(type)
{
	SetColorRGB(r, g, b);
}

Direct3DRMLightImpl::~Direct3DRMLightImpl()
{
	if (m_enableFrame) {
		m_enableFrame->Release();
	}
}

HRESULT Direct3DRMLightImpl::SetColorRGB(float r, float g, float b)
{
	m_color = {r, g, b, 1.f};
	return DD_OK;
}

D3DRMLIGHTTYPE Direct3DRMLightImpl::GetType()
{
	return m_type;
}

D3DCOLOR Direct3DRMLightImpl::GetColor()
{
	auto toByte = [](float c) { return static_cast<DWORD>(std::min(1.f, std::max(0.f, c)) * 255.f); };
	return (0xFFu << 24) | (toByte(m_color.r) << 16) | (toByte(m_color.g) << 8) | toByte(m_color.b);
}

// The alpha byte is ignored, as D3DRM ignores it.
HRESULT Direct3DRMLightImpl::SetColor(D3DCOLOR color)
{
	m_color = {((color >> 16) & 0xFF) / 255.f, ((color >> 8) & 0xFF) / 255.f, (color & 0xFF) / 255.f, 1.f};
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::SetRange(D3DVALUE range)
{
	m_range = range;
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::SetUmbra(D3DVALUE angle)
{
	m_umbra = angle;
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::SetPenumbra(D3DVALUE angle)
{
	m_penumbra = angle;
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::SetConstantAttenuation(D3DVALUE attenuation)
{
	m_attenuation[0] = attenuation;
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::SetLinearAttenuation(D3DVALUE attenuation)
{
	m_attenuation[1] = attenuation;
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::SetQuadraticAttenuation(D3DVALUE attenuation)
{
	m_attenuation[2] = attenuation;
	return DD_OK;
}

// The light lights only the visuals of this frame and its descendants;
// nullptr lights every frame again.
HRESULT Direct3DRMLightImpl::SetEnableFrame(IDirect3DRMFrame* frame)
{
	if (frame) {
		frame->AddRef();
	}
	if (m_enableFrame) {
		m_enableFrame->Release();
	}
	m_enableFrame = frame;
	return DD_OK;
}

HRESULT Direct3DRMLightImpl::GetEnableFrame(IDirect3DRMFrame** frame)
{
	if (!frame) {
		return DDERR_INVALIDPARAMS;
	}
	*frame = m_enableFrame;
	if (m_enableFrame) {
		m_enableFrame->AddRef();
	}
	return DD_OK;
}
