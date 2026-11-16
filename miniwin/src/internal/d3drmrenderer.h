#pragma once

#include "d3drmmesh_impl.h"
#include "mathutils.h"
#include "miniwin/d3drm.h"
#include "miniwin/miniwind3d.h"
#include "miniwin/miniwindevice.h"
#include "structs.h"

#include <SDL3/SDL.h>
#include <unordered_set>

#define NO_TEXTURE_ID 0xffffffff

static_assert(sizeof(D3DRMVERTEX) == 32);

struct SceneLight {
	FColor color;
	D3DVECTOR position;
	float positional = 0.f; // position is valid if 1.f
	D3DVECTOR direction;
	float directional = 0.f; // direction is valid if 1.f
};
static_assert(sizeof(SceneLight) == 48);

struct Plane {
	D3DVECTOR normal;
	float d;
};

class Direct3DRMRenderer : public IDirect3DDevice3 {
public:
	~Direct3DRMRenderer() override;
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirect3DDevice3 interface
	HRESULT GetDirect3D(IDirect3D3** d3d) override;
	HRESULT SetCurrentViewport(IDirect3DViewport3* viewport) override;
	HRESULT GetRenderState(D3DRENDERSTATETYPE state, LPDWORD value) override;
	HRESULT SetRenderState(D3DRENDERSTATETYPE state, DWORD value) override;
	HRESULT GetLightState(D3DLIGHTSTATETYPE state, LPDWORD value) override;
	HRESULT SetLightState(D3DLIGHTSTATETYPE state, DWORD value) override;
	HRESULT SetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX matrix) override;
	HRESULT GetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX matrix) override;
	HRESULT DrawIndexedPrimitive(
		D3DPRIMITIVETYPE primitiveType,
		DWORD vertexType,
		LPVOID vertices,
		DWORD vertexCount,
		WORD* indices,
		DWORD indexCount,
		DWORD flags
	) override;
	HRESULT GetTexture(DWORD stage, IDirect3DTexture2** texture) override;
	HRESULT SetTexture(DWORD stage, IDirect3DTexture2* texture) override;
	HRESULT GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state, LPDWORD value) override;
	HRESULT SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value) override;

	// Direct3DRMRenderer interface
	virtual void PushLights(const SceneLight* vertices, size_t count) = 0;
	virtual void SetProjection(const D3DRMMATRIX4D& projection, D3DVALUE front, D3DVALUE back) = 0;
	virtual void SetFrustumPlanes(const Plane* frustumPlanes) = 0;
	virtual Uint32 GetTextureId(IDirect3DRMTexture* texture, bool isUI = false, float scaleX = 0, float scaleY = 0) = 0;
	virtual Uint32 GetMeshId(IDirect3DRMMesh* mesh, const MeshGroup* meshGroup) = 0;
	int GetWidth() { return m_width; }
	int GetHeight() { return m_height; }
	int GetVirtualWidth() { return m_virtualWidth; }
	int GetVirtualHeight() { return m_virtualHeight; }
	virtual HRESULT BeginFrame() = 0;
	virtual void EnableTransparency() = 0;
	virtual void SubmitDraw(
		DWORD meshId,
		const D3DRMMATRIX4D& modelViewMatrix,
		const D3DRMMATRIX4D& worldMatrix,
		const D3DRMMATRIX4D& viewMatrix,
		const Matrix3x3& normalMatrix,
		const Appearance& appearance
	) = 0;
	virtual HRESULT FinalizeFrame() = 0;
	virtual void Resize(int width, int height, const ViewportTransform& viewportTransform) = 0;
	virtual void Clear(float r, float g, float b) = 0;
	virtual void Flip() = 0;
	virtual void Draw2DImage(Uint32 textureId, const SDL_Rect& srcRect, const SDL_Rect& dstRect, FColor color) = 0;
	virtual void Download(SDL_Surface* target) = 0;
	virtual void SetDither(bool dither) = 0;
	virtual void SetPalette(SDL_Palette* palette) {}
	virtual bool UsesPalettedSurfaces() const { return false; }

protected:
	using DestroyCallback = void (*)(Direct3DRMRenderer* renderer, Uint32 id);
	void RegisterDestroyCallback(IDirect3DRMObject* object, Uint32 id, DestroyCallback callback);

	int m_width, m_height;
	int m_virtualWidth, m_virtualHeight;
	ViewportTransform m_viewportTransform;

private:
	struct DestroyCallbackContext {
		Direct3DRMRenderer* renderer;
		IDirect3DRMObject* object;
		Uint32 id;
		DestroyCallback callback;
	};
	static void OnObjectDestroyed(IDirect3DRMObject* object, void* arg);

	std::unordered_set<DestroyCallbackContext*> m_destroyCallbacks;
};

Direct3DRMRenderer* CreateDirect3DRMRenderer(
	const IDirect3DMiniwin* d3d,
	const DDSURFACEDESC& DDSDesc,
	const GUID* guid
);
void Direct3DRMRenderer_EnumDevices(const IDirect3DMiniwin* d3d, LPD3DENUMDEVICESCALLBACK cb, void* ctx);
