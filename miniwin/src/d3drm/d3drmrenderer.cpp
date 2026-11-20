#include "d3dmaterial_impl.h"
#include "d3drmrenderer.h"
#include "d3drm_impl.h"
#include "d3drmviewport_impl.h"
#include "d3dviewport_impl.h"
#include "ddsurface_impl.h"
#include "miniwin.h"
#ifdef USE_OPENGL1
#include "d3drmrenderer_opengl1.h"
#endif
#ifdef USE_OPENGLES2
#include "d3drmrenderer_opengles2.h"
#endif
#ifdef USE_OPENGLES3
#include "d3drmrenderer_opengles3.h"
#endif
#ifdef USE_CITRO3D
#include "d3drmrenderer_citro3d.h"
#endif
#ifdef USE_DIRECTX9
#include "d3drmrenderer_directx9.h"
#endif
#ifdef USE_SDL_GPU
#include "d3drmrenderer_sdl3gpu.h"
#endif
#ifdef USE_SOFTWARE_RENDER
#include "d3drmrenderer_software.h"
#endif
#ifdef USE_PALETTE_SW_RENDER
#include "d3drmrenderer_palettesw.h"
#endif
#ifdef USE_GXM
#include "d3drmrenderer_gxm.h"
#endif
#ifdef USE_GLIDE
#include "d3drmrenderer_glide.h"
#endif

ImmediateState::ImmediateState()
{
	SDL_memset(renderState, 0, sizeof(renderState));
	SDL_memset(lightState, 0, sizeof(lightState));
	SDL_memset(textureStageState, 0, sizeof(textureStageState));

	// Direct3D's defaults, as a device with a z-buffer reports them.
	renderState[4] = TRUE;  // TEXTUREPERSPECTIVE
	renderState[7] = TRUE;  // ZENABLE
	renderState[8] = 3;     // FILLMODE: SOLID
	renderState[9] = static_cast<DWORD>(D3DSHADE_GOURAUD);
	renderState[10] = 0xffff; // LINEPATTERN
	renderState[14] = TRUE;   // ZWRITEENABLE
	renderState[16] = TRUE;   // LASTPIXEL
	renderState[17] = 1;      // TEXTUREMAG: NEAREST
	renderState[18] = 1;      // TEXTUREMIN: NEAREST
	renderState[19] = static_cast<DWORD>(D3DBLEND_ONE);
	renderState[20] = static_cast<DWORD>(D3DBLEND_ZERO);
	renderState[21] = 4; // TEXTUREMAPBLEND: MODULATE
	renderState[22] = static_cast<DWORD>(D3DCULL_CCW);
	renderState[23] = static_cast<DWORD>(D3DCMP_LESSEQUAL);
	renderState[25] = static_cast<DWORD>(D3DCMP_ALWAYS); // ALPHAFUNC
	renderState[26] = FALSE;                             // DITHERENABLE
	const float one = 1.0f;
	SDL_memcpy(&renderState[37], &one, sizeof(DWORD)); // FOGTABLEEND
	SDL_memcpy(&renderState[38], &one, sizeof(DWORD)); // FOGTABLEDENSITY
	SDL_memcpy(&lightState[6], &one, sizeof(DWORD));   // FOGEND
	SDL_memcpy(&lightState[7], &one, sizeof(DWORD));   // FOGDENSITY

	for (DWORD stage = 0; stage < TextureStageCount; ++stage) {
		DWORD* tss = textureStageState[stage];
		tss[1] = stage == 0 ? 4 : 1; // COLOROP: MODULATE, or DISABLE
		tss[2] = 2;                  // COLORARG1: TEXTURE
		tss[3] = 1;                  // COLORARG2: CURRENT
		tss[4] = stage == 0 ? 2 : 1; // ALPHAOP: SELECTARG1, or DISABLE
		tss[5] = 2;                  // ALPHAARG1: TEXTURE
		tss[6] = 1;                  // ALPHAARG2: CURRENT
		tss[11] = stage;             // TEXCOORDINDEX
		tss[12] = 1;                 // ADDRESS: WRAP
		tss[16] = 1;                 // MAGFILTER: POINT
		tss[17] = 1;                 // MINFILTER: POINT
		tss[18] = 1;                 // MIPFILTER: NONE
	}
}

Direct3DRMRenderer::Direct3DRMRenderer()
{
	for (D3DRMMATRIX4D& m : m_transforms) {
		IdentityMatrix(m);
	}
}

Direct3DRMRenderer::~Direct3DRMRenderer()
{
	for (IDirect3DTexture2*& texture : m_textures) {
		if (texture) {
			texture->Release();
			texture = nullptr;
		}
	}
	if (m_currentViewport) {
		m_currentViewport->Release();
		m_currentViewport = nullptr;
	}
	for (DestroyCallbackContext* ctx : m_destroyCallbacks) {
		ctx->object->DeleteDestroyCallback(OnObjectDestroyed, ctx);
		delete ctx;
	}
	if (m_direct3d) {
		m_direct3d->Release();
	}
}

void Direct3DRMRenderer::SetDirect3D(IDirect3D3* d3d)
{
	if (d3d) {
		d3d->AddRef();
	}
	if (m_direct3d) {
		m_direct3d->Release();
	}
	m_direct3d = d3d;
}

HRESULT Direct3DRMRenderer::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DDevice2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DDevice2*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DDevice3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DDevice3*>(this);
		return DD_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

HRESULT Direct3DRMRenderer::GetDirect3D(IDirect3D3** d3d)
{
	if (!d3d) {
		return DDERR_INVALIDPARAMS;
	}
	*d3d = m_direct3d;
	if (!m_direct3d) {
		return DDERR_GENERIC;
	}
	m_direct3d->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMRenderer::SetCurrentViewport(IDirect3DViewport3* viewport)
{
	if (viewport) {
		viewport->AddRef();
	}
	if (m_currentViewport) {
		m_currentViewport->Release();
	}
	m_currentViewport = viewport;
	// Draws now project into and clip to that viewport's rectangle.
	if (auto* impl = dynamic_cast<Direct3DViewportImpl*>(viewport)) {
		if (impl->m_owner && impl->m_owner->GetRenderer() == this) {
			impl->m_owner->ApplyToRenderer();
		}
	}
	return DD_OK;
}

HRESULT Direct3DRMRenderer::ClearRect(const D3DRECT& rect, DWORD flags, D3DCOLOR color, float z)
{
	if (!(flags & D3DCLEAR_TARGET)) {
		return DD_OK; // this backend clears its z-buffer in every BeginFrame
	}
	if (rect.x1 > 0 || rect.y1 > 0 || rect.x2 < m_virtualWidth || rect.y2 < m_virtualHeight) {
		MINIWIN_NOT_IMPLEMENTED();
		return DDERR_UNSUPPORTED;
	}
	Clear(((color >> 16) & 0xff) / 255.0f, ((color >> 8) & 0xff) / 255.0f, (color & 0xff) / 255.0f);
	return DD_OK;
}

HRESULT Direct3DRMRenderer::GetRenderState(D3DRENDERSTATETYPE state, LPDWORD value)
{
	DWORD index = static_cast<DWORD>(state);
	if (!value || index >= ImmediateState::RenderStateCount) {
		return DDERR_INVALIDPARAMS;
	}
	*value = m_immediateState.renderState[index];
	return DD_OK;
}

HRESULT Direct3DRMRenderer::SetRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
	DWORD index = static_cast<DWORD>(state);
	if (index >= ImmediateState::RenderStateCount) {
		return DDERR_INVALIDPARAMS;
	}
	m_immediateState.renderState[index] = value;
	return DD_OK;
}

HRESULT Direct3DRMRenderer::GetLightState(D3DLIGHTSTATETYPE state, LPDWORD value)
{
	DWORD index = static_cast<DWORD>(state);
	if (!value || index >= ImmediateState::LightStateCount) {
		return DDERR_INVALIDPARAMS;
	}
	*value = m_immediateState.lightState[index];
	return DD_OK;
}

HRESULT Direct3DRMRenderer::SetLightState(D3DLIGHTSTATETYPE state, DWORD value)
{
	DWORD index = static_cast<DWORD>(state);
	if (index >= ImmediateState::LightStateCount) {
		return DDERR_INVALIDPARAMS;
	}
	m_immediateState.lightState[index] = value;
	return DD_OK;
}

HRESULT Direct3DRMRenderer::SetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX matrix)
{
	DWORD index = static_cast<DWORD>(state);
	if (!matrix || index < 1 || index > 3) {
		return DDERR_INVALIDPARAMS;
	}
	SDL_memcpy(m_transforms[index - 1], matrix, sizeof(D3DRMMATRIX4D));
	return DD_OK;
}

HRESULT Direct3DRMRenderer::GetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX matrix)
{
	DWORD index = static_cast<DWORD>(state);
	if (!matrix || index < 1 || index > 3) {
		return DDERR_INVALIDPARAMS;
	}
	SDL_memcpy(matrix, m_transforms[index - 1], sizeof(D3DRMMATRIX4D));
	return DD_OK;
}

HRESULT Direct3DRMRenderer::DrawIndexedPrimitive(
	D3DPRIMITIVETYPE primitiveType,
	DWORD vertexType,
	LPVOID vertices,
	DWORD vertexCount,
	WORD* indices,
	DWORD indexCount,
	DWORD flags
)
{
	if (primitiveType != D3DPT_TRIANGLELIST || vertexType != D3DFVF_VERTEX) {
		MINIWIN_NOT_IMPLEMENTED();
		return DDERR_UNSUPPORTED;
	}
	if (!vertices || !indices || indexCount % 3 != 0) {
		return DDERR_INVALIDPARAMS;
	}
	for (DWORD i = 0; i < indexCount; ++i) {
		if (indices[i] >= vertexCount) {
			return DDERR_INVALIDPARAMS;
		}
	}
	if (indexCount == 0) {
		return DD_OK;
	}

	ImmediateDraw draw = {};
	draw.vertices = static_cast<const D3DVERTEX*>(vertices);
	draw.vertexCount = vertexCount;
	draw.indices = indices;
	draw.indexCount = indexCount;
	SDL_memcpy(draw.world, m_transforms[0], sizeof(D3DRMMATRIX4D));
	SDL_memcpy(draw.view, m_transforms[1], sizeof(D3DRMMATRIX4D));
	SDL_memcpy(draw.projection, m_transforms[2], sizeof(D3DRMMATRIX4D));
	draw.state = &m_immediateState;
	DWORD materialHandle = m_immediateState.lightState[static_cast<DWORD>(D3DLIGHTSTATE_MATERIAL)];
	if (const D3DMATERIAL* material = Direct3DMaterial_FromHandle(materialHandle)) {
		draw.material = *material;
	}
	DirectDrawSurfaceImpl* surface = static_cast<DirectDrawSurfaceImpl*>(m_textures[0]);
	if (!surface) {
		surface = DirectDrawSurfaceImpl::FromTextureHandle(m_immediateState.Render(D3DRENDERSTATE_TEXTUREHANDLE));
	}
	draw.texture = surface ? surface->ToTexture() : nullptr;
	return SubmitImmediate(draw);
}

HRESULT Direct3DRMRenderer::GetTexture(DWORD stage, IDirect3DTexture2** texture)
{
	if (!texture || stage >= ImmediateState::TextureStageCount) {
		return DDERR_INVALIDPARAMS;
	}
	*texture = m_textures[stage];
	if (*texture) {
		(*texture)->AddRef();
	}
	return DD_OK;
}

HRESULT Direct3DRMRenderer::SetTexture(DWORD stage, IDirect3DTexture2* texture)
{
	if (stage >= ImmediateState::TextureStageCount) {
		return DDERR_INVALIDPARAMS;
	}
	if (texture) {
		texture->AddRef();
	}
	if (m_textures[stage]) {
		m_textures[stage]->Release();
	}
	m_textures[stage] = texture;
	return DD_OK;
}

HRESULT Direct3DRMRenderer::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state, LPDWORD value)
{
	DWORD index = static_cast<DWORD>(state);
	if (!value || stage >= ImmediateState::TextureStageCount || index >= ImmediateState::TextureStageStateCount) {
		return DDERR_INVALIDPARAMS;
	}
	*value = m_immediateState.textureStageState[stage][index];
	return DD_OK;
}

HRESULT Direct3DRMRenderer::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
	DWORD index = static_cast<DWORD>(state);
	if (stage >= ImmediateState::TextureStageCount || index >= ImmediateState::TextureStageStateCount) {
		return DDERR_INVALIDPARAMS;
	}
	m_immediateState.textureStageState[stage][index] = value;
	return DD_OK;
}

void Direct3DRMRenderer::RegisterDestroyCallback(IDirect3DRMObject* object, Uint32 id, DestroyCallback callback)
{
	auto* ctx = new DestroyCallbackContext{this, object, id, callback};
	m_destroyCallbacks.insert(ctx);
	object->AddDestroyCallback(OnObjectDestroyed, ctx);
}

void Direct3DRMRenderer::OnObjectDestroyed(IDirect3DRMObject* object, void* arg)
{
	auto* ctx = static_cast<DestroyCallbackContext*>(arg);
	ctx->renderer->m_destroyCallbacks.erase(ctx);
	ctx->callback(ctx->renderer, ctx->id);
	delete ctx;
}

void Miniwin_SetupWindowCreateProperties(SDL_PropertiesID props)
{
#if (defined(USE_OPENGL1) || defined(USE_OPENGLES2) || defined(USE_OPENGLES3)) && !defined(__3DS__) &&                 \
	!defined(WINDOWS_STORE) && !defined(__vita__) && !defined(__DJGPP__)
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif
}

Direct3DRMRenderer* CreateDirect3DRMRenderer(
	const IDirect3DMiniwin* d3d,
	const DDSURFACEDESC& DDSDesc,
	const GUID* guid
)
{
#ifdef USE_SDL_GPU
	if (SDL_memcmp(guid, &SDL3_GPU_GUID, sizeof(GUID)) == 0) {
		return Direct3DRMSDL3GPURenderer::Create(DDSDesc.dwWidth, DDSDesc.dwHeight);
	}
#endif
#ifdef USE_SOFTWARE_RENDER
	if (SDL_memcmp(guid, &SOFTWARE_GUID, sizeof(GUID)) == 0) {
		return Direct3DRMSoftwareRenderer::Create(DDSDesc.dwWidth, DDSDesc.dwHeight, d3d->GetLightingModel());
	}
#endif
#ifdef USE_OPENGLES3
	if (SDL_memcmp(guid, &OpenGLES3_GUID, sizeof(GUID)) == 0) {
		return OpenGLES3Renderer::Create(
			DDSDesc.dwWidth,
			DDSDesc.dwHeight,
			d3d->GetMSAASamples(),
			d3d->GetAnisotropic(),
			d3d->GetLightingModel(),
			d3d->GetRenderStyle()
		);
	}
#endif
#ifdef USE_OPENGLES2
	if (SDL_memcmp(guid, &OpenGLES2_GUID, sizeof(GUID)) == 0) {
		return OpenGLES2Renderer::Create(
			DDSDesc.dwWidth,
			DDSDesc.dwHeight,
			d3d->GetAnisotropic(),
			d3d->GetLightingModel(),
			d3d->GetRenderStyle()
		);
	}
#endif
#ifdef USE_OPENGL1
	if (SDL_memcmp(guid, &OpenGL1_GUID, sizeof(GUID)) == 0) {
		return OpenGL1Renderer::Create(DDSDesc.dwWidth, DDSDesc.dwHeight, d3d->GetMSAASamples());
	}
#endif
#ifdef USE_CITRO3D
	if (SDL_memcmp(guid, &Citro3D_GUID, sizeof(GUID)) == 0) {
		return new Citro3DRenderer(DDSDesc.dwWidth, DDSDesc.dwHeight);
	}
#endif
#ifdef USE_DIRECTX9
	if (SDL_memcmp(guid, &DirectX9_GUID, sizeof(GUID)) == 0) {
		return DirectX9Renderer::Create(DDSDesc.dwWidth, DDSDesc.dwHeight);
	}
#endif
#ifdef USE_GXM
	if (SDL_memcmp(guid, &GXM_GUID, sizeof(GUID)) == 0) {
		return GXMRenderer::Create(DDSDesc.dwWidth, DDSDesc.dwHeight, d3d->GetMSAASamples());
	}
#endif
#ifdef USE_GLIDE
	if (SDL_memcmp(guid, &GLIDE_GUID, sizeof(GUID)) == 0) {
		return new Direct3DRMGlideRenderer(DDSDesc.dwWidth, DDSDesc.dwHeight);
	}
#endif
#ifdef USE_PALETTE_SW_RENDER
	if (SDL_memcmp(guid, &PALETTE_SW_GUID, sizeof(GUID)) == 0) {
		return new Direct3DRMPaletteSWRenderer(DDSDesc.dwWidth, DDSDesc.dwHeight);
	}
#endif
	return nullptr;
}

void Direct3DRMRenderer_EnumDevices(const IDirect3DMiniwin* d3d, LPD3DENUMDEVICESCALLBACK cb, void* ctx)
{
#ifdef USE_SDL_GPU
	Direct3DRMSDL3GPU_EnumDevice(cb, ctx);
#endif
#ifdef USE_OPENGLES3
	OpenGLES3Renderer_EnumDevice(d3d, cb, ctx);
#endif
#ifdef USE_OPENGLES2
	OpenGLES2Renderer_EnumDevice(d3d, cb, ctx);
#endif
#ifdef USE_OPENGL1
	OpenGL1Renderer_EnumDevice(d3d, cb, ctx);
#endif
#ifdef USE_CITRO3D
	Citro3DRenderer_EnumDevice(cb, ctx);
#endif
#ifdef USE_DIRECTX9
	DirectX9Renderer_EnumDevice(cb, ctx);
#endif
#ifdef USE_PALETTE_SW_RENDER
	Direct3DRMPaletteSW_EnumDevice(cb, ctx);
#endif
#ifdef USE_SOFTWARE_RENDER
	Direct3DRMSoftware_EnumDevice(cb, ctx);
#endif
#ifdef USE_GLIDE
	Direct3DRMGlide_EnumDevice(cb, ctx);
#endif
#ifdef USE_GXM
	GXMRenderer_EnumDevice(cb, ctx);
#endif
}
