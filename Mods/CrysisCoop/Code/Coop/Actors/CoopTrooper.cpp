#include "StdAfx.h"
#include "CoopTrooper.h"
#include "Coop\CoopSystem.h"

#include "CompatibilityAlienMovementController.h"
#include <Coop\Utilities\DedicatedServerHackScope.h>

CCoopTrooper::CCoopTrooper() :
	m_vLookTarget(Vec3(0,0,0)),
	m_vAimTarget(Vec3(0,0,0)),
	m_bHidden(false)
{
}

CCoopTrooper::~CCoopTrooper()
{
}

bool CCoopTrooper::Init(IGameObject * pGameObject)
{
	CTrooper::Init(pGameObject);

	return true;
}

void CCoopTrooper::PostInit( IGameObject * pGameObject )
{
	CTrooper::PostInit(pGameObject);

	pGameObject->SetAIActivation(eGOAIAM_Always);

	if (gEnv->bServer)
		GetEntity()->SetTimer(eTIMER_WEAPONDELAY, 1000);
}

void CCoopTrooper::RegisterMultiplayerAI()
{
	if (GetHealth() <= 0 && GetEntity()->GetAI())
	{
		gEnv->bMultiplayer = false;

		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		gEnv->pScriptSystem->BeginCall(pScriptTable, "UnregisterAI");
		gEnv->pScriptSystem->PushFuncParam(pScriptTable);
		gEnv->pScriptSystem->EndCall(pScriptTable);
		CryLogAlways("AI Unregistered for Trooper %s", GetEntity()->GetName());

		gEnv->bMultiplayer = true;
	}
	else if (!GetEntity()->GetAI() && GetHealth() > 0)
	{
		gEnv->bMultiplayer = false;

		IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
		gEnv->pScriptSystem->BeginCall(pScriptTable, "RegisterAI");
		gEnv->pScriptSystem->PushFuncParam(pScriptTable);
		gEnv->pScriptSystem->EndCall(pScriptTable);
		CryLogAlways("AI Registered for Trooper %s", GetEntity()->GetName());

		gEnv->bMultiplayer = true;
	}
}

void CCoopTrooper::Update(SEntityUpdateContext& ctx, int updateSlot)
{
	CTrooper::Update(ctx, updateSlot);

	// Register AI System in MP
	if (gEnv->bServer && !gEnv->bEditor)
		RegisterMultiplayerAI();

	// Movement reqeust stuff so proper anims play on client
	if (gEnv->bServer)
	{
		SMovementState currMovement = static_cast<CCompatibilityAlienMovementController*>(GetMovementController())->GetMovementReqState();

		// Vec3
		m_vLookTarget = currMovement.eyePosition + currMovement.bodyDirection;
		m_vAimTarget = currMovement.eyePosition + currMovement.aimDirection;

		// Int
		m_nStance = (int)currMovement.stance;

		if (GetHealth() > 0.f)
			GetGameObject()->ChangedNetworkState(ASPECT_ALIVE);
	}
	else
	{
		UpdateMovementState();
	}

	
	if (IAnimationGraphState* pGraphState = this->GetAnimationGraphState())
	{
		CDedicatedServerHackScope HackScope = CDedicatedServerHackScope();
		pGraphState->Update();
	}

}

void CCoopTrooper::UpdateMovementState()
{
	CMovementRequest request;
	request.SetBodyTarget(m_vLookTarget);
	request.SetLookTarget(m_vLookTarget);
	request.SetAimTarget(m_vAimTarget);

	request.SetStance((EStance)m_nStance);

	//request.set
	//static_cast<CCompatibilityAlienMovementController*>(GetMovementController())->RequestMovement()
	SetActorMovement(SMovementRequestParams(request));
}

void CCoopTrooper::ProcessEvent(SEntityEvent& event)
{
	CTrooper::ProcessEvent(event);

	switch(event.event)
	{
		case ENTITY_EVENT_HIDE:
		{
			if (gEnv->bServer)
			{
				GetInventory()->Clear();
				m_bHidden = true;
				GetGameObject()->ChangedNetworkState(ASPECT_HIDE);
			}

			break;
		}
		case ENTITY_EVENT_UNHIDE:
		{
			if (gEnv->bServer)
			{
				GetEntity()->SetTimer(eTIMER_WEAPONDELAY, 1000);
				m_bHidden = false;
				GetGameObject()->ChangedNetworkState(ASPECT_HIDE);
			}

			break;
		}
	case ENTITY_EVENT_TIMER:
		{
			switch(event.nParam[0])
			{
			case eTIMER_WEAPONDELAY:
				IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
				SmartScriptTable props;
				if(pScriptTable->GetValue("Properties", props))
				{
					char* equip;
					if (props->GetValue("equip_EquipmentPack", equip))
						gEnv->pGame->GetIGameFramework()->GetIItemSystem()->GetIEquipmentManager()->GiveEquipmentPack(this, equip, true, true);
				}

				break;
			}
		}
		break;
	}
}

bool CCoopTrooper::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	if (!CTrooper::NetSerialize(ser, aspect, profile, flags))
		return false;

	bool bReading = ser.IsReading();

	switch (aspect)
	{
		case ASPECT_ALIVE:
		{
			//Vec3
			ser.Value("vLookTarget", m_vLookTarget, 'wrld');
			ser.Value("vAimTarget", m_vAimTarget, 'wrld');

			//Int
			ser.Value("nStance", m_nStance, 'i8');

			break;
		}
		case ASPECT_HIDE:
		{
			ser.Value("bHide", m_bHidden, 'bool');

			if (bReading)
				GetEntity()->Hide(m_bHidden);

			break;
		}
	}

	return true;
}

bool CCoopTrooper::IsAnimEvent(const char* sAnimSignal, string* sAnimEventName, float* fEventTime)
{
	/*if (strcmp(sAnimSignal, "meleeAttack") == 0)
	{
		*sAnimEventName = "MeleeDamage";
		*fEventTime = 0.5f;
		return true;
	}
	else if (strcmp(sAnimSignal, "fly") == 0)
	{
		*sAnimEventName = "Jump";
		*fEventTime = 0.0f;
		return true;
	}

	*/
	return false;
}