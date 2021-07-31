#include "human.h"

#include <engine/shared/config.h>
#include <game/server/classes.h>
#include <game/server/gamecontext.h>
#include <game/server/infclass/entities/infccharacter.h>
#include <game/server/infclass/infcgamecontroller.h>
#include <game/server/infclass/infcplayer.h>
#include <game/server/teeinfo.h>

MACRO_ALLOC_POOL_ID_IMPL(CInfClassHuman, MAX_CLIENTS)

CInfClassHuman::CInfClassHuman(CInfClassPlayer *pPlayer)
	: CInfClassPlayerClass(pPlayer)
{
}

void CInfClassHuman::OnCharacterPreCoreTick()
{
	CInfClassPlayerClass::OnCharacterPreCoreTick();

	switch (GetPlayerClass())
	{
		case PLAYERCLASS_SNIPER:
		{
			if(m_pCharacter->m_PositionLocked)
			{
				if(m_pCharacter->m_Input.m_Jump && !m_pCharacter->m_PrevInput.m_Jump)
				{
					m_pCharacter->UnlockPosition();
				}
				else
				{
					m_pCharacter->ResetMovementsInput();
				}
			}

			if(m_pCharacter->GetInAirTick() <= Server()->TickSpeed())
			{
				// Allow to re-lock in during the first second
				m_pCharacter->m_PositionLockAvailable = true;
			}
		}
			break;
		case PLAYERCLASS_NINJA:
		{
			if(m_pCharacter->IsGrounded() && m_pCharacter->m_DartLifeSpan <= 0)
			{
				m_pCharacter->m_DartLeft = Config()->m_InfNinjaJump;
			}
		}
			break;
		default:
			break;
	}
}

void CInfClassHuman::OnCharacterTick()
{
}

void CInfClassHuman::OnCharacterSnap(int SnappingClient)
{
	if(SnappingClient == m_pPlayer->GetCID())
	{
		switch(GetPlayerClass())
		{
			case PLAYERCLASS_SCIENTIST:
			{
				if(m_pCharacter->GetActiveWeapon() == WEAPON_GRENADE)
				{
					vec2 PortalShift = vec2(m_pCharacter->m_Input.m_TargetX, m_pCharacter->m_Input.m_TargetY);
					vec2 PortalDir = normalize(PortalShift);
					if(length(PortalShift) > 500.0f)
						PortalShift = PortalDir * 500.0f;
					vec2 PortalPos;

					if(m_pCharacter->FindPortalPosition(GetPos() + PortalShift, PortalPos))
					{
						CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_CursorID, sizeof(CNetObj_Projectile)));
						if(!pObj)
							return;

						pObj->m_X = (int)PortalPos.x;
						pObj->m_Y = (int)PortalPos.y;
						pObj->m_VelX = 0;
						pObj->m_VelY = 0;
						pObj->m_StartTick = Server()->Tick();
						pObj->m_Type = WEAPON_HAMMER;
					}
				}
			}
				break;
			default:
				break;
		}
	}
}

void CInfClassHuman::GiveClassAttributes()
{
	CInfClassPlayerClass::GiveClassAttributes();

	switch(GetPlayerClass())
	{
		case PLAYERCLASS_ENGINEER:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_LASER);
			break;
		case PLAYERCLASS_SOLDIER:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_GRENADE);
			break;
		case PLAYERCLASS_MERCENARY:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			if(!GameServer()->m_FunRound)
				m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_GUN);
			break;
		case PLAYERCLASS_SNIPER:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_LASER);
			break;
		case PLAYERCLASS_SCIENTIST:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_LASER);
			break;
		case PLAYERCLASS_BIOLOGIST:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->GiveWeapon(WEAPON_SHOTGUN, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_SHOTGUN);
			break;
		case PLAYERCLASS_LOOPER:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_LASER);
			break;
		case PLAYERCLASS_MEDIC:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_SHOTGUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_SHOTGUN);
			break;
		case PLAYERCLASS_HERO:
			if(GameController()->AreTurretsEnabled())
				m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_SHOTGUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_GRENADE);
			break;
		case PLAYERCLASS_NINJA:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->GiveWeapon(WEAPON_GUN, -1);
			m_pCharacter->GiveWeapon(WEAPON_GRENADE, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_HAMMER);
			break;
		case PLAYERCLASS_NONE:
			m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
			m_pCharacter->SetActiveWeapon(WEAPON_HAMMER);
			break;
	}
}

bool CInfClassHuman::SetupSkin(int PlayerClass, CTeeInfo *output)
{
	switch(PlayerClass)
	{
		case PLAYERCLASS_ENGINEER:
			output->m_UseCustomColor = 0;
			output->SetSkinName("limekitty");
			break;
		case PLAYERCLASS_SOLDIER:
			output->SetSkinName("brownbear");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_SNIPER:
			output->SetSkinName("warpaint");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_MERCENARY:
			output->SetSkinName("bluestripe");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_SCIENTIST:
			output->SetSkinName("toptri");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_BIOLOGIST:
			output->SetSkinName("twintri");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_LOOPER:
			output->SetSkinName("bluekitty");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 255;
			output->m_ColorFeet = 0;
			break;
		case PLAYERCLASS_MEDIC:
			output->SetSkinName("twinbop");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_HERO:
			output->SetSkinName("redstripe");
			output->m_UseCustomColor = 0;
			break;
		case PLAYERCLASS_NINJA:
			output->SetSkinName("default");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 255;
			output->m_ColorFeet = 0;
			break;
		default:
			output->SetSkinName("default");
			output->m_UseCustomColor = 0;
			return false;
	}

	return true;
}

void CInfClassHuman::SetupSkin(CTeeInfo *output)
{
	SetupSkin(GetPlayerClass(), output);
}

void CInfClassHuman::OnSlimeEffect(int Owner)
{
	int Count = Config()->m_InfSlimePoisonDuration;
	Poison(Count, Owner);
}
