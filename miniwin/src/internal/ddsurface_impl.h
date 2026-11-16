#pragma once

#include <SDL3/SDL.h>
#include <miniwin/d3d.h>
#include <miniwin/ddraw.h>

struct DirectDrawSurfaceImpl : public IDirectDrawSurface4, public IDirect3DTexture2 {
	DirectDrawSurfaceImpl(int width, int height, SDL_PixelFormat format);
	~DirectDrawSurfaceImpl() override;

	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirectDrawSurface interface
	HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface) override;
	HRESULT Blt(
		LPRECT lpDestRect,
		LPDIRECTDRAWSURFACE lpDDSrcSurface,
		LPRECT lpSrcRect,
		DDBltFlags dwFlags,
		LPDDBLTFX lpDDBltFx
	) override;
	HRESULT BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DDBltFastFlags dwTrans)
		override;
	HRESULT Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DDFlipFlags dwFlags) override;
	HRESULT GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE* lplpDDAttachedSurface) override;
	HRESULT GetDC(HDC* lphDC) override;
	HRESULT GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette) override;
	HRESULT GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat) override;
	HRESULT GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc) override;
	HRESULT IsLost() override;
	HRESULT Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DDLockFlags dwFlags, HANDLE hEvent) override;
	HRESULT ReleaseDC(HDC hDC) override;
	HRESULT Restore() override;
	HRESULT SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper) override;
	HRESULT SetColorKey(DDColorKeyFlags dwFlags, LPDDCOLORKEY lpDDColorKey) override;
	HRESULT SetPalette(LPDIRECTDRAWPALETTE lpDDPalette) override;
	HRESULT Unlock(LPVOID lpSurfaceData) override;
	// IDirectDrawSurface4 interface
	HRESULT Blt(
		LPRECT lpDestRect,
		LPDIRECTDRAWSURFACE4 lpDDSrcSurface,
		LPRECT lpSrcRect,
		DDBltFlags dwFlags,
		LPDDBLTFX lpDDBltFx
	) override;
	HRESULT Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DDFlipFlags dwFlags) override;
	HRESULT GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4* lplpDDAttachedSurface) override;
	HRESULT GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc) override;
	HRESULT Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DDLockFlags dwFlags, HANDLE hEvent) override;
	HRESULT Unlock(LPRECT lpRect) override;
	// IDirect3DTexture2 interface
	HRESULT GetHandle(IDirect3DDevice2* device, LPD3DTEXTUREHANDLE handle) override;

	IDirect3DRMTexture2* ToTexture();

	SDL_Surface* m_surface = nullptr;

private:
	IDirect3DRMTexture2* m_texture = nullptr;
	IDirectDrawPalette* m_palette = nullptr;
};
