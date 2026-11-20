#pragma once

#include "d3drmobject_impl.h"

struct Direct3DRMUserVisualImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMUserVisual> {
	Direct3DRMUserVisualImpl(D3DRMUSERVISUALCALLBACK callback, void* arg) : m_callback(callback), m_arg(arg) {}
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// Run the callback, as Viewport::Render does: CANSEE asks whether the
	// visual can be seen, RENDER draws it on the device.
	int Invoke(D3DRMUSERVISUALREASON reason, IDirect3DRMDevice* device, IDirect3DRMViewport* viewport)
	{
		return m_callback ? m_callback(this, m_arg, reason, device, viewport) : 0;
	}

private:
	D3DRMUSERVISUALCALLBACK m_callback;
	void* m_arg;
};
