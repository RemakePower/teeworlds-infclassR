#include "infected.h"

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/infclass/entities/infccharacter.h>
#include <game/server/infclass/infcplayer.h>
#include <game/server/teeinfo.h>

MACRO_ALLOC_POOL_ID_IMPL(CInfClassInfected, MAX_CLIENTS)

CInfClassInfected::CInfClassInfected(CInfClassPlayer *pPlayer)
	: CInfClassPlayerClass(pPlayer)
{
}

int CInfClassInfected::GetDefaultEmote() const
{
	int EmoteNormal = EMOTE_ANGRY;

	if(m_pCharacter->IsInvisible())
		EmoteNormal = EMOTE_BLINK;

	if(m_pCharacter->IsInLove() || m_pCharacter->IsInSlowMotion() || m_pCharacter->HasHallucination())
		EmoteNormal = EMOTE_SURPRISE;

	if(m_pCharacter->IsFrozen())
		EmoteNormal = EMOTE_PAIN;

	return EmoteNormal;
}

bool CInfClassInfected::CanDie() const
{
	if ((GetPlayerClass() == PLAYERCLASS_UNDEAD) && m_pCharacter->IsFrozen()) {
		return false;
	}
	if ((GetPlayerClass() == PLAYERCLASS_VOODOO) && m_VoodooAboutToDie) {
		return false;
	}

	return true;
}

void CInfClassInfected::OnCharacterPreCoreTick()
{
	CInfClassPlayerClass::OnCharacterPreCoreTick();

	switch(GetPlayerClass())
	{
		case PLAYERCLASS_SPIDER:
		{
			if(m_pCharacter->WebHookLength() > 48.0f && m_pCharacter->GetHookedPlayer() < 0)
			{
				// Find other players
				for(CInfClassCharacter *p = (CInfClassCharacter*) GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CInfClassCharacter *)p->TypeNext())
				{
					if(p->IsZombie())
						continue;

					vec2 IntersectPos = closest_point_on_line(GetPos(), m_pCharacter->GetHookPos(), p->GetPos());
					float Len = distance(p->GetPos(), IntersectPos);
					if(Len < p->GetProximityRadius())
					{
						m_pCharacter->SetHookedPlayer(p->GetCID());
						m_pCharacter->m_HookMode = 0;

						break;
					}
				}
			}
		}
			break;
		default:
			break;
	}
}

void CInfClassInfected::OnCharacterTick()
{
	CInfClassPlayerClass::OnCharacterTick();

	if(GetPlayerClass() == PLAYERCLASS_VOODOO && m_VoodooAboutToDie)
	{
		// Delayed Death
		if (m_VoodooTimeAlive > 0)
			m_VoodooTimeAlive-=1000;
		else
			m_pCharacter->Die(m_VoodooKiller, m_VoodooWeapon);

		// Display time left to live
		int Time = m_VoodooTimeAlive/Server()->TickSpeed();
		GameServer()->SendBroadcast_Localization(GetCID(), BROADCAST_PRIORITY_WEAPONSTATE, BROADCAST_DURATION_REALTIME,
			_("Staying alive for: {int:RemainingTime}"),
			"RemainingTime", &Time,
			NULL
		);
	}
	if(GetPlayerClass() == PLAYERCLASS_SPIDER)
	{
		const bool HookIsOnTheLimit = m_pCharacter->WebHookLength() > Config()->m_InfSpiderWebHookLength - 48.0f;
		SetHookOnLimit(HookIsOnTheLimit);
	}
}

void CInfClassInfected::OnCharacterSpawned(const SpawnContext &Context)
{
	CInfClassPlayerClass::OnCharacterSpawned(Context);

	m_SlimeHealTick = 0;

	if(Context.SpawnType == SpawnContext::MapSpawn)
	{
		m_pCharacter->GrantSpawnProtection();
	}
}

void CInfClassInfected::OnCharacterDeath(int Weapon)
{
	CInfClassPlayerClass::OnCharacterDeath(Weapon);

	if(GetPlayerClass() == PLAYERCLASS_GHOUL)
	{
		IncreaseGhoulLevel(-20);
		UpdateSkin();
	}

	if(GetPlayerClass() == PLAYERCLASS_BOOMER)
	{
		if(!m_pCharacter->IsFrozen() && Weapon != WEAPON_GAME && !(m_pCharacter->IsInLove() && Weapon == WEAPON_SELF))
		{
			GameServer()->CreateSound(GetPos(), SOUND_GRENADE_EXPLODE);
			GameServer()->CreateExplosionDisk(GetPos(), 60.0f, 80.5f, 14, 52.0f, m_pPlayer->GetCID(), WEAPON_HAMMER, TAKEDAMAGEMODE_INFECTION);
		}
	}
}

void CInfClassInfected::GiveClassAttributes()
{
	CInfClassPlayerClass::GiveClassAttributes();

	m_pCharacter->GiveWeapon(WEAPON_HAMMER, -1);
	m_pCharacter->SetActiveWeapon(WEAPON_HAMMER);

	if(m_pCharacter->CanOpenPortals())
	{
		m_pCharacter->GiveWeapon(WEAPON_LASER, -1);
	}

	m_VoodooAboutToDie = false;
	m_VoodooTimeAlive = Server()->TickSpeed()*Config()->m_InfVoodooAliveTime;
}

void CInfClassInfected::SetupSkin(CTeeInfo *output)
{
	switch(GetPlayerClass())
	{
		case PLAYERCLASS_SMOKER:
			output->m_UseCustomColor = 1;
			output->SetSkinName("cammostripes");
			output->m_ColorBody = 3866368;
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_BOOMER:
			output->SetSkinName("saddo");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3866368;
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_HUNTER:
			output->SetSkinName("warpaint");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3866368;
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_BAT:
			output->SetSkinName("limekitty");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3866368;
			output->m_ColorFeet = 2866368;
			break;
		case PLAYERCLASS_GHOST:
			output->SetSkinName("twintri");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3866368;
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_SPIDER:
			output->SetSkinName("pinky");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3866368;
			if(m_HookOnTheLimit)
			{
				output->m_ColorFeet = 16776960; // Dark red
			}
			else
			{
				output->m_ColorFeet = 65414;
			}
			break;
		case PLAYERCLASS_GHOUL:
			output->SetSkinName("cammo");
			output->m_UseCustomColor = 1;
			{
				int Hue = 58 * (1.0f - GetGhoulPercent() * 0.8f);
				output->m_ColorBody = (Hue<<16) + (255<<8);
			}
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_SLUG:
			output->SetSkinName("coala");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3866368;
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_VOODOO:
			output->SetSkinName("bluestripe");
			output->m_UseCustomColor = 1;
			if(!m_VoodooAboutToDie)
			{
				output->m_ColorBody = 3866368;
			}
			else
			{
				output->m_ColorBody = 6183936; // grey-green
			}
			output->m_ColorFeet = 65414;
			break;
		case PLAYERCLASS_UNDEAD:
			output->SetSkinName("redstripe");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 3014400;
			output->m_ColorFeet = 13168;
			break;
		case PLAYERCLASS_WITCH:
			output->SetSkinName("redbopp");
			output->m_UseCustomColor = 1;
			output->m_ColorBody = 16776744;
			output->m_ColorFeet = 13168;
			break;
		default:
			output->m_UseCustomColor = 0;
			output->SetSkinName("default");
	}
}

void CInfClassInfected::BroadcastWeaponState()
{
	if(GetPlayerClass() == PLAYERCLASS_SPIDER)
	{
		if(m_pCharacter->m_HookMode > 0)
		{
			GameServer()->SendBroadcast_Localization(GetCID(), BROADCAST_PRIORITY_WEAPONSTATE,
				BROADCAST_DURATION_REALTIME, _("Web mode enabled"), NULL);
		}
	}
	else if(GetPlayerClass() == PLAYERCLASS_GHOUL)
	{
		if(m_pPlayer->GetGhoulLevel())
		{
			float FodderInStomach = GetGhoulPercent();
			GameServer()->SendBroadcast_Localization(GetCID(), BROADCAST_PRIORITY_WEAPONSTATE,
				BROADCAST_DURATION_REALTIME,
				_("Stomach filled by {percent:FodderInStomach}"),
				"FodderInStomach", &FodderInStomach,
				NULL
			);
		}
	}
	else if(GetPlayerClass() == PLAYERCLASS_WITCH)
	{
		if (m_pCharacter->hasPortalIn() && m_pCharacter->hasPortalOut())
		{
			GameServer()->SendBroadcast_Localization(GetCID(), BROADCAST_PRIORITY_WEAPONSTATE,
				BROADCAST_DURATION_REALTIME,
				_("The portals system is active!"),
				nullptr
			);
		}
		else if (m_pCharacter->hasPortalIn())
		{
			GameServer()->SendBroadcast_Localization(GetCID(), BROADCAST_PRIORITY_WEAPONSTATE,
				BROADCAST_DURATION_REALTIME,
				_("The IN portal is open"),
				nullptr
			);
		}
		else if (m_pCharacter->hasPortalOut())
		{
			GameServer()->SendBroadcast_Localization(GetCID(), BROADCAST_PRIORITY_WEAPONSTATE,
				BROADCAST_DURATION_REALTIME,
				_("The OUT portal is open"),
				nullptr
			);
		}
	}
}

void CInfClassInfected::SetHookOnLimit(bool OnLimit)
{
	if(m_HookOnTheLimit == OnLimit)
		return;

	m_HookOnTheLimit = OnLimit;
	UpdateSkin();
}

void CInfClassInfected::OnSlimeEffect(int Owner)
{
	m_pCharacter->SetEmote(EMOTE_HAPPY, Server()->Tick());
	if(Server()->Tick() >= m_SlimeHealTick + (Server()->TickSpeed() / Config()->m_InfSlimeHealRate))
	{
		if(m_pCharacter->GetHealthArmorSum() < Config()->m_InfSlimeMaxHeal)
		{
			m_pCharacter->IncreaseOverallHp(1);
		}
		m_SlimeHealTick = Server()->Tick();
	}
}

void CInfClassInfected::OnFloatingPointCollected(int Points)
{
	if(GetPlayerClass() != PLAYERCLASS_GHOUL)
		return;

	m_pCharacter->IncreaseOverallHp(4);
	IncreaseGhoulLevel(Points);
}

float CInfClassInfected::GetGhoulPercent() const
{
	return GetPlayer()->GetGhoulPercent();
}

void CInfClassInfected::IncreaseGhoulLevel(int Diff)
{
	GetPlayer()->IncreaseGhoulLevel(Diff);
}

int CInfClassInfected::GetGhoulLevel() const
{
	return GetPlayer()->GetGhoulLevel();
}

void CInfClassInfected::PrepareToDie(int Killer, int Weapon, bool *pRefusedToDie)
{
	if((Killer == GetCID()) && (Weapon == WEAPON_SELF))
	{
		// Accept the death to go with the default self kill routine
		return;
	}

	if(m_pCharacter->IsInvincible())
	{
		*pRefusedToDie = true;
		return;
	}

	// Start counting down, delay killer message for later
	if(GetPlayerClass() == PLAYERCLASS_VOODOO)
	{
		if(!m_VoodooAboutToDie)
		{
			m_VoodooAboutToDie = true;
			m_VoodooKiller = Killer;
			m_VoodooWeapon = Weapon;
			UpdateSkin();

			*pRefusedToDie = true;
			// If about to die, yet killed again, dont kill him either
		}
		else if(m_VoodooAboutToDie && m_VoodooTimeAlive > 0)
		{
			*pRefusedToDie = true;
		}
	}
}
