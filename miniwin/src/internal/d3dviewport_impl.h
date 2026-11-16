#pragma once

#include "miniwin/d3d.h"

struct Direct3DViewportImpl : public IDirect3DViewport3 {
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirect3DViewport3 interface
	HRESULT Clear2(DWORD count, LPD3DRECT rects, DWORD flags, DWORD color, float z, DWORD stencil) override;
};
