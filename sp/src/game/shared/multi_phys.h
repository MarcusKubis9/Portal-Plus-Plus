#include "cbase.h"
#ifdef GAME_DLL
#include "multi_phys.h"
#include "physics_saverestore.h"
#else
#include "c_multi_phys.h"
#define CMultiPhys C_MultiPhys
#endif

#define MODEL "models/props_junk/TrashBin01a.mdl"
#define MODEL2 "models/props_c17/FurnitureDresser001a.mdl"

void CMultiPhys::SpawnTestObjects()
{
	m_physList.AddToTail(PhysModelCreate(this, modelinfo->GetModelIndex(MODEL), vec3_origin, vec3_angle));
	m_physList[0]->SetGameData(this);
	m_physList[0]->SetGameIndex(0);
	m_physList[0]->SetGameFlags(FVPHYSICS_MULTIOBJECT_ENTITY);

	m_physList.AddToTail(PhysModelCreate(this, modelinfo->GetModelIndex(MODEL2), vec3_origin, vec3_angle));
	m_physList[1]->SetGameData(this);
	m_physList[1]->SetGameIndex(1);
	m_physList[1]->SetGameFlags(FVPHYSICS_MULTIOBJECT_ENTITY);
}

// Generates the bounding box
void CMultiPhys::ComputeWorldSpaceSurroundingBox(Vector *pMins, Vector *pMaxs)
{
	Vector Mins(GetAbsOrigin()), Maxs(Mins);

	for (int i = 0; i < m_physList.Count(); i++)
	{
		Vector curMin, curMax;

		Vector curPos;
		QAngle curAng;
		m_physList[i]->GetPosition(&curPos, &curAng);

		physcollision->CollideGetAABB(&curMin, &curMax, m_physList[i]->GetCollide(), curPos, curAng);

		for (int j = 0; j<3; j++)
		{
			Mins[j] = min(Mins[j], curMin[j]);
			Maxs[j] = max(Maxs[j], curMax[j]);
		}

	}

	*pMins = Mins;
	*pMaxs = Maxs;
}

// Should a VPhysics collision happen?
bool CMultiPhys::TestCollision(const Ray_t &ray, unsigned int mask, trace_t& trace)
{
	for (int i = 0; i < m_physList.Count(); i++)
	{
		Vector position;
		QAngle angles;
		m_physList[i]->GetPosition(&position, &angles);

		trace_t curTrace;
		physcollision->TraceBox(ray, m_physList[i]->GetCollide(), position, angles, &curTrace);

		if (curTrace.fraction < trace.fraction)
		{
			curTrace.surface.surfaceProps = m_physList[i]->GetMaterialIndex();
			trace = curTrace;
		}
	}
	return trace.fraction < 1;
}

void CMultiPhys::UpdateOnRemove()
{
	for (int i = 1; i < m_physList.Count(); i++)
	{
#ifdef GAME_DLL
		g_pPhysSaveRestoreManager->ForgetModel(m_physList[i]);
#endif
		physenv->DestroyObject(m_physList[i]);
	}
	BaseClass::UpdateOnRemove();
}