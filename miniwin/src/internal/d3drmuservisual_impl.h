#pragma once

#include "d3drmobject_impl.h"

struct Direct3DRMUserVisualImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMUserVisual> {
	Direct3DRMUserVisualImpl(D3DRMUSERVISUALCALLBACK callback, void* arg) : m_callback(callback), m_arg(arg) {}
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;

private:
	D3DRMUSERVISUALCALLBACK m_callback;
	void* m_arg;
};
