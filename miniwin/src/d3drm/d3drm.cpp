#include "miniwin/d3drm.h"

#include "d3drm_impl.h"
#include "d3drmanimationset_impl.h"
#include "d3drmdevice_impl.h"
#include "d3drmframe_impl.h"
#include "d3drmlight_impl.h"
#include "d3drmmesh_impl.h"
#include "d3drmmeshbuilder_impl.h"
#include "d3drmobject_impl.h"
#include "d3drmrenderer.h"
#include "d3drmtexture_impl.h"
#include "d3drmuservisual_impl.h"
#include "d3drmviewport_impl.h"
#include "ddraw_impl.h"
#include "ddsurface_impl.h"
#include "miniwin.h"

#include <SDL3/SDL.h>

Direct3DRMPickedArrayImpl::Direct3DRMPickedArrayImpl(const PickRecord* inputPicks, size_t count)
{
	picks.reserve(count);
	for (size_t i = 0; i < count; ++i) {
		const PickRecord& pick = inputPicks[i];
		if (pick.visual) {
			pick.visual->AddRef();
		}
		if (pick.frameArray) {
			pick.frameArray->AddRef();
		}
		picks.push_back(pick);
	}
}

Direct3DRMPickedArrayImpl::~Direct3DRMPickedArrayImpl()
{
	for (PickRecord& pick : picks) {
		if (pick.visual) {
			pick.visual->Release();
		}
		if (pick.frameArray) {
			pick.frameArray->Release();
		}
	}
}

DWORD Direct3DRMPickedArrayImpl::GetSize()
{
	return static_cast<DWORD>(picks.size());
}

HRESULT Direct3DRMPickedArrayImpl::GetPick(
	DWORD index,
	IDirect3DRMVisual** visual,
	IDirect3DRMFrameArray** frameArray,
	D3DRMPICKDESC* desc
)
{
	if (index >= picks.size()) {
		return DDERR_INVALIDPARAMS;
	}

	const PickRecord& pick = picks[index];

	*visual = pick.visual;
	*frameArray = pick.frameArray;
	*desc = pick.desc;

	if (*visual) {
		(*visual)->AddRef();
	}
	if (*frameArray) {
		(*frameArray)->AddRef();
	}

	return DD_OK;
}

struct Direct3DRMWinDeviceImpl : public IDirect3DRMWinDevice {
	HRESULT Activate() override
	{
		MINIWIN_NOT_IMPLEMENTED();
		return DD_OK;
	}
	HRESULT Paint() override
	{
		MINIWIN_NOT_IMPLEMENTED();
		return DD_OK;
	}
	void HandleActivate(WORD wParam) override { MINIWIN_NOT_IMPLEMENTED(); }
	void HandlePaint(void* p_dc) override { MINIWIN_NOT_IMPLEMENTED(); }
};

struct Direct3DRMMaterialImpl : public Direct3DRMObjectBaseImpl<IDirect3DRMMaterial2> {
	Direct3DRMMaterialImpl(D3DVALUE power) : m_power(power) {}
	HRESULT QueryInterface(const GUID& riid, void** ppvObject) override
	{
		if (SDL_memcmp(&riid, &IID_IDirect3DRMMaterial, sizeof(GUID)) == 0) {
			this->IUnknown::AddRef();
			*ppvObject = static_cast<IDirect3DRMMaterial*>(this);
			return DD_OK;
		}
		if (SDL_memcmp(&riid, &IID_IDirect3DRMMaterial2, sizeof(GUID)) == 0) {
			this->IUnknown::AddRef();
			*ppvObject = static_cast<IDirect3DRMMaterial2*>(this);
			return DD_OK;
		}
		MINIWIN_NOT_IMPLEMENTED();
		return E_NOINTERFACE;
	}
	D3DVALUE GetPower() override { return m_power; }
	HRESULT SetSpecular(D3DVALUE r, D3DVALUE g, D3DVALUE b) override
	{
		m_specular[0] = r;
		m_specular[1] = g;
		m_specular[2] = b;
		return DD_OK;
	}
	HRESULT SetEmissive(D3DVALUE r, D3DVALUE g, D3DVALUE b) override
	{
		m_emissive[0] = r;
		m_emissive[1] = g;
		m_emissive[2] = b;
		return DD_OK;
	}
	HRESULT GetSpecular(D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) override { return Get(m_specular, r, g, b); }
	HRESULT GetEmissive(D3DVALUE* r, D3DVALUE* g, D3DVALUE* b) override { return Get(m_emissive, r, g, b); }

private:
	static HRESULT Get(const D3DVALUE (&value)[3], D3DVALUE* r, D3DVALUE* g, D3DVALUE* b)
	{
		if (!r || !g || !b) {
			return DDERR_INVALIDPARAMS;
		}
		*r = value[0];
		*g = value[1];
		*b = value[2];
		return DD_OK;
	}
	D3DVALUE m_power;
	// D3DRM's defaults: white specular, no emission.
	D3DVALUE m_specular[3] = {1.f, 1.f, 1.f};
	D3DVALUE m_emissive[3] = {0.f, 0.f, 0.f};
};

HRESULT Direct3DRMImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DRM, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRM*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRM2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRM2*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRM3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRM3*>(this);
		return DD_OK;
	}
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Direct3DRMImpl does not implement guid");
	return E_NOINTERFACE;
}

HRESULT Direct3DRMImpl::CreateDeviceFromD3D(
	const IDirect3D2* d3d,
	IDirect3DDevice2* d3dDevice,
	IDirect3DRMDevice2** outDevice
)
{
	auto renderer = static_cast<Direct3DRMRenderer*>(d3dDevice);
	*outDevice = static_cast<IDirect3DRMDevice2*>(
		new Direct3DRMDevice2Impl(renderer->GetVirtualWidth(), renderer->GetVirtualHeight(), renderer)
	);
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateDeviceFromSurface(
	const GUID* guid,
	IDirectDraw* dd,
	IDirectDrawSurface* surface,
	IDirect3DRMDevice2** outDevice
)
{
	DDSURFACEDESC DDSDesc;
	DDSDesc.dwSize = sizeof(DDSURFACEDESC);
	surface->GetSurfaceDesc(&DDSDesc);

	IDirect3DMiniwin* miniwind3d = nullptr;
	dd->QueryInterface(IID_IDirect3DMiniwin, (void**) &miniwind3d);
	SDL_assert(miniwind3d);

	DDRenderer = CreateDirect3DRMRenderer(miniwind3d, DDSDesc, guid);
	if (!DDRenderer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create a renderer for the selected device GUID");
		return E_NOINTERFACE;
	}
	IDirect3D3* d3d = nullptr;
	if (dd->QueryInterface(IID_IDirect3D3, (void**) &d3d) == DD_OK) {
		DDRenderer->SetDirect3D(d3d);
		d3d->Release();
	}
	*outDevice =
		static_cast<IDirect3DRMDevice2*>(new Direct3DRMDevice2Impl(DDSDesc.dwWidth, DDSDesc.dwHeight, DDRenderer));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateTexture(D3DRMIMAGE* image, IDirect3DRMTexture2** outTexture)
{
	*outTexture = static_cast<IDirect3DRMTexture2*>(new Direct3DRMTextureImpl(image));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateTextureFromSurface(LPDIRECTDRAWSURFACE surface, IDirect3DRMTexture2** outTexture)
{
	*outTexture = static_cast<IDirect3DRMTexture2*>(new Direct3DRMTextureImpl(surface, true));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateMesh(IDirect3DRMMesh** outMesh)
{
	*outMesh = static_cast<IDirect3DRMMesh*>(new Direct3DRMMeshImpl);
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateMaterial(D3DVAL power, IDirect3DRMMaterial** outMaterial)
{
	*outMaterial = static_cast<IDirect3DRMMaterial*>(new Direct3DRMMaterialImpl(power));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateLightRGB(D3DRMLIGHTTYPE type, D3DVAL r, D3DVAL g, D3DVAL b, IDirect3DRMLight** outLight)
{
	*outLight = static_cast<IDirect3DRMLight*>(new Direct3DRMLightImpl(type, r, g, b));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateFrame(IDirect3DRMFrame* parent, IDirect3DRMFrame2** outFrame)
{
	auto parentImpl = static_cast<Direct3DRMFrameImpl*>(parent);
	*outFrame = static_cast<IDirect3DRMFrame2*>(new Direct3DRMFrameImpl{parentImpl});
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateViewport(
	IDirect3DRMDevice2* iDevice,
	IDirect3DRMFrame* camera,
	int x,
	int y,
	int width,
	int height,
	IDirect3DRMViewport** outViewport
)
{
	auto device = static_cast<Direct3DRMDevice2Impl*>(iDevice);
	auto* viewport =
		new Direct3DRMViewportImpl(width, height, device->m_renderer, static_cast<IDirect3DRMDevice*>(device), x, y);
	if (camera) {
		viewport->SetCamera(camera);
	}
	*outViewport = static_cast<IDirect3DRMViewport*>(viewport);
	device->AddViewport(*outViewport);
	return DD_OK;
}

HRESULT Direct3DRMImpl::SetDefaultTextureShades(DWORD count)
{
	return DD_OK;
}

HRESULT Direct3DRMImpl::SetDefaultTextureColors(DWORD count)
{
	return DD_OK;
}

// IDirect3DRM3 interface
HRESULT Direct3DRMImpl::CreateFrame(IDirect3DRMFrame3* parent, IDirect3DRMFrame3** outFrame)
{
	auto parentImpl = static_cast<Direct3DRMFrameImpl*>(parent);
	*outFrame = static_cast<IDirect3DRMFrame3*>(new Direct3DRMFrameImpl{parentImpl});
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateMaterial(D3DVALUE power, IDirect3DRMMaterial2** outMaterial)
{
	*outMaterial = static_cast<IDirect3DRMMaterial2*>(new Direct3DRMMaterialImpl(power));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateDeviceFromSurface(
	GUID* guid,
	IDirectDraw* dd,
	IDirectDrawSurface* surface,
	DWORD flags,
	IDirect3DRMDevice3** outDevice
)
{
	if (flags) {
		MINIWIN_NOT_IMPLEMENTED();
		return E_NOTIMPL;
	}
	IDirect3DRMDevice2* device;
	HRESULT result = CreateDeviceFromSurface(static_cast<const GUID*>(guid), dd, surface, &device);
	if (result == DD_OK) {
		*outDevice = static_cast<Direct3DRMDevice2Impl*>(device);
	}
	return result;
}

HRESULT Direct3DRMImpl::CreateTextureFromSurface(LPDIRECTDRAWSURFACE surface, IDirect3DRMTexture3** outTexture)
{
	*outTexture = static_cast<IDirect3DRMTexture3*>(new Direct3DRMTextureImpl(surface, true));
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateViewport(
	IDirect3DRMDevice3* device,
	IDirect3DRMFrame3* camera,
	DWORD x,
	DWORD y,
	DWORD width,
	DWORD height,
	IDirect3DRMViewport2** outViewport
)
{
	IDirect3DRMViewport* viewport;
	HRESULT result = CreateViewport(
		static_cast<IDirect3DRMDevice2*>(device),
		static_cast<IDirect3DRMFrame*>(camera),
		x,
		y,
		width,
		height,
		&viewport
	);
	if (result == DD_OK) {
		*outViewport = static_cast<Direct3DRMViewportImpl*>(viewport);
	}
	return result;
}

HRESULT Direct3DRMImpl::LoadTexture(const char* filename, IDirect3DRMTexture3** outTexture)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMImpl::CreateMeshBuilder(IDirect3DRMMeshBuilder3** outMeshBuilder)
{
	*outMeshBuilder = static_cast<IDirect3DRMMeshBuilder3*>(new Direct3DRMMeshBuilderImpl);
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateAnimationSet(IDirect3DRMAnimationSet2** outAnimationSet)
{
	*outAnimationSet = static_cast<IDirect3DRMAnimationSet2*>(new Direct3DRMAnimationSetImpl);
	return DD_OK;
}

HRESULT Direct3DRMImpl::CreateUserVisual(
	D3DRMUSERVISUALCALLBACK callback,
	void* arg,
	IDirect3DRMUserVisual** outUserVisual
)
{
	*outUserVisual = static_cast<IDirect3DRMUserVisual*>(new Direct3DRMUserVisualImpl(callback, arg));
	return DD_OK;
}

HRESULT WINAPI Direct3DRMCreate(IDirect3DRM** direct3DRM)
{
	*direct3DRM = new Direct3DRMImpl;
	return DD_OK;
}

D3DCOLOR D3DRMCreateColorRGBA(D3DVALUE red, D3DVALUE green, D3DVALUE blue, D3DVALUE alpha)
{
	int a = static_cast<int>(255.f * alpha);
	int r = static_cast<int>(255.f * red);
	int g = static_cast<int>(255.f * green);
	int b = static_cast<int>(255.f * blue);

	return (a << 24) | (r << 16) | (g << 8) | b;
}

D3DVECTOR* D3DRMVectorRandom(D3DVECTOR* d)
{
	float length;
	do {
		d->x = SDL_randf() * 2.0f - 1.0f;
		d->y = SDL_randf() * 2.0f - 1.0f;
		d->z = SDL_randf() * 2.0f - 1.0f;
		length = d->x * d->x + d->y * d->y + d->z * d->z;
	} while (length > 1.0f || length < 1e-6f);
	length = SDL_sqrtf(length);
	d->x /= length;
	d->y /= length;
	d->z /= length;
	return d;
}

D3DVECTOR* D3DRMVectorRotate(D3DVECTOR* r, D3DVECTOR* v, D3DVECTOR* axis, D3DVALUE theta)
{
	// Rodrigues' rotation about the normalised axis.
	float length = SDL_sqrtf(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
	if (length < 1e-6f) {
		*r = *v;
		return r;
	}
	const float kx = axis->x / length;
	const float ky = axis->y / length;
	const float kz = axis->z / length;
	const float c = SDL_cosf(theta);
	const float s = SDL_sinf(theta);
	const float dot = kx * v->x + ky * v->y + kz * v->z;
	const D3DVECTOR in = *v;
	r->x = in.x * c + (ky * in.z - kz * in.y) * s + kx * dot * (1.0f - c);
	r->y = in.y * c + (kz * in.x - kx * in.z) * s + ky * dot * (1.0f - c);
	r->z = in.z * c + (kx * in.y - ky * in.x) * s + kz * dot * (1.0f - c);
	return r;
}

