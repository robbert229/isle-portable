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

// A light in world space. A spot light is both positional and directional.
// The first 48 bytes are the shading layout the GPU backends upload; the
// rest are D3DRM's range, attenuation and spot cone, with D3DRM's defaults.
struct SceneLight {
	FColor color;
	D3DVECTOR position;
	float positional = 0.f; // position is valid if 1.f
	D3DVECTOR direction;
	float directional = 0.f; // direction is valid if 1.f
	float range = 256.f;     // positional lights light nothing beyond it
	float attenuation[3] = {1.f, 0.f, 0.f}; // 1 / (a0 + a1 d + a2 d^2)
	// The spot's cones, as angles in radians from its axis: full strength
	// inside the umbra, falling off to nothing at the penumbra.
	float umbra = 0.4f;
	float penumbra = 0.5f;
};
static_assert(sizeof(SceneLight) == 72);

struct Plane {
	D3DVECTOR normal;
	float d;
};

// The immediate-mode state of the device (IDirect3DDevice2/3): render
// states, light states and texture stage states, indexed by their Direct3D
// values. Untouched states hold Direct3D's defaults.
struct ImmediateState {
	static constexpr DWORD RenderStateCount = 256;
	static constexpr DWORD LightStateCount = 16;
	static constexpr DWORD TextureStageCount = 8;
	static constexpr DWORD TextureStageStateCount = 32;

	ImmediateState();
	DWORD Render(D3DRENDERSTATETYPE state) const { return renderState[static_cast<DWORD>(state)]; }

	DWORD renderState[RenderStateCount];
	DWORD lightState[LightStateCount];
	DWORD textureStageState[TextureStageCount][TextureStageStateCount];
};

// One DrawIndexedPrimitive call: a D3DFVF_VERTEX triangle list with the
// device's transforms, state and current material. The vertices and indices
// belong to the caller and are only valid during SubmitImmediate.
struct ImmediateDraw {
	const D3DVERTEX* vertices;
	DWORD vertexCount;
	const WORD* indices;
	DWORD indexCount;
	D3DRMMATRIX4D world;
	D3DRMMATRIX4D view;
	D3DRMMATRIX4D projection;
	const ImmediateState* state;
	D3DMATERIAL material; // all zero when no material is selected
	// Stage 0's texture, from SetTexture or else the TEXTUREHANDLE render
	// state, or nullptr. A backend maps it with GetTextureId.
	IDirect3DRMTexture* texture;
};

class Direct3DRMRenderer : public IDirect3DDevice3 {
public:
	Direct3DRMRenderer();
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
	// Immediate mode: draw one triangle list now, with the device's state.
	// Backends that do not implement it leave DrawIndexedPrimitive
	// unsupported.
	virtual HRESULT SubmitImmediate(const ImmediateDraw& draw) { return DDERR_UNSUPPORTED; }
	// The rectangle 3D drawing is projected into and clipped to, in the
	// game's (virtual) coordinates; nullptr is the whole target. Backends
	// that ignore it draw every viewport over the whole target.
	virtual void SetViewportRect(const D3DRECT* rect) {}
	// Clear a rectangle, in virtual coordinates, of the target (D3DCLEAR_TARGET,
	// to an ARGB colour) and of the z-buffer (D3DCLEAR_ZBUFFER, to z). The
	// default clears only the whole target, through Clear.
	virtual HRESULT ClearRect(const D3DRECT& rect, DWORD flags, D3DCOLOR color, float z);
	virtual void SetPalette(SDL_Palette* palette) {}
	virtual bool UsesPalettedSurfaces() const { return false; }
	// The IDirect3D3 (the DirectDraw object) the device was created from.
	void SetDirect3D(IDirect3D3* d3d);

protected:
	using DestroyCallback = void (*)(Direct3DRMRenderer* renderer, Uint32 id);
	void RegisterDestroyCallback(IDirect3DRMObject* object, Uint32 id, DestroyCallback callback);

	int m_width, m_height;
	int m_virtualWidth, m_virtualHeight;
	ViewportTransform m_viewportTransform;

	ImmediateState m_immediateState;
	D3DRMMATRIX4D m_transforms[3]; // world, view, projection
	IDirect3DViewport3* m_currentViewport = nullptr;
	IDirect3DTexture2* m_textures[ImmediateState::TextureStageCount] = {};

private:
	struct DestroyCallbackContext {
		Direct3DRMRenderer* renderer;
		IDirect3DRMObject* object;
		Uint32 id;
		DestroyCallback callback;
	};
	static void OnObjectDestroyed(IDirect3DRMObject* object, void* arg);

	std::unordered_set<DestroyCallbackContext*> m_destroyCallbacks;
	IDirect3D3* m_direct3d = nullptr;
};

Direct3DRMRenderer* CreateDirect3DRMRenderer(
	const IDirect3DMiniwin* d3d,
	const DDSURFACEDESC& DDSDesc,
	const GUID* guid
);
void Direct3DRMRenderer_EnumDevices(const IDirect3DMiniwin* d3d, LPD3DENUMDEVICESCALLBACK cb, void* ctx);
