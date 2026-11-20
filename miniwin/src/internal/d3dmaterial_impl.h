#pragma once

#include "miniwin/d3d.h"

struct Direct3DMaterialImpl : public IDirect3DMaterial3 {
	~Direct3DMaterialImpl() override;
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirect3DMaterial3 interface
	HRESULT SetMaterial(LPD3DMATERIAL data) override;
	HRESULT GetHandle(IDirect3DDevice3* device, LPD3DMATERIALHANDLE handle) override;

	D3DMATERIAL m_material{};

private:
	D3DMATERIALHANDLE m_handle = 0;
};

// The material a handle from GetHandle names, or nullptr: what
// SetLightState(D3DLIGHTSTATE_MATERIAL, handle) selects for drawing.
const D3DMATERIAL* Direct3DMaterial_FromHandle(D3DMATERIALHANDLE handle);
