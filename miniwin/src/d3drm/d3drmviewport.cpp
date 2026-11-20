#include "d3drm_impl.h"
#include "d3drmframe_impl.h"
#include "d3drmlight_impl.h"
#include "d3drmmesh_impl.h"
#include "d3drmrenderer.h"
#include "d3drmuservisual_impl.h"
#include "d3drmviewport_impl.h"
#include "d3dviewport_impl.h"
#include "ddraw_impl.h"
#include "mathutils.h"
#include "miniwin.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <cassert>
#include <float.h>
#include <functional>
#include <math.h>

Direct3DRMViewportImpl::Direct3DRMViewportImpl(
	DWORD width,
	DWORD height,
	Direct3DRMRenderer* renderer,
	IDirect3DRMDevice* device,
	LONG x,
	LONG y
)
	: m_virtualWidth(width), m_virtualHeight(height), m_renderer(renderer), m_device(device), m_x(x), m_y(y)
{
	if (m_renderer) {
		UpdateProjectionMatrix();
	}
}

Direct3DRMViewportImpl::~Direct3DRMViewportImpl()
{
	if (m_camera) {
		m_camera->Release();
		m_camera = nullptr;
	}
	if (m_d3dViewport) {
		m_d3dViewport->m_owner = nullptr;
		m_d3dViewport->Release();
		m_d3dViewport = nullptr;
	}
}

HRESULT Direct3DRMViewportImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirect3DRMViewport, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMViewport*>(this);
		return DD_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DRMViewport2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DRMViewport2*>(this);
		return DD_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

static void D3DRMMatrixMultiply(D3DRMMATRIX4D out, const D3DRMMATRIX4D a, const D3DRMMATRIX4D b)
{
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			out[i][j] = 0.0f;
			for (int k = 0; k < 4; ++k) {
				out[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

static void D3DRMMatrixInvertOrthogonal(D3DRMMATRIX4D out, const D3DRMMATRIX4D m)
{
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			out[i][j] = m[j][i];
		}
	}

	out[0][3] = out[1][3] = out[2][3] = 0.f;
	out[3][3] = 1.f;

	D3DVECTOR t = {m[3][0], m[3][1], m[3][2]};

	out[3][0] = -(out[0][0] * t.x + out[1][0] * t.y + out[2][0] * t.z);
	out[3][1] = -(out[0][1] * t.x + out[1][1] * t.y + out[2][1] * t.z);
	out[3][2] = -(out[0][2] * t.x + out[1][2] * t.y + out[2][2] * t.z);
}

static void ComputeFrameWorldMatrix(IDirect3DRMFrame* frame, D3DRMMATRIX4D out)
{
	D3DRMMATRIX4D acc = {{1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}};

	IDirect3DRMFrame* cur = frame;
	while (cur) {
		auto* impl = static_cast<Direct3DRMFrameImpl*>(cur);
		D3DRMMATRIX4D tmp;
		D3DRMMatrixMultiply(tmp, acc, impl->m_transform);
		memcpy(acc, tmp, sizeof(acc));

		if (cur == impl->m_parent) {
			break;
		}
		cur = impl->m_parent;
	}
	memcpy(out, acc, sizeof(acc));
}

void Direct3DRMViewportImpl::CollectLightsFromFrame(IDirect3DRMFrame* frame, D3DRMMATRIX4D parentToWorld)
{
	auto* frameImpl = static_cast<Direct3DRMFrameImpl*>(frame);
	// Row vectors, as D3DRM: a point goes through the frame's own transform
	// first, then its parent's.
	D3DRMMATRIX4D worldMatrix;
	D3DRMMatrixMultiply(worldMatrix, frameImpl->m_transform, parentToWorld);

	IDirect3DRMLightArray* lightArray = nullptr;
	frame->GetLights(&lightArray);
	DWORD lightCount = lightArray->GetSize();
	for (DWORD li = 0; li < lightCount; ++li) {
		IDirect3DRMLight* light = nullptr;
		lightArray->GetElement(li, &light);
		auto* lightImpl = static_cast<Direct3DRMLightImpl*>(light);
		SceneLight extracted;
		extracted.color = lightImpl->GetColorValue();
		extracted.range = light->GetRange();
		extracted.attenuation[0] = light->GetConstantAttenuation();
		extracted.attenuation[1] = light->GetLinearAttenuation();
		extracted.attenuation[2] = light->GetQuadraticAttenuation();
		extracted.umbra = light->GetUmbra();
		extracted.penumbra = light->GetPenumbra();

		D3DRMLIGHTTYPE type = light->GetType();
		if (type == D3DRMLIGHT_POINT || type == D3DRMLIGHT_SPOT || type == D3DRMLIGHT_PARALLELPOINT) {
			extracted.position = {worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2]};
			extracted.positional = 1.f;
		}
		if (type == D3DRMLIGHT_DIRECTIONAL || type == D3DRMLIGHT_SPOT) {
			extracted.direction = {worldMatrix[2][0], worldMatrix[2][1], worldMatrix[2][2]};
			extracted.directional = 1.f;
		}

		m_lights.push_back(extracted);
		m_lightEnableFrames.push_back(lightImpl->PeekEnableFrame());
		light->Release();
	}
	lightArray->Release();

	IDirect3DRMFrameArray* children = nullptr;
	frame->GetChildren(&children);
	DWORD n = children->GetSize();
	for (DWORD i = 0; i < n; ++i) {
		IDirect3DRMFrame* childFrame = nullptr;
		children->GetElement(i, &childFrame);
		CollectLightsFromFrame(childFrame, worldMatrix);
		childFrame->Release();
	}
	children->Release();
}

static DWORD FloatBits(float value)
{
	DWORD bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

void Direct3DRMViewportImpl::ApplySceneFog()
{
	auto* root = static_cast<Direct3DRMFrameImpl*>(m_rootFrame);
	m_renderer->SetRenderState(D3DRENDERSTATE_FOGENABLE, root->m_fogEnable ? TRUE : FALSE);
	if (!root->m_fogEnable) {
		return;
	}
	D3DFOGMODE mode = D3DFOG_LINEAR;
	if (root->m_fogMode == D3DRMFOG_EXPONENTIAL) {
		mode = D3DFOG_EXP;
	}
	else if (root->m_fogMode == D3DRMFOG_EXPONENTIALSQUARED) {
		mode = D3DFOG_EXP2;
	}
	bool vertex = (root->m_fogMethod & (D3DRMFOGMETHOD_TABLE | D3DRMFOGMETHOD_ANY)) == 0 &&
				  (root->m_fogMethod & D3DRMFOGMETHOD_VERTEX) == D3DRMFOGMETHOD_VERTEX;
	m_renderer->SetRenderState(D3DRENDERSTATE_FOGCOLOR, root->m_fogColor);
	m_renderer->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, static_cast<DWORD>(vertex ? D3DFOG_NONE : mode));
	m_renderer->SetLightState(D3DLIGHTSTATE_FOGMODE, static_cast<DWORD>(vertex ? mode : D3DFOG_NONE));
	if (vertex) {
		m_renderer->SetLightState(D3DLIGHTSTATE_FOGSTART, FloatBits(root->m_fogStart));
		m_renderer->SetLightState(D3DLIGHTSTATE_FOGEND, FloatBits(root->m_fogEnd));
		m_renderer->SetLightState(D3DLIGHTSTATE_FOGDENSITY, FloatBits(root->m_fogDensity));
	}
	else {
		m_renderer->SetRenderState(D3DRENDERSTATE_FOGTABLESTART, FloatBits(root->m_fogStart));
		m_renderer->SetRenderState(D3DRENDERSTATE_FOGTABLEEND, FloatBits(root->m_fogEnd));
		m_renderer->SetRenderState(D3DRENDERSTATE_FOGTABLEDENSITY, FloatBits(root->m_fogDensity));
	}
}

void Direct3DRMViewportImpl::BuildViewFrustumPlanes()
{

	// The half-extents of the view at z = 1, as the projection maps them.
	float tanFovX = 1.0f / m_projectionMatrix[0][0];
	float tanFovY = 1.0f / m_projectionMatrix[1][1];

	// View-space frustum planes
	m_frustumPlanes[0] = {Normalize({tanFovX, 0.0f, 1.0f}), 0.0f};  // Left
	m_frustumPlanes[1] = {Normalize({-tanFovX, 0.0f, 1.0f}), 0.0f}; // Right
	m_frustumPlanes[2] = {Normalize({0.0f, -tanFovY, 1.0f}), 0.0f}; // Top
	m_frustumPlanes[3] = {Normalize({0.0f, tanFovY, 1.0f}), 0.0f};  // Bottom

	// Near and far planes
	m_frustumPlanes[4] = {{0.0f, 0.0f, 1.0f}, -m_front}; // Near (Z >= m_front)
	m_frustumPlanes[5] = {{0.0f, 0.0f, -1.0f}, m_back};  // Far  (Z <= m_back)
}

bool IsMeshInFrustum(Direct3DRMMeshImpl* mesh, const D3DRMMATRIX4D& worldViewMatrix, const Plane* frustumPlanes)
{
	D3DRMBOX box;
	mesh->GetBox(&box);

	D3DVECTOR boxCorners[8] = {
		{box.min.x, box.min.y, box.min.z},
		{box.min.x, box.min.y, box.max.z},
		{box.min.x, box.max.y, box.min.z},
		{box.min.x, box.max.y, box.max.z},
		{box.max.x, box.min.y, box.min.z},
		{box.max.x, box.min.y, box.max.z},
		{box.max.x, box.max.y, box.min.z},
		{box.max.x, box.max.y, box.max.z},
	};

	for (D3DVECTOR& corner : boxCorners) {
		corner = TransformPoint(corner, worldViewMatrix);
	}

	for (int i = 0; i < 6; ++i) {
		const Plane& plane = frustumPlanes[i];
		int out = 0;
		for (int j = 0; j < 8; ++j) {
			const D3DVECTOR& corner = boxCorners[j];
			float dist = plane.normal.x * corner.x + plane.normal.y * corner.y + plane.normal.z * corner.z + plane.d;
			if (dist < 0.0f) {
				++out;
			}
		}
		if (out == 8) {
			return false;
		}
	}
	return true;
}

inline D3DRMVECTOR4D TransformPoint4(const D3DRMVECTOR4D& p, const D3DRMMATRIX4D& m)
{
	return {
		p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + p.w * m[3][0],
		p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + p.w * m[3][1],
		p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + p.w * m[3][2],
		p.x * m[0][3] + p.y * m[1][3] + p.z * m[2][3] + p.w * m[3][3]
	};
}

float CalculateDepth(const D3DRMMATRIX4D& viewProj, const D3DRMMATRIX4D& worldMatrix)
{
	D3DRMVECTOR4D position = {worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2], 1.0f};
	D3DRMVECTOR4D clipPos = TransformPoint4(position, viewProj);
	return (clipPos.z / clipPos.w + 1.0f) * 0.5f;
}

void Direct3DRMViewportImpl::UseLights(uint64_t mask)
{
	if (mask == m_pushedLightMask) {
		return;
	}
	m_activeLights.clear();
	for (size_t i = 0; i < m_lights.size(); ++i) {
		if (i >= 64 || (mask >> i) & 1) {
			m_activeLights.push_back(m_lights[i]);
		}
	}
	m_renderer->PushLights(m_activeLights.data(), m_activeLights.size());
	m_pushedLightMask = mask;
}

void Direct3DRMViewportImpl::CollectMeshesFromFrame(
	IDirect3DRMFrame* frame,
	D3DRMMATRIX4D parentMatrix,
	uint64_t lightMask
)
{
	Direct3DRMFrameImpl* frameImpl = static_cast<Direct3DRMFrameImpl*>(frame);
	// A light with an enable frame lights that frame's hierarchy only.
	for (size_t i = 0; i < m_lightEnableFrames.size() && i < 64; ++i) {
		if (m_lightEnableFrames[i] && static_cast<Direct3DRMFrameImpl*>(m_lightEnableFrames[i]) == frameImpl) {
			lightMask |= uint64_t(1) << i;
		}
	}
	D3DRMMATRIX4D localMatrix;
	memcpy(localMatrix, frameImpl->m_transform, sizeof(D3DRMMATRIX4D));

	D3DRMMATRIX4D worldMatrix;
	D3DRMMatrixMultiply(worldMatrix, localMatrix, parentMatrix);

	Matrix3x3 worldMatrixInvert;
	NormalMatrix(worldMatrixInvert, worldMatrix);

	IDirect3DRMVisualArray* visuals = nullptr;
	frame->GetVisuals(&visuals);
	DWORD n = visuals->GetSize();
	for (DWORD i = 0; i < n; ++i) {
		IDirect3DRMVisual* visual = nullptr;
		visuals->GetElement(i, &visual);

		IDirect3DRMFrame* childFrame = nullptr;
		visual->QueryInterface(IID_IDirect3DRMFrame, (void**) &childFrame);
		if (childFrame) {
			CollectMeshesFromFrame(childFrame, worldMatrix, lightMask);
			childFrame->Release();
			visual->Release();
			continue;
		}

		Direct3DRMMeshImpl* mesh = nullptr;
		visual->QueryInterface(IID_IDirect3DRMMesh, (void**) &mesh);
		if (mesh) {
			D3DRMMATRIX4D modelViewMatrix;
			MultiplyMatrix(modelViewMatrix, worldMatrix, m_viewMatrix);
			if (IsMeshInFrustum(mesh, modelViewMatrix, m_frustumPlanes)) {
				DWORD groupCount = mesh->GetGroupCount();
				for (DWORD gi = 0; gi < groupCount; ++gi) {
					const MeshGroup& meshGroup = mesh->GetGroup(gi);

					Appearance appearance = {
						meshGroup.color,
						meshGroup.material ? meshGroup.material->GetPower() : 0.0f,
						meshGroup.texture ? m_renderer->GetTextureId(meshGroup.texture) : NO_TEXTURE_ID,
						meshGroup.quality == D3DRMRENDER_FLAT || meshGroup.quality == D3DRMRENDER_UNLITFLAT
					};

					if (appearance.color.a != 255) {
						m_deferredDraws.push_back(
							{m_renderer->GetMeshId(mesh, &meshGroup),
							 {},
							 {},
							 {},
							 appearance,
							 CalculateDepth(m_viewProjectionwMatrix, worldMatrix),
							 lightMask}
						);
						memcpy(m_deferredDraws.back().modelViewMatrix, modelViewMatrix, sizeof(D3DRMMATRIX4D));
						memcpy(m_deferredDraws.back().worldMatrix, worldMatrix, sizeof(D3DRMMATRIX4D));
						memcpy(m_deferredDraws.back().normalMatrix, worldMatrixInvert, sizeof(Matrix3x3));
					}
					else {
						UseLights(lightMask);
						m_renderer->SubmitDraw(
							m_renderer->GetMeshId(mesh, &meshGroup),
							modelViewMatrix,
							worldMatrix,
							m_viewMatrix,
							worldMatrixInvert,
							appearance
						);
					}
				}
			}
			mesh->Release();
			visual->Release();
			continue;
		}

		IDirect3DRMUserVisual* userVisual = nullptr;
		visual->QueryInterface(IID_IDirect3DRMUserVisual, (void**) &userVisual);
		if (userVisual) {
			UseLights(lightMask);
			RenderUserVisual(static_cast<Direct3DRMUserVisualImpl*>(userVisual), worldMatrix);
			userVisual->Release();
		}
		visual->Release();
	}
	visuals->Release();

	// Child frames (AddChild, or CreateFrame with a parent) render with the
	// frame, as D3DRM does; a frame used as a visual was walked above.
	IDirect3DRMFrameArray* children = nullptr;
	frame->GetChildren(&children);
	DWORD childCount = children->GetSize();
	for (DWORD i = 0; i < childCount; ++i) {
		IDirect3DRMFrame* childFrame = nullptr;
		children->GetElement(i, &childFrame);
		CollectMeshesFromFrame(childFrame, worldMatrix, lightMask);
		childFrame->Release();
	}
	children->Release();
}

// A user visual draws itself on the immediate-mode device. As D3DRM does,
// the device's world, view and projection transforms and its current
// viewport are set before the callback is asked whether the visual can be
// seen (CANSEE) and then to draw it (RENDER).
void Direct3DRMViewportImpl::RenderUserVisual(Direct3DRMUserVisualImpl* visual, const D3DRMMATRIX4D worldMatrix)
{
	m_renderer->SetTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX) worldMatrix);
	m_renderer->SetTransform(D3DTRANSFORMSTATE_VIEW, (LPD3DMATRIX) m_viewMatrix);
	m_renderer->SetTransform(D3DTRANSFORMSTATE_PROJECTION, (LPD3DMATRIX) m_projectionMatrix);

	IDirect3DViewport* d3dViewport = nullptr;
	IDirect3DViewport3* d3dViewport3 = nullptr;
	if (GetDirect3DViewport(&d3dViewport) == DD_OK) {
		d3dViewport->QueryInterface(IID_IDirect3DViewport3, (void**) &d3dViewport3);
		d3dViewport->Release();
	}
	m_renderer->SetCurrentViewport(d3dViewport3);
	if (d3dViewport3) {
		d3dViewport3->Release();
	}

	IDirect3DRMViewport* self = static_cast<IDirect3DRMViewport*>(this);
	if (visual->Invoke(D3DRMUSERVISUAL_CANSEE, m_device, self)) {
		visual->Invoke(D3DRMUSERVISUAL_RENDER, m_device, self);
	}
}

HRESULT Direct3DRMViewportImpl::RenderScene()
{
	m_backgroundColor = static_cast<Direct3DRMFrameImpl*>(m_rootFrame)->m_backgroundColor;

	// Compute view-projection matrix
	D3DRMMATRIX4D cameraWorld;
	ComputeFrameWorldMatrix(m_camera, cameraWorld);
	D3DRMMatrixInvertOrthogonal(m_viewMatrix, cameraWorld);
	D3DRMMatrixMultiply(m_viewProjectionwMatrix, m_viewMatrix, m_projectionMatrix);

	D3DRMMATRIX4D identity = {{1.f, 0.f, 0.f, 0.f}, {0.f, 1.f, 0.f, 0.f}, {0.f, 0.f, 1.f, 0.f}, {0.f, 0.f, 0.f, 1.f}};

	ApplyToRenderer();

	m_lights.clear();
	m_lightEnableFrames.clear();
	CollectLightsFromFrame(m_rootFrame, identity);
	uint64_t sceneMask = 0;
	for (size_t i = 0; i < m_lightEnableFrames.size() && i < 64; ++i) {
		if (!m_lightEnableFrames[i]) {
			sceneMask |= uint64_t(1) << i;
		}
	}
	m_pushedLightMask = ~sceneMask; // force the first push
	UseLights(sceneMask);
	HRESULT status = m_renderer->BeginFrame();
	if (status != DD_OK) {
		return status;
	}
	ApplySceneFog();

	CollectMeshesFromFrame(m_rootFrame, identity, sceneMask);

	std::sort(
		m_deferredDraws.begin(),
		m_deferredDraws.end(),
		[](const DeferredDrawCommand& a, const DeferredDrawCommand& b) { return a.depth > b.depth; }
	);
	m_renderer->EnableTransparency();
	for (const DeferredDrawCommand& cmd : m_deferredDraws) {
		UseLights(cmd.lightMask);
		m_renderer->SubmitDraw(
			cmd.meshId,
			cmd.modelViewMatrix,
			cmd.worldMatrix,
			m_viewMatrix,
			cmd.normalMatrix,
			cmd.appearance
		);
	}
	m_deferredDraws.clear();

	return m_renderer->FinalizeFrame();
}

HRESULT Direct3DRMViewportImpl::Render(IDirect3DRMFrame* rootFrame)
{
	if (!m_renderer) {
		return DDERR_GENERIC;
	}
	m_rootFrame = rootFrame;
	return RenderScene();
}

HRESULT Direct3DRMViewportImpl::ForceUpdate(int x, int y, int w, int h)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::Clear()
{
	return Clear(D3DRMCLEAR_ALL);
}

void Direct3DRMViewportImpl::ApplyToRenderer()
{
	if (!m_renderer) {
		return;
	}
	bool whole = m_x == 0 && m_y == 0 && (int) m_virtualWidth == m_renderer->GetVirtualWidth() &&
				 (int) m_virtualHeight == m_renderer->GetVirtualHeight();
	D3DRECT rect = {m_x, m_y, m_x + (LONG) m_virtualWidth, m_y + (LONG) m_virtualHeight};
	m_renderer->SetViewportRect(whole ? nullptr : &rect);
	m_renderer->SetProjection(m_projectionMatrix, m_front, m_back);
	BuildViewFrustumPlanes();
	m_renderer->SetFrustumPlanes(m_frustumPlanes);
}

HRESULT Direct3DRMViewportImpl::ClearRect(const D3DRECT* rect, DWORD flags, D3DCOLOR color, float z)
{
	if (!m_renderer) {
		return DDERR_GENERIC;
	}
	D3DRECT area = {m_x, m_y, m_x + (LONG) m_virtualWidth, m_y + (LONG) m_virtualHeight};
	if (rect) {
		area.x1 = std::max(area.x1, rect->x1);
		area.y1 = std::max(area.y1, rect->y1);
		area.x2 = std::min(area.x2, rect->x2);
		area.y2 = std::min(area.y2, rect->y2);
	}
	if (area.x1 >= area.x2 || area.y1 >= area.y2) {
		return DD_OK;
	}
	return m_renderer->ClearRect(area, flags, color, z);
}

HRESULT Direct3DRMViewportImpl::SetCamera(IDirect3DRMFrame* camera)
{
	if (camera) {
		camera->AddRef();
	}
	if (m_camera) {
		m_camera->Release();
	}
	m_camera = camera;
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::GetCamera(IDirect3DRMFrame** camera)
{
	if (m_camera) {
		m_camera->AddRef();
	}
	*camera = m_camera;
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::SetProjection(D3DRMPROJECTIONTYPE type)
{
	return DD_OK;
}

D3DRMPROJECTIONTYPE Direct3DRMViewportImpl::GetProjection()
{
	return D3DRMPROJECTIONTYPE::PERSPECTIVE;
}

HRESULT Direct3DRMViewportImpl::SetFront(D3DVALUE z)
{
	m_front = z;
	UpdateProjectionMatrix();
	return DD_OK;
}

D3DVALUE Direct3DRMViewportImpl::GetFront()
{
	return m_front;
}

HRESULT Direct3DRMViewportImpl::SetBack(D3DVALUE z)
{
	m_back = z;
	UpdateProjectionMatrix();
	return DD_OK;
}

D3DVALUE Direct3DRMViewportImpl::GetBack()
{
	return m_back;
}

HRESULT Direct3DRMViewportImpl::SetField(D3DVALUE field)
{
	m_field = field;
	UpdateProjectionMatrix();
	return DD_OK;
}

void Direct3DRMViewportImpl::UpdateProjectionMatrix()
{
	float virtualAspect = (float) m_virtualWidth / (float) m_virtualHeight;
	float windowAspect = (float) m_renderer->GetWidth() / (float) m_renderer->GetHeight();
	// A viewport smaller than the device is drawn into its own rectangle,
	// scaled with the 2D image, so it keeps its own aspect.
	bool whole = m_x == 0 && m_y == 0 && (int) m_virtualWidth == m_renderer->GetVirtualWidth() &&
				 (int) m_virtualHeight == m_renderer->GetVirtualHeight();
	if (!whole) {
		windowAspect = virtualAspect;
	}

	float base_f = m_front / m_field;
	float f_v = base_f * virtualAspect;
	float f_h = base_f;
	if (windowAspect >= virtualAspect) {
		f_h *= virtualAspect / windowAspect;
	}
	else {
		f_v *= windowAspect / virtualAspect;
	}

	float depth = m_back - m_front;

	D3DRMMATRIX4D projection = {
		{f_h, 0, 0, 0},
		{0, f_v, 0, 0},
		{0, 0, m_back / depth, 1},
		{0, 0, (-m_front * m_back) / depth, 0},
	};
	memcpy(m_projectionMatrix, projection, sizeof(D3DRMMATRIX4D));

	m_renderer->SetProjection(projection, m_front, m_back);
}

D3DVALUE Direct3DRMViewportImpl::GetField()
{
	return m_field;
}

inline float FromNDC(float ndcCoord, float dim)
{
	return (ndcCoord * 0.5f + 0.5f) * dim;
}

inline void MultiplyMatrixVec4(D3DRMVECTOR4D& out, const D3DRMMATRIX4D& mat, const D3DRMVECTOR4D& vec)
{
	out.x = mat[0][0] * vec.x + mat[1][0] * vec.y + mat[2][0] * vec.z + mat[3][0] * vec.w;
	out.y = mat[0][1] * vec.x + mat[1][1] * vec.y + mat[2][1] * vec.z + mat[3][1] * vec.w;
	out.z = mat[0][2] * vec.x + mat[1][2] * vec.y + mat[2][2] * vec.z + mat[3][2] * vec.w;
	out.w = mat[0][3] * vec.x + mat[1][3] * vec.y + mat[2][3] * vec.z + mat[3][3] * vec.w;
}

// D3DRM's screen vectors are homogeneous: x / w and y / w are device
// coordinates (the viewport's rectangle included) and z / w is the depth,
// 0 at the front clipping plane and 1 at the back one.
HRESULT Direct3DRMViewportImpl::Transform(D3DRMVECTOR4D* screen, D3DVECTOR* world)
{
	D3DRMVECTOR4D worldVec = {world->x, world->y, world->z, 1.0f};
	D3DRMVECTOR4D viewVec, projVec;

	MultiplyMatrixVec4(viewVec, m_viewMatrix, worldVec);
	MultiplyMatrixVec4(projVec, m_projectionMatrix, viewVec);

	float w = projVec.w;
	float invW = w != 0.0f ? 1.0f / w : 1.0f;
	float screenX = m_x + FromNDC(projVec.x * invW, m_virtualWidth);
	float screenY = m_y + FromNDC(-projVec.y * invW, m_virtualHeight); // y grows down the screen
	float depth = projVec.z * invW;

	*screen = {screenX * w, screenY * w, depth * w, w};
	return DD_OK;
}

HRESULT Direct3DRMViewportImpl::InverseTransform(D3DVECTOR* world, D3DRMVECTOR4D* screen)
{
	float w = screen->w != 0.0f ? screen->w : 1.0f;
	float screenX = screen->x / w;
	float screenY = screen->y / w;
	float depth = screen->z / w;

	float ndcX = (screenX - m_x) / m_virtualWidth * 2.0f - 1.0f;
	float ndcY = 1.0f - (screenY - m_y) / m_virtualHeight * 2.0f;

	// Undo the projection: depth = back / (back - front) * (1 - front / z).
	float viewZ = m_front * m_back / (m_back - depth * (m_back - m_front));
	D3DRMVECTOR4D viewVec = {
		ndcX * viewZ / m_projectionMatrix[0][0],
		ndcY * viewZ / m_projectionMatrix[1][1],
		viewZ,
		1.0f
	};

	D3DRMMATRIX4D inverseViewMatrix;
	D3DRMMatrixInvertOrthogonal(inverseViewMatrix, m_viewMatrix);

	D3DRMVECTOR4D worldVec;
	MultiplyMatrixVec4(worldVec, inverseViewMatrix, viewVec);
	*world = {worldVec.x, worldVec.y, worldVec.z};
	return DD_OK;
}

struct Ray {
	D3DVECTOR origin;
	D3DVECTOR direction;
};

// Ray-box intersection: slab method
bool RayIntersectsBox(const Ray& ray, const D3DRMBOX& box, float& outT)
{
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; ++i) {
		float origin = (&ray.origin.x)[i];
		float dir = (&ray.direction.x)[i];
		float minB = (&box.min.x)[i];
		float maxB = (&box.max.x)[i];

		if (fabs(dir) < 1e-6f) {
			if (origin < minB || origin > maxB) {
				return false;
			}
		}
		else {
			float invD = 1.0f / dir;
			float t1 = (minB - origin) * invD;
			float t2 = (maxB - origin) * invD;
			if (t1 > t2) {
				std::swap(t1, t2);
			}
			if (t1 > tmin) {
				tmin = t1;
			}
			if (t2 < tmax) {
				tmax = t2;
			}
			if (tmin > tmax) {
				return false;
			}
			if (tmax < 0) {
				return false;
			}
		}
	}

	outT = tmin >= 0 ? tmin : tmax; // closest positive hit
	return true;
}

// Convert screen (x,y) in viewport to picking ray in world space
Ray BuildPickingRay(
	float x,
	float y,
	int width,
	int height,
	IDirect3DRMFrame* camera,
	float front,
	float back,
	float field,
	float aspect
)
{
	float nx = (2.0f * x) / width - 1.0f;
	float ny = 1.0f - (2.0f * y) / height;

	float f = front / field;

	// Ray in view space at near plane:
	D3DVECTOR rayDirView = {nx / f, ny / (f * aspect), 1.0f};

	// Normalize ray direction
	float len = sqrt(DotProduct(rayDirView, rayDirView));
	rayDirView = Normalize(rayDirView);

	// Compute camera world matrix and invert it to get view->world
	D3DRMMATRIX4D cameraWorld;
	ComputeFrameWorldMatrix(camera, cameraWorld);

	// Transform ray origin (camera position) and direction to world space
	D3DVECTOR rayOriginWorld = {cameraWorld[3][0], cameraWorld[3][1], cameraWorld[3][2]};
	// Multiply direction by rotation part of matrix only (3x3 upper-left)
	D3DVECTOR rayDirWorld = {
		rayDirView.x * cameraWorld[0][0] + rayDirView.y * cameraWorld[1][0] + rayDirView.z * cameraWorld[2][0],
		rayDirView.x * cameraWorld[0][1] + rayDirView.y * cameraWorld[1][1] + rayDirView.z * cameraWorld[2][1],
		rayDirView.x * cameraWorld[0][2] + rayDirView.y * cameraWorld[1][2] + rayDirView.z * cameraWorld[2][2]
	};

	len = sqrt(rayDirWorld.x * rayDirWorld.x + rayDirWorld.y * rayDirWorld.y + rayDirWorld.z * rayDirWorld.z);
	rayDirWorld = Normalize(rayDirWorld);

	return Ray{rayOriginWorld, rayDirWorld};
}

bool RayIntersectsTriangle(
	const Ray& ray,
	const D3DVECTOR& v0,
	const D3DVECTOR& v1,
	const D3DVECTOR& v2,
	float& outDist
)
{
	const float EPSILON = 1e-6f;
	D3DVECTOR edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
	D3DVECTOR edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

	D3DVECTOR h = CrossProduct(ray.direction, edge2);
	float a = DotProduct(edge1, h);
	if (fabs(a) < EPSILON) {
		return false;
	}

	float f = 1.0f / a;
	D3DVECTOR s = {ray.origin.x - v0.x, ray.origin.y - v0.y, ray.origin.z - v0.z};
	float u = f * DotProduct(s, h);
	if (u < 0.0f || u > 1.0f) {
		return false;
	}

	D3DVECTOR q = CrossProduct(s, edge1);
	float v = f * DotProduct(ray.direction, q);
	if (v < 0.0f || u + v > 1.0f) {
		return false;
	}

	float t = f * DotProduct(edge2, q);
	if (t > EPSILON) {
		outDist = t;
		return true;
	}
	return false;
}

bool RayIntersectsMeshTriangles(
	const Ray& ray,
	Direct3DRMMeshImpl& mesh,
	const D3DRMMATRIX4D& worldMatrix,
	float& outDistance
)
{
	DWORD groupCount = mesh.GetGroupCount();
	for (DWORD gi = 0; gi < groupCount; ++gi) {
		const MeshGroup& meshGroup = mesh.GetGroup(gi);

		// Iterate over each face and do ray-triangle tests
		for (DWORD fi = 0; fi < meshGroup.indices.size(); fi += 3) {
			DWORD i0 = meshGroup.indices[fi + 0];
			DWORD i1 = meshGroup.indices[fi + 1];
			DWORD i2 = meshGroup.indices[fi + 2];

			// Transform vertices to world space
			D3DVECTOR tri[3];
			for (int j = 0; j < 3; ++j) {
				const D3DVECTOR& v = meshGroup.vertices[(j == 0 ? i0 : (j == 1 ? i1 : i2))].position;
				tri[j] = TransformPoint(v, worldMatrix);
			}

			float dist;
			if (RayIntersectsTriangle(ray, tri[0], tri[1], tri[2], dist)) {
				if (dist < outDistance) {
					outDistance = dist;
				}
				return true;
			}
		}
	}
	return false;
}

inline D3DVECTOR TransformVector(const D3DRMMATRIX4D& mat, const D3DVECTOR& vec)
{
	return {
		vec.x * mat[0][0] + vec.y * mat[1][0] + vec.z * mat[2][0] + mat[3][0],
		vec.x * mat[0][1] + vec.y * mat[1][1] + vec.z * mat[2][1] + mat[3][1],
		vec.x * mat[0][2] + vec.y * mat[1][2] + vec.z * mat[2][2] + mat[3][2]
	};
}

D3DRMBOX ComputeTransformedAABB(const D3DRMBOX& box, const D3DRMMATRIX4D& mat)
{
	D3DVECTOR corners[8] = {
		{box.min.x, box.min.y, box.min.z},
		{box.min.x, box.min.y, box.max.z},
		{box.min.x, box.max.y, box.min.z},
		{box.min.x, box.max.y, box.max.z},
		{box.max.x, box.min.y, box.min.z},
		{box.max.x, box.min.y, box.max.z},
		{box.max.x, box.max.y, box.min.z},
		{box.max.x, box.max.y, box.max.z}
	};

	D3DVECTOR transformed = TransformVector(mat, corners[0]);
	D3DRMBOX worldBox = {transformed, transformed};

	for (int i = 1; i < 8; ++i) {
		D3DVECTOR v = TransformVector(mat, corners[i]);
		worldBox.min.x = std::min(worldBox.min.x, v.x);
		worldBox.min.y = std::min(worldBox.min.y, v.y);
		worldBox.min.z = std::min(worldBox.min.z, v.z);
		worldBox.max.x = std::max(worldBox.max.x, v.x);
		worldBox.max.y = std::max(worldBox.max.y, v.y);
		worldBox.max.z = std::max(worldBox.max.z, v.z);
	}
	return worldBox;
}

HRESULT Direct3DRMViewportImpl::Pick(float x, float y, LPDIRECT3DRMPICKEDARRAY* pickedArray)
{
	if (!m_rootFrame) {
		return DDERR_GENERIC;
	}

	std::vector<PickRecord> hits;

	Ray pickRay = BuildPickingRay(
		x,
		y,
		m_virtualWidth,
		m_virtualHeight,
		m_camera,
		m_front,
		m_back,
		m_field,
		(float) m_virtualWidth / (float) m_virtualHeight
	);

	std::function<void(IDirect3DRMFrame*, std::vector<IDirect3DRMFrame*>&)> recurse;
	recurse = [&](IDirect3DRMFrame* frame, std::vector<IDirect3DRMFrame*>& path) {
		path.push_back(frame);

		IDirect3DRMVisualArray* visuals = nullptr;
		frame->GetVisuals(&visuals);
		DWORD count = visuals->GetSize();
		for (DWORD i = 0; i < count; ++i) {
			IDirect3DRMVisual* visual = nullptr;
			visuals->GetElement(i, &visual);

			IDirect3DRMFrame* subFrame = nullptr;
			visual->QueryInterface(IID_IDirect3DRMFrame, (void**) &subFrame);
			if (subFrame) {
				recurse(subFrame, path);
				subFrame->Release();
				visual->Release();
				continue;
			}

			Direct3DRMMeshImpl* mesh = nullptr;
			visual->QueryInterface(IID_IDirect3DRMMesh, (void**) &mesh);
			if (mesh) {
				D3DRMBOX box;
				mesh->GetBox(&box);
				// Transform box corners to world space
				D3DRMMATRIX4D worldMatrix;
				ComputeFrameWorldMatrix(frame, worldMatrix);
				D3DRMBOX worldBox = ComputeTransformedAABB(box, worldMatrix);

				float boxDist = FLT_MAX;

				// Check box first as an optimization
				if (RayIntersectsBox(pickRay, worldBox, boxDist)) {
					float meshDist = FLT_MAX;

					if (RayIntersectsMeshTriangles(pickRay, *mesh, worldMatrix, meshDist)) {
						auto* arr = new Direct3DRMFrameArrayImpl();
						for (IDirect3DRMFrame* f : path) {
							arr->AddElement(f);
						}

						PickRecord rec = {visual, arr, {meshDist}};
						hits.push_back(rec);
					}
				}
				mesh->Release();
			}
			visual->Release();
		}
		visuals->Release();
		path.pop_back(); // Pop after recursion
	};

	std::vector<IDirect3DRMFrame*> framePath;
	recurse(m_rootFrame, framePath);

	std::sort(hits.begin(), hits.end(), [](const PickRecord& a, const PickRecord& b) {
		return a.desc.dist < b.desc.dist;
	});

	*pickedArray = new Direct3DRMPickedArrayImpl(hits.data(), hits.size());
	for (PickRecord& hit : hits) {
		hit.frameArray->Release();
	}

	return D3DRM_OK;
}

void Direct3DRMViewportImpl::CloseDevice()
{
	m_renderer = nullptr;
}

// IDirect3DRMViewport2 interface
// Clear the viewport's rectangle: the target to the scene's background
// colour, the z-buffer to the far plane.
HRESULT Direct3DRMViewportImpl::Clear(DWORD flags)
{
	if (!m_renderer) {
		return DDERR_GENERIC;
	}
	if (m_camera) {
		auto* scene = static_cast<Direct3DRMFrameImpl*>(m_camera);
		while (scene->m_parent && scene->m_parent != scene) {
			scene = scene->m_parent;
		}
		m_backgroundColor = scene->m_backgroundColor;
	}
	DWORD d3dFlags = 0;
	if (flags & D3DRMCLEAR_TARGET) {
		d3dFlags |= D3DCLEAR_TARGET;
	}
	if (flags & D3DRMCLEAR_ZBUFFER) {
		d3dFlags |= D3DCLEAR_ZBUFFER;
	}
	return ClearRect(nullptr, d3dFlags, m_backgroundColor | 0xff000000, 1.0f);
}

HRESULT Direct3DRMViewportImpl::Render(IDirect3DRMFrame3* frame)
{
	return Render(static_cast<IDirect3DRMFrame*>(frame));
}

HRESULT Direct3DRMViewportImpl::SetCamera(IDirect3DRMFrame3* camera)
{
	return SetCamera(static_cast<IDirect3DRMFrame*>(camera));
}

HRESULT Direct3DRMViewportImpl::GetCamera(IDirect3DRMFrame3** camera)
{
	IDirect3DRMFrame* frame;
	HRESULT result = GetCamera(&frame);
	*camera = frame ? static_cast<Direct3DRMFrameImpl*>(frame) : nullptr;
	return result;
}

LONG Direct3DRMViewportImpl::GetX()
{
	return m_x;
}

LONG Direct3DRMViewportImpl::GetY()
{
	return m_y;
}

HRESULT Direct3DRMViewportImpl::GetDirect3DViewport(IDirect3DViewport** viewport)
{
	if (!viewport) {
		return DDERR_INVALIDPARAMS;
	}
	if (!m_d3dViewport) {
		m_d3dViewport = new Direct3DViewportImpl(this);
	}
	m_d3dViewport->AddRef();
	*viewport = static_cast<IDirect3DViewport*>(m_d3dViewport);
	return DD_OK;
}
