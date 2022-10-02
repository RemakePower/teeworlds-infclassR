/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <base/tl/ic_array.h>
#include <base/tl/string.h>

#include <engine/server.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <game/collision.h>
#include <game/layers.h>
#include <game/voting.h>
#include <game/server/classes.h>

#include <teeuniverses/components/localization.h>

#include "entities/character.h"
#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"

#include <fstream>

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/

#define BROADCAST_DURATION_REALTIME (0)
#define BROADCAST_DURATION_GAMEANNOUNCE (Server()->TickSpeed()*2)

struct FunRoundConfiguration
{
	FunRoundConfiguration() = default;
	FunRoundConfiguration(int Infected, int Human)
	: InfectedClass(Infected),
	  HumanClass(Human)
	{
	}

	int InfectedClass = 0;
	int HumanClass = 0;
};

enum
{
	BROADCAST_PRIORITY_LOWEST=0,
	BROADCAST_PRIORITY_WEAPONSTATE,
	BROADCAST_PRIORITY_EFFECTSTATE,
	BROADCAST_PRIORITY_GAMEANNOUNCE,
	BROADCAST_PRIORITY_SERVERANNOUNCE,
	BROADCAST_PRIORITY_INTERFACE,
};

class CConfig;

class CGameContext : public IGameServer
{
	IServer *m_pServer;
	CConfig *m_pConfig;
	IStorage *m_pStorage;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConSkipMap(IConsole::IResult *pResult, void *pUserData);
	static void ConQueueMap(IConsole::IResult *pResult, void *pUserData);
	static void ConAddMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConAddMapVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConStartFunRound(IConsole::IResult *pResult, void *pUserData);
	static void ConStartSpecialFunRound(IConsole::IResult *pResult, void *pUserData);
	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSyncMapRotation(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);
	void AddVote(const char *pDescription, const char *pCommand);
	static int MapScan(const char *pName, int IsDir, int DirType, void *pUserData);

	bool m_Resetting;

public:
	struct CPersistentClientData
	{
		bool m_IsSpectator;
		bool m_ClientNameLocked;
	};

public:
	IServer *Server() const { return m_pServer; }
	CConfig *Config() { return m_pConfig; }
	IStorage *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	CGameWorld *GameWorld() { return &m_World; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	virtual class CLayers *Layers() { return &m_Layers; }

	CGameContext();
	~CGameContext();

	void Clear();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
	CGameWorld m_World;

	// helper functions
	CPlayer *GetPlayer(int ClientID) const;
	class CCharacter *GetPlayerChar(int ClientID);
	// InfClassR
	int GetZombieCount();
	int GetZombieCount(int zombie_class);

	// InfClassR fun round
	bool StartFunRound(const FunRoundConfiguration &Configuration);
	void EndFunRound();
	bool m_FunRound;
	int m_FunRoundsPassed;
	std::vector<int> m_DefaultAvailabilities, m_DefaultProbabilities;
	void SetAvailabilities(std::vector<int> value);
	void SetProbabilities(std::vector<int> value);

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	bool HasActiveVote() const;

	int m_VoteCreator;
	int64_t m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, int64_t Mask = -1);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, int64_t Mask = -1);
	void CreateHammerHit(vec2 Pos, int64_t Mask = -1);
	void CreatePlayerSpawn(vec2 Pos, int64_t Mask = -1);
	void CreateDeath(vec2 Pos, int Who, int64_t Mask = -1);
	void CreateSound(vec2 Pos, int Sound, int64_t Mask = -1);
	void CreateSoundGlobal(int Sound, int Target = -1);

	enum
	{
		CHAT_ALL = -2,
		CHAT_SPEC = -1,
		CHAT_RED = 0,
		CHAT_BLUE = 1,
		CHAT_WHISPER_SEND = 2,
		CHAT_WHISPER_RECV = 3,
	};

	// network
	void CallVote(int ClientID, const char *aDesc, const char *aCmd, const char *pReason, const char *aChatmsg);
	void SendChatTarget(int To, const char *pText);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendMotd(int ClientID);
	void SendKillMessage(int Killer, int Victim, int Weapon, int ModeSpecial);

	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);
	void SendTuningParams(int ClientID, const CTuningParams &params);

	struct CVoteOptionServer *GetVoteOption(int Index);
	void ProgressVoteOptions(int ClientID);

	// engine events
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	void CensorMessage(char *pCensoredMessage, const char *pMessage, int Size);
	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual bool OnClientDataPersist(int ClientID, void *pData);
	virtual void OnClientConnected(int ClientID, void *pData);
	virtual void OnClientEnter(int ClientID);
	virtual void OnClientDrop(int ClientID, int Type, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	virtual bool IsClientBot(int ClientID) const;
	virtual bool IsClientReady(int ClientID) const;
	virtual bool IsClientPlayer(int ClientID) const;

	virtual int PersistentClientDataSize() const;

	virtual const char *GameType() const;
	virtual const char *Version() const;
	virtual const char *NetVersion() const;
	int GetClientVersion(int ClientID) const;

	bool RateLimitPlayerVote(int ClientID);

/* INFECTION MODIFICATION START ***************************************/
public:
	int m_ChatResponseTargetID;
	int m_ChatPrintCBIndex;

private:
	static void ChatConsolePrintCallback(const char *pLine, void *pUser, ColorRGBA PrintColor);

	static void ConWhisper(IConsole::IResult *pResult, void *pUserData);
	static void ConConverse(IConsole::IResult *pResult, void *pUserData);

	static void ConCredits(IConsole::IResult *pResult, void *pUserData);
	static void ConAbout(IConsole::IResult *pResult, void *pUserData);
	void ConAbout(IConsole::IResult *pResult);
	static void ConVersion(IConsole::IResult *pResult, void *pUserData);
	static void ConRegister(IConsole::IResult *pResult, void *pUserData);
	static void ConLogin(IConsole::IResult *pResult, void *pUserData);
	static void ConLogout(IConsole::IResult *pResult, void *pUserData);
#ifdef CONF_SQL
	static void ConSetEmail(IConsole::IResult *pResult, void *pUserData);
	static void ConTop10(IConsole::IResult *pResult, void *pUserData);
	static void ConChallenge(IConsole::IResult *pResult, void *pUserData);
	static void ConRank(IConsole::IResult *pResult, void *pUserData);
	static void ConGoal(IConsole::IResult *pResult, void *pUserData);
	static void ConStats(IConsole::IResult *pResult, void *pUserData);
#endif
	static void ChatHelp(IConsole::IResult *pResult, void *pUserData);
	void ChatHelp(int ClientID, const char *pHelpPage);
	bool WriteClassHelpPage(dynamic_string *pOutput, const char *pLanguage, PLAYERCLASS PlayerClass);
	static void ConLanguage(IConsole::IResult *pResult, void *pUserData);
	static void ConCmdList(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeLog(IConsole::IResult *pResult, void *pUserData);
	void ConChangeLog(IConsole::IResult *pResult);
	static void ConReloadChangeLog(IConsole::IResult *pResult, void *pUserData);

	static void ConClearFunRounds(IConsole::IResult *pResult, void *pUserData);
	static void ConAddFunRound(IConsole::IResult *pResult, void *pUserData);

	bool PrivateMessage(const char* pStr, int ClientID, bool TeamChat);
	void Whisper(int ClientID, char *pStr);
	void WhisperID(int ClientID, int VictimID, const char *pMessage);
	void Converse(int ClientID, const char *pStr);
	void MutePlayer(const char* pStr, int ClientID);

	void InitGeolocation();

	enum OPTION_VOTE_TYPE
	{
		OTHER_OPTION_VOTE_TYPE = 0,
		SV_MAP = 1,
		CHANGE_MAP = 2,
		SKIP_MAP = 3,
		PLAY_MORE_VOTE_TYPE = 4,
		QUEUED_VOTE = 8,

		MAP_VOTE_BITS = SV_MAP | CHANGE_MAP | SKIP_MAP, // Yeah, this is just '3'
	};
	
	void OnCallVote(void *pRawMsg, int ClientID);
	static OPTION_VOTE_TYPE GetOptionVoteType(const char *pVoteCommand);
	void GetMapNameFromCommand(char* pMapName, const char *pCommand);

public:
	virtual void OnSetAuthed(int ClientID,int Level);
	
	virtual void SendBroadcast(int To, const char *pText, int Priority, int LifeSpan);
	virtual void SendBroadcast_Localization(int To, int Priority, int LifeSpan, const char* pText, ...);
	virtual void SendBroadcast_Localization_P(int To, int Priority, int LifeSpan, int Number, const char* pText, ...);
	virtual void SendBroadcast_ClassIntro(int To, int Class);
	virtual void ClearBroadcast(int To, int Priority);
	
	static const char *GetChatCategoryPrefix(int Category);
	virtual void SendChatTarget_Localization(int To, int Category, const char* pText, ...);
	virtual void SendChatTarget_Localization_P(int To, int Category, int Number, const char* pText, ...);
	
	virtual void SendMOTD(int To, const char* pParam);
	virtual void SendMOTD_Localization(int To, const char* pText, ...);
	
	void CreateLaserDotEvent(vec2 Pos0, vec2 Pos1, int LifeSpan);
	void CreateHammerDotEvent(vec2 Pos, int LifeSpan);
	void CreateLoveEvent(vec2 Pos);
	void SendHitSound(int ClientID);
	void SendScoreSound(int ClientID);
	void AddBroadcast(int ClientID, const char* pText, int Priority, int LifeSpan);
	void SetClientLanguage(int ClientID, const char *pLanguage);
	void InitChangelog();
	void ReloadChangelog();

	bool MapExists(const char *pMapName) const;
	
private:
	int m_VoteLanguageTick[MAX_CLIENTS];
	char m_VoteLanguage[MAX_CLIENTS][16];
	int m_VoteBanClientID;
	static bool m_ClientMuted[MAX_CLIENTS][MAX_CLIENTS]; // m_ClientMuted[i][j]: i muted j
	static icArray<string, 256> m_aChangeLogEntries;
	static icArray<int, 16> m_aChangeLogPageIndices;
	
	class CBroadcastState
	{
	public:
		int m_NoChangeTick;
		char m_PrevMessage[1024];
		
		int m_Priority;
		char m_NextMessage[1024];
		
		int m_LifeSpanTick;
		int m_TimedPriority;
		char m_TimedMessage[1024];
	};

	static void ConList(IConsole::IResult *pResult, void *pUserData);

	
	CBroadcastState m_BroadcastStates[MAX_CLIENTS];
	
	struct LaserDotState
	{
		vec2 m_Pos0;
		vec2 m_Pos1;
		int m_LifeSpan;
		int m_SnapID;
	};
	array<LaserDotState> m_LaserDots;
	
	struct HammerDotState
	{
		vec2 m_Pos;
		int m_LifeSpan;
		int m_SnapID;
	};
	array<HammerDotState> m_HammerDots;
	
	struct LoveDotState
	{
		vec2 m_Pos;
		int m_LifeSpan;
		int m_SnapID;
	};
	array<LoveDotState> m_LoveDots;
	
	int m_aHitSoundState[MAX_CLIENTS]; //1 for hit, 2 for kill (no sounds must be sent)	

public:
	std::vector<FunRoundConfiguration> m_FunRoundConfigurations;

public:
/* INFECTION MODIFICATION END *****************************************/
	// InfClassR begin
	std::ofstream fout;
	// InfClassR end
	bool IsVersionBanned(int Version);
};

inline int64_t CmaskAll() { return -1LL; }
inline int64_t CmaskOne(int ClientID) { return 1LL<<ClientID; }
inline int64_t CmaskAllExceptOne(int ClientID) { return CmaskAll()^CmaskOne(ClientID); }
inline bool CmaskIsSet(int64_t Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
#endif
