#include "d3drmtexture_impl.h"
#include "ddpalette_impl.h"
#include "ddraw_impl.h"
#include "ddsurface_impl.h"
#include "framebuffer_impl.h"
#include "miniwin.h"

#include <assert.h>
#include <unordered_map>

static std::unordered_map<D3DTEXTUREHANDLE, DirectDrawSurfaceImpl*>& TextureHandles()
{
	static std::unordered_map<D3DTEXTUREHANDLE, DirectDrawSurfaceImpl*> handles;
	return handles;
}

DirectDrawSurfaceImpl::DirectDrawSurfaceImpl(int width, int height, SDL_PixelFormat format)
{
	m_surface = SDL_CreateSurface(width, height, format);
	if (!m_surface) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create surface: %s", SDL_GetError());
	}
}

DirectDrawSurfaceImpl::~DirectDrawSurfaceImpl()
{
	if (m_handle) {
		TextureHandles().erase(m_handle);
	}
	SDL_DestroySurface(m_surface);
	if (m_palette) {
		m_palette->Release();
	}
	if (m_texture) {
		m_texture->Release();
	}
}

// IUnknown interface
HRESULT DirectDrawSurfaceImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirectDrawSurface, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectDrawSurface*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirectDrawSurface3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectDrawSurface3*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirectDrawSurface4, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectDrawSurface4*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DTexture2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DTexture2*>(this);
		return S_OK;
	}
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "DirectDrawSurfaceImpl does not implement guid");
	return E_NOINTERFACE;
}

// IDirectDrawSurface interface
HRESULT DirectDrawSurfaceImpl::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Blt(
	LPRECT lpDestRect,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DDBltFlags dwFlags,
	LPDDBLTFX lpDDBltFx
)
{
	if ((dwFlags & DDBLT_COLORFILL) == DDBLT_COLORFILL) {
		SDL_Palette* palette = m_palette ? static_cast<DirectDrawPaletteImpl*>(m_palette)->m_palette : nullptr;
		Uint32 color = FillColorToPixel(m_surface->format, palette, lpDDBltFx->dwFillColor);
		if (lpDestRect) {
			SDL_Rect dstRect = ConvertRect(lpDestRect);
			SDL_FillSurfaceRect(m_surface, &dstRect, color);
		}
		else {
			SDL_FillSurfaceRect(m_surface, nullptr, color);
		}
		if (m_texture) {
			m_texture->Changed(TRUE, FALSE);
		}
		return DD_OK;
	}

	if (!lpDDSrcSurface) {
		return DDERR_INVALIDPARAMS;
	}
	if (auto frameBuffer = dynamic_cast<FrameBufferImpl*>(lpDDSrcSurface)) {
		// Reading the screen back: copy from what the renderer has drawn.
		lpDDSrcSurface = frameBuffer->DownloadTransferBuffer();
		if (!lpDDSrcSurface) {
			return DDERR_GENERIC;
		}
	}

	auto other = static_cast<DirectDrawSurfaceImpl*>(lpDDSrcSurface);

	SDL_Rect srcRect = lpSrcRect ? ConvertRect(lpSrcRect) : SDL_Rect{0, 0, other->m_surface->w, other->m_surface->h};
	SDL_Rect dstRect = lpDestRect ? ConvertRect(lpDestRect) : SDL_Rect{0, 0, m_surface->w, m_surface->h};

	SDL_Surface* blitSource = other->m_surface;

	if (other->m_surface->format != m_surface->format) {
		blitSource = SDL_ConvertSurface(other->m_surface, m_surface->format);
		if (!blitSource) {
			return DDERR_GENERIC;
		}
	}

	// DirectDraw applies the source colour key only when asked to.
	bool keyed = SDL_SurfaceHasColorKey(blitSource);
	Uint32 key = 0;
	bool useKey = (dwFlags & DDBLT_KEYSRC) == DDBLT_KEYSRC;
	if (keyed && !useKey) {
		SDL_GetSurfaceColorKey(blitSource, &key);
		SDL_SetSurfaceColorKey(blitSource, false, 0);
	}

	bool blitted = SDL_BlitSurfaceScaled(blitSource, &srcRect, m_surface, &dstRect, SDL_SCALEMODE_NEAREST);

	if (keyed && !useKey) {
		SDL_SetSurfaceColorKey(blitSource, true, key);
	}
	if (blitSource != other->m_surface) {
		SDL_DestroySurface(blitSource);
	}
	if (!blitted) {
		return DDERR_GENERIC;
	}
	if (m_texture) {
		m_texture->Changed(TRUE, FALSE);
	}
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::BltFast(
	DWORD dwX,
	DWORD dwY,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DDBltFastFlags dwTrans
)
{
	if (!lpDDSrcSurface) {
		return DDERR_INVALIDPARAMS;
	}
	int width, height;
	if (lpSrcRect) {
		width = lpSrcRect->right - lpSrcRect->left;
		height = lpSrcRect->bottom - lpSrcRect->top;
	}
	else {
		DDSURFACEDESC desc = {};
		desc.dwSize = sizeof(desc);
		lpDDSrcSurface->GetSurfaceDesc(&desc);
		width = desc.dwWidth;
		height = desc.dwHeight;
	}
	RECT destRect = {(int) dwX, (int) dwY, (int) dwX + width, (int) dwY + height};
	DDBltFlags flags = DDBLT_NONE;
	if ((dwTrans & DDBLTFAST_SRCCOLORKEY) == DDBLTFAST_SRCCOLORKEY) {
		flags = flags | DDBLT_KEYSRC;
	}
	return Blt(&destRect, lpDDSrcSurface, lpSrcRect, flags, nullptr);
}

HRESULT DirectDrawSurfaceImpl::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DDFlipFlags dwFlags)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE* lplpDDAttachedSurface)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetDC(HDC* lphDC)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette)
{
	if (!m_palette) {
		return DDERR_GENERIC;
	}
	m_palette->AddRef();
	*lplpDDPalette = m_palette;
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	memset(lpDDPixelFormat, 0, sizeof(*lpDDPixelFormat));
	lpDDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(m_surface->format);
	if (details->bits_per_pixel == 8) {
		lpDDPixelFormat->dwFlags |= DDPF_PALETTEINDEXED8;
	}
	lpDDPixelFormat->dwRGBBitCount = details->bits_per_pixel;
	lpDDPixelFormat->dwRBitMask = details->Rmask;
	lpDDPixelFormat->dwGBitMask = details->Gmask;
	lpDDPixelFormat->dwBBitMask = details->Bmask;
	lpDDPixelFormat->dwRGBAlphaBitMask = details->Amask;
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	lpDDSurfaceDesc->dwFlags = DDSD_PIXELFORMAT;
	GetPixelFormat(&lpDDSurfaceDesc->ddpfPixelFormat);
	lpDDSurfaceDesc->dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
	lpDDSurfaceDesc->dwWidth = m_surface->w;
	lpDDSurfaceDesc->dwHeight = m_surface->h;
	lpDDSurfaceDesc->lPitch = m_surface->pitch;
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::IsLost()
{
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Lock(
	LPRECT lpDestRect,
	LPDDSURFACEDESC lpDDSurfaceDesc,
	DDLockFlags dwFlags,
	HANDLE hEvent
)
{
	if (!SDL_LockSurface(m_surface)) {
		return DDERR_GENERIC;
	}

	GetSurfaceDesc(lpDDSurfaceDesc);
	Uint8* pixels = static_cast<Uint8*>(m_surface->pixels);
	if (lpDestRect) {
		pixels += lpDestRect->top * m_surface->pitch + lpDestRect->left * SDL_BYTESPERPIXEL(m_surface->format);
	}
	lpDDSurfaceDesc->lpSurface = pixels;
	lpDDSurfaceDesc->lPitch = m_surface->pitch;

	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::ReleaseDC(HDC hDC)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Restore()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::SetColorKey(DDColorKeyFlags dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if (!lpDDColorKey) {
		SDL_SetSurfaceColorKey(m_surface, false, 0);
	}
	else {
		if (lpDDColorKey->dwColorSpaceLowValue != lpDDColorKey->dwColorSpaceHighValue) {
			// SDL keys a single value; the low end of the range is used.
			MINIWIN_NOT_IMPLEMENTED();
		}
		if (!SDL_SetSurfaceColorKey(m_surface, true, lpDDColorKey->dwColorSpaceLowValue)) {
			return DDERR_GENERIC;
		}
	}
	if (m_texture) {
		m_texture->Changed(TRUE, FALSE);
	}

	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if (m_surface->format != SDL_PIXELFORMAT_INDEX8) {
		MINIWIN_NOT_IMPLEMENTED();
	}

	if (m_texture) {
		m_texture->Changed(FALSE, TRUE);
	}

	lpDDPalette->AddRef();

	if (m_palette) {
		m_palette->Release();
	}

	m_palette = lpDDPalette;
	SDL_SetSurfacePalette(m_surface, ((DirectDrawPaletteImpl*) m_palette)->m_palette);
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Unlock(LPVOID lpSurfaceData)
{
	SDL_UnlockSurface(m_surface);
	if (m_texture) {
		m_texture->Changed(TRUE, FALSE);
	}
	return DD_OK;
}

// IDirectDrawSurface4 interface
HRESULT DirectDrawSurfaceImpl::Blt(
	LPRECT lpDestRect,
	LPDIRECTDRAWSURFACE4 lpDDSrcSurface,
	LPRECT lpSrcRect,
	DDBltFlags dwFlags,
	LPDDBLTFX lpDDBltFx
)
{
	return Blt(lpDestRect, static_cast<LPDIRECTDRAWSURFACE>(lpDDSrcSurface), lpSrcRect, dwFlags, lpDDBltFx);
}

HRESULT DirectDrawSurfaceImpl::Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DDFlipFlags dwFlags)
{
	return DDERR_NOTFLIPPABLE;
}

HRESULT DirectDrawSurfaceImpl::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4* lplpDDAttachedSurface)
{
	return DDERR_NOTFOUND;
}

HRESULT DirectDrawSurfaceImpl::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	DDSURFACEDESC desc = {};
	desc.dwSize = sizeof(desc);
	HRESULT result = GetSurfaceDesc(&desc);
	if (result == DD_OK) {
		FromSurfaceDesc(desc, *lpDDSurfaceDesc);
	}
	return result;
}

HRESULT DirectDrawSurfaceImpl::Lock(
	LPRECT lpDestRect,
	LPDDSURFACEDESC2 lpDDSurfaceDesc,
	DDLockFlags dwFlags,
	HANDLE hEvent
)
{
	DDSURFACEDESC desc = {};
	desc.dwSize = sizeof(desc);
	HRESULT result = Lock(lpDestRect, &desc, dwFlags, hEvent);
	if (result == DD_OK) {
		FromSurfaceDesc(desc, *lpDDSurfaceDesc);
	}
	return result;
}

HRESULT DirectDrawSurfaceImpl::Unlock(LPRECT lpRect)
{
	return Unlock(static_cast<LPVOID>(nullptr));
}

// IDirect3DTexture2 interface
// The handle is the same for every device: there is only one renderer.
HRESULT DirectDrawSurfaceImpl::GetHandle(IDirect3DDevice2* device, LPD3DTEXTUREHANDLE handle)
{
	if (!handle) {
		return DDERR_INVALIDPARAMS;
	}
	if (!m_handle) {
		static D3DTEXTUREHANDLE next = 0;
		do {
			++next;
		} while (next == 0 || TextureHandles().count(next));
		m_handle = next;
		TextureHandles()[m_handle] = this;
	}
	*handle = m_handle;
	return DD_OK;
}

DirectDrawSurfaceImpl* DirectDrawSurfaceImpl::FromTextureHandle(D3DTEXTUREHANDLE handle)
{
	auto it = TextureHandles().find(handle);
	return it == TextureHandles().end() ? nullptr : it->second;
}

IDirect3DRMTexture2* DirectDrawSurfaceImpl::ToTexture()
{
	if (!m_texture) {
		m_texture = new Direct3DRMTextureImpl(this, false);
	}
	return m_texture;
}
