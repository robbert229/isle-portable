#include "d3drm_impl.h"
#include "d3drmframe_impl.h"
#include "d3drmlight_impl.h"
#include "d3drmtexture_impl.h"
#include "d3drmvisual_impl.h"
#include "mathutils.h"
#include "miniwin.h"

#include <cstring>

// out = a * b (row vectors); out may alias neither input.
static void MatrixMultiply(D3DRMMATRIX4D out, const D3DRMMATRIX4D a, const D3DRMMATRIX4D b)
{
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			out[row][col] = 0.0f;
			for (int k = 0; k < 4; ++k) {
				out[row][col] += a[row][k] * b[k][col];
			}
		}
	}
}

Direct3DRMFrameImpl::Direct3DRMFrameImpl(Direct3DRMFrameImpl* parent)
{
	m_children = new Direct3DRMFrameArrayImpl;
	m_lights = new Direct3DRMLightArrayImpl;
	m_visuals = new Direct3DRMVisualArrayImpl;
	if (parent) {
		parent->AddChild(this);
	}
}

Direct3DRMFrameImpl::~Direct3DRMFrameImpl()
{
	m_children->Release();
	m_lights->Release();
	m_visuals->Release();
	if (m_texture) {
		m_texture->Release();
	}
}

HRESULT Direct3DRMFrameImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DRMFrame, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMFrame*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMFrame2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMFrame2*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMFrame3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMFrame3*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMMesh, sizeof(GUID)) == 0) {
		return E_NOINTERFACE;
	}
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Direct3DRMFrameImpl does not implement guid");
	return E_NOINTERFACE;
}

HRESULT Direct3DRMFrameImpl::AddChild(IDirect3DRMFrame* child)
{
	Direct3DRMFrameImpl* childImpl = static_cast<Direct3DRMFrameImpl*>(child);
	if (childImpl->m_parent) {
		if (childImpl->m_parent == this) {
			return DD_OK;
		}
		auto result = childImpl->m_parent->m_children->DeleteElement(childImpl);
		SDL_assert(result == DD_OK);
	}
	childImpl->m_parent = this;
	return m_children->AddElement(child);
}

HRESULT Direct3DRMFrameImpl::DeleteChild(IDirect3DRMFrame* child)
{
	Direct3DRMFrameImpl* childImpl = static_cast<Direct3DRMFrameImpl*>(child);
	HRESULT result = m_children->DeleteElement(childImpl);
	if (result == DD_OK) {
		childImpl->m_parent = nullptr;
	}
	return result;
}

HRESULT Direct3DRMFrameImpl::SetSceneBackgroundRGB(float r, float g, float b)
{
	m_backgroundColor = (0xFF << 24) | (static_cast<BYTE>(r * 255.0f) << 16) | (static_cast<BYTE>(g * 255.0f) << 8) |
						(static_cast<BYTE>(b * 255.0f));
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddLight(IDirect3DRMLight* light)
{
	return m_lights->AddElement(light);
}

HRESULT Direct3DRMFrameImpl::GetLights(IDirect3DRMLightArray** lightArray)
{
	*lightArray = m_lights;
	m_lights->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddTransform(D3DRMCOMBINETYPE combine, D3DRMMATRIX4D matrix)
{
	D3DRMMATRIX4D result;
	switch (combine) {
	case D3DRMCOMBINETYPE::REPLACE:
		std::memcpy(m_transform, matrix, sizeof(m_transform));
		return DD_OK;
	case D3DRMCOMBINETYPE::BEFORE:
		MatrixMultiply(result, matrix, m_transform);
		break;
	case D3DRMCOMBINETYPE::AFTER:
		MatrixMultiply(result, m_transform, matrix);
		break;
	default:
		return DDERR_INVALIDPARAMS;
	}
	std::memcpy(m_transform, result, sizeof(m_transform));
	return DD_OK;
}

void Direct3DRMFrameImpl::GetWorldTransform(D3DRMMATRIX4D out)
{
	std::memcpy(out, m_transform, sizeof(m_transform));
	for (Direct3DRMFrameImpl* frame = m_parent; frame; frame = frame->m_parent) {
		D3DRMMATRIX4D tmp;
		MatrixMultiply(tmp, out, frame->m_transform);
		std::memcpy(out, tmp, sizeof(tmp));
	}
}

void Direct3DRMFrameImpl::GetTransformTo(IDirect3DRMFrame* reference, D3DRMMATRIX4D out)
{
	GetWorldTransform(out);
	if (reference) {
		D3DRMMATRIX4D referenceWorld, worldToReference, tmp;
		static_cast<Direct3DRMFrameImpl*>(reference)->GetWorldTransform(referenceWorld);
		InvertAffineMatrix(worldToReference, referenceWorld);
		MatrixMultiply(tmp, out, worldToReference);
		std::memcpy(out, tmp, sizeof(tmp));
	}
}

void Direct3DRMFrameImpl::SetTransformFrom(IDirect3DRMFrame* reference, const D3DRMMATRIX4D toReference)
{
	// local = toReference * referenceWorld * inverse(parentWorld)
	D3DRMMATRIX4D world, tmp;
	if (reference) {
		D3DRMMATRIX4D referenceWorld;
		static_cast<Direct3DRMFrameImpl*>(reference)->GetWorldTransform(referenceWorld);
		MatrixMultiply(world, toReference, referenceWorld);
	}
	else {
		std::memcpy(world, toReference, sizeof(world));
	}
	if (m_parent) {
		D3DRMMATRIX4D parentWorld, worldToParent;
		m_parent->GetWorldTransform(parentWorld);
		InvertAffineMatrix(worldToParent, parentWorld);
		MatrixMultiply(tmp, world, worldToParent);
		std::memcpy(m_transform, tmp, sizeof(tmp));
	}
	else {
		std::memcpy(m_transform, world, sizeof(world));
	}
}

HRESULT Direct3DRMFrameImpl::GetPosition(IDirect3DRMFrame* reference, D3DVECTOR* position)
{
	if (!reference) {
		// Relative to the parent, as miniwin has always reported it.
		position->x = m_transform[3][0] / m_transform[3][3];
		position->y = m_transform[3][1] / m_transform[3][3];
		position->z = m_transform[3][2] / m_transform[3][3];
		return DD_OK;
	}
	D3DRMMATRIX4D toReference;
	GetTransformTo(reference, toReference);
	position->x = toReference[3][0];
	position->y = toReference[3][1];
	position->z = toReference[3][2];
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddVisual(IDirect3DRMVisual* visual)
{
	return m_visuals->AddElement(visual);
}

HRESULT Direct3DRMFrameImpl::DeleteVisual(IDirect3DRMVisual* visual)
{
	return m_visuals->DeleteElement(visual);
}

HRESULT Direct3DRMFrameImpl::GetVisuals(IDirect3DRMVisualArray** visuals)
{
	*visuals = m_visuals;
	m_visuals->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetTexture(IDirect3DRMTexture* texture)
{
	auto textureImpl = static_cast<Direct3DRMTextureImpl*>(texture);
	if (m_texture) {
		m_texture->Release();
	}

	m_texture = textureImpl;
	m_texture->AddRef();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetTexture(IDirect3DRMTexture** texture)
{
	if (!m_texture) {
		return DDERR_GENERIC;
	}

	*texture = m_texture;
	if (m_texture) {
		m_texture->AddRef();
	}
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetColor(float r, float g, float b, float a)
{
	m_color = (static_cast<BYTE>(a * 255.0f) << 24) | (static_cast<BYTE>(r * 255.0f) << 16) |
			  (static_cast<BYTE>(g * 255.0f) << 8) | (static_cast<BYTE>(b * 255.0f));
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetColor(D3DCOLOR c)
{
	m_color = c;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetColorRGB(float r, float g, float b)
{
	return SetColor(r, g, b, 1.f);
}

HRESULT Direct3DRMFrameImpl::SetMaterialMode(D3DRMMATERIALMODE mode)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetChildren(IDirect3DRMFrameArray** children)
{
	*children = m_children;
	m_children->AddRef();
	return DD_OK;
}

// IDirect3DRMFrame3 interface
HRESULT Direct3DRMFrameImpl::AddChild(IDirect3DRMFrame3* child)
{
	return AddChild(static_cast<IDirect3DRMFrame*>(child));
}

HRESULT Direct3DRMFrameImpl::DeleteChild(IDirect3DRMFrame3* child)
{
	return DeleteChild(static_cast<IDirect3DRMFrame*>(child));
}

// The IDirect3DRMFrame3 entry points take a null reference as the scene
// (world space).
HRESULT Direct3DRMFrameImpl::GetPosition(IDirect3DRMFrame3* reference, D3DVECTOR* position)
{
	D3DRMMATRIX4D toReference;
	GetTransformTo(static_cast<IDirect3DRMFrame*>(reference), toReference);
	position->x = toReference[3][0];
	position->y = toReference[3][1];
	position->z = toReference[3][2];
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::AddTranslation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
	D3DRMMATRIX4D matrix;
	IdentityMatrix(matrix);
	matrix[3][0] = x;
	matrix[3][1] = y;
	matrix[3][2] = z;
	return AddTransform(combine, matrix);
}

HRESULT Direct3DRMFrameImpl::AddScale(D3DRMCOMBINETYPE combine, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
	D3DRMMATRIX4D matrix;
	IdentityMatrix(matrix);
	matrix[0][0] = sx;
	matrix[1][1] = sy;
	matrix[2][2] = sz;
	return AddTransform(combine, matrix);
}

HRESULT Direct3DRMFrameImpl::AddRotation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta)
{
	D3DVECTOR axis = Normalize({x, y, z});
	float s = sinf(theta);
	float c = cosf(theta);
	float t = 1.0f - c;
	D3DRMMATRIX4D matrix;
	IdentityMatrix(matrix);
	matrix[0][0] = t * axis.x * axis.x + c;
	matrix[0][1] = t * axis.x * axis.y + s * axis.z;
	matrix[0][2] = t * axis.x * axis.z - s * axis.y;
	matrix[1][0] = t * axis.y * axis.x - s * axis.z;
	matrix[1][1] = t * axis.y * axis.y + c;
	matrix[1][2] = t * axis.y * axis.z + s * axis.x;
	matrix[2][0] = t * axis.z * axis.x + s * axis.y;
	matrix[2][1] = t * axis.z * axis.y - s * axis.x;
	matrix[2][2] = t * axis.z * axis.z + c;
	return AddTransform(combine, matrix);
}

HRESULT Direct3DRMFrameImpl::AddVisual(IUnknown* visual)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::GetParent(IDirect3DRMFrame3** parent)
{
	if (!parent) {
		return DDERR_INVALIDPARAMS;
	}
	*parent = m_parent;
	if (m_parent) {
		m_parent->AddRef();
	}
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetScene(IDirect3DRMFrame3** scene)
{
	if (!scene) {
		return DDERR_INVALIDPARAMS;
	}
	Direct3DRMFrameImpl* root = this;
	while (root->m_parent) {
		root = root->m_parent;
	}
	root->AddRef();
	*scene = root;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetTransform(IDirect3DRMFrame3* reference, D3DRMMATRIX4D matrix)
{
	GetTransformTo(static_cast<IDirect3DRMFrame*>(reference), matrix);
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetOrientation(IDirect3DRMFrame3* reference, D3DVECTOR* dir, D3DVECTOR* up)
{
	D3DRMMATRIX4D toReference;
	GetTransformTo(static_cast<IDirect3DRMFrame*>(reference), toReference);
	if (dir) {
		*dir = Normalize({toReference[2][0], toReference[2][1], toReference[2][2]});
	}
	if (up) {
		*up = Normalize({toReference[1][0], toReference[1][1], toReference[1][2]});
	}
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::GetVisuals(DWORD* count, IUnknown** visuals)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::InverseTransform(D3DVECTOR* d, D3DVECTOR* s)
{
	D3DRMMATRIX4D world, inverse;
	GetWorldTransform(world);
	InvertAffineMatrix(inverse, world);
	*d = TransformPoint(*s, inverse);
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::Load(
	void* filename,
	void* name,
	D3DRMLOADOPTIONS flags,
	D3DRMLOADTEXTURE3CALLBACK callback,
	void* arg
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::LookAt(
	IDirect3DRMFrame3* target,
	IDirect3DRMFrame3* reference,
	D3DRMFRAMECONSTRAINT constraint
)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

HRESULT Direct3DRMFrameImpl::DeleteVisual(IUnknown* visual)
{
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOTIMPL;
}

D3DCOLOR Direct3DRMFrameImpl::GetSceneBackground()
{
	return m_backgroundColor;
}

HRESULT Direct3DRMFrameImpl::SetSceneBackground(D3DCOLOR color)
{
	m_backgroundColor = color;
	return DD_OK;
}

// The fog state is kept for the frame's getters; the renderers do not draw
// fog yet.
HRESULT Direct3DRMFrameImpl::SetSceneFogEnable(BOOL enable)
{
	m_fogEnable = enable;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogColor(D3DCOLOR color)
{
	m_fogColor = color;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogMode(D3DRMFOGMODE mode)
{
	m_fogMode = mode;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogParams(D3DVALUE start, D3DVALUE end, D3DVALUE density)
{
	m_fogStart = start;
	m_fogEnd = end;
	m_fogDensity = density;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetOrientation(
	IDirect3DRMFrame3* reference,
	D3DVALUE dx,
	D3DVALUE dy,
	D3DVALUE dz,
	D3DVALUE ux,
	D3DVALUE uy,
	D3DVALUE uz
)
{
	// The new axes, in the reference frame: z along dir, y as close to up as
	// is perpendicular to it, x = y x z (left-handed).
	D3DVECTOR dir = Normalize({dx, dy, dz});
	D3DVECTOR right = Normalize(CrossProduct({ux, uy, uz}, dir));
	D3DVECTOR up = CrossProduct(dir, right);
	if (DotProduct(dir, dir) == 0.0f || DotProduct(right, right) == 0.0f) {
		return DDERR_INVALIDPARAMS;
	}

	D3DRMMATRIX4D toReference;
	GetTransformTo(static_cast<IDirect3DRMFrame*>(reference), toReference);
	D3DVECTOR axes[3] = {right, up, dir};
	for (int row = 0; row < 3; ++row) {
		toReference[row][0] = axes[row].x;
		toReference[row][1] = axes[row].y;
		toReference[row][2] = axes[row].z;
	}
	SetTransformFrom(static_cast<IDirect3DRMFrame*>(reference), toReference);
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetPosition(IDirect3DRMFrame3* reference, D3DVALUE x, D3DVALUE y, D3DVALUE z)
{
	D3DRMMATRIX4D toReference;
	GetTransformTo(static_cast<IDirect3DRMFrame*>(reference), toReference);
	toReference[3][0] = x;
	toReference[3][1] = y;
	toReference[3][2] = z;
	SetTransformFrom(static_cast<IDirect3DRMFrame*>(reference), toReference);
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetSortMode(D3DRMSORTMODE mode)
{
	m_sortMode = mode;
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::Transform(D3DVECTOR* d, D3DVECTOR* s)
{
	D3DRMMATRIX4D world;
	GetWorldTransform(world);
	*d = TransformPoint(*s, world);
	return DD_OK;
}

HRESULT Direct3DRMFrameImpl::SetSceneFogMethod(DWORD flags)
{
	m_fogMethod = flags;
	return DD_OK;
}
