#pragma once

#include "miniwin/d3d.h"

struct Direct3DMaterialImpl : public IDirect3DMaterial3 {
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirect3DMaterial3 interface
	HRESULT SetMaterial(LPD3DMATERIAL data) override;
	HRESULT GetHandle(IDirect3DDevice3* device, LPD3DMATERIALHANDLE handle) override;
};
