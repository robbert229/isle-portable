#pragma once

#include "d3drmobject_impl.h"
#include "d3drmrenderer.h"
#include "miniwin/d3drm.h"

#include <SDL3/SDL.h>
#include <vector>

struct DeferredDrawCommand {
	DWORD meshId;
	D3DRMMATRIX4D modelViewMatrix;
	D3DRMMATRIX4D worldMatrix;
	Matrix3x3 normalMatrix;
	Appearance appearance;
	float depth;
	uint64_t lightMask;
};

class Direct3DRMDeviceImpl;
class Direct3DRMFrameImpl;

struct Direct3DRMViewportImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMViewport> {
	Direct3DRMViewportImpl(DWORD width, DWORD height, Direct3DRMRenderer* renderer);
	~Direct3DRMViewportImpl() override;
	HRESULT Render(IDirect3DRMFrame* group) override;
	/**
	 * @brief Blit the render back to our backbuffer
	 */
	HRESULT ForceUpdate(int x, int y, int w, int h) override;
	HRESULT Clear() override;
	HRESULT SetCamera(IDirect3DRMFrame* camera) override;
	HRESULT GetCamera(IDirect3DRMFrame** camera) override;
	HRESULT SetProjection(D3DRMPROJECTIONTYPE type) override;
	D3DRMPROJECTIONTYPE GetProjection() override;
	HRESULT SetFront(D3DVALUE z) override;
	D3DVALUE GetFront() override;
	HRESULT SetBack(D3DVALUE z) override;
	D3DVALUE GetBack() override;
	HRESULT SetField(D3DVALUE field) override;
	D3DVALUE GetField() override;
	DWORD GetWidth() override { return m_virtualWidth; }
	DWORD GetHeight() override { return m_virtualHeight; }
	HRESULT Transform(D3DRMVECTOR4D* screen, D3DVECTOR* world) override;
	HRESULT InverseTransform(D3DVECTOR* world, D3DRMVECTOR4D* screen) override;
	HRESULT Pick(float x, float y, LPDIRECT3DRMPICKEDARRAY* pickedArray) override;
	void CloseDevice();
	void UpdateProjectionMatrix();

private:
	HRESULT RenderScene();
	void CollectLightsFromFrame(IDirect3DRMFrame* frame, D3DRMMATRIX4D parentMatrix);
	// lightMask has a bit for each of m_lights that lights the frame's
	// parent; the frame adds the lights whose enable frame it is.
	void CollectMeshesFromFrame(IDirect3DRMFrame* frame, D3DRMMATRIX4D parentMatrix, uint64_t lightMask);
	// Push the lights in mask to the renderer, unless they are pushed already.
	void UseLights(uint64_t mask);
	void BuildViewFrustumPlanes();
	Direct3DRMRenderer* m_renderer;
	std::vector<DeferredDrawCommand> m_deferredDraws;
	// The scene's lights, and for each its enable frame (nullptr when it
	// lights every frame). Lights past the mask's 64 bits light every frame.
	std::vector<SceneLight> m_lights;
	std::vector<IDirect3DRMFrame*> m_lightEnableFrames;
	std::vector<SceneLight> m_activeLights;
	uint64_t m_pushedLightMask = 0;
	D3DCOLOR m_backgroundColor = 0xFF000000;
	DWORD m_virtualWidth;
	DWORD m_virtualHeight;
	D3DRMMATRIX4D m_viewProjectionwMatrix;
	D3DRMMATRIX4D m_viewMatrix;
	D3DRMMATRIX4D m_projectionMatrix;
	D3DRMMATRIX4D m_inverseProjectionMatrix;
	IDirect3DRMFrame* m_rootFrame = nullptr;
	IDirect3DRMFrame* m_camera = nullptr;
	D3DVALUE m_front = 1.f;
	D3DVALUE m_back = 10.f;
	D3DVALUE m_field = 0.5f;
	Plane m_frustumPlanes[6];
};

struct Direct3DRMViewportArrayImpl
	: public Direct3DRMArrayBaseImpl<IDirect3DRMViewport, Direct3DRMViewportImpl, IDirect3DRMViewportArray> {
	using Direct3DRMArrayBaseImpl::Direct3DRMArrayBaseImpl;
};
