#include "cbase.h"

class CMultiPhys : public CBaseAnimating
{
public:
	DECLARE_CLASS(CMultiPhys, CBaseAnimating);
	DECLARE_SERVERCLASS();

	void Spawn();
	void SpawnTestObjects();

	void UpdateOnRemove();

	int VPhysicsGetObjectList(IPhysicsObject **pList, int listMax);
	void VPhysicsUpdate(IPhysicsObject *pPhysics);

	void ComputeWorldSpaceSurroundingBox(Vector *pMins, Vector *pMaxs);
	bool TestCollision(const Ray_t &ray, unsigned int mask, trace_t& trace);
	void PhysicsImpact(CBaseEntity *other, trace_t &trace);

	void Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);

	CUtlVector<IPhysicsObject*> m_physList;

	// Have to choose a limit, 2 in this case. Remember to update the client if you raise this.
	CNetworkArray(Vector, m_physPos, 2);
	CNetworkArray(QAngle, m_physAng, 2);
};

IMPLEMENT_SERVERCLASS_ST(CMultiPhys, DTMultiPhys)
SendPropArray(SendPropVector(SENDINFO_ARRAY(m_physPos)), m_physPos),
SendPropArray(SendPropQAngles(SENDINFO_ARRAY(m_physAng)), m_physAng),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(multi_phys, CMultiPhys);

#define MODEL "models/props_junk/TrashBin01a.mdl"
#define MODEL2 "models/props_c17/FurnitureDresser001a.mdl"

void CMultiPhys::Spawn()
{
	BaseClass::Spawn();
	PrecacheModel(MODEL);
	PrecacheModel(MODEL2);
	SetModel(MODEL);

	SpawnTestObjects();
	VPhysicsSetObject(m_physList[0]);

	SetMoveType(MOVETYPE_VPHYSICS);

	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_CUSTOMBOXTEST | FSOLID_CUSTOMRAYTEST);
	CollisionProp()->SetSurroundingBoundsType(USE_GAME_CODE);

	Teleport(&GetAbsOrigin(), &GetAbsAngles(), &GetAbsVelocity());
}

int CMultiPhys::VPhysicsGetObjectList(IPhysicsObject **pList, int listMax)
{
	int count = 0;
	for (int i = 0; i < m_physList.Count() && i < listMax; i++)
	{
		pList[i] = m_physList[i];
		count++;
	}
	return count;
}

void CMultiPhys::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	for (int i = 0; i < m_physList.Count(); i++)
	{
		Vector pos;
		QAngle ang;
		m_physList[i]->GetPosition(&pos, &ang);

		m_physPos.Set(i, pos);
		m_physAng.Set(i, ang);

		if (i == 0)
		{
			SetAbsOrigin(pos);
			SetAbsAngles(ang);
		}
	}

	PhysicsTouchTriggers();
	SetSimulationTime(gpGlobals->curtime); // otherwise various game systems think the entity is inactive
}

// Called whenever bounding boxes intersect; *not* a VPhysics call
void CMultiPhys::PhysicsImpact(CBaseEntity *other, trace_t &trace)
{
	for (int i = 0; i < m_physList.Count(); i++)
	{
		trace_t curTrace;
		Vector position;
		QAngle angles;

		m_physList[i]->GetPosition(&position, &angles);

		physcollision->TraceBox(trace.startpos, trace.endpos, Vector(-1), Vector(1), m_physList[i]->GetCollide(), position, angles, &curTrace);

		if (curTrace.fraction < trace.fraction)
			BaseClass::PhysicsImpact(other, trace);
	}
}

void CMultiPhys::Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity)
{
	BaseClass::Teleport(newPosition, newAngles, newVelocity);

	for (int i = 1; i < m_physList.Count(); i++) // skip the first object, that's handled by CBaseEntity
	{
		Vector curPos;
		QAngle curAng;
		m_physList[i]->GetPosition(&curPos, &curAng);

		// Redirect to current value if there isn't a new one
		if (!newPosition)
			newPosition = &curPos;
		if (!newAngles)
			newAngles = &curAng;

		// In a real entity there would be something better than this
		Vector OffsetPos = *newPosition + Vector(20 * i, 0, 0);
		newPosition = &OffsetPos;

		// Move the physics object
		m_physList[i]->SetPosition(*newPosition, *newAngles, true);

		// Network the new values
		m_physPos.Set(i, *newPosition);
		m_physAng.Set(i, *newAngles);

		// This doesn't need to be networked
		if (newVelocity)
		{
			AngularImpulse angImp;
			m_physList[i]->SetVelocity(newVelocity, &angImp);
		}
	}
}