#pragma once

#include "d3drm_impl.h"
#include "d3drmobject_impl.h"
#include "miniwin.h"
#include "structs.h"

struct Direct3DRMLightImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMLight> {
	Direct3DRMLightImpl(D3DRMLIGHTTYPE type, float r, float g, float b);
	~Direct3DRMLightImpl() override;
	HRESULT SetColor(D3DCOLOR color) override;
	HRESULT SetColorRGB(float r, float g, float b) override;
	HRESULT SetRange(D3DVALUE range) override;
	HRESULT SetUmbra(D3DVALUE angle) override;
	HRESULT SetPenumbra(D3DVALUE angle) override;
	HRESULT SetEnableFrame(IDirect3DRMFrame* frame) override;
	D3DRMLIGHTTYPE GetType() override;
	D3DCOLOR GetColor() override;
	HRESULT SetConstantAttenuation(D3DVALUE attenuation) override;
	HRESULT SetLinearAttenuation(D3DVALUE attenuation) override;
	HRESULT SetQuadraticAttenuation(D3DVALUE attenuation) override;
	D3DVALUE GetRange() override { return m_range; }
	D3DVALUE GetUmbra() override { return m_umbra; }
	D3DVALUE GetPenumbra() override { return m_penumbra; }
	D3DVALUE GetConstantAttenuation() override { return m_attenuation[0]; }
	D3DVALUE GetLinearAttenuation() override { return m_attenuation[1]; }
	D3DVALUE GetQuadraticAttenuation() override { return m_attenuation[2]; }
	HRESULT GetEnableFrame(IDirect3DRMFrame** frame) override;

	// The colour as D3DRM keeps it: floats, which may exceed 1.
	const FColor& GetColorValue() const { return m_color; }
	// The enable frame without a reference, or nullptr when the light
	// lights every frame.
	IDirect3DRMFrame* PeekEnableFrame() const { return m_enableFrame; }

private:
	D3DRMLIGHTTYPE m_type;
	FColor m_color = {1.f, 1.f, 1.f, 1.f};
	// D3DRM's defaults.
	D3DVALUE m_range = 256.f;
	D3DVALUE m_umbra = 0.4f;
	D3DVALUE m_penumbra = 0.5f;
	D3DVALUE m_attenuation[3] = {1.f, 0.f, 0.f};
	IDirect3DRMFrame* m_enableFrame = nullptr;
};

struct Direct3DRMLightArrayImpl
	: public Direct3DRMArrayBaseImpl<IDirect3DRMLight, Direct3DRMLightImpl, IDirect3DRMLightArray> {
	using Direct3DRMArrayBaseImpl::Direct3DRMArrayBaseImpl;
};
