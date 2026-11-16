#pragma once

#include "d3drmrenderer.h"
#include "framebuffer_impl.h"
#include "miniwin/d3d.h"
#include "miniwin/ddraw.h"
#include "miniwin/miniwind3d.h"

#include <SDL3/SDL.h>

extern SDL_Window* DDWindow;
extern Direct3DRMRenderer* DDRenderer;

inline static SDL_Rect ConvertRect(const RECT* r)
{
	return {r->left, r->top, r->right - r->left, r->bottom - r->top};
}

// DDSURFACEDESC2 <-> DDSURFACEDESC for the fields miniwin uses, so the
// IDirectDraw4/IDirectDrawSurface4 entry points share the version-1 code.
inline static void ToSurfaceDesc(const DDSURFACEDESC2& in, DDSURFACEDESC& out)
{
	out = {};
	out.dwSize = sizeof(DDSURFACEDESC);
	out.dwFlags = in.dwFlags;
	out.dwHeight = in.dwHeight;
	out.dwWidth = in.dwWidth;
	out.lPitch = in.lPitch;
	out.dwBackBufferCount = in.dwBackBufferCount;
	out.lpSurface = in.lpSurface;
	out.ddpfPixelFormat = in.ddpfPixelFormat;
	out.ddsCaps.dwCaps = in.ddsCaps.dwCaps;
	if ((in.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER) {
		// DDSURFACEDESC2 carries the z-buffer depth in the pixel format.
		out.dwFlags = out.dwFlags | DDSD_ZBUFFERBITDEPTH;
		out.dwZBufferBitDepth = in.ddpfPixelFormat.dwRGBBitCount ? in.ddpfPixelFormat.dwRGBBitCount : 16;
	}
}

inline static void FromSurfaceDesc(const DDSURFACEDESC& in, DDSURFACEDESC2& out)
{
	out = {};
	out.dwSize = sizeof(DDSURFACEDESC2);
	out.dwFlags = in.dwFlags;
	out.dwHeight = in.dwHeight;
	out.dwWidth = in.dwWidth;
	out.lPitch = in.lPitch;
	out.dwBackBufferCount = in.dwBackBufferCount;
	out.lpSurface = in.lpSurface;
	out.ddpfPixelFormat = in.ddpfPixelFormat;
	out.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	out.ddsCaps.dwCaps = in.ddsCaps.dwCaps;
}

// A DDBLTFX fill colour as an SDL pixel of `format`. DirectDraw passes a
// pixel in the surface's own format; miniwin has always read 32-bit fills as
// A8R8G8B8, which is the same thing on a 32-bit display, and keeps that.
inline static Uint32 FillColorToPixel(SDL_PixelFormat format, SDL_Palette* palette, DWORD fill)
{
	if (format == SDL_PIXELFORMAT_INDEX8) {
		return fill & 0xFF;
	}
	const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(format);
	if (details->bits_per_pixel <= 16) {
		return fill & ((1u << details->bits_per_pixel) - 1);
	}
	return SDL_MapRGBA(details, palette, (fill >> 16) & 0xFF, (fill >> 8) & 0xFF, fill & 0xFF, (fill >> 24) & 0xFF);
}

struct DirectDrawImpl : public IDirectDraw4, public IDirect3D3, public IDirect3DMiniwin {
	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirectDraw interface
	HRESULT CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter) override;
	HRESULT
	CreatePalette(
		DDPixelCaps dwFlags,
		LPPALETTEENTRY lpColorTable,
		LPDIRECTDRAWPALETTE* lplpDDPalette,
		IUnknown* pUnkOuter
	) override;
	HRESULT CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter)
		override;
	HRESULT EnumDisplayModes(
		DWORD dwFlags,
		LPDDSURFACEDESC lpDDSurfaceDesc,
		LPVOID lpContext,
		LPDDENUMMODESCALLBACK lpEnumModesCallback
	) override;
	HRESULT FlipToGDISurface() override;
	HRESULT GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps) override;
	HRESULT GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc) override;
	HRESULT RestoreDisplayMode() override;
	HRESULT SetCooperativeLevel(HWND hWnd, DDSCLFlags dwFlags) override;
	HRESULT SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP) override;
	// IDirectDraw4 interface
	HRESULT CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE4* lplpDDSurface, IUnknown* pUnkOuter)
		override;
	HRESULT EnumDisplayModes(
		DWORD dwFlags,
		LPDDSURFACEDESC2 lpDDSurfaceDesc,
		LPVOID lpContext,
		LPDDENUMMODESCALLBACK2 lpEnumModesCallback
	) override;
	HRESULT SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) override;
	HRESULT GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) override;
	// IDirect3D2 interface
	HRESULT CreateDevice(const GUID& guid, IDirectDrawSurface* pBackBuffer, IDirect3DDevice2** ppDirect3DDevice)
		override;
	HRESULT EnumDevices(LPD3DENUMDEVICESCALLBACK cb, void* ctx) override;
	// IDirect3D3 interface
	HRESULT CreateMaterial(IDirect3DMaterial3** material, IUnknown* outer) override;
	// IDirect3DMiniwin interface
	HRESULT RequestMSAA(DWORD msaaSamples) override
	{
		m_msaaSamples = msaaSamples;
		return DD_OK;
	}
	DWORD GetMSAASamples() const override { return m_msaaSamples; }
	HRESULT RequestAnisotropic(float anisotropic) override
	{
		m_anisotropic = anisotropic;
		return DD_OK;
	}
	float GetAnisotropic() const override { return m_anisotropic; }
	HRESULT RequestLightingModel(D3DLightingModel lightingModel) override
	{
		m_lightingModel = lightingModel;
		return DD_OK;
	}
	D3DLightingModel GetLightingModel() const override { return m_lightingModel; }
	HRESULT RequestRenderStyle(D3DRenderStyle renderStyle) override
	{
		m_renderStyle = renderStyle;
		return DD_OK;
	}
	D3DRenderStyle GetRenderStyle() const override { return m_renderStyle; }

private:
	HRESULT CreateSurface4(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE4* lplpDDSurface);

	FrameBufferImpl* m_frameBuffer = nullptr;
	int m_virtualWidth = 0;
	int m_virtualHeight = 0;
	int m_virtualBPP = 0;
	DWORD m_msaaSamples = 0;
	float m_anisotropic = 0.0f;
	D3DLightingModel m_lightingModel = D3DLightingModel::DirectX5;
	D3DRenderStyle m_renderStyle = D3DRenderStyle::Smooth;
};

HRESULT DirectDrawEnumerate(LPDDENUMCALLBACKA cb, void* context);

SDL_PixelFormat MiniwinDisplayPixelFormat();

HRESULT DirectDrawCreate(LPGUID lpGuid, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter);

void EnumDevice(
	LPD3DENUMDEVICESCALLBACK cb,
	void* ctx,
	const char* name,
	D3DDEVICEDESC* halDesc,
	D3DDEVICEDESC* helDesc,
	GUID deviceGuid
);
