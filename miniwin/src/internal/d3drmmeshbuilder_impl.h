#pragma once

#include "d3drmobject_impl.h"

struct Direct3DRMMeshBuilderImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMMeshBuilder3> {
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirect3DRMMeshBuilder interface
	HRESULT SetPerspective(BOOL perspective) override;
	HRESULT CreateMesh(IDirect3DRMMesh** mesh) override;
	// IDirect3DRMMeshBuilder3 interface
	HRESULT Load(void* filename, void* name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURE3CALLBACK callback, void* arg)
		override;
};
