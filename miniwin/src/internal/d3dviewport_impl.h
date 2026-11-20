#pragma once

#include "miniwin/d3d.h"

struct Direct3DRMViewportImpl;

// The Direct3D viewport of a Direct3DRM viewport (GetDirect3DViewport).
struct Direct3DViewportImpl : public IDirect3DViewport3 {
	explicit Direct3DViewportImpl(Direct3DRMViewportImpl* owner) : m_owner(owner) {}
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirect3DViewport3 interface
	HRESULT Clear2(DWORD count, LPD3DRECT rects, DWORD flags, DWORD color, float z, DWORD stencil) override;

	// The Direct3DRM viewport this one belongs to; null once it is gone.
	Direct3DRMViewportImpl* m_owner;
};
