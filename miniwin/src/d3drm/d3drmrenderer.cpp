#include "d3drmrenderer.h"
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

Direct3DRMRenderer::~Direct3DRMRenderer()
{
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
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::GetRenderState(D3DRENDERSTATETYPE state, LPDWORD value)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::SetRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::GetLightState(D3DLIGHTSTATETYPE state, LPDWORD value)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::SetLightState(D3DLIGHTSTATETYPE state, DWORD value)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::SetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX matrix)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::GetTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX matrix)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
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
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::GetTexture(DWORD stage, IDirect3DTexture2** texture)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::SetTexture(DWORD stage, IDirect3DTexture2* texture)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state, LPDWORD value)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
}

HRESULT Direct3DRMRenderer::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DDERR_UNSUPPORTED;
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
