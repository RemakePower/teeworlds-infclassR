#include "infcplayer.h"

#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/infclass/infcgamecontroller.h>

#include "classes/humans/human.h"
#include "classes/infcplayerclass.h"
#include "classes/infected/infected.h"
#include "entities/infccharacter.h"

MACRO_ALLOC_POOL_ID_IMPL(CInfClassPlayer, MAX_CLIENTS)

CInfClassPlayer::CInfClassPlayer(CInfClassGameController *pGameController, int ClientID, int Team)
	: CPlayer(pGameController->GameServer(), ClientID, Team)
	, m_pGameController(pGameController)
{
	m_class = PLAYERCLASS_INVALID;
	SetClass(PLAYERCLASS_NONE);
}

CInfClassPlayer::~CInfClassPlayer()
{
	SetCharacterClass(nullptr);
}

CInfClassGameController *CInfClassPlayer::GameController()
{
	return m_pGameController;
}

void CInfClassPlayer::TryRespawn()
{
	SpawnContext Context;
	if(!GameController()->TryRespawn(this, &Context))
		return;

	m_Spawning = false;
	CInfClassCharacter *pCharacter = new(m_ClientID) CInfClassCharacter(GameController());

	m_pCharacter = pCharacter;
	pCharacter->Spawn(this, Context.SpawnPos);
	m_pInfcPlayerClass->SetCharacter(pCharacter);

	pCharacter->OnCharacterSpawned(Context);
}

void CInfClassPlayer::Tick()
{
	if(!Server()->ClientIngame(m_ClientID))
		return;

	HandleInfection();

	CPlayer::Tick();

	if(!GameServer()->m_World.m_Paused)
	{
		if(IsHuman())
			m_HumanTime++;
	}

	if(m_MapMenu > 0)
		m_MapMenuTick++;

	if(GetClass() == PLAYERCLASS_GHOUL)
	{
		if(m_GhoulLevel > 0)
		{
			m_GhoulLevelTick--;

			if(m_GhoulLevelTick <= 0)
			{
				m_GhoulLevelTick = (Server()->TickSpeed() * GameServer()->Config()->m_InfGhoulDigestion);
				IncreaseGhoulLevel(-1);
				GetCharacterClass()->UpdateSkin();
			}
		}
	}

	HandleTuningParams();
}

void CInfClassPlayer::SnapClientInfo(int SnappingClient)
{
	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));
	StrToInts(&pClientInfo->m_Clan0, 3, GetClan(SnappingClient));
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);

	IServer::CClientInfo SnappingClientInfo = {0};
	if(SnappingClient != DemoClientID)
	{
		Server()->GetClientInfo(SnappingClient, &SnappingClientInfo);
	}

	if(
		GameServer()->GetPlayer(SnappingClient) && IsHuman() &&
		(
			(Server()->GetClientCustomSkin(SnappingClient) == 1 && SnappingClient == GetCID()) ||
			(Server()->GetClientCustomSkin(SnappingClient) == 2)
		)
	)
	{
		StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_CustomSkinName);
	}
	else
	{
		StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	}

	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	if(SnappingClientInfo.m_InfClassVersion || (SnappingClient == DemoClientID))
	{
		CNetObj_InfClassPlayer *pInfClassPlayer = static_cast<CNetObj_InfClassPlayer *>(Server()->SnapNewItem(NETOBJTYPE_INFCLASSPLAYER, m_ClientID, sizeof(CNetObj_InfClassPlayer)));
		if(!pInfClassPlayer)
			return;

		pInfClassPlayer->m_Class = m_class;
		pInfClassPlayer->m_Flags = 0;
		if(IsZombie())
		{
			pInfClassPlayer->m_Flags |= INFCLASS_PLAYER_FLAG_INFECTED;
		}
	}
}

void CInfClassPlayer::HandleInfection()
{
	if(m_DoInfection == DO_INFECTION::NO)
	{
		return;
	}
	if(IsZombie() && (m_DoInfection != DO_INFECTION::FORCED))
	{
		// Do not infect if inf class already set
		m_DoInfection = DO_INFECTION::NO;
		return;
	}

	if(IsHuman())
	{
		m_InfectionTick = Server()->Tick();
	}

	const int PreviousClass = GetClass();
	CInfClassPlayer *pInfectiousPlayer = GameController()->GetPlayer(m_InfectiousPlayerCID);

	m_DoInfection = DO_INFECTION::NO;
	m_InfectiousPlayerCID = -1;

	GameController()->OnPlayerInfected(this, pInfectiousPlayer, PreviousClass);
}

void CInfClassPlayer::KillCharacter(int Weapon)
{
	if((Weapon == WEAPON_SELF) && IsHuman())
	{
		static const float SelfKillConfirmationTime = 3;
		if(Server()->Tick() > m_SelfKillAttemptTick + Server()->TickSpeed() * SelfKillConfirmationTime)
		{
			GameServer()->SendChatTarget_Localization(GetCID(), CHATCATEGORY_PLAYER,
				_("Self kill attempt prevented. Trigger self kill again to confirm."));
			m_SelfKillAttemptTick = Server()->Tick();
			// Reset last kill tick:
			m_LastKill = -1; // This could be done in the GameContext but let's keep it here to avoid conflicts
			return;
		}
	}

	CPlayer::KillCharacter(Weapon);
}

int CInfClassPlayer::GetDefaultEmote() const
{
	if(m_pInfcPlayerClass)
		return m_pInfcPlayerClass->GetDefaultEmote();

	return CPlayer::GetDefaultEmote();
}

CInfClassCharacter *CInfClassPlayer::GetCharacter()
{
	return static_cast<CInfClassCharacter*>(m_pCharacter);
}

void CInfClassPlayer::SetCharacterClass(CInfClassPlayerClass *pClass)
{
	if(m_pInfcPlayerClass)
	{
		m_pInfcPlayerClass->SetCharacter(nullptr);
		delete m_pInfcPlayerClass;
	}

	m_pInfcPlayerClass = pClass;
}

void CInfClassPlayer::SetClass(int newClass)
{
	if(m_class == newClass)
		return;

	if(newClass > START_HUMANCLASS && newClass < END_HUMANCLASS)
	{
		bool ClassFound = false;
		for(unsigned int i=0; i<sizeof(m_LastHumanClasses)/sizeof(int); i++)
		{
			if(m_LastHumanClasses[i] == newClass)
				ClassFound = true;
		}
		if(!ClassFound)
		{
			for(unsigned int i=0; i<sizeof(m_LastHumanClasses)/sizeof(int)-1; i++)
			{
				m_LastHumanClasses[i] = m_LastHumanClasses[i+1];
			}
			m_LastHumanClasses[sizeof(m_LastHumanClasses)/sizeof(int)-1] = newClass;
		}
	}

	if(newClass < END_HUMANCLASS)
	{
		m_LastHumanClass = newClass;
	}

	m_GhoulLevel = 0;
	m_GhoulLevelTick = 0;

	if(m_pInfcPlayerClass)
	{
		m_pInfcPlayerClass->SetCharacter(nullptr);
	}

	m_class = newClass;

	const bool HadHumanClass = GetCharacterClass() && GetCharacterClass()->IsHuman();
	const bool HadInfectedClass = GetCharacterClass() && GetCharacterClass()->IsZombie();

	if(IsHuman() && !HadHumanClass)
	{
		SetCharacterClass(new(m_ClientID) CInfClassHuman(this));
	}
	else if (IsZombie() && !HadInfectedClass)
	{
		SetCharacterClass(new(m_ClientID) CInfClassInfected(this));
	}

	// Skip the SetCharacter() routine if the World ResetRequested because it
	// means that the Character is going to be destroyed during this
	// IGameServer::Tick() which also invalidates possible auto class selection.
	if(!GameServer()->m_World.m_ResetRequested)
	{
		m_pInfcPlayerClass->SetCharacter(GetCharacter());
	}
	m_pInfcPlayerClass->OnPlayerClassChanged();
}

void CInfClassPlayer::Infect(CPlayer *pInfectiousPlayer)
{
	StartInfection(/* force */ false, pInfectiousPlayer);
}

void CInfClassPlayer::StartInfection(bool force, CPlayer *pInfectiousPlayer)
{
	if(!force && IsZombie())
		return;

	m_DoInfection = force ? DO_INFECTION::FORCED : DO_INFECTION::REGULAR;
	m_InfectiousPlayerCID = pInfectiousPlayer ? pInfectiousPlayer->GetCID() : -1;
}

void CInfClassPlayer::OpenMapMenu(int Menu)
{
	m_MapMenu = Menu;
	m_MapMenuTick = 0;
}

void CInfClassPlayer::CloseMapMenu()
{
	m_MapMenu = 0;
	m_MapMenuTick = -1;
}

bool CInfClassPlayer::MapMenuClickable()
{
	return (m_MapMenu > 0 && (m_MapMenuTick > Server()->TickSpeed()/2));
}

float CInfClassPlayer::GetGhoulPercent() const
{
	return clamp(m_GhoulLevel/static_cast<float>(g_Config.m_InfGhoulStomachSize), 0.0f, 1.0f);
}

void CInfClassPlayer::IncreaseGhoulLevel(int Diff)
{
	int NewGhoulLevel = m_GhoulLevel + Diff;
	m_GhoulLevel = clamp(NewGhoulLevel, 0, g_Config.m_InfGhoulStomachSize);
}

const char *CInfClassPlayer::GetClan(int SnappingClient) const
{
	if(GetTeam() == TEAM_SPECTATORS)
	{
		return Server()->ClientClan(m_ClientID);
	}

	int SnapScoreMode = PLAYERSCOREMODE_SCORE;
	if(GameServer()->GetPlayer(SnappingClient))
	{
		SnapScoreMode = GameServer()->m_apPlayers[SnappingClient]->GetScoreMode();
	}
	
	static char aBuf[32];

	if(SnapScoreMode == PLAYERSCOREMODE_TIME)
	{
		float RoundDuration = static_cast<float>(m_HumanTime/((float)Server()->TickSpeed()))/60.0f;
		int Minutes = static_cast<int>(RoundDuration);
		int Seconds = static_cast<int>((RoundDuration - Minutes)*60.0f);
		
		str_format(aBuf, sizeof(aBuf), "%i:%s%i min", Minutes,((Seconds < 10) ? "0" : ""), Seconds);
	}
	else
	{
		const char *ClassName = CInfClassGameController::GetClanForClass(GetClass(), "?????");
		str_format(aBuf, sizeof(aBuf), "%s%s", Server()->IsClientLogged(GetCID()) ? "@" : " ", ClassName);
	}

	// This is not thread-safe but we don't have threads.
	return aBuf;
}
