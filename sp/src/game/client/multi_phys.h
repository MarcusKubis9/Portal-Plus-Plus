#include "cbase.h"


class C_MultiPhys : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_MultiPhys, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

	C_MultiPhys();

	void Spawn();
	void SpawnTestObjects();

	void ClientThink();

	void UpdateOnRemove();

	void GetRenderBounds(Vector& theMins, Vector& theMaxs);
	void ComputeWorldSpaceSurroundingBox(Vector *pMins, Vector *pMaxs);
	bool TestCollision(const Ray_t &ray, unsigned int mask, trace_t& trace);

	int InternalDrawModel(int flags);

	CUtlVector<IPhysicsObject*> m_physList;

	Vector m_physPos[2];
	QAngle m_physAng[2];

	CInterpolatedVarArray<Vector, 2> m_iv_physPos;
	CInterpolatedVarArray<QAngle, 2> m_iv_physAng;

private:
	bool SpawnedPhys;
};

IMPLEMENT_CLIENTCLASS_DT(C_MultiPhys, DTMultiPhys, CMultiPhys)
RecvPropArray(RecvPropVector(RECVINFO(m_physPos[0])), m_physPos),
RecvPropArray(RecvPropQAngles(RECVINFO(m_physAng[0])), m_physAng),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS(multi_phys, C_MultiPhys);

C_MultiPhys::C_MultiPhys() :
m_iv_physPos("C_MultiPhys::m_iv_physPos"),
m_iv_physAng("C_MultiPhys::m_iv_physAng")
{
	AddVar(m_physPos, &m_iv_physPos, LATCH_SIMULATION_VAR);
	AddVar(m_physAng, &m_iv_physAng, LATCH_SIMULATION_VAR);
}

void C_MultiPhys::Spawn()
{
	BaseClass::Spawn();
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_MultiPhys::ClientThink()
{
	if (!SpawnedPhys && physenv) // The client physics environment might not exist when Spawn() is called!
	{
		SpawnTestObjects();
		VPhysicsSetObject(m_physList[0]);
		SpawnedPhys = true;
	}

	// Pos/Ang are interpolated every frame, so keep refreshing them
	for (int i = 0; i < m_physList.Count(); i++)
		m_physList[i]->SetPosition(m_physPos[i], m_physAng[i], false);

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

// FIXME: a little too large, despite WSSB being a tight fit
void C_MultiPhys::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	ComputeWorldSpaceSurroundingBox(&theMins, &theMaxs);
	theMins -= GetAbsOrigin();
	theMaxs -= GetAbsOrigin();

	IRotateAABB(EntityToWorldTransform(), theMins, theMaxs, theMins, theMaxs);
}

// Quick visualisation of where the other models are. Actually rendering them is a whole new can of worms...
int C_MultiPhys::InternalDrawModel(int flags)
{
	int ret = BaseClass::InternalDrawModel(flags);

	IMaterial *pWireframe = materials->FindMaterial("shadertest/wireframevertexcolor", TEXTURE_GROUP_OTHER);
	matrix3x4_t matrix;
	static color32 debugColor = { 0, 255, 255, 0 };

	for (int i = 1; i < m_physList.Count(); i++) // skip first object
	{
		m_physList[i]->GetPositionMatrix(&matrix);
		engine->DebugDrawPhysCollide(m_physList[i]->GetCollide(), pWireframe, matrix, debugColor);
	}

	return ret;
}