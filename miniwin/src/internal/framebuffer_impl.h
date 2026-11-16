#pragma once

#include <SDL3/SDL.h>
#include <ddsurface_impl.h>
#include <miniwin/ddraw.h>

struct FrameBufferImpl : public IDirectDrawSurface4 {
	FrameBufferImpl(DWORD virtualWidth, DWORD virtualHeight);
	~FrameBufferImpl() override;

	// IUnknown interface
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override;
	// IDirectDrawSurface interface
	HRESULT AddAttachedSurface(IDirectDrawSurface* lpDDSAttachedSurface) override;
	HRESULT Blt(
		LPRECT lpDestRect,
		IDirectDrawSurface* lpDDSrcSurface,
		LPRECT lpSrcRect,
		DDBltFlags dwFlags,
		LPDDBLTFX lpDDBltFx
	) override;
	HRESULT BltFast(DWORD dwX, DWORD dwY, IDirectDrawSurface* lpDDSrcSurface, LPRECT lpSrcRect, DDBltFastFlags dwTrans)
		override;
	HRESULT Flip(IDirectDrawSurface* lpDDSurfaceTargetOverride, DDFlipFlags dwFlags) override;
	HRESULT GetAttachedSurface(LPDDSCAPS lpDDSCaps, IDirectDrawSurface** lplpDDAttachedSurface) override;
	HRESULT GetDC(HDC* lphDC) override;
	HRESULT GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette) override;
	HRESULT GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat) override;
	HRESULT GetSurfaceDesc(DDSURFACEDESC* lpDDSurfaceDesc) override;
	HRESULT IsLost() override { return DD_OK; }
	HRESULT Lock(LPRECT lpDestRect, DDSURFACEDESC* lpDDSurfaceDesc, DDLockFlags dwFlags, HANDLE hEvent) override;
	HRESULT ReleaseDC(HDC hDC) override;
	HRESULT Restore() override;
	HRESULT SetClipper(IDirectDrawClipper* lpDDClipper) override { return DD_OK; }
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

	bool IsIndex8() const { return m_transferBuffer->m_surface->format == SDL_PIXELFORMAT_INDEX8; }
	// The renderer's current image, read back into the transfer buffer.
	DirectDrawSurfaceImpl* DownloadTransferBuffer();

private:
	void DumpFrame();

	uint32_t m_virtualWidth;
	uint32_t m_virtualHeight;
	bool m_readOnlyLock;
	DirectDrawSurfaceImpl* m_transferBuffer;
	IDirectDrawPalette* m_palette = nullptr;
};
