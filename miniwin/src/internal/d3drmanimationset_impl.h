#pragma once

#include "d3drmobject_impl.h"

struct Direct3DRMAnimationSetImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMAnimationSet2> {
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	HRESULT Load(
		void* filename,
		void* name,
		D3DRMLOADOPTIONS flags,
		D3DRMLOADTEXTURE3CALLBACK callback,
		void* arg,
		IDirect3DRMFrame3* parent
	) override;
	HRESULT SetTime(D3DVALUE time) override;
};
