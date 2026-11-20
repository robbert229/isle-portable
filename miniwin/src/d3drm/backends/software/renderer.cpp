#include "d3drmrenderer.h"
#include "d3drmrenderer_software.h"
#include "ddsurface_impl.h"
#include "mathutils.h"
#include "meshutils.h"
#include "miniwin.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Perspective-correct UVs are computed exactly every PERSP_STEP pixels and
// interpolated affinely in between.  8 pixels keeps the error well below a
// texel at the resolutions this renderer runs at.
static constexpr int PERSP_STEP = 8;

// Reciprocals for perspective-block lengths 0..PERSP_STEP, so the affine
// step inside a block needs no division.
static const float SW_BLOCK_INV[PERSP_STEP + 1] = {
	0.0f,
	1.0f / 1.0f,
	1.0f / 2.0f,
	1.0f / 3.0f,
	1.0f / 4.0f,
	1.0f / 5.0f,
	1.0f / 6.0f,
	1.0f / 7.0f,
	1.0f / 8.0f,
};

Direct3DRMRenderer* Direct3DRMSoftwareRenderer::Create(DWORD width, DWORD height, D3DLightingModel lightingModel)
{
#ifdef __EMSCRIPTEN__
	const char* canvasId = SDL_GetStringProperty(
		SDL_GetWindowProperties(DDWindow),
		SDL_PROP_WINDOW_EMSCRIPTEN_CANVAS_ID_STRING,
		"#canvas"
	);
	int canDraw = MAIN_THREAD_EM_ASM_INT(
		{
			try {
				var canvas = document.querySelector(UTF8ToString($0));
				return canvas && canvas.getContext('2d') ? 1 : 0;
			}
			catch (e) {
				return 0;
			}
		},
		canvasId
	);
	if (!canDraw) {
		SDL_Log("Software renderer: no 2D context for canvas %s", canvasId);
		return nullptr;
	}
#endif
	return new Direct3DRMSoftwareRenderer(width, height, lightingModel);
}

Direct3DRMSoftwareRenderer::Direct3DRMSoftwareRenderer(DWORD width, DWORD height, D3DLightingModel lightingModel)
	: m_lightingModel(lightingModel)
{
	SDL_Log("Lighting model: %s", m_lightingModel == D3DLightingModel::DirectX8 ? "DirectX 8" : "DirectX 5");

	m_virtualWidth = width;
	m_virtualHeight = height;

	m_renderer = SDL_CreateRenderer(DDWindow, NULL);

	ViewportTransform viewportTransform = {1.0f, 0.0f, 0.0f};
	Resize(width, height, viewportTransform);
}

Direct3DRMSoftwareRenderer::~Direct3DRMSoftwareRenderer()
{
	SDL_DestroySurface(m_renderedImage);
	SDL_DestroyTexture(m_uploadBuffer);
	SDL_DestroyRenderer(m_renderer);
}

void Direct3DRMSoftwareRenderer::PushLights(const SceneLight* lights, size_t count)
{
	// Fold ambient lights into a single base color and pre-normalize the
	// directional light vectors so per-vertex lighting does no sqrt for them.
	m_preparedLights.clear();
	m_ambientR = 0.0f;
	m_ambientG = 0.0f;
	m_ambientB = 0.0f;

	for (size_t i = 0; i < count; ++i) {
		const SceneLight& light = lights[i];

		if (light.positional == 0.0f && light.directional == 0.0f) {
			m_ambientR += light.color.r;
			m_ambientG += light.color.g;
			m_ambientB += light.color.b;
			continue;
		}

		PreparedLight prepared;
		prepared.color = light.color;
		prepared.spot = light.positional == 1.0f && light.directional == 1.0f;
		prepared.range = light.range;
		for (int a = 0; a < 3; ++a) {
			prepared.attenuation[a] = light.attenuation[a];
		}
		float penumbra = std::max(light.penumbra, light.umbra);
		prepared.cosUmbra = std::cos(light.umbra);
		prepared.cosPenumbra = std::cos(penumbra);
		if (light.directional == 1.0f) {
			prepared.spotDirection = Normalize(light.direction);
		}
		if (light.directional == 1.0f && !prepared.spot) {
			prepared.positional = false;
			prepared.vec = Normalize({-light.direction.x, -light.direction.y, -light.direction.z});
		}
		else {
			prepared.positional = true;
			prepared.vec = light.position;
		}
		m_preparedLights.push_back(prepared);
	}
}

void Direct3DRMSoftwareRenderer::SetFrustumPlanes(const Plane* frustumPlanes)
{
	memcpy(m_frustumPlanes, frustumPlanes, sizeof(m_frustumPlanes));
}

void Direct3DRMSoftwareRenderer::SetProjection(const D3DRMMATRIX4D& projection, D3DVALUE front, D3DVALUE back)
{
	m_front = front;
	m_back = back;
	memcpy(m_projection, projection, sizeof(D3DRMMATRIX4D));
}

void Direct3DRMSoftwareRenderer::ClearZBuffer()
{
	// 0x7F7F7F7F interpreted as a float is ~3.4e38 — far beyond any depth
	// value we produce, so a byte-pattern memset (vectorized in libc on
	// every platform) is a valid "infinitely far" fill.
	memset(m_zBuffer.data(), 0x7F, m_zBuffer.size() * sizeof(float));
}

void Direct3DRMSoftwareRenderer::ProjectVertex(const D3DVECTOR& v, D3DRMVECTOR4D& p) const
{
	float px = m_projection[0][0] * v.x + m_projection[1][0] * v.y + m_projection[2][0] * v.z + m_projection[3][0];
	float py = m_projection[0][1] * v.x + m_projection[1][1] * v.y + m_projection[2][1] * v.z + m_projection[3][1];
	float pz = m_projection[0][2] * v.x + m_projection[1][2] * v.y + m_projection[2][2] * v.z + m_projection[3][2];
	float pw = m_projection[0][3] * v.x + m_projection[1][3] * v.y + m_projection[2][3] * v.z + m_projection[3][3];

	p.w = pw;

	// Perspective divide
	if (pw != 0.0f) {
		float invW = 1.0f / pw;
		px *= invW;
		py *= invW;
		pz *= invW;
	}

	// Map from NDC [-1,1] to the viewport's pixels
	p.x = m_viewportX + (px * 0.5f + 0.5f) * m_viewportW;
	p.y = m_viewportY + (1.0f - (py * 0.5f + 0.5f)) * m_viewportH;
	p.z = pz;
}

static SWLitVertex SplitEdge(SWLitVertex a, const SWLitVertex& b, float plane)
{
	float t = (plane - a.position.z) / (b.position.z - a.position.z);
	a.position.x += t * (b.position.x - a.position.x);
	a.position.y += t * (b.position.y - a.position.y);
	a.position.z = plane;

	a.texCoord.u += t * (b.texCoord.u - a.texCoord.u);
	a.texCoord.v += t * (b.texCoord.v - a.texCoord.v);

	a.color.r = static_cast<Uint8>(a.color.r + t * (b.color.r - a.color.r));
	a.color.g = static_cast<Uint8>(a.color.g + t * (b.color.g - a.color.g));
	a.color.b = static_cast<Uint8>(a.color.b + t * (b.color.b - a.color.b));
	a.color.a = static_cast<Uint8>(a.color.a + t * (b.color.a - a.color.a));

	a.specular.r = static_cast<Uint8>(a.specular.r + t * (b.specular.r - a.specular.r));
	a.specular.g = static_cast<Uint8>(a.specular.g + t * (b.specular.g - a.specular.g));
	a.specular.b = static_cast<Uint8>(a.specular.b + t * (b.specular.b - a.specular.b));
	a.specular.a = static_cast<Uint8>(a.specular.a + t * (b.specular.a - a.specular.a));

	return a;
}

static bool IsTriangleOutsideViewCone(
	const D3DVECTOR& v0,
	const D3DVECTOR& v1,
	const D3DVECTOR& v2,
	const Plane* frustumPlanes
)
{
	for (int i = 0; i < 4; ++i) {
		const Plane& plane = frustumPlanes[i];

		float d0 = DotProduct(plane.normal, v0) + plane.d;
		float d1 = DotProduct(plane.normal, v1) + plane.d;
		float d2 = DotProduct(plane.normal, v2) + plane.d;

		if (d0 < 0 && d1 < 0 && d2 < 0) {
			return true;
		}
	}
	return false;
}

// Clip a view-space triangle against the near plane z = front and pass the
// zero, one or two triangles that remain to draw(a, b, c).
template <typename Draw>
static void ClipNear(const SWLitVertex (&v)[3], float front, Draw&& draw)
{
	bool in0 = v[0].position.z >= front;
	bool in1 = v[1].position.z >= front;
	bool in2 = v[2].position.z >= front;

	int insideCount = in0 + in1 + in2;
	if (insideCount == 3) {
		draw(v[0], v[1], v[2]);
	}
	else if (insideCount == 2) {
		SWLitVertex split;
		if (!in0) {
			split = SplitEdge(v[2], v[0], front);
			draw(v[1], v[2], split);
			draw(v[1], split, SplitEdge(v[1], v[0], front));
		}
		else if (!in1) {
			split = SplitEdge(v[0], v[1], front);
			draw(v[2], v[0], split);
			draw(v[2], split, SplitEdge(v[2], v[1], front));
		}
		else {
			split = SplitEdge(v[1], v[2], front);
			draw(v[0], v[1], split);
			draw(v[0], split, SplitEdge(v[0], v[2], front));
		}
	}
	else if (in0) {
		draw(v[0], SplitEdge(v[0], v[1], front), SplitEdge(v[0], v[2], front));
	}
	else if (in1) {
		draw(SplitEdge(v[1], v[0], front), v[1], SplitEdge(v[1], v[2], front));
	}
	else if (in2) {
		draw(SplitEdge(v[2], v[0], front), SplitEdge(v[2], v[1], front), v[2]);
	}
}

void Direct3DRMSoftwareRenderer::DrawTriangleClipped(const SWLitVertex (&v)[3], const Appearance& appearance)
{
	bool in0 = v[0].position.z >= m_front;
	bool in1 = v[1].position.z >= m_front;
	bool in2 = v[2].position.z >= m_front;

	int insideCount = in0 + in1 + in2;

	if (insideCount == 0 || v[0].position.z > m_back && v[1].position.z > m_back && v[2].position.z > m_back) {
		return; // Outside clipping
	}
	if (IsTriangleOutsideViewCone(v[0].position, v[1].position, v[2].position, m_frustumPlanes)) {
		return;
	}

	ClipNear(v, m_front, [&](const SWLitVertex& a, const SWLitVertex& b, const SWLitVertex& c) {
		DrawTriangleProjected(a, b, c, appearance);
	});
}

// The render surface and all cached textures are SDL_PIXELFORMAT_RGBA32
// (bytes R,G,B,A in memory regardless of endianness), so pixels are accessed
// directly instead of through SDL_GetRGBA/SDL_MapRGBA calls per pixel.
inline static void BlendRGBA(Uint8* p, int r, int g, int b, int a)
{
	int inv = 255 - a;
	p[0] = static_cast<Uint8>((r * a + p[0] * inv + 127) / 255);
	p[1] = static_cast<Uint8>((g * a + p[1] * inv + 127) / 255);
	p[2] = static_cast<Uint8>((b * a + p[2] * inv + 127) / 255);
	int outA = a + (p[3] * inv + 127) / 255;
	p[3] = static_cast<Uint8>(outA > 255 ? 255 : outA);
}

float Direct3DRMSoftwareRenderer::LightReach(
	const PreparedLight& light,
	const D3DVECTOR& worldPos,
	D3DVECTOR& toLight
) const
{
	D3DVECTOR d = {light.vec.x - worldPos.x, light.vec.y - worldPos.y, light.vec.z - worldPos.z};
	float distance = std::sqrt(DotProduct(d, d));
	if (distance > light.range) {
		return 0.0f;
	}
	toLight = distance > 0.0f ? D3DVECTOR{d.x / distance, d.y / distance, d.z / distance} : D3DVECTOR{0, 0, 0};
	float denominator =
		light.attenuation[0] + light.attenuation[1] * distance + light.attenuation[2] * distance * distance;
	float reach = denominator > 0.0f ? 1.0f / denominator : 1.0f;
	if (light.spot) {
		float rho = -DotProduct(toLight, light.spotDirection);
		if (rho <= light.cosPenumbra) {
			return 0.0f;
		}
		if (rho < light.cosUmbra) {
			reach *= (rho - light.cosPenumbra) / (light.cosUmbra - light.cosPenumbra);
		}
	}
	return reach;
}

void Direct3DRMSoftwareRenderer::ApplyLighting(
	const D3DVECTOR& position,
	const D3DVECTOR& oNormal,
	const Appearance& appearance,
	SDL_Color& color,
	SDL_Color& specular
)
{
	FColor spec = {0, 0, 0, 0};
	FColor diffuse = {m_ambientR, m_ambientG, m_ambientB, 0};

	D3DVECTOR normal = Normalize(TransformNormal(oNormal, m_normalMatrix));

	bool haveWorldPos = false;
	D3DVECTOR worldPos;

	for (size_t i = 0; i < m_preparedLights.size(); ++i) {
		const PreparedLight& light = m_preparedLights[i];

		D3DVECTOR lightVec;
		float reach = 1.0f;
		if (light.positional) {
			if (!haveWorldPos) {
				worldPos = TransformPoint(position, m_worldMatrix);
				haveWorldPos = true;
			}
			reach = LightReach(light, worldPos, lightVec);
			if (reach <= 0.0f) {
				continue;
			}
		}
		else {
			lightVec = light.vec;
		}

		float dotNL = DotProduct(normal, lightVec) * reach;
		if (dotNL > 0.0f) {
			diffuse.r += dotNL * light.color.r;
			diffuse.g += dotNL * light.color.g;
			diffuse.b += dotNL * light.color.b;

			if (m_specular) {
				float dotNH = DotProduct(normal, m_halfway[i]);
				if (dotNH > 0.0f) {
					float power = std::pow(dotNH, appearance.shininess);
					spec.r += power * light.color.r;
					spec.g += power * light.color.g;
					spec.b += power * light.color.b;
				}
			}
		}
	}

	color = SDL_Color{
		static_cast<Uint8>(std::min(255.0f, diffuse.r * appearance.color.r)),
		static_cast<Uint8>(std::min(255.0f, diffuse.g * appearance.color.g)),
		static_cast<Uint8>(std::min(255.0f, diffuse.b * appearance.color.b)),
		appearance.color.a
	};
	specular = SDL_Color{
		static_cast<Uint8>(std::min(255.0f, spec.r * 255.0f)),
		static_cast<Uint8>(std::min(255.0f, spec.g * 255.0f)),
		static_cast<Uint8>(std::min(255.0f, spec.b * 255.0f)),
		0
	};
}

inline D3DVECTOR Subtract(const D3DVECTOR& a, const D3DVECTOR& b)
{
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline bool IsBackface(const D3DVECTOR& v0, const D3DVECTOR& v1, const D3DVECTOR& v2)
{
	D3DVECTOR normal = CrossProduct(Subtract(v1, v0), Subtract(v2, v0));

	return DotProduct(normal, v0) >= 0.0f;
}

// Rasterizer vertex: screen position, vertex color and perspective UVs.
struct SWVertexXY {
	float x, y, z;
	float r, g, b;
	float sr, sg, sb;
	float u_over_w, v_over_w, one_over_w;
};

// Interpolated values along a triangle edge, stepped once per scanline.
struct SWEdge {
	float x, z, r, g, b, sr, sg, sb, uw, vw, ow;
};

inline static void SWSetupEdge(const SWVertexXY& a, const SWVertexXY& b, int startY, SWEdge& e, SWEdge& step)
{
	float dy = b.y - a.y;
	float invDy = (dy != 0.0f) ? 1.0f / dy : 0.0f;
	float t0 = (startY - a.y) * invDy;
	e.x = a.x + t0 * (b.x - a.x);
	e.z = a.z + t0 * (b.z - a.z);
	e.r = a.r + t0 * (b.r - a.r);
	e.g = a.g + t0 * (b.g - a.g);
	e.b = a.b + t0 * (b.b - a.b);
	e.sr = a.sr + t0 * (b.sr - a.sr);
	e.sg = a.sg + t0 * (b.sg - a.sg);
	e.sb = a.sb + t0 * (b.sb - a.sb);
	e.uw = a.u_over_w + t0 * (b.u_over_w - a.u_over_w);
	e.vw = a.v_over_w + t0 * (b.v_over_w - a.v_over_w);
	e.ow = a.one_over_w + t0 * (b.one_over_w - a.one_over_w);
	step.x = (b.x - a.x) * invDy;
	step.z = (b.z - a.z) * invDy;
	step.r = (b.r - a.r) * invDy;
	step.g = (b.g - a.g) * invDy;
	step.b = (b.b - a.b) * invDy;
	step.sr = (b.sr - a.sr) * invDy;
	step.sg = (b.sg - a.sg) * invDy;
	step.sb = (b.sb - a.sb) * invDy;
	step.uw = (b.u_over_w - a.u_over_w) * invDy;
	step.vw = (b.v_over_w - a.v_over_w) * invDy;
	step.ow = (b.one_over_w - a.one_over_w) * invDy;
}

inline static void SWStepEdge(SWEdge& e, const SWEdge& s)
{
	e.x += s.x;
	e.z += s.z;
	e.r += s.r;
	e.g += s.g;
	e.b += s.b;
	e.sr += s.sr;
	e.sg += s.sg;
	e.sb += s.sb;
	e.uw += s.uw;
	e.vw += s.vw;
	e.ow += s.ow;
}

inline static float SWClampChannel(float c)
{
	return c < 0.0f ? 0.0f : (c > 255.0f ? 255.0f : c);
}

void Direct3DRMSoftwareRenderer::DrawTriangleProjected(
	const SWLitVertex& v0,
	const SWLitVertex& v1,
	const SWLitVertex& v2,
	const Appearance& appearance
)
{
	const Uint8 triAlpha = appearance.color.a;

	Uint32 textureId = appearance.textureId;
	int texturePitch = 0;
	Uint8* texels = nullptr;
	int texWidthScale = 0;
	int texHeightScale = 0;
	// Power-of-two textures (the normal case) are sampled with integer
	// 16.16 coordinates and shift+mask wrapping.
	bool fastTex = false;
	int uMask = 0, vMask = 0, pitchShift = 0;
	float texWFix = 0.0f, texHFix = 0.0f;

	if (textureId != NO_TEXTURE_ID) {
		SDL_Surface* texture = m_textures[textureId].cached;
		if (texture) {
			texturePitch = texture->pitch;
			texels = static_cast<Uint8*>(texture->pixels);
			int tw = texture->w;
			int th = texture->h;
			texWidthScale = tw - 1;
			texHeightScale = th - 1;
			if ((tw & (tw - 1)) == 0 && (th & (th - 1)) == 0 && (texturePitch & (texturePitch - 1)) == 0) {
				fastTex = true;
				uMask = tw - 1;
				vMask = th - 1;
				while ((1 << pitchShift) < texturePitch) {
					++pitchShift;
				}
				texWFix = static_cast<float>(tw) * 65536.0f;
				texHFix = static_cast<float>(th) * 65536.0f;
			}
		}
	}

	if (!texels && triAlpha == 0) {
		// Fully transparent material — nothing to draw.
		return;
	}

	D3DRMVECTOR4D p0, p1, p2;
	ProjectVertex(v0.position, p0);
	ProjectVertex(v1.position, p1);
	ProjectVertex(v2.position, p2);

	SWVertexXY verts[3];
	const SWLitVertex* lit[3] = {&v0, &v1, &v2};
	const D3DRMVECTOR4D* projected[3] = {&p0, &p1, &p2};
	bool hasSpecular = false;
	for (int i = 0; i < 3; ++i) {
		const SDL_Color& c = lit[i]->color;
		const SDL_Color& sp = lit[i]->specular;
		SWVertexXY& vx = verts[i];
		vx.x = projected[i]->x;
		vx.y = projected[i]->y;
		vx.z = projected[i]->z;
		if (texels) {
			vx.r = c.r;
			vx.g = c.g;
			vx.b = c.b;
			vx.sr = sp.r;
			vx.sg = sp.g;
			vx.sb = sp.b;
			hasSpecular |= (sp.r | sp.g | sp.b) != 0;
		}
		else {
			vx.r = std::min(255, c.r + sp.r);
			vx.g = std::min(255, c.g + sp.g);
			vx.b = std::min(255, c.b + sp.b);
			vx.sr = vx.sg = vx.sb = 0.0f;
		}
		vx.u_over_w = vx.v_over_w = vx.one_over_w = 0.0f;
	}

	if (texels) {
		verts[0].u_over_w = v0.texCoord.u / p0.w;
		verts[0].v_over_w = v0.texCoord.v / p0.w;
		verts[0].one_over_w = 1.0f / p0.w;

		verts[1].u_over_w = v1.texCoord.u / p1.w;
		verts[1].v_over_w = v1.texCoord.v / p1.w;
		verts[1].one_over_w = 1.0f / p1.w;

		verts[2].u_over_w = v2.texCoord.u / p2.w;
		verts[2].v_over_w = v2.texCoord.v / p2.w;
		verts[2].one_over_w = 1.0f / p2.w;
	}

	// Sort verts
	if (verts[0].y > verts[1].y) {
		std::swap(verts[0], verts[1]);
	}
	if (verts[1].y > verts[2].y) {
		std::swap(verts[1], verts[2]);
	}
	if (verts[0].y > verts[1].y) {
		std::swap(verts[0], verts[1]);
	}

	int minY = std::max(m_clipMinY, (int) std::ceil(verts[0].y));
	int maxY = std::min(m_clipMaxY, (int) std::floor(verts[2].y));
	if (minY > maxY) {
		return;
	}

	Uint8* pixels = (Uint8*) m_renderedImage->pixels;
	int pitch = m_renderedImage->pitch;

	// Incremental edge walking: interpolate along the long edge (0->2) and
	// the two short segments (0->1, 1->2), stepping once per scanline
	// instead of re-interpolating every attribute per pixel.
	SWEdge longEdge, longStep;
	SWSetupEdge(verts[0], verts[2], minY, longEdge, longStep);

	SWEdge shortEdge, shortStep;
	int midY = static_cast<int>(std::ceil(verts[1].y));
	bool pastMid = (minY >= midY);
	if (pastMid) {
		SWSetupEdge(verts[1], verts[2], minY, shortEdge, shortStep);
	}
	else {
		SWSetupEdge(verts[0], verts[1], minY, shortEdge, shortStep);
	}

	for (int y = minY; y <= maxY; ++y) {
		if (!pastMid && y >= midY) {
			pastMid = true;
			SWSetupEdge(verts[1], verts[2], y, shortEdge, shortStep);
		}

		const SWEdge& left = (shortEdge.x <= longEdge.x) ? shortEdge : longEdge;
		const SWEdge& right = (shortEdge.x <= longEdge.x) ? longEdge : shortEdge;

		int startX = std::max(m_clipMinX, (int) std::ceil(left.x));
		int endX = std::min(m_clipMaxX, (int) std::floor(right.x));

		float span = right.x - left.x;
		if (span <= 0.0f || startX > endX) {
			SWStepEdge(shortEdge, shortStep);
			SWStepEdge(longEdge, longStep);
			continue;
		}

		float invSpan = 1.0f / span;
		float startT = (startX - left.x) * invSpan;

		float z = left.z + startT * (right.z - left.z);
		float zStep = (right.z - left.z) * invSpan;

		// Clamp colors at the span endpoints; interpolated values stay
		// inside, so the pixel loops need no per-pixel clamp.
		float lr = SWClampChannel(left.r), lg = SWClampChannel(left.g), lb = SWClampChannel(left.b);
		float rr = SWClampChannel(right.r), rg = SWClampChannel(right.g), rb = SWClampChannel(right.b);

		// Vertex colors in 8.8 fixed point, stepped with integer adds.
		int rFix = static_cast<int>((lr + startT * (rr - lr)) * 256.0f);
		int gFix = static_cast<int>((lg + startT * (rg - lg)) * 256.0f);
		int bFix = static_cast<int>((lb + startT * (rb - lb)) * 256.0f);
		int rStep = static_cast<int>((rr - lr) * invSpan * 256.0f);
		int gStep = static_cast<int>((rg - lg) * invSpan * 256.0f);
		int bStep = static_cast<int>((rb - lb) * invSpan * 256.0f);

		int srFix = 0, sgFix = 0, sbFix = 0;
		int srStep = 0, sgStep = 0, sbStep = 0;
		if (hasSpecular) {
			float lsr = SWClampChannel(left.sr), lsg = SWClampChannel(left.sg), lsb = SWClampChannel(left.sb);
			float rsr = SWClampChannel(right.sr), rsg = SWClampChannel(right.sg), rsb = SWClampChannel(right.sb);
			srFix = static_cast<int>((lsr + startT * (rsr - lsr)) * 256.0f);
			sgFix = static_cast<int>((lsg + startT * (rsg - lsg)) * 256.0f);
			sbFix = static_cast<int>((lsb + startT * (rsb - lsb)) * 256.0f);
			srStep = static_cast<int>((rsr - lsr) * invSpan * 256.0f);
			sgStep = static_cast<int>((rsg - lsg) * invSpan * 256.0f);
			sbStep = static_cast<int>((rsb - lsb) * invSpan * 256.0f);
		}

		Uint8* row = pixels + y * pitch;
		float* zPtr = &m_zBuffer[y * m_width + startX];

		if (texels) {
			float uow = left.uw + startT * (right.uw - left.uw);
			float vow = left.vw + startT * (right.vw - left.vw);
			float oow = left.ow + startT * (right.ow - left.ow);
			float uowStep = (right.uw - left.uw) * invSpan;
			float vowStep = (right.vw - left.vw) * invSpan;
			float oowStep = (right.ow - left.ow) * invSpan;

			// Perspective-correct UV at the span start; carried block to
			// block so each block costs a single divide.
			float invW = 1.0f / oow;
			float u0 = uow * invW;
			float vv0 = vow * invW;

			int x = startX;
			while (x <= endX) {
				int remaining = endX - x + 1;
				int blockLen = (remaining > PERSP_STEP) ? PERSP_STEP : remaining;

				uow += uowStep * blockLen;
				vow += vowStep * blockLen;
				oow += oowStep * blockLen;
				invW = 1.0f / oow;
				float u1 = uow * invW;
				float vv1 = vow * invW;

				float invBlock = SW_BLOCK_INV[blockLen];

				if (fastTex) {
					Sint32 uFix = static_cast<Sint32>(u0 * texWFix);
					Sint32 vFix = static_cast<Sint32>(vv0 * texHFix);
					Sint32 uStepFix = static_cast<Sint32>((u1 - u0) * invBlock * texWFix);
					Sint32 vStepFix = static_cast<Sint32>((vv1 - vv0) * invBlock * texHFix);

					for (int i = 0; i < blockLen; ++i, ++x) {
						if (z < *zPtr) {
							const Uint8* t =
								texels + ((((vFix >> 16) & vMask) << pitchShift) | (((uFix >> 16) & uMask) << 2));
							int ta = t[3];
							if (ta != 0) {
								int cr = ((rFix >> 8) * t[0] + 127) / 255;
								int cg = ((gFix >> 8) * t[1] + 127) / 255;
								int cb = ((bFix >> 8) * t[2] + 127) / 255;
								if (hasSpecular) {
									cr = std::min(255, cr + (srFix >> 8));
									cg = std::min(255, cg + (sgFix >> 8));
									cb = std::min(255, cb + (sbFix >> 8));
								}
								Uint8* p = row + x * 4;
								if (ta == 255) {
									*zPtr = z;
									p[0] = static_cast<Uint8>(cr);
									p[1] = static_cast<Uint8>(cg);
									p[2] = static_cast<Uint8>(cb);
									p[3] = 255;
								}
								else {
									BlendRGBA(p, cr, cg, cb, ta);
								}
							}
						}
						z += zStep;
						rFix += rStep;
						gFix += gStep;
						bFix += bStep;
						uFix += uStepFix;
						vFix += vStepFix;
						if (hasSpecular) {
							srFix += srStep;
							sgFix += sgStep;
							sbFix += sbStep;
						}
						++zPtr;
					}
				}
				else {
					// Non-power-of-two texture fallback: affine float UVs
					float uAffStep = (u1 - u0) * invBlock;
					float vAffStep = (vv1 - vv0) * invBlock;
					float uAff = u0;
					float vAff = vv0;

					for (int i = 0; i < blockLen; ++i, ++x) {
						if (z < *zPtr) {
							float u = uAff - std::floor(uAff);
							float v = vAff - std::floor(vAff);

							int texX = static_cast<int>(u * texWidthScale);
							int texY = static_cast<int>(v * texHeightScale);

							const Uint8* t = texels + texY * texturePitch + texX * 4;
							int ta = t[3];
							if (ta != 0) {
								int cr = ((rFix >> 8) * t[0] + 127) / 255;
								int cg = ((gFix >> 8) * t[1] + 127) / 255;
								int cb = ((bFix >> 8) * t[2] + 127) / 255;
								if (hasSpecular) {
									cr = std::min(255, cr + (srFix >> 8));
									cg = std::min(255, cg + (sgFix >> 8));
									cb = std::min(255, cb + (sbFix >> 8));
								}
								Uint8* p = row + x * 4;
								if (ta == 255) {
									*zPtr = z;
									p[0] = static_cast<Uint8>(cr);
									p[1] = static_cast<Uint8>(cg);
									p[2] = static_cast<Uint8>(cb);
									p[3] = 255;
								}
								else {
									BlendRGBA(p, cr, cg, cb, ta);
								}
							}
						}
						z += zStep;
						rFix += rStep;
						gFix += gStep;
						bFix += bStep;
						uAff += uAffStep;
						vAff += vAffStep;
						if (hasSpecular) {
							srFix += srStep;
							sgFix += sgStep;
							sbFix += sbStep;
						}
						++zPtr;
					}
				}

				u0 = u1;
				vv0 = vv1;
			}
		}
		else if (triAlpha == 255) {
			// Opaque untextured span
			for (int x = startX; x <= endX; ++x, ++zPtr, z += zStep, rFix += rStep, gFix += gStep, bFix += bStep) {
				if (z >= *zPtr) {
					continue;
				}
				*zPtr = z;
				Uint8* p = row + x * 4;
				p[0] = static_cast<Uint8>(rFix >> 8);
				p[1] = static_cast<Uint8>(gFix >> 8);
				p[2] = static_cast<Uint8>(bFix >> 8);
				p[3] = 255;
			}
		}
		else {
			// Alpha-blended untextured span (does not write depth)
			for (int x = startX; x <= endX; ++x, ++zPtr, z += zStep, rFix += rStep, gFix += gStep, bFix += bStep) {
				if (z >= *zPtr) {
					continue;
				}
				BlendRGBA(row + x * 4, rFix >> 8, gFix >> 8, bFix >> 8, triAlpha);
			}
		}

		SWStepEdge(shortEdge, shortStep);
		SWStepEdge(longEdge, longStep);
	}
}

void Direct3DRMSoftwareRenderer::AddTextureDestroyCallback(Uint32 id, IDirect3DRMTexture* texture)
{
	RegisterDestroyCallback(texture, id, [](Direct3DRMRenderer* renderer, Uint32 textureId) {
		auto& cacheEntry = static_cast<Direct3DRMSoftwareRenderer*>(renderer)->m_textures[textureId];
		if (cacheEntry.cached) {
			SDL_UnlockSurface(cacheEntry.cached);
			SDL_DestroySurface(cacheEntry.cached);
			cacheEntry.cached = nullptr;
			cacheEntry.texture = nullptr;
		}
	});
}

// The texture in the render format. A colour key on the surface becomes
// alpha 0 (SDL does this when converting), and so does decal transparency's
// colour.
SDL_Surface* Direct3DRMSoftwareRenderer::ConvertTexture(Direct3DRMTextureImpl* texture, bool& keyed) const
{
	auto surface = static_cast<DirectDrawSurfaceImpl*>(texture->m_surface);
	SDL_Surface* converted = SDL_ConvertSurface(surface->m_surface, m_renderedImage->format);
	if (!converted) {
		keyed = false;
		return nullptr;
	}
	keyed = SDL_SurfaceHasColorKey(surface->m_surface);
	if (texture->m_decalTransparency) {
		keyed = true;
		const Uint8 r = (texture->m_decalTransparentColor >> 16) & 0xff;
		const Uint8 g = (texture->m_decalTransparentColor >> 8) & 0xff;
		const Uint8 b = texture->m_decalTransparentColor & 0xff;
		for (int y = 0; y < converted->h; ++y) {
			Uint8* px = static_cast<Uint8*>(converted->pixels) + y * converted->pitch;
			for (int x = 0; x < converted->w; ++x, px += 4) {
				if (px[0] == r && px[1] == g && px[2] == b) {
					px[3] = 0;
				}
			}
		}
	}
	SDL_LockSurface(converted);
	return converted;
}

Uint32 Direct3DRMSoftwareRenderer::GetTextureId(IDirect3DRMTexture* iTexture, bool isUI, float scaleX, float scaleY)
{
	auto texture = static_cast<Direct3DRMTextureImpl*>(iTexture);

	// Check if already mapped
	for (Uint32 i = 0; i < m_textures.size(); ++i) {
		auto& texRef = m_textures[i];
		if (texRef.texture == texture) {
			if (texRef.version != texture->m_version) {
				// Update animated textures
				SDL_DestroySurface(texRef.cached);
				texRef.cached = ConvertTexture(texture, texRef.keyed);
				texRef.version = texture->m_version;
			}
			return i;
		}
	}

	bool keyed = false;
	SDL_Surface* convertedRender = ConvertTexture(texture, keyed);

	// Reuse freed slot
	for (Uint32 i = 0; i < m_textures.size(); ++i) {
		auto& texRef = m_textures[i];
		if (!texRef.texture) {
			texRef = {texture, texture->m_version, convertedRender, keyed};
			AddTextureDestroyCallback(i, texture);
			return i;
		}
	}

	// Append new
	m_textures.push_back({texture, texture->m_version, convertedRender, keyed});
	AddTextureDestroyCallback(static_cast<Uint32>(m_textures.size() - 1), texture);
	return static_cast<Uint32>(m_textures.size() - 1);
}

MeshCache UploadMesh(const MeshGroup& meshGroup)
{
	MeshCache cache{&meshGroup, meshGroup.version};

	cache.flat = meshGroup.quality == D3DRMRENDER_FLAT || meshGroup.quality == D3DRMRENDER_UNLITFLAT;

	if (cache.flat) {
		FlattenSurfaces(
			meshGroup.vertices.data(),
			meshGroup.vertices.size(),
			meshGroup.indices.data(),
			meshGroup.indices.size(),
			meshGroup.texture != nullptr,
			cache.vertices,
			cache.indices
		);
	}
	else {
		cache.vertices.assign(meshGroup.vertices.begin(), meshGroup.vertices.end());
		cache.indices.assign(meshGroup.indices.begin(), meshGroup.indices.end());
	}

	return cache;
}

void Direct3DRMSoftwareRenderer::AddMeshDestroyCallback(Uint32 id, IDirect3DRMMesh* mesh)
{
	RegisterDestroyCallback(mesh, id, [](Direct3DRMRenderer* renderer, Uint32 meshId) {
		auto& cacheEntry = static_cast<Direct3DRMSoftwareRenderer*>(renderer)->m_meshs[meshId];
		if (cacheEntry.meshGroup) {
			cacheEntry.meshGroup = nullptr;
			cacheEntry.vertices.clear();
			cacheEntry.indices.clear();
		}
	});
}

Uint32 Direct3DRMSoftwareRenderer::GetMeshId(IDirect3DRMMesh* mesh, const MeshGroup* meshGroup)
{
	for (Uint32 i = 0; i < m_meshs.size(); ++i) {
		auto& cache = m_meshs[i];
		if (cache.meshGroup == meshGroup) {
			if (cache.version != meshGroup->version) {
				cache = std::move(UploadMesh(*meshGroup));
			}
			return i;
		}
	}

	auto newCache = UploadMesh(*meshGroup);

	for (Uint32 i = 0; i < m_meshs.size(); ++i) {
		auto& cache = m_meshs[i];
		if (!cache.meshGroup) {
			cache = std::move(newCache);
			AddMeshDestroyCallback(i, mesh);
			return i;
		}
	}

	m_meshs.push_back(std::move(newCache));
	AddMeshDestroyCallback((Uint32) (m_meshs.size() - 1), mesh);
	return (Uint32) (m_meshs.size() - 1);
}

HRESULT Direct3DRMSoftwareRenderer::BeginFrame()
{
	if (!m_renderedImage || !SDL_LockSurface(m_renderedImage)) {
		return DDERR_GENERIC;
	}
	if (!m_clientClearsZ) {
		ClearZBuffer();
	}

	m_format = SDL_GetPixelFormatDetails(m_renderedImage->format);
	m_palette = SDL_GetSurfacePalette(m_renderedImage);

	return DD_OK;
}

void Direct3DRMSoftwareRenderer::EnableTransparency()
{
}

void Direct3DRMSoftwareRenderer::SubmitDraw(
	DWORD meshId,
	const D3DRMMATRIX4D& modelViewMatrix,
	const D3DRMMATRIX4D& worldMatrix,
	const D3DRMMATRIX4D& viewMatrix,
	const Matrix3x3& normalMatrix,
	const Appearance& appearance
)
{
	memcpy(m_normalMatrix, normalMatrix, sizeof(Matrix3x3));
	memcpy(m_worldMatrix, worldMatrix, sizeof(D3DRMMATRIX4D));

	m_specular = m_lightingModel == D3DLightingModel::DirectX5 && appearance.shininess > 0.0f;
	if (m_specular) {
		D3DVECTOR modelOrigin = {worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2]};
		D3DVECTOR cameraPos = {
			-(viewMatrix[3][0] * viewMatrix[0][0] + viewMatrix[3][1] * viewMatrix[0][1] +
			  viewMatrix[3][2] * viewMatrix[0][2]),
			-(viewMatrix[3][0] * viewMatrix[1][0] + viewMatrix[3][1] * viewMatrix[1][1] +
			  viewMatrix[3][2] * viewMatrix[1][2]),
			-(viewMatrix[3][0] * viewMatrix[2][0] + viewMatrix[3][1] * viewMatrix[2][1] +
			  viewMatrix[3][2] * viewMatrix[2][2])
		};
		D3DVECTOR camDir = Normalize(Subtract(cameraPos, modelOrigin));

		m_halfway.resize(m_preparedLights.size());
		for (size_t i = 0; i < m_preparedLights.size(); ++i) {
			const PreparedLight& light = m_preparedLights[i];
			D3DVECTOR towardLight = light.positional ? Normalize(Subtract(light.vec, modelOrigin)) : light.vec;
			m_halfway[i] = Normalize({camDir.x + towardLight.x, camDir.y + towardLight.y, camDir.z + towardLight.z});
		}
	}

	auto& mesh = m_meshs[meshId];
	const size_t vertexCount = mesh.vertices.size();

	// Pre-transform all vertex positions
	m_transformedPositions.resize(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i) {
		m_transformedPositions[i] = TransformPoint(mesh.vertices[i].position, modelViewMatrix);
	}

	// Lighting is computed lazily, once per unique vertex, and only for
	// vertices of triangles that survive backface culling.  Shared vertices
	// previously got re-lit for every triangle that used them.
	m_vertexColors.resize(vertexCount);
	m_vertexSpeculars.resize(vertexCount);
	m_vertexLit.assign(vertexCount, 0);

	const bool flat = appearance.flat;

	// Assemble triangles using index buffer
	for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
		const uint16_t i0 = mesh.indices[i];
		const uint16_t i1 = mesh.indices[i + 1];
		const uint16_t i2 = mesh.indices[i + 2];
		const D3DVECTOR& p0 = m_transformedPositions[i0];
		const D3DVECTOR& p1 = m_transformedPositions[i1];
		const D3DVECTOR& p2 = m_transformedPositions[i2];

		// Cull before lighting and clipping; sub-triangles produced by the
		// near-plane clip are coplanar with the original, so this test is
		// equivalent to the old per-projected-triangle one.
		if (IsBackface(p0, p1, p2)) {
			continue;
		}

		if (!m_vertexLit[i0]) {
			m_vertexLit[i0] = 1;
			ApplyLighting(
				mesh.vertices[i0].position,
				mesh.vertices[i0].normal,
				appearance,
				m_vertexColors[i0],
				m_vertexSpeculars[i0]
			);
		}
		SDL_Color c0 = m_vertexColors[i0];
		SDL_Color s0 = m_vertexSpeculars[i0];
		SDL_Color c1, c2, s1, s2;
		if (flat) {
			c1 = c2 = c0;
			s1 = s2 = s0;
		}
		else {
			if (!m_vertexLit[i1]) {
				m_vertexLit[i1] = 1;
				ApplyLighting(
					mesh.vertices[i1].position,
					mesh.vertices[i1].normal,
					appearance,
					m_vertexColors[i1],
					m_vertexSpeculars[i1]
				);
			}
			if (!m_vertexLit[i2]) {
				m_vertexLit[i2] = 1;
				ApplyLighting(
					mesh.vertices[i2].position,
					mesh.vertices[i2].normal,
					appearance,
					m_vertexColors[i2],
					m_vertexSpeculars[i2]
				);
			}
			c1 = m_vertexColors[i1];
			c2 = m_vertexColors[i2];
			s1 = m_vertexSpeculars[i1];
			s2 = m_vertexSpeculars[i2];
		}

		SWLitVertex tri[3] = {
			{p0, mesh.vertices[i0].texCoord, c0, s0},
			{p1, mesh.vertices[i1].texCoord, c1, s1},
			{p2, mesh.vertices[i2].texCoord, c2, s2},
		};
		DrawTriangleClipped(tri, appearance);
	}
}

HRESULT Direct3DRMSoftwareRenderer::FinalizeFrame()
{
	SDL_UnlockSurface(m_renderedImage);

	return DD_OK;
}

void Direct3DRMSoftwareRenderer::Resize(int width, int height, const ViewportTransform& viewportTransform)
{
	m_viewportTransform = viewportTransform;
	float aspect = static_cast<float>(width) / height;
	float virtualAspect = static_cast<float>(m_virtualWidth) / m_virtualHeight;

	// Cap to virtual canvase for performance
	if (aspect > virtualAspect) {
		m_height = std::min(height, m_virtualHeight);
		m_width = static_cast<int>(m_height * aspect);
	}
	else {
		m_width = std::min(width, m_virtualWidth);
		m_height = static_cast<int>(m_width / aspect);
	}

	m_viewportTransform.scale =
		std::min(static_cast<float>(m_width) / m_virtualWidth, static_cast<float>(m_height) / m_virtualHeight);

	m_viewportTransform.offsetX = (m_width - (m_virtualWidth * m_viewportTransform.scale)) / 2.0f;
	m_viewportTransform.offsetY = (m_height - (m_virtualHeight * m_viewportTransform.scale)) / 2.0f;

	if (m_renderedImage) {
		SDL_DestroySurface(m_renderedImage);
	}
	m_renderedImage = SDL_CreateSurface(m_width, m_height, SDL_PIXELFORMAT_RGBA32);

	if (m_uploadBuffer) {
		SDL_DestroyTexture(m_uploadBuffer);
	}
	m_uploadBuffer =
		SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);

	m_zBuffer.resize(m_width * m_height);
	SetViewportRect(nullptr);
}

void Direct3DRMSoftwareRenderer::Clear(float r, float g, float b)
{
	const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(m_renderedImage->format);
	Uint32 color = SDL_MapRGB(details, m_palette, r * 255, g * 255, b * 255);
	SDL_FillSurfaceRect(m_renderedImage, nullptr, color);
}

void Direct3DRMSoftwareRenderer::Flip()
{
	SDL_UpdateTexture(m_uploadBuffer, nullptr, m_renderedImage->pixels, m_renderedImage->pitch);
	SDL_RenderTexture(m_renderer, m_uploadBuffer, nullptr, nullptr);
	SDL_RenderPresent(m_renderer);
}

void Direct3DRMSoftwareRenderer::Draw2DImage(
	Uint32 textureId,
	const SDL_Rect& srcRect,
	const SDL_Rect& dstRect,
	FColor color
)
{
	SDL_Rect centeredRect = {
		static_cast<int>(dstRect.x * m_viewportTransform.scale + m_viewportTransform.offsetX),
		static_cast<int>(dstRect.y * m_viewportTransform.scale + m_viewportTransform.offsetY),
		static_cast<int>(dstRect.w * m_viewportTransform.scale),
		static_cast<int>(dstRect.h * m_viewportTransform.scale),
	};

	if (textureId == NO_TEXTURE_ID) {
		Uint32 sdlColor = SDL_MapRGBA(
			m_format,
			m_palette,
			static_cast<Uint8>(color.r * 255),
			static_cast<Uint8>(color.g * 255),
			static_cast<Uint8>(color.b * 255),
			static_cast<Uint8>(color.a * 255)
		);
		SDL_FillSurfaceRect(m_renderedImage, &centeredRect, sdlColor);
		return;
	}

	bool isUpscaling = centeredRect.w > srcRect.w || centeredRect.h > srcRect.h;

	SDL_Surface* surface = m_textures[textureId].cached;
	SDL_UnlockSurface(surface);
	SDL_BlitSurfaceScaled(
		surface,
		&srcRect,
		m_renderedImage,
		&centeredRect,
		isUpscaling ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR
	);
	SDL_LockSurface(surface);
}

void Direct3DRMSoftwareRenderer::SetDither(bool dither)
{
}

void Direct3DRMSoftwareRenderer::SetPalette(SDL_Palette* palette)
{
	if (m_renderedImage) {
		SDL_SetSurfacePalette(m_renderedImage, palette);
	}
}

void Direct3DRMSoftwareRenderer::Download(SDL_Surface* target)
{
	SDL_Rect srcRect = {
		static_cast<int>(m_viewportTransform.offsetX),
		static_cast<int>(m_viewportTransform.offsetY),
		static_cast<int>(m_virtualWidth * m_viewportTransform.scale),
		static_cast<int>(m_virtualHeight * m_viewportTransform.scale),
	};

	SDL_BlitSurfaceScaled(m_renderedImage, &srcRect, target, nullptr, SDL_SCALEMODE_LINEAR);
}

// ---- immediate mode ---------------------------------------------------------

void Direct3DRMSoftwareRenderer::DrawImmediateClipped(const SWLitVertex (&v)[3], const ImmediatePixelState& state)
{
	if (v[0].position.z < m_front && v[1].position.z < m_front && v[2].position.z < m_front) {
		return;
	}
	if (v[0].position.z > m_back && v[1].position.z > m_back && v[2].position.z > m_back) {
		return;
	}
	if (IsTriangleOutsideViewCone(v[0].position, v[1].position, v[2].position, m_frustumPlanes)) {
		return;
	}
	ClipNear(v, m_front, [&](const SWLitVertex& a, const SWLitVertex& b, const SWLitVertex& c) {
		DrawImmediateTriangle(a, b, c, state);
	});
}

// Direct3D's fog factor at view depth z: 1 leaves the colour, 0 is all fog.
static float FogFactor(DWORD mode, float start, float end, float density, float z)
{
	float f = 1.0f;
	switch (static_cast<D3DFOGMODE>(mode)) {
	case D3DFOG_LINEAR:
		f = end != start ? (end - z) / (end - start) : (z < start ? 1.0f : 0.0f);
		break;
	case D3DFOG_EXP:
		f = std::exp(-density * z);
		break;
	case D3DFOG_EXP2:
		f = std::exp(-(density * z) * (density * z));
		break;
	default:
		break;
	}
	return std::min(1.0f, std::max(0.0f, f));
}

static float FloatState(DWORD bits)
{
	float value;
	SDL_memcpy(&value, &bits, sizeof(value));
	return value;
}

static bool ImmediateDepthTest(DWORD func, float z, float stored)
{
	switch (static_cast<D3DCMPFUNC>(func)) {
	case D3DCMP_NEVER:
		return false;
	case D3DCMP_LESS:
		return z < stored;
	case D3DCMP_EQUAL:
		return z == stored;
	case D3DCMP_LESSEQUAL:
		return z <= stored;
	case D3DCMP_GREATER:
		return z > stored;
	case D3DCMP_NOTEQUAL:
		return z != stored;
	case D3DCMP_GREATEREQUAL:
		return z >= stored;
	default:
		return true;
	}
}

// One channel's blend factor, in [0, 1].
static float ImmediateBlendFactor(DWORD factor, float src, float srcAlpha, float dst, float dstAlpha)
{
	switch (static_cast<D3DBLEND>(factor)) {
	case D3DBLEND_ZERO:
		return 0.0f;
	case D3DBLEND_ONE:
		return 1.0f;
	case D3DBLEND_SRCCOLOR:
		return src;
	case D3DBLEND_INVSRCCOLOR:
		return 1.0f - src;
	case D3DBLEND_SRCALPHA:
		return srcAlpha;
	case D3DBLEND_INVSRCALPHA:
		return 1.0f - srcAlpha;
	case D3DBLEND_DESTALPHA:
		return dstAlpha;
	case D3DBLEND_INVDESTALPHA:
		return 1.0f - dstAlpha;
	case D3DBLEND_DESTCOLOR:
		return dst;
	case D3DBLEND_INVDESTCOLOR:
		return 1.0f - dst;
	case D3DBLEND_SRCALPHASAT:
		return std::min(srcAlpha, 1.0f - dstAlpha);
	default:
		return 1.0f;
	}
}

struct ImmediateTexel {
	float r, g, b, a;
	bool discard;
};

// Stage 0's texel at (u, v), wrapped. A keyed texture's alpha-0 texels are
// the colour key: with COLORKEYENABLE they are not drawn (and do not bleed
// into a filtered neighbour), and the texels that are drawn are opaque.
template <typename State>
static ImmediateTexel SampleImmediateTexture(const State& state, float u, float v)
{
	auto texel = [&](int x, int y) {
		x %= state.texWidth;
		y %= state.texHeight;
		x += x < 0 ? state.texWidth : 0;
		y += y < 0 ? state.texHeight : 0;
		return state.texels + y * state.texPitch + x * 4;
	};
	float fx = u * state.texWidth;
	float fy = v * state.texHeight;
	ImmediateTexel out = {};
	if (!state.linear) {
		const Uint8* t = texel(static_cast<int>(std::floor(fx)), static_cast<int>(std::floor(fy)));
		out = {t[0] / 255.0f, t[1] / 255.0f, t[2] / 255.0f, t[3] / 255.0f, false};
		if (state.keyed) {
			out.discard = state.colorKeyEnable && t[3] == 0;
			out.a = 1.0f;
		}
		return out;
	}

	fx -= 0.5f;
	fy -= 0.5f;
	const float x0 = std::floor(fx), y0 = std::floor(fy);
	const float ax = fx - x0, ay = fy - y0;
	const int ix = static_cast<int>(x0), iy = static_cast<int>(y0);
	const Uint8* t[4] = {texel(ix, iy), texel(ix + 1, iy), texel(ix, iy + 1), texel(ix + 1, iy + 1)};
	const float w[4] = {(1 - ax) * (1 - ay), ax * (1 - ay), (1 - ax) * ay, ax * ay};
	const bool keyOut = state.keyed && state.colorKeyEnable;
	float coverage = 0.0f;
	for (int i = 0; i < 4; ++i) {
		float wi = w[i];
		if (keyOut) {
			wi *= t[i][3] / 255.0f;
		}
		out.r += wi * t[i][0];
		out.g += wi * t[i][1];
		out.b += wi * t[i][2];
		out.a += w[i] * t[i][3];
		coverage += wi;
	}
	if (keyOut) {
		if (coverage < 0.5f) {
			out.discard = true;
			return out;
		}
		const float scale = 1.0f / (coverage * 255.0f);
		out.r *= scale;
		out.g *= scale;
		out.b *= scale;
		out.a = 1.0f;
		return out;
	}
	out.r /= 255.0f;
	out.g /= 255.0f;
	out.b /= 255.0f;
	out.a = state.keyed ? 1.0f : out.a / 255.0f;
	return out;
}

// A texture stage argument. Stage 0's CURRENT is the diffuse colour.
static float ImmediateStageArg(DWORD arg, float diffuse, float texture, float diffuseAlpha, float textureAlpha)
{
	float value;
	const bool alpha = (arg & D3DTA_ALPHAREPLICATE) != 0;
	switch (arg & D3DTA_SELECTMASK) {
	case D3DTA_TEXTURE:
		value = alpha ? textureAlpha : texture;
		break;
	case D3DTA_TFACTOR:
		value = 1.0f;
		break;
	case D3DTA_SPECULAR:
		value = 0.0f;
		break;
	default: // DIFFUSE, CURRENT
		value = alpha ? diffuseAlpha : diffuse;
		break;
	}
	return (arg & D3DTA_COMPLEMENT) ? 1.0f - value : value;
}

static float ImmediateStageOp(DWORD op, float arg1, float arg2, float current)
{
	switch (static_cast<D3DTEXTUREOP>(op)) {
	case D3DTOP_DISABLE:
		return current;
	case D3DTOP_SELECTARG1:
		return arg1;
	case D3DTOP_SELECTARG2:
		return arg2;
	case D3DTOP_MODULATE:
		return arg1 * arg2;
	case D3DTOP_MODULATE2X:
		return std::min(1.0f, 2.0f * arg1 * arg2);
	case D3DTOP_MODULATE4X:
		return std::min(1.0f, 4.0f * arg1 * arg2);
	case D3DTOP_ADD:
		return std::min(1.0f, arg1 + arg2);
	default:
		return arg1 * arg2;
	}
}

// Edge-function rasterizer: samples at integer pixel coordinates, as the
// retained-mode rasterizer does, with a top-left rule so that pixels on an
// edge shared by two triangles are drawn once (which matters when blending
// is additive).
void Direct3DRMSoftwareRenderer::DrawImmediateTriangle(
	const SWLitVertex& v0,
	const SWLitVertex& v1,
	const SWLitVertex& v2,
	const ImmediatePixelState& state
)
{
	D3DRMVECTOR4D p[3];
	ProjectVertex(v0.position, p[0]);
	ProjectVertex(v1.position, p[1]);
	ProjectVertex(v2.position, p[2]);
	const SWLitVertex* lit[3] = {&v0, &v1, &v2};

	float area = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[2].x - p[0].x) * (p[1].y - p[0].y);
	if (area == 0.0f || !std::isfinite(area)) {
		return;
	}
	if (area < 0.0f) {
		std::swap(p[1], p[2]);
		std::swap(lit[1], lit[2]);
		area = -area;
	}

	int minX = std::max(m_clipMinX, static_cast<int>(std::ceil(std::min({p[0].x, p[1].x, p[2].x}))));
	int maxX = std::min(m_clipMaxX, static_cast<int>(std::floor(std::max({p[0].x, p[1].x, p[2].x}))));
	int minY = std::max(m_clipMinY, static_cast<int>(std::ceil(std::min({p[0].y, p[1].y, p[2].y}))));
	int maxY = std::min(m_clipMaxY, static_cast<int>(std::floor(std::max({p[0].y, p[1].y, p[2].y}))));
	if (minX > maxX || minY > maxY) {
		return;
	}

	// Edge k runs from vertex k+1 to k+2 and weighs vertex k.
	struct Edge {
		float ax, ay, dx, dy;
		bool topLeft;
		float At(float x, float y) const { return dx * (y - ay) - dy * (x - ax); }
	};
	Edge edges[3];
	for (int k = 0; k < 3; ++k) {
		const D3DRMVECTOR4D& a = p[(k + 1) % 3];
		const D3DRMVECTOR4D& b = p[(k + 2) % 3];
		edges[k] = {a.x, a.y, b.x - a.x, b.y - a.y, false};
		edges[k].topLeft = edges[k].dy < 0.0f || (edges[k].dy == 0.0f && edges[k].dx > 0.0f);
	}

	float invArea = 1.0f / area;
	const float diffuseAlpha = lit[0]->color.a / 255.0f;
	float oneOverW[3], uOverW[3], vOverW[3];
	for (int k = 0; k < 3; ++k) {
		oneOverW[k] = 1.0f / p[k].w;
		uOverW[k] = lit[k]->texCoord.u * oneOverW[k];
		vOverW[k] = lit[k]->texCoord.v * oneOverW[k];
	}
	Uint8* pixels = static_cast<Uint8*>(m_renderedImage->pixels);
	int pitch = m_renderedImage->pitch;

	for (int y = minY; y <= maxY; ++y) {
		Uint8* row = pixels + y * pitch;
		float* zRow = &m_zBuffer[y * m_width];
		for (int x = minX; x <= maxX; ++x) {
			float w[3];
			bool inside = true;
			for (int k = 0; k < 3 && inside; ++k) {
				float e = edges[k].At(static_cast<float>(x), static_cast<float>(y));
				inside = e > 0.0f || (e == 0.0f && edges[k].topLeft);
				w[k] = e * invArea;
			}
			if (!inside) {
				continue;
			}

			float z = w[0] * p[0].z + w[1] * p[1].z + w[2] * p[2].z;
			if (state.zEnable && !ImmediateDepthTest(state.zFunc, z, zRow[x])) {
				continue;
			}

			float r = (w[0] * lit[0]->color.r + w[1] * lit[1]->color.r + w[2] * lit[2]->color.r) / 255.0f;
			float g = (w[0] * lit[0]->color.g + w[1] * lit[1]->color.g + w[2] * lit[2]->color.g) / 255.0f;
			float b = (w[0] * lit[0]->color.b + w[1] * lit[1]->color.b + w[2] * lit[2]->color.b) / 255.0f;
			float srcAlpha = diffuseAlpha;

			if (state.texels) {
				// Perspective-correct texture coordinates.
				float oow = w[0] * oneOverW[0] + w[1] * oneOverW[1] + w[2] * oneOverW[2];
				float u = (w[0] * uOverW[0] + w[1] * uOverW[1] + w[2] * uOverW[2]) / oow;
				float v = (w[0] * vOverW[0] + w[1] * vOverW[1] + w[2] * vOverW[2]) / oow;
				ImmediateTexel t = SampleImmediateTexture(state, u, v);
				if (t.discard) {
					continue;
				}
				auto stage = [&](DWORD op, DWORD arg1, DWORD arg2, float diffuse, float texture) {
					return ImmediateStageOp(
						op,
						ImmediateStageArg(arg1, diffuse, texture, diffuseAlpha, t.a),
						ImmediateStageArg(arg2, diffuse, texture, diffuseAlpha, t.a),
						diffuse
					);
				};
				r = stage(state.colorOp, state.colorArg1, state.colorArg2, r, t.r);
				g = stage(state.colorOp, state.colorArg1, state.colorArg2, g, t.g);
				b = stage(state.colorOp, state.colorArg1, state.colorArg2, b, t.b);
				srcAlpha = stage(state.alphaOp, state.alphaArg1, state.alphaArg2, diffuseAlpha, t.a);
			}
			if (state.specular) {
				auto spec = [&](Uint8 SDL_Color::*c) {
					return (w[0] * (lit[0]->specular.*c) + w[1] * (lit[1]->specular.*c) + w[2] * (lit[2]->specular.*c)) /
						   255.0f;
				};
				r = std::min(1.0f, r + spec(&SDL_Color::r));
				g = std::min(1.0f, g + spec(&SDL_Color::g));
				b = std::min(1.0f, b + spec(&SDL_Color::b));
			}
			if (state.fog != ImmediatePixelState::Fog::None) {
				float f;
				if (state.fog == ImmediatePixelState::Fog::Vertex) {
					f = (w[0] * lit[0]->specular.a + w[1] * lit[1]->specular.a + w[2] * lit[2]->specular.a) / 255.0f;
				}
				else {
					float oow = w[0] * oneOverW[0] + w[1] * oneOverW[1] + w[2] * oneOverW[2];
					f = FogFactor(state.fogMode, state.fogStart, state.fogEnd, state.fogDensity, 1.0f / oow);
				}
				r = f * r + (1.0f - f) * state.fogR;
				g = f * g + (1.0f - f) * state.fogG;
				b = f * b + (1.0f - f) * state.fogB;
			}

			// The render target stands for the display surface, which has no
			// alpha channel: destination alpha reads as one and stays opaque.
			Uint8* px = row + x * 4;
			if (state.blend) {
				float dr = px[0] / 255.0f, dg = px[1] / 255.0f, db = px[2] / 255.0f, da = 1.0f;
				float outR = r * ImmediateBlendFactor(state.srcBlend, r, srcAlpha, dr, da) +
							 dr * ImmediateBlendFactor(state.destBlend, r, srcAlpha, dr, da);
				float outG = g * ImmediateBlendFactor(state.srcBlend, g, srcAlpha, dg, da) +
							 dg * ImmediateBlendFactor(state.destBlend, g, srcAlpha, dg, da);
				float outB = b * ImmediateBlendFactor(state.srcBlend, b, srcAlpha, db, da) +
							 db * ImmediateBlendFactor(state.destBlend, b, srcAlpha, db, da);
				px[0] = static_cast<Uint8>(SWClampChannel(outR * 255.0f + 0.5f));
				px[1] = static_cast<Uint8>(SWClampChannel(outG * 255.0f + 0.5f));
				px[2] = static_cast<Uint8>(SWClampChannel(outB * 255.0f + 0.5f));
				px[3] = 255;
			}
			else {
				px[0] = static_cast<Uint8>(SWClampChannel(r * 255.0f + 0.5f));
				px[1] = static_cast<Uint8>(SWClampChannel(g * 255.0f + 0.5f));
				px[2] = static_cast<Uint8>(SWClampChannel(b * 255.0f + 0.5f));
				px[3] = 255;
			}
			if (state.zEnable && state.zWrite) {
				zRow[x] = z;
			}
		}
	}
}

void Direct3DRMSoftwareRenderer::LightImmediateVertex(
	const D3DVECTOR& position,
	const D3DVECTOR& oNormal,
	const D3DMATERIAL& material,
	const D3DVECTOR& eye,
	bool specular,
	SDL_Color& color,
	SDL_Color& specularColor
)
{
	D3DVECTOR normal = Normalize(TransformNormal(oNormal, m_normalMatrix));
	D3DVECTOR worldPos = TransformPoint(position, m_worldMatrix);
	D3DVECTOR toEye = Normalize(Subtract(eye, worldPos));

	FColor diffuse = {
		material.emissive.r + m_ambientR * material.ambient.r,
		material.emissive.g + m_ambientG * material.ambient.g,
		material.emissive.b + m_ambientB * material.ambient.b,
		0
	};
	FColor spec = {0, 0, 0, 0};
	for (const PreparedLight& light : m_preparedLights) {
		D3DVECTOR lightVec = light.vec;
		float reach = 1.0f;
		if (light.positional) {
			reach = LightReach(light, worldPos, lightVec);
			if (reach <= 0.0f) {
				continue;
			}
		}
		float dotNL = DotProduct(normal, lightVec);
		if (dotNL <= 0.0f) {
			continue;
		}
		diffuse.r += reach * dotNL * light.color.r * material.diffuse.r;
		diffuse.g += reach * dotNL * light.color.g * material.diffuse.g;
		diffuse.b += reach * dotNL * light.color.b * material.diffuse.b;
		if (specular && material.power > 0.0f) {
			D3DVECTOR halfway = Normalize({lightVec.x + toEye.x, lightVec.y + toEye.y, lightVec.z + toEye.z});
			float dotNH = DotProduct(normal, halfway);
			if (dotNH > 0.0f) {
				float power = reach * std::pow(dotNH, material.power);
				spec.r += power * light.color.r * material.specular.r;
				spec.g += power * light.color.g * material.specular.g;
				spec.b += power * light.color.b * material.specular.b;
			}
		}
	}

	auto toByte = [](float c) { return static_cast<Uint8>(std::min(255.0f, std::max(0.0f, c * 255.0f + 0.5f))); };
	color = {toByte(diffuse.r), toByte(diffuse.g), toByte(diffuse.b), toByte(material.diffuse.a)};
	specularColor = {toByte(spec.r), toByte(spec.g), toByte(spec.b), 0};
}

// Each vertex is lit as Direct3D lights it, from the scene's lights and the
// current material; texture stage 0 then combines that with the texture.
HRESULT Direct3DRMSoftwareRenderer::SubmitImmediate(const ImmediateDraw& draw)
{
	if (!m_renderedImage || !m_renderedImage->pixels) {
		return DDERR_GENERIC;
	}
	const ImmediateState& st = *draw.state;

	D3DRMMATRIX4D modelView;
	MultiplyMatrix(modelView, draw.world, draw.view);
	memcpy(m_worldMatrix, draw.world, sizeof(D3DRMMATRIX4D));
	NormalMatrix(m_normalMatrix, draw.world);
	m_specular = false;

	auto toByte = [](float c) { return static_cast<Uint8>(std::min(255.0f, std::max(0.0f, c * 255.0f + 0.5f))); };
	Appearance appearance = {};
	appearance.color = {
		toByte(draw.material.diffuse.r),
		toByte(draw.material.diffuse.g),
		toByte(draw.material.diffuse.b),
		toByte(draw.material.diffuse.a)
	};
	appearance.shininess = draw.material.power;
	appearance.textureId = NO_TEXTURE_ID;
	appearance.flat = st.Render(D3DRENDERSTATE_SHADEMODE) == static_cast<DWORD>(D3DSHADE_FLAT);

	ImmediatePixelState pixelState;
	pixelState.zEnable = st.Render(D3DRENDERSTATE_ZENABLE) != 0;
	pixelState.zWrite = st.Render(D3DRENDERSTATE_ZWRITEENABLE) != 0;
	pixelState.zFunc = st.Render(D3DRENDERSTATE_ZFUNC);
	pixelState.blend = st.Render(D3DRENDERSTATE_ALPHABLENDENABLE) != 0;
	pixelState.srcBlend = st.Render(D3DRENDERSTATE_SRCBLEND);
	pixelState.destBlend = st.Render(D3DRENDERSTATE_DESTBLEND);
	pixelState.texels = nullptr;
	if (draw.texture) {
		Uint32 textureId = GetTextureId(draw.texture, false, 1.0f, 1.0f);
		const TextureCache& cache = m_textures[textureId];
		if (cache.cached && st.textureStageState[0][static_cast<DWORD>(D3DTSS_COLOROP)] !=
								static_cast<DWORD>(D3DTOP_DISABLE)) {
			pixelState.texels = static_cast<const Uint8*>(cache.cached->pixels);
			pixelState.texWidth = cache.cached->w;
			pixelState.texHeight = cache.cached->h;
			pixelState.texPitch = cache.cached->pitch;
			pixelState.keyed = cache.keyed;
		}
	}
	pixelState.colorKeyEnable = st.Render(D3DRENDERSTATE_COLORKEYENABLE) != 0;
	pixelState.linear = st.Render(D3DRENDERSTATE_TEXTUREMAG) != static_cast<DWORD>(D3DFILTER_NEAREST);
	const DWORD* tss = st.textureStageState[0];
	pixelState.colorOp = tss[static_cast<DWORD>(D3DTSS_COLOROP)];
	pixelState.colorArg1 = tss[static_cast<DWORD>(D3DTSS_COLORARG1)];
	pixelState.colorArg2 = tss[static_cast<DWORD>(D3DTSS_COLORARG2)];
	pixelState.alphaOp = tss[static_cast<DWORD>(D3DTSS_ALPHAOP)];
	pixelState.alphaArg1 = tss[static_cast<DWORD>(D3DTSS_ALPHAARG1)];
	pixelState.alphaArg2 = tss[static_cast<DWORD>(D3DTSS_ALPHAARG2)];
	pixelState.specular = st.Render(D3DRENDERSTATE_SPECULARENABLE) != 0;
	pixelState.fog = ImmediatePixelState::Fog::None;
	DWORD vertexFogMode = st.lightState[static_cast<DWORD>(D3DLIGHTSTATE_FOGMODE)];
	float vertexFogStart = FloatState(st.lightState[static_cast<DWORD>(D3DLIGHTSTATE_FOGSTART)]);
	float vertexFogEnd = FloatState(st.lightState[static_cast<DWORD>(D3DLIGHTSTATE_FOGEND)]);
	float vertexFogDensity = FloatState(st.lightState[static_cast<DWORD>(D3DLIGHTSTATE_FOGDENSITY)]);
	if (st.Render(D3DRENDERSTATE_FOGENABLE)) {
		D3DCOLOR fogColor = st.Render(D3DRENDERSTATE_FOGCOLOR);
		pixelState.fogR = ((fogColor >> 16) & 0xff) / 255.0f;
		pixelState.fogG = ((fogColor >> 8) & 0xff) / 255.0f;
		pixelState.fogB = (fogColor & 0xff) / 255.0f;
		pixelState.fogMode = st.Render(D3DRENDERSTATE_FOGTABLEMODE);
		if (pixelState.fogMode != static_cast<DWORD>(D3DFOG_NONE)) {
			pixelState.fog = ImmediatePixelState::Fog::Table;
			pixelState.fogStart = FloatState(st.Render(D3DRENDERSTATE_FOGTABLESTART));
			pixelState.fogEnd = FloatState(st.Render(D3DRENDERSTATE_FOGTABLEEND));
			pixelState.fogDensity = FloatState(st.Render(D3DRENDERSTATE_FOGTABLEDENSITY));
		}
		else if (vertexFogMode != static_cast<DWORD>(D3DFOG_NONE)) {
			pixelState.fog = ImmediatePixelState::Fog::Vertex;
		}
	}
	const DWORD cull = st.Render(D3DRENDERSTATE_CULLMODE);

	// The eye in world space, for the specular half vector (LOCALVIEWER).
	D3DVECTOR eye = {0, 0, 0};
	{
		const D3DRMMATRIX4D& v = draw.view;
		D3DVECTOR t = {v[3][0], v[3][1], v[3][2]};
		eye = {
			-(t.x * v[0][0] + t.y * v[0][1] + t.z * v[0][2]),
			-(t.x * v[1][0] + t.y * v[1][1] + t.z * v[1][2]),
			-(t.x * v[2][0] + t.y * v[2][1] + t.z * v[2][2])
		};
	}

	const DWORD vertexCount = draw.vertexCount;
	m_transformedPositions.resize(vertexCount);
	for (DWORD i = 0; i < vertexCount; ++i) {
		const D3DVERTEX& v = draw.vertices[i];
		m_transformedPositions[i] = TransformPoint({v.x, v.y, v.z}, modelView);
	}
	m_vertexColors.resize(vertexCount);
	m_vertexSpeculars.resize(vertexCount);
	m_vertexLit.assign(vertexCount, 0);

	auto lightVertex = [&](WORD i) {
		if (!m_vertexLit[i]) {
			m_vertexLit[i] = 1;
			const D3DVERTEX& v = draw.vertices[i];
			LightImmediateVertex(
				{v.x, v.y, v.z},
				{v.nx, v.ny, v.nz},
				draw.material,
				eye,
				pixelState.specular,
				m_vertexColors[i],
				m_vertexSpeculars[i]
			);
			if (pixelState.fog == ImmediatePixelState::Fog::Vertex) {
				float f = FogFactor(
					vertexFogMode,
					vertexFogStart,
					vertexFogEnd,
					vertexFogDensity,
					m_transformedPositions[i].z
				);
				m_vertexSpeculars[i].a = static_cast<Uint8>(f * 255.0f + 0.5f);
			}
		}
	};

	for (DWORD t = 0; t + 2 < draw.indexCount; t += 3) {
		const WORD i0 = draw.indices[t];
		const WORD i1 = draw.indices[t + 1];
		const WORD i2 = draw.indices[t + 2];
		const D3DVECTOR& p0 = m_transformedPositions[i0];
		const D3DVECTOR& p1 = m_transformedPositions[i1];
		const D3DVECTOR& p2 = m_transformedPositions[i2];

		// Counter-clockwise on screen is IsBackface's test (D3DCULL_CCW).
		if (cull == static_cast<DWORD>(D3DCULL_CCW) && IsBackface(p0, p1, p2)) {
			continue;
		}
		if (cull == static_cast<DWORD>(D3DCULL_CW) && IsBackface(p0, p2, p1)) {
			continue;
		}

		lightVertex(i0);
		SDL_Color c0 = m_vertexColors[i0], c1, c2;
		if (appearance.flat) {
			c1 = c2 = c0; // flat shading takes the first vertex's colour
		}
		else {
			lightVertex(i1);
			lightVertex(i2);
			c1 = m_vertexColors[i1];
			c2 = m_vertexColors[i2];
		}
		SDL_Color s0 = m_vertexSpeculars[i0], s1, s2;
		if (appearance.flat) {
			s1 = s2 = s0;
		}
		else {
			s1 = m_vertexSpeculars[i1];
			s2 = m_vertexSpeculars[i2];
		}
		SWLitVertex tri[3] = {
			{p0, {draw.vertices[i0].tu, draw.vertices[i0].tv}, c0, s0},
			{p1, {draw.vertices[i1].tu, draw.vertices[i1].tv}, c1, s1},
			{p2, {draw.vertices[i2].tu, draw.vertices[i2].tv}, c2, s2},
		};
		DrawImmediateClipped(tri, pixelState);
	}
	return DD_OK;
}

// ---- viewports and clears ---------------------------------------------------

// A rectangle in the game's coordinates as render pixels, scaled and
// offset as the 2D image is, and clipped to the target.
SDL_Rect Direct3DRMSoftwareRenderer::ToRenderPixels(const D3DRECT& rect) const
{
	float scale = m_viewportTransform.scale;
	int x1 = static_cast<int>(std::lround(rect.x1 * scale + m_viewportTransform.offsetX));
	int y1 = static_cast<int>(std::lround(rect.y1 * scale + m_viewportTransform.offsetY));
	int x2 = static_cast<int>(std::lround(rect.x2 * scale + m_viewportTransform.offsetX));
	int y2 = static_cast<int>(std::lround(rect.y2 * scale + m_viewportTransform.offsetY));
	x1 = std::max(0, std::min(x1, m_width));
	y1 = std::max(0, std::min(y1, m_height));
	x2 = std::max(x1, std::min(x2, m_width));
	y2 = std::max(y1, std::min(y2, m_height));
	return {x1, y1, x2 - x1, y2 - y1};
}

void Direct3DRMSoftwareRenderer::SetViewportRect(const D3DRECT* rect)
{
	// The whole-device viewport keeps the whole target (its projection
	// already accounts for the window's aspect).
	SDL_Rect pixels = rect ? ToRenderPixels(*rect) : SDL_Rect{0, 0, m_width, m_height};
	m_viewportX = static_cast<float>(pixels.x);
	m_viewportY = static_cast<float>(pixels.y);
	m_viewportW = static_cast<float>(pixels.w);
	m_viewportH = static_cast<float>(pixels.h);
	m_clipMinX = pixels.x;
	m_clipMinY = pixels.y;
	m_clipMaxX = pixels.x + pixels.w - 1;
	m_clipMaxY = pixels.y + pixels.h - 1;
}

HRESULT Direct3DRMSoftwareRenderer::ClearRect(const D3DRECT& rect, DWORD flags, D3DCOLOR color, float z)
{
	if (!m_renderedImage) {
		return DDERR_GENERIC;
	}
	SDL_Rect pixels = ToRenderPixels(rect);
	if (pixels.w <= 0 || pixels.h <= 0) {
		return DD_OK;
	}
	if (flags & D3DCLEAR_TARGET) {
		const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(m_renderedImage->format);
		Uint32 fill = SDL_MapRGBA(
			details,
			SDL_GetSurfacePalette(m_renderedImage),
			(color >> 16) & 0xff,
			(color >> 8) & 0xff,
			color & 0xff,
			0xff
		);
		SDL_FillSurfaceRect(m_renderedImage, &pixels, fill);
	}
	if (flags & D3DCLEAR_ZBUFFER) {
		m_clientClearsZ = true;
		for (int y = pixels.y; y < pixels.y + pixels.h; ++y) {
			std::fill_n(&m_zBuffer[y * m_width + pixels.x], pixels.w, z);
		}
	}
	return DD_OK;
}
