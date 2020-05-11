//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef IVMODELRENDER_H
#define IVMODELRENDER_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "idatacache.h"
#include "vector.h"
#include "mathlib.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct mstudioanimdesc_t;
struct mstudioseqdesc_t;
struct model_t;
class IClientRenderable;
class Vector;
struct studiohdr_t;
class IMaterial;
class CStudioHdr;
class IMesh;
class IMorph;
struct MaterialLightingState_t;
struct Ray_t;
struct DrawModelState_t;

enum OverrideType_t
{
	OVERRIDE_NORMAL = 0,
	OVERRIDE_BUILD_SHADOWS,
	OVERRIDE_DEPTH_WRITE,
};


//-----------------------------------------------------------------------------
// Model rendering state
//-----------------------------------------------------------------------------


struct studiomeshgroup_t
{

	int				m_NumStrips;
	int				m_Flags;			// see studiomeshgroupflags_t
	unsigned short* m_pGroupIndexToMeshIndex;
	int				m_NumVertices;
	int* m_pUniqueFaces;	// for performance measurements
	unsigned short* m_pIndices;
	unsigned short* m_pTopologyIndices;
	bool			m_MeshNeedsRestore;
	short			m_ColorMeshID;
};


struct studiomeshdata_t
{
	int					m_NumGroup;
	studiomeshgroup_t* m_pMeshGroup;
};

struct studioloddata_t
{
	// not needed - this is really the same as studiohwdata_t.m_NumStudioMeshes
	//int					m_NumMeshes; 
	studiomeshdata_t* m_pMeshData; // there are studiohwdata_t.m_NumStudioMeshes of these.
	float				m_SwitchPoint;
	// one of these for each lod since we can switch to simpler materials on lower lods.
	int					numMaterials;
	IMaterial** ppMaterials; /* will have studiohdr_t.numtextures elements allocated */
	// hack - this needs to go away.
	int* pMaterialFlags; /* will have studiohdr_t.numtextures elements allocated */

	// For decals on hardware morphing, we must actually do hardware skinning
	// For this to work, we have to hope that the total # of bones used by
	// hw flexed verts is < than the max possible for the dx level we're running under
	int* m_pHWMorphDecalBoneRemap;
	int					m_nDecalBoneCount;
};

enum
{
	ADDDECAL_TO_ALL_LODS = -1
};

struct studiohwdata_t
{
	int					m_RootLOD;	// calced and clamped, nonzero for lod culling
	int					m_NumLODs;
	studioloddata_t* m_pLODs;
	int					m_NumStudioMeshes;

	inline float LODMetric(float unitSphereSize) const { return (unitSphereSize != 0.0f) ? (100.0f / unitSphereSize) : 0.0f; }
	inline int GetLODForMetric(float lodMetric) const
	{
		if (!m_NumLODs)
			return 0;

		// shadow lod is specified on the last lod with a negative switch
		// never consider shadow lod as viable candidate
		int numLODs = (m_pLODs[m_NumLODs - 1].m_SwitchPoint < 0.0f) ? m_NumLODs - 1 : m_NumLODs;

		for (int i = m_RootLOD; i < numLODs - 1; i++)
		{
			if (m_pLODs[i + 1].m_SwitchPoint > lodMetric)
				return i;
		}

		return numLODs - 1;
	}
};

struct ColorMeshInfo_t
{
	// A given color mesh can own a unique Mesh, or it can use a shared Mesh
	// (in which case it uses a sub-range defined by m_nVertOffset and m_nNumVerts)
	IMesh* m_pMesh;
	int* m_pPooledVBAllocator;
	int						m_nVertOffsetInBytes;
	int						m_nNumVerts;
};

class IPooledVBAllocator;

struct DrawModelInfo_t
{
	studiohdr_t* m_pStudioHdr;
	studiohwdata_t* m_pHardwareData;
	int m_Decals;
	int				m_Skin;
	int				m_Body;
	int				m_HitboxSet;
	void* m_pClientEntity;
	int				m_Lod;
	ColorMeshInfo_t* m_pColorMeshes;
	bool			m_bStaticLighting;
	int	m_LightingState;

};
//-----------------------------------------------------------------------------
// Model Rendering + instance data
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VENGINE_HUDMODEL_INTERFACE_VERSION	"VEngineModel016"

typedef unsigned short ModelInstanceHandle_t;

enum
{
	MODEL_INSTANCE_INVALID = (ModelInstanceHandle_t)~0
};

struct ModelRenderInfo_t
{
	Vector origin;
	QAngle angles; 
	IClientRenderable *pRenderable;
	const model_t *pModel;
	const matrix3x4_t *pModelToWorld;
	const matrix3x4_t *pLightingOffset;
	const Vector *pLightingOrigin;
	int flags;
	int entity_index;
	int skin;
	int body;
	int hitboxset;
	ModelInstanceHandle_t instance;

	ModelRenderInfo_t()
	{
		pModelToWorld = NULL;
		pLightingOffset = NULL;
		pLightingOrigin = NULL;
	}
};

struct StaticPropRenderInfo_t
{
	const matrix3x4_t		*pModelToWorld;
	const model_t			*pModel;
	IClientRenderable		*pRenderable;
	Vector					*pLightingOrigin;
	ModelInstanceHandle_t	instance;
	uint8					skin;
	uint8					alpha;
};

struct LightingQuery_t
{
	Vector m_LightingOrigin;
	ModelInstanceHandle_t m_InstanceHandle;
	bool m_bAmbientBoost;
};

struct StaticLightingQuery_t : public LightingQuery_t
{
	IClientRenderable *m_pRenderable;
};


// UNDONE: Move this to hud export code, subsume previous functions
abstract_class IVModelRender
{
public:
	virtual int		DrawModel(	int flags,
								IClientRenderable *pRenderable,
								ModelInstanceHandle_t instance,
								int entity_index, 
								const model_t *model, 
								Vector const& origin, 
								QAngle const& angles, 
								int skin,
								int body,
								int hitboxset,
								const matrix3x4_t *modelToWorld = NULL,
								const matrix3x4_t *pLightingOffset = NULL ) = 0;

	// This causes a material to be used when rendering the model instead 
	// of the materials the model was compiled with
	virtual void	ForcedMaterialOverride( IMaterial *newMaterial, OverrideType_t nOverrideType = OVERRIDE_NORMAL) = 0;

	virtual void	SetViewTarget( const CStudioHdr *pStudioHdr, int nBodyIndex, const Vector& target ) = 0;

	// Creates, destroys instance data to be associated with the model
	virtual ModelInstanceHandle_t CreateInstance( IClientRenderable *pRenderable, void *pCache = NULL ) = 0;
	virtual void DestroyInstance( ModelInstanceHandle_t handle ) = 0;

	// Associates a particular lighting condition with a model instance handle.
	// FIXME: This feature currently only works for static props. To make it work for entities, etc.,
	// we must clean up the lightcache handles as the model instances are removed.
	// At the moment, since only the static prop manager uses this, it cleans up all LightCacheHandles 
	// at level shutdown.
	virtual void SetStaticLighting( ModelInstanceHandle_t handle, void* pHandle ) = 0;
	virtual void GetStaticLighting( ModelInstanceHandle_t handle ) = 0;

	// moves an existing InstanceHandle to a nex Renderable to keep decals etc. Models must be the same
	virtual bool ChangeInstance( ModelInstanceHandle_t handle, IClientRenderable *pRenderable ) = 0;

	// Creates a decal on a model instance by doing a planar projection
	// along the ray. The material is the decal material, the radius is the
	// radius of the decal to create.
	virtual void AddDecal( ModelInstanceHandle_t handle, Ray_t const& ray, 
		Vector const& decalUp, int decalIndex, int body, bool noPokeThru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS ) = 0;

	// Removes all the decals on a model instance
	virtual void RemoveAllDecals( ModelInstanceHandle_t handle ) = 0;

	// Remove all decals from all models
	virtual void RemoveAllDecalsFromAllModels() = 0;

	// Shadow rendering, DrawModelShadowSetup returns the address of the bone-to-world array, NULL in case of error
	virtual matrix3x4a_t* DrawModelShadowSetup( IClientRenderable *pRenderable, int body, int skin, DrawModelInfo_t *pInfo, matrix3x4a_t *pCustomBoneToWorld = NULL ) = 0;
	virtual void DrawModelShadow(  IClientRenderable *pRenderable, const DrawModelInfo_t &info, matrix3x4a_t *pCustomBoneToWorld = NULL ) = 0;

	// This gets called when overbright, etc gets changed to recompute static prop lighting.
	virtual bool RecomputeStaticLighting( ModelInstanceHandle_t handle ) = 0;

	virtual void ReleaseAllStaticPropColorData( void ) = 0;
	virtual void RestoreAllStaticPropColorData( void ) = 0;

	// Extended version of drawmodel
	virtual int	DrawModelEx( ModelRenderInfo_t &pInfo ) = 0;

	virtual int	DrawModelExStaticProp( ModelRenderInfo_t &pInfo ) = 0;

	virtual bool DrawModelSetup( ModelRenderInfo_t &pInfo, DrawModelState_t *pState, matrix3x4_t **ppBoneToWorldOut ) = 0;
	virtual void DrawModelExecute( const DrawModelState_t &state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld = NULL ) = 0;

	// Sets up lighting context for a point in space
	virtual void SetupLighting( const Vector &vecCenter ) = 0;
	
	// doesn't support any debug visualization modes or other model options, but draws static props in the
	// fastest way possible
	virtual int DrawStaticPropArrayFast( StaticPropRenderInfo_t *pProps, int count, bool bShadowDepth ) = 0;

	// Allow client to override lighting state
	virtual void SuppressEngineLighting( bool bSuppress ) = 0;

	virtual void SetupColorMeshes( int nTotalVerts ) = 0;

	// Sets up lighting context for a point in space, with smooth interpolation per model.
	// Passing MODEL_INSTANCE_INVALID as a handle is equivalent to calling SetupLighting.
	virtual void SetupLightingEx( const Vector &vecCenter, ModelInstanceHandle_t handle ) = 0;

	// Finds the brightest light source illuminating a point. Returns false if there isn't any.
	virtual bool GetBrightestShadowingLightSource( const Vector &vecCenter, Vector& lightPos, Vector& lightBrightness, bool bAllowNonTaggedLights ) = 0;

	// Computes lighting state for an array of lighting requests
	virtual void ComputeLightingState( int nCount, const LightingQuery_t *pQuery, MaterialLightingState_t *pState, void **ppEnvCubemapTexture ) = 0;

	// Gets an array of decal handles given model instances
	virtual void GetModelDecalHandles( void *pDecals, int nDecalStride, int nCount, const ModelInstanceHandle_t *pHandles ) = 0;

	// Computes lighting state for an array of lighting requests for renderables which use static lighting
	virtual void ComputeStaticLightingState( int nCount, const StaticLightingQuery_t *pQuery, MaterialLightingState_t *pState, MaterialLightingState_t *pDecalState, ColorMeshInfo_t **ppStaticLighting, void **ppEnvCubemapTexture, DataCacheHandle_t *pColorMeshHandles ) = 0;

	// Cleans up lighting state. Must be called after the draw call that uses
	// the color meshes return from ComputeStaticLightingState has been issued
	virtual void CleanupStaticLightingState( int nCount, DataCacheHandle_t *pColorMeshHandles ) = 0;
};


#endif // IVMODELRENDER_H
