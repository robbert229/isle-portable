#pragma once

#include "miniwin/d3d.h"

#include <stdlib.h> // abort // FIXME: remove

// --- Defines and Macros ---
#define D3DRM_OK DD_OK
#define MAXSHORT ((short) 0x7fff)
#define SUCCEEDED(hr) ((hr) >= D3DRM_OK)
#define D3DRMERR_NOTFOUND MAKE_DDHRESULT(785)

// --- Typedefs ---
typedef float D3DVAL;
typedef void* LPD3DRM_APPDATA;
typedef DWORD D3DRMGROUPINDEX;
typedef DWORD D3DCOLOR, *LPD3DCOLOR;
typedef float D3DVALUE, *LPD3DVALUE;

typedef struct IDirect3DRMObject* LPDIRECT3DRMOBJECT;
typedef void (*D3DRMOBJECTCALLBACK)(LPDIRECT3DRMOBJECT obj, LPVOID arg);

// --- Enums ---
#define D3DRMCOMBINE_REPLACE D3DRMCOMBINETYPE::REPLACE
#define D3DRMCOMBINE_BEFORE D3DRMCOMBINETYPE::BEFORE
#define D3DRMCOMBINE_AFTER D3DRMCOMBINETYPE::AFTER
enum class D3DRMCOMBINETYPE {
	REPLACE,
	BEFORE,
	AFTER
};
typedef D3DRMCOMBINETYPE* LPD3DRMCOMBINETYPE;

#define D3DRMPALETTE_READONLY D3DRMPALETTEFLAGS::READONLY
enum class D3DRMPALETTEFLAGS {
	READONLY = 1
};
typedef D3DRMPALETTEFLAGS* LPD3DRMPALETTEFLAGS;

#define D3DRMTEXTURE_LINEAR D3DRMTEXTUREQUALITY::LINEAR
enum class D3DRMTEXTUREQUALITY {
	LINEAR = 1
};
typedef D3DRMTEXTUREQUALITY* LPD3DRMTEXTUREQUALITY;

#define D3DRMRENDERMODE_BLENDEDTRANSPARENCY D3DRMRENDERMODE::BLENDEDTRANSPARENCY
enum class D3DRMRENDERMODE {
	BLENDEDTRANSPARENCY = 1
};

#define D3DRMMAP_NONE D3DRMMAPPING::NONE
#define D3DRMMAP_PERSPCORRECT D3DRMMAPPING::PERSPCORRECT
enum class D3DRMMAPPING {
	NONE = 0,
	PERSPCORRECT = 4
};
typedef D3DRMMAPPING* LPD3DRMMAPPING;

#define D3DRMLIGHT_AMBIENT D3DRMLIGHTTYPE::AMBIENT
#define D3DRMLIGHT_POINT D3DRMLIGHTTYPE::POINT
#define D3DRMLIGHT_SPOT D3DRMLIGHTTYPE::SPOT
#define D3DRMLIGHT_DIRECTIONAL D3DRMLIGHTTYPE::DIRECTIONAL
#define D3DRMLIGHT_PARALLELPOINT D3DRMLIGHTTYPE::PARALLELPOINT
enum class D3DRMLIGHTTYPE {
	AMBIENT,
	POINT,
	SPOT,
	DIRECTIONAL,
	PARALLELPOINT
};
typedef D3DRMLIGHTTYPE* LPD3DRMLIGHTTYPE;

#define D3DRMMATERIAL_FROMMESH D3DRMMATERIALMODE::FROMMESH
#define D3DRMMATERIAL_FROMPARENT D3DRMMATERIALMODE::FROMPARENT
#define D3DRMMATERIAL_FROMFRAME D3DRMMATERIALMODE::FROMFRAME
enum class D3DRMMATERIALMODE {
	FROMMESH,
	FROMPARENT,
	FROMFRAME
};

#define D3DRMRENDER_WIREFRAME D3DRMRENDERQUALITY::WIREFRAME
#define D3DRMRENDER_UNLITFLAT D3DRMRENDERQUALITY::UNLITFLAT
#define D3DRMRENDER_FLAT D3DRMRENDERQUALITY::FLAT
#define D3DRMRENDER_GOURAUD D3DRMRENDERQUALITY::GOURAUD
#define D3DRMRENDER_PHONG D3DRMRENDERQUALITY::PHONG

enum class D3DRMRENDERQUALITY {
	WIREFRAME = 64,
	UNLITFLAT = 128,
	FLAT = 136,
	GOURAUD = 137,
	PHONG = 138
};
typedef D3DRMRENDERQUALITY* LPD3DRMRENDERQUALITY;

#define D3DRMPROJECT_PERSPECTIVE D3DRMPROJECTIONTYPE::PERSPECTIVE
#define D3DRMPROJECT_ORTHOGRAPHIC D3DRMPROJECTIONTYPE::ORTHOGRAPHIC
enum class D3DRMPROJECTIONTYPE {
	PERSPECTIVE,
	ORTHOGRAPHIC
};
typedef D3DRMPROJECTIONTYPE* LPD3DRMPROJECTIONTYPE;

#define D3DRMFOG_LINEAR D3DRMFOGMODE::LINEAR
#define D3DRMFOG_EXPONENTIAL D3DRMFOGMODE::EXPONENTIAL
#define D3DRMFOG_EXPONENTIALSQUARED D3DRMFOGMODE::EXPONENTIALSQUARED
enum class D3DRMFOGMODE {
	LINEAR,
	EXPONENTIAL,
	EXPONENTIALSQUARED
};

#define D3DRMSORT_FROMPARENT D3DRMSORTMODE::FROMPARENT
#define D3DRMSORT_NONE D3DRMSORTMODE::NONE
#define D3DRMSORT_FRONTTOBACK D3DRMSORTMODE::FRONTTOBACK
#define D3DRMSORT_BACKTOFRONT D3DRMSORTMODE::BACKTOFRONT
enum class D3DRMSORTMODE {
	FROMPARENT,
	NONE,
	FRONTTOBACK,
	BACKTOFRONT
};

#define D3DRMCONSTRAIN_Z D3DRMFRAMECONSTRAINT::Z
#define D3DRMCONSTRAIN_Y D3DRMFRAMECONSTRAINT::Y
#define D3DRMCONSTRAIN_X D3DRMFRAMECONSTRAINT::X
enum class D3DRMFRAMECONSTRAINT {
	Z,
	Y,
	X
};

typedef DWORD D3DRMLOADOPTIONS;
#define D3DRMLOAD_FROMFILE 0x000
#define D3DRMLOAD_FROMRESOURCE 0x001
#define D3DRMLOAD_FROMMEMORY 0x002
#define D3DRMLOAD_BYNAME 0x010
#define D3DRMLOAD_BYPOSITION 0x020
#define D3DRMLOAD_BYGUID 0x040
#define D3DRMLOAD_FIRST 0x080
#define D3DRMLOAD_INSTANCEBYREFERENCE 0x100
#define D3DRMLOAD_INSTANCEBYCOPYING 0x200

#define D3DRMUSERVISUAL_CANSEE D3DRMUSERVISUALREASON::CANSEE
#define D3DRMUSERVISUAL_RENDER D3DRMUSERVISUALREASON::RENDER
enum class D3DRMUSERVISUALREASON {
	CANSEE,
	RENDER
};

#define D3DRMFOGMETHOD_VERTEX 0x1
#define D3DRMFOGMETHOD_TABLE 0x2
#define D3DRMFOGMETHOD_ANY 0x4

#define D3DRMFPTF_ALPHA 0x1
#define D3DRMFPTF_NOALPHA 0x2
#define D3DRMFPTF_PALETTIZED 0x4
#define D3DRMFPTF_ANYDEPTH 0x8

#define D3DRMCLEAR_TARGET 0x1
#define D3DRMCLEAR_ZBUFFER 0x2
#define D3DRMCLEAR_DIRTYRECTS 0x4
#define D3DRMCLEAR_ALL (D3DRMCLEAR_TARGET | D3DRMCLEAR_ZBUFFER | D3DRMCLEAR_DIRTYRECTS)

// --- GUIDs ---
DEFINE_GUID(IID_IDirect3DRM, 0x2bc49361, 0x8327, 0x11cf, 0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRM2, 0x4516ecc8, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRM3, 0x4516ec83, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMDevice, 0xe9e19280, 0x6e05, 0x11cf, 0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMDevice2, 0x4516ec78, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMDevice3, 0x549f498b, 0xbfeb, 0x11d1, 0x8e, 0xd8, 0x00, 0xa0, 0xc9, 0x67, 0xa4, 0x82);
DEFINE_GUID(IID_IDirect3DRMWinDevice, 0xc5016cc0, 0xd273, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMFrame, 0xeb16cb03, 0xd271, 0x11ce, 0xac, 0x48, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMFrame2, 0xc3dfbd60, 0x3988, 0x11d0, 0x9e, 0xc2, 0x00, 0x00, 0xc0, 0x29, 0x1a, 0xc3);
DEFINE_GUID(IID_IDirect3DRMFrame3, 0xff6b7f70, 0xa40e, 0x11d1, 0x91, 0xf9, 0x00, 0x00, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMViewport, 0xeb16cb02, 0xd271, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMViewport2, 0x4a1b1be6, 0xbfed, 0x11d1, 0x8e, 0xd8, 0x00, 0xa0, 0xc9, 0x67, 0xa4, 0x82);
DEFINE_GUID(IID_IDirect3DRMMesh, 0xa3a80d01, 0x6e12, 0x11cf, 0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMeshBuilder, 0xa3a80d02, 0x6e12, 0x11cf, 0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMeshBuilder2, 0x4516ec77, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMMeshBuilder3, 0x4516ec82, 0x8f20, 0x11d0, 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3);
DEFINE_GUID(IID_IDirect3DRMAnimationSet2, 0xff6b7f79, 0xa40e, 0x11d1, 0x91, 0xf9, 0x00, 0x00, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMUserVisual, 0x59163de0, 0x6d43, 0x11cf, 0xac, 0x4a, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMTexture, 0xeb16cb09, 0xd271, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMTexture2, 0x120f30c0, 0x1629, 0x11d0, 0x94, 0x1c, 0x00, 0x80, 0xc8, 0x0c, 0xfa, 0x7b);
DEFINE_GUID(IID_IDirect3DRMTexture3, 0xff6b7f73, 0xa40e, 0x11d1, 0x91, 0xf9, 0x00, 0x00, 0xf8, 0x75, 0x8e, 0x66);
DEFINE_GUID(IID_IDirect3DRMMaterial, 0xeb16cb0b, 0xd271, 0x11ce, 0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1);
DEFINE_GUID(IID_IDirect3DRMMaterial2, 0xff6b7f75, 0xa40e, 0x11d1, 0x91, 0xf9, 0x00, 0x00, 0xf8, 0x75, 0x8e, 0x66);

// --- Structs ---
typedef struct D3DRMVECTOR4D {
	float x, y, z, w;
} D3DRMVECTOR4D;

typedef struct D3DRMPALETTEENTRY {
	unsigned char red, green, blue;
	D3DRMPALETTEFLAGS flags;
} D3DRMPALETTEENTRY;

typedef struct D3DRMIMAGE {
	DWORD width, height, depth, bytes_per_line;
	DWORD red_mask, green_mask, blue_mask, alpha_mask;
	DWORD palette_size;
	D3DRMPALETTEENTRY* palette;
	void* buffer1;
	void* buffer2;
	void* data;
	int rgb;
	int aspectx, aspecty;
	DWORD format;
} D3DRMIMAGE;

typedef D3DVALUE D3DRMMATRIX4D[4][4];

struct D3DRMLOADMEMORY {
	LPVOID lpMemory;
	DWORD dSize;
};
typedef D3DRMLOADMEMORY* LPD3DRMLOADMEMORY;

struct D3DRMBOX {
	D3DVECTOR min;
	D3DVECTOR max;
};

struct TexCoord {
	float u, v;
};

struct D3DRMVERTEX {
	D3DVECTOR position;
	D3DVECTOR normal;
	union {
		TexCoord texCoord;
		struct {
			D3DVALUE tu, tv;
		};
	};
};

struct IDirect3DRMObject : virtual public IUnknown {
	virtual HRESULT AddDestroyCallback(D3DRMOBJECTCALLBACK callback, void* arg) = 0;
	virtual HRESULT DeleteDestroyCallback(D3DRMOBJECTCALLBACK callback, void* arg) = 0;
	virtual HRESULT SetAppData(LPD3DRM_APPDATA appData) = 0;
	virtual LPVOID GetAppData() = 0;
	virtual HRESULT SetName(const char* name) = 0;
	virtual HRESULT GetName(DWORD* size, char* name) = 0;
};

struct IDirect3DRMVisual : public IDirect3DRMObject {};
typedef IDirect3DRMVisual* LPDIRECT3DRMVISUAL;

struct IDirect3DRMTexture : public IDirect3DRMVisual {
	virtual HRESULT Changed(BOOL pixels, BOOL palette) = 0;
};
typedef IDirect3DRMTexture* LPDIRECT3DRMTEXTURE;

struct IDirect3DRMTexture2 : public IDirect3DRMTexture {};
typedef IDirect3DRMTexture2* LPDIRECT3DRMTEXTURE2;

struct IDirect3DRMTexture3 : public IDirect3DRMTexture2 {
	virtual D3DRMIMAGE* GetImage() = 0;
	virtual HRESULT SetDecalTransparency(BOOL transparency) = 0;
	virtual HRESULT SetDecalTransparentColor(D3DCOLOR color) = 0;
	virtual HRESULT GenerateMIPMap(DWORD flags) = 0;
};
typedef IDirect3DRMTexture3* LPDIRECT3DRMTEXTURE3;

typedef HRESULT (*D3DRMLOADTEXTURE3CALLBACK)(char* name, void* arg, IDirect3DRMTexture3** texture);

struct IDirect3DRMMaterial : public IDirect3DRMObject {
	virtual D3DVALUE GetPower() = 0;
};
typedef IDirect3DRMMaterial *LPDIRECT3DRMMATERIAL, **LPLPDIRECT3DRMMATERIAL;

struct IDirect3DRMMaterial2 : public IDirect3DRMMaterial {
	virtual HRESULT SetSpecular(D3DVALUE r, D3DVALUE g, D3DVALUE b) = 0;
	virtual HRESULT SetEmissive(D3DVALUE r, D3DVALUE g, D3DVALUE b) = 0;
};
typedef IDirect3DRMMaterial2* LPDIRECT3DRMMATERIAL2;

struct IDirect3DRMMesh : public IDirect3DRMVisual {
	virtual HRESULT Clone(int flags, GUID iid, void** object) = 0;
	virtual HRESULT Scale(D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) = 0;
	virtual HRESULT GetBox(D3DRMBOX* box) = 0;
	virtual HRESULT AddGroup(
		unsigned int vertexCount,
		unsigned int faceCount,
		unsigned int vertexPerFace,
		unsigned int* faceBuffer,
		D3DRMGROUPINDEX* groupIndex
	) = 0;
	virtual HRESULT GetGroup(
		D3DRMGROUPINDEX groupIndex,
		unsigned int* vertexCount,
		unsigned int* faceCount,
		unsigned int* vertexPerFace,
		DWORD* indexCount,
		unsigned int* indices
	) = 0;
	virtual DWORD GetGroupCount() = 0;
	virtual HRESULT SetGroupColor(D3DRMGROUPINDEX groupIndex, D3DCOLOR color) = 0;
	virtual HRESULT SetGroupColorRGB(D3DRMGROUPINDEX groupIndex, float r, float g, float b) = 0;
	virtual HRESULT SetGroupTexture(D3DRMGROUPINDEX groupIndex, IDirect3DRMTexture* texture) = 0;
	virtual HRESULT SetGroupMaterial(D3DRMGROUPINDEX groupIndex, IDirect3DRMMaterial* material) = 0;
	virtual HRESULT SetGroupMapping(D3DRMGROUPINDEX groupIndex, D3DRMMAPPING mapping) = 0;
	virtual HRESULT SetGroupQuality(D3DRMGROUPINDEX groupIndex, D3DRMRENDERQUALITY quality) = 0;
	virtual HRESULT SetVertices(D3DRMGROUPINDEX groupIndex, int offset, int count, D3DRMVERTEX* vertices) = 0;
	virtual HRESULT GetGroupTexture(D3DRMGROUPINDEX groupIndex, LPDIRECT3DRMTEXTURE* texture) = 0;
	virtual D3DRMMAPPING GetGroupMapping(D3DRMGROUPINDEX groupIndex) = 0;
	virtual HRESULT GetGroupMaterial(D3DRMGROUPINDEX groupIndex, LPDIRECT3DRMMATERIAL* material) = 0;
	virtual D3DRMRENDERQUALITY GetGroupQuality(D3DRMGROUPINDEX groupIndex) = 0;
	virtual D3DCOLOR GetGroupColor(D3DRMGROUPINDEX index) = 0;
	virtual HRESULT GetVertices(D3DRMGROUPINDEX groupIndex, int startIndex, int count, D3DRMVERTEX* vertices) = 0;
};
typedef IDirect3DRMMesh* LPDIRECT3DRMMESH;

struct IDirect3DRMFrame;
struct IDirect3DRMMeshBuilder : public IDirect3DRMVisual {
	virtual HRESULT SetPerspective(BOOL perspective) = 0;
	virtual HRESULT CreateMesh(IDirect3DRMMesh** mesh) = 0;
};
typedef IDirect3DRMMeshBuilder* LPDIRECT3DRMMESHBUILDER;

struct IDirect3DRMMeshBuilder2 : public IDirect3DRMMeshBuilder {};
typedef IDirect3DRMMeshBuilder2* LPDIRECT3DRMMESHBUILDER2;

struct IDirect3DRMMeshBuilder3 : public IDirect3DRMMeshBuilder2 {
	virtual HRESULT Load(
		void* filename,
		void* name,
		D3DRMLOADOPTIONS flags,
		D3DRMLOADTEXTURE3CALLBACK callback,
		void* arg
	) = 0;
};
typedef IDirect3DRMMeshBuilder3* LPDIRECT3DRMMESHBUILDER3;

struct IDirect3DRMLight : public IDirect3DRMObject {
	virtual HRESULT SetColor(D3DCOLOR color) = 0;
	virtual HRESULT SetColorRGB(float r, float g, float b) = 0;
	virtual HRESULT SetRange(D3DVALUE range) = 0;
	virtual HRESULT SetUmbra(D3DVALUE angle) = 0;
	virtual HRESULT SetPenumbra(D3DVALUE angle) = 0;
	virtual HRESULT SetEnableFrame(IDirect3DRMFrame* frame) = 0;
	virtual D3DRMLIGHTTYPE GetType() = 0;
	virtual D3DCOLOR GetColor() = 0;
};
typedef IDirect3DRMLight* LPDIRECT3DRMLIGHT;

struct IDirect3DRMArray : public IUnknown {
	virtual DWORD GetSize() = 0;
};

struct IDirect3DRMLightArray : public IDirect3DRMArray {
	virtual HRESULT GetElement(DWORD index, IDirect3DRMLight** out) = 0;
	virtual HRESULT AddElement(IDirect3DRMLight* in) = 0;
	virtual HRESULT DeleteElement(IDirect3DRMLight* element) = 0;
};

struct IDirect3DRMVisualArray : public IDirect3DRMArray {
	virtual HRESULT GetElement(DWORD index, IDirect3DRMVisual** out) = 0;
	virtual HRESULT AddElement(IDirect3DRMVisual* in) = 0;
	virtual HRESULT DeleteElement(IDirect3DRMVisual* element) = 0;
};

typedef struct IDirect3DRMFrameArray* LPDIRECT3DRMFRAMEARRAY;
struct IDirect3DRMFrame : public IDirect3DRMVisual {
	virtual HRESULT AddChild(IDirect3DRMFrame* child) = 0;
	virtual HRESULT DeleteChild(IDirect3DRMFrame* child) = 0;
	virtual HRESULT SetSceneBackgroundRGB(float r, float g, float b) = 0;
	virtual HRESULT AddLight(IDirect3DRMLight* light) = 0;
	virtual HRESULT GetLights(IDirect3DRMLightArray** lightArray) = 0;
	virtual HRESULT AddTransform(D3DRMCOMBINETYPE combine, D3DRMMATRIX4D matrix) = 0;
	virtual HRESULT GetPosition(IDirect3DRMFrame* reference, D3DVECTOR* position) = 0;
	virtual HRESULT AddVisual(IDirect3DRMVisual* visual) = 0;
	virtual HRESULT DeleteVisual(IDirect3DRMVisual* visual) = 0;
	virtual HRESULT GetVisuals(IDirect3DRMVisualArray** visuals) = 0;
	virtual HRESULT SetTexture(IDirect3DRMTexture* texture) = 0;
	virtual HRESULT GetTexture(IDirect3DRMTexture** texture) = 0;
	virtual HRESULT SetColor(float r, float g, float b, float a) = 0;
	virtual HRESULT SetColor(D3DCOLOR) = 0;
	virtual HRESULT SetColorRGB(float r, float g, float b) = 0;
	virtual HRESULT SetMaterialMode(D3DRMMATERIALMODE mode) = 0;
	virtual HRESULT GetChildren(IDirect3DRMFrameArray** children) = 0;
};
typedef IDirect3DRMFrame* LPDIRECT3DRMFRAME;

struct IDirect3DRMFrameArray : public IDirect3DRMArray {
	virtual HRESULT GetElement(DWORD index, IDirect3DRMFrame** out) = 0;
	virtual HRESULT AddElement(IDirect3DRMFrame* in) = 0;
	virtual HRESULT DeleteElement(IDirect3DRMFrame* element) = 0;
};

struct IDirect3DRMFrame2 : public IDirect3DRMFrame {};
typedef IDirect3DRMFrame2* LPDIRECT3DRMFRAME2;

typedef struct IDirect3DRMFrame3* LPDIRECT3DRMFRAME3;
struct IDirect3DRMFrame3 : public IDirect3DRMFrame2 {
	virtual HRESULT AddChild(IDirect3DRMFrame3* child) = 0;
	virtual HRESULT AddTranslation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z) = 0;
	virtual HRESULT AddScale(D3DRMCOMBINETYPE combine, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz) = 0;
	virtual HRESULT AddRotation(D3DRMCOMBINETYPE combine, D3DVALUE x, D3DVALUE y, D3DVALUE z, D3DVALUE theta) = 0;
	virtual HRESULT AddVisual(IUnknown* visual) = 0;
	virtual HRESULT GetParent(IDirect3DRMFrame3** parent) = 0;
	virtual HRESULT GetPosition(IDirect3DRMFrame3* reference, D3DVECTOR* position) = 0;
	virtual HRESULT GetScene(IDirect3DRMFrame3** scene) = 0;
	virtual HRESULT GetTransform(IDirect3DRMFrame3* reference, D3DRMMATRIX4D matrix) = 0;
	virtual HRESULT GetOrientation(IDirect3DRMFrame3* reference, D3DVECTOR* dir, D3DVECTOR* up) = 0;
	virtual HRESULT GetVisuals(DWORD* count, IUnknown** visuals) = 0;
	virtual HRESULT InverseTransform(D3DVECTOR* d, D3DVECTOR* s) = 0;
	virtual HRESULT Load(
		void* filename,
		void* name,
		D3DRMLOADOPTIONS flags,
		D3DRMLOADTEXTURE3CALLBACK callback,
		void* arg
	) = 0;
	virtual HRESULT LookAt(
		IDirect3DRMFrame3* target,
		IDirect3DRMFrame3* reference,
		D3DRMFRAMECONSTRAINT constraint
	) = 0;
	virtual HRESULT DeleteChild(IDirect3DRMFrame3* child) = 0;
	virtual HRESULT DeleteVisual(IUnknown* visual) = 0;
	virtual D3DCOLOR GetSceneBackground() = 0;
	virtual HRESULT SetSceneBackground(D3DCOLOR color) = 0;
	virtual HRESULT SetSceneFogEnable(BOOL enable) = 0;
	virtual HRESULT SetSceneFogColor(D3DCOLOR color) = 0;
	virtual HRESULT SetSceneFogMode(D3DRMFOGMODE mode) = 0;
	virtual HRESULT SetSceneFogParams(D3DVALUE start, D3DVALUE end, D3DVALUE density) = 0;
	virtual HRESULT SetOrientation(
		IDirect3DRMFrame3* reference,
		D3DVALUE dx,
		D3DVALUE dy,
		D3DVALUE dz,
		D3DVALUE ux,
		D3DVALUE uy,
		D3DVALUE uz
	) = 0;
	virtual HRESULT SetPosition(IDirect3DRMFrame3* reference, D3DVALUE x, D3DVALUE y, D3DVALUE z) = 0;
	virtual HRESULT SetSortMode(D3DRMSORTMODE mode) = 0;
	virtual HRESULT Transform(D3DVECTOR* d, D3DVECTOR* s) = 0;
	virtual HRESULT SetSceneFogMethod(DWORD flags) = 0;
};

struct D3DRMPICKDESC {
	float dist;
};

struct IDirect3DRMPickedArray : public IDirect3DRMArray {
	virtual HRESULT GetPick(
		DWORD index,
		IDirect3DRMVisual** visual,
		IDirect3DRMFrameArray** frameArray,
		D3DRMPICKDESC* desc
	) = 0;
};
typedef IDirect3DRMPickedArray* LPDIRECT3DRMPICKEDARRAY;

struct IDirect3DRMViewport : public IDirect3DRMObject {
	virtual HRESULT Render(IDirect3DRMFrame* group) = 0;
	virtual HRESULT ForceUpdate(int x, int y, int w, int h) = 0;
	virtual HRESULT Clear() = 0;
	virtual HRESULT SetCamera(IDirect3DRMFrame* camera) = 0;
	virtual HRESULT GetCamera(IDirect3DRMFrame** camera) = 0;
	virtual HRESULT SetProjection(D3DRMPROJECTIONTYPE type) = 0;
	virtual D3DRMPROJECTIONTYPE GetProjection() = 0;
	virtual HRESULT SetFront(D3DVALUE z) = 0;
	virtual D3DVALUE GetFront() = 0;
	virtual HRESULT SetBack(D3DVALUE z) = 0;
	virtual D3DVALUE GetBack() = 0;
	virtual HRESULT SetField(D3DVALUE field) = 0;
	virtual D3DVALUE GetField() = 0;
	virtual DWORD GetWidth() = 0;
	virtual DWORD GetHeight() = 0;
	virtual HRESULT Transform(D3DRMVECTOR4D* screen, D3DVECTOR* world) = 0;
	virtual HRESULT InverseTransform(D3DVECTOR* world, D3DRMVECTOR4D* screen) = 0;
	virtual HRESULT Pick(float x, float y, LPDIRECT3DRMPICKEDARRAY* pickedArray) = 0;
};

struct IDirect3DRMAnimationSet2 : public IDirect3DRMObject {
	virtual HRESULT Load(
		void* filename,
		void* name,
		D3DRMLOADOPTIONS flags,
		D3DRMLOADTEXTURE3CALLBACK callback,
		void* arg,
		IDirect3DRMFrame3* parent
	) = 0;
	virtual HRESULT SetTime(D3DVALUE time) = 0;
};
typedef IDirect3DRMAnimationSet2* LPDIRECT3DRMANIMATIONSET2;

struct IDirect3DRMViewport2 : public IDirect3DRMViewport {
	virtual HRESULT Clear(DWORD flags) = 0;
	virtual HRESULT Render(IDirect3DRMFrame3* frame) = 0;
	virtual HRESULT SetCamera(IDirect3DRMFrame3* camera) = 0;
	virtual HRESULT GetCamera(IDirect3DRMFrame3** camera) = 0;
	virtual LONG GetX() = 0;
	virtual LONG GetY() = 0;
	virtual HRESULT GetDirect3DViewport(IDirect3DViewport** viewport) = 0;
};
typedef IDirect3DRMViewport2* LPDIRECT3DRMVIEWPORT2;

struct IDirect3DRMViewportArray : public IDirect3DRMArray {
	virtual HRESULT GetElement(DWORD index, IDirect3DRMViewport** out) = 0;
	virtual HRESULT AddElement(IDirect3DRMViewport* in) = 0;
	virtual HRESULT DeleteElement(IDirect3DRMViewport* element) = 0;
};

struct IDirect3DRMWinDevice : virtual public IDirect3DRMObject {
	virtual HRESULT Activate() = 0;
	virtual HRESULT Paint() = 0;
	virtual void HandleActivate(WORD wParam) = 0;
	virtual void HandlePaint(void* p_dc) = 0;
};

struct IDirect3DRMDevice : virtual public IDirect3DRMObject {
	virtual DWORD GetWidth() = 0;
	virtual DWORD GetHeight() = 0;
	virtual HRESULT SetBufferCount(int count) = 0;
	virtual DWORD GetBufferCount() = 0;
	virtual HRESULT SetShades(DWORD shadeCount) = 0;
	virtual DWORD GetShades() = 0;
	virtual HRESULT SetQuality(D3DRMRENDERQUALITY quality) = 0;
	virtual D3DRMRENDERQUALITY GetQuality() = 0;
	virtual HRESULT SetDither(BOOL dither) = 0;
	virtual BOOL GetDither() = 0;
	virtual HRESULT SetTextureQuality(D3DRMTEXTUREQUALITY quality) = 0;
	virtual D3DRMTEXTUREQUALITY GetTextureQuality() = 0;
	virtual HRESULT SetRenderMode(D3DRMRENDERMODE mode) = 0;
	virtual D3DRMRENDERMODE GetRenderMode() = 0;
	virtual HRESULT Update() = 0;
	virtual HRESULT AddViewport(IDirect3DRMViewport* viewport) = 0;
	virtual HRESULT GetViewports(IDirect3DRMViewportArray** ppViewportArray) = 0;
};

struct IDirect3DRMDevice2 : virtual public IDirect3DRMDevice {};

struct IDirect3DRMDevice3 : public IDirect3DRMDevice2 {
	virtual HRESULT GetDirect3DDevice2(IDirect3DDevice2** d3dDevice) = 0;
	virtual HRESULT FindPreferredTextureFormat(DDBitDepths bitDepths, DWORD flags, LPDDPIXELFORMAT pixelFormat) = 0;
};
typedef IDirect3DRMDevice3* LPDIRECT3DRMDEVICE3;

struct IDirect3DRMUserVisual;
typedef int (*D3DRMUSERVISUALCALLBACK)(
	IDirect3DRMUserVisual* visual,
	void* arg,
	D3DRMUSERVISUALREASON reason,
	IDirect3DRMDevice* device,
	IDirect3DRMViewport* viewport
);
struct IDirect3DRMUserVisual : public IDirect3DRMVisual {};
typedef IDirect3DRMUserVisual* LPDIRECT3DRMUSERVISUAL;

struct IDirect3DRM : virtual public IUnknown {
	virtual HRESULT CreateDeviceFromD3D(
		const IDirect3D2* d3d,
		IDirect3DDevice2* d3dDevice,
		IDirect3DRMDevice2** outDevice
	) = 0;
	virtual HRESULT CreateDeviceFromSurface(
		const GUID* guid,
		IDirectDraw* dd,
		IDirectDrawSurface* surface,
		IDirect3DRMDevice2** outDevice
	) = 0;
	virtual HRESULT CreateTexture(D3DRMIMAGE* image, IDirect3DRMTexture2** outTexture) = 0;
	virtual HRESULT CreateTextureFromSurface(LPDIRECTDRAWSURFACE surface, IDirect3DRMTexture2** outTexture) = 0;
	virtual HRESULT CreateMesh(IDirect3DRMMesh** outMesh) = 0;
	virtual HRESULT CreateMaterial(D3DVAL power, IDirect3DRMMaterial** outMaterial) = 0;
	virtual HRESULT CreateLightRGB(D3DRMLIGHTTYPE type, D3DVAL r, D3DVAL g, D3DVAL b, IDirect3DRMLight** outLight) = 0;
	virtual HRESULT CreateFrame(IDirect3DRMFrame* parent, IDirect3DRMFrame2** outFrame) = 0;
	virtual HRESULT CreateViewport(
		IDirect3DRMDevice2* device,
		IDirect3DRMFrame* camera,
		int x,
		int y,
		int width,
		int height,
		IDirect3DRMViewport** outViewport
	) = 0;
	virtual HRESULT SetDefaultTextureShades(DWORD count) = 0;
	virtual HRESULT SetDefaultTextureColors(DWORD count) = 0;
};
typedef IDirect3DRM *LPDIRECT3DRM, **LPLPDIRECT3DRM;

struct IDirect3DRM2 : public IDirect3DRM {};

struct IDirect3DRM3 : public IDirect3DRM2 {
	virtual HRESULT CreateFrame(IDirect3DRMFrame3* parent, IDirect3DRMFrame3** outFrame) = 0;
	virtual HRESULT CreateMaterial(D3DVALUE power, IDirect3DRMMaterial2** outMaterial) = 0;
	virtual HRESULT CreateDeviceFromSurface(
		GUID* guid,
		IDirectDraw* dd,
		IDirectDrawSurface* surface,
		DWORD flags,
		IDirect3DRMDevice3** outDevice
	) = 0;
	virtual HRESULT CreateTextureFromSurface(LPDIRECTDRAWSURFACE surface, IDirect3DRMTexture3** outTexture) = 0;
	virtual HRESULT CreateViewport(
		IDirect3DRMDevice3* device,
		IDirect3DRMFrame3* camera,
		DWORD x,
		DWORD y,
		DWORD width,
		DWORD height,
		IDirect3DRMViewport2** outViewport
	) = 0;
	virtual HRESULT LoadTexture(const char* filename, IDirect3DRMTexture3** outTexture) = 0;
	virtual HRESULT CreateMeshBuilder(IDirect3DRMMeshBuilder3** outMeshBuilder) = 0;
	virtual HRESULT CreateAnimationSet(IDirect3DRMAnimationSet2** outAnimationSet) = 0;
	virtual HRESULT CreateUserVisual(
		D3DRMUSERVISUALCALLBACK callback,
		void* arg,
		IDirect3DRMUserVisual** outUserVisual
	) = 0;
};
typedef IDirect3DRM3* LPDIRECT3DRM3;

// Functions
HRESULT WINAPI Direct3DRMCreate(IDirect3DRM** direct3DRM);

D3DCOLOR D3DRMCreateColorRGBA(D3DVALUE red, D3DVALUE green, D3DVALUE blue, D3DVALUE alpha);
