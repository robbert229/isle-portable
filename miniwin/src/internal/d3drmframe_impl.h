#pragma once

#include "d3drmobject_impl.h"

class Direct3DRMTextureImpl;
class Direct3DRMLightArrayImpl;
class Direct3DRMVisualArrayImpl;
class Direct3DRMFrameArrayImpl;

struct Direct3DRMFrameImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMFrame3> {
	Direct3DRMFrameImpl(Direct3DRMFrameImpl* parent = nullptr);
	~Direct3DRMFrameImpl() override;
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	HRESULT AddChild(IDirect3DRMFrame* child) override;
	HRESULT DeleteChild(IDirect3DRMFrame* child) override;
	HRESULT SetSceneBackgroundRGB(float r, float g, float b) override;
	HRESULT AddLight(IDirect3DRMLight* light) override;
	HRESULT GetLights(IDirect3DRMLightArray** lightArray) override;
	HRESULT AddTransform(D3DRMCOMBINETYPE combine, D3DRMMATRIX4D matrix) override;
	HRESULT GetPosition(IDirect3DRMFrame* reference, D3DVECTOR* position) override;
	HRESULT AddVisual(IDirect3DRMVisual* visual) override;
	HRESULT DeleteVisual(IDirect3DRMVisual* visual) override;
	HRESULT GetVisuals(IDirect3DRMVisualArray** visuals) override;
	HRESULT SetTexture(IDirect3DRMTexture* texture) override;
	HRESULT GetTexture(IDirect3DRMTexture** texture) override;
	HRESULT SetColor(float r, float g, float b, float a) override;
	HRESULT SetColor(D3DCOLOR) override;
	HRESULT SetColorRGB(float r, float g, float b) override;
	HRESULT SetMaterialMode(D3DRMMATERIALMODE mode) override;
	HRESULT GetChildren(IDirect3DRMFrameArray** children) override;
	// IDirect3DRMFrame3 interface
	HRESULT AddChild(IDirect3DRMFrame3* child) override;
	HRESULT DeleteChild(IDirect3DRMFrame3* child) override;
	HRESULT GetPosition(IDirect3DRMFrame3* reference, D3DVECTOR* position) override;
	HRESULT AddTranslation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z) override;
	HRESULT AddScale(D3DRMCOMBINETYPE combine, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) override;
	HRESULT AddRotation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) override;
	HRESULT AddVisual(IUnknown* visual) override;
	HRESULT GetParent(IDirect3DRMFrame3** parent) override;
	HRESULT GetScene(IDirect3DRMFrame3** scene) override;
	HRESULT GetTransform(IDirect3DRMFrame3* reference, D3DRMMATRIX4D matrix) override;
	HRESULT GetOrientation(IDirect3DRMFrame3* reference, D3DVECTOR* dir, D3DVECTOR* up) override;
	HRESULT GetVisuals(DWORD* count, IUnknown** visuals) override;
	HRESULT InverseTransform(D3DVECTOR* d, D3DVECTOR* s) override;
	HRESULT Load(void* filename, void* name, D3DRMLOADOPTIONS flags, D3DRMLOADTEXTURE3CALLBACK callback, void* arg)
		override;
	HRESULT LookAt(IDirect3DRMFrame3* target, IDirect3DRMFrame3* reference, D3DRMFRAMECONSTRAINT constraint) override;
	HRESULT DeleteVisual(IUnknown* visual) override;
	D3DCOLOR GetSceneBackground() override;
	HRESULT SetSceneBackground(D3DCOLOR color) override;
	HRESULT SetSceneFogEnable(BOOL enable) override;
	HRESULT SetSceneFogColor(D3DCOLOR color) override;
	HRESULT SetSceneFogMode(D3DRMFOGMODE mode) override;
	HRESULT SetSceneFogParams(D3DVALUE start, D3DVALUE end, D3DVALUE density) override;
	HRESULT SetOrientation(
		IDirect3DRMFrame3* reference,
		D3DVALUE dx,
		D3DVALUE dy,
		D3DVALUE dz,
		D3DVALUE ux,
		D3DVALUE uy,
		D3DVALUE uz
	) override;
	HRESULT SetPosition(IDirect3DRMFrame3* reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) override;
	HRESULT SetSortMode(D3DRMSORTMODE mode) override;
	HRESULT Transform(D3DVECTOR* d, D3DVECTOR* s) override;
	HRESULT SetSceneFogMethod(DWORD flags) override;

	Direct3DRMFrameImpl* m_parent{};
	D3DRMMATRIX4D m_transform =
		{{1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}};

private:
	Direct3DRMFrameArrayImpl* m_children{};
	Direct3DRMLightArrayImpl* m_lights{};
	Direct3DRMVisualArrayImpl* m_visuals{};
	Direct3DRMTextureImpl* m_texture{};

	D3DCOLOR m_backgroundColor = 0xFF000000;
	D3DCOLOR m_color = 0xffffff;

	friend class Direct3DRMViewportImpl;
};

struct Direct3DRMFrameArrayImpl
	: public Direct3DRMArrayBaseImpl<IDirect3DRMFrame, Direct3DRMFrameImpl, IDirect3DRMFrameArray> {
	using Direct3DRMArrayBaseImpl::Direct3DRMArrayBaseImpl;

	friend class Direct3DRMFrameImpl;
};
