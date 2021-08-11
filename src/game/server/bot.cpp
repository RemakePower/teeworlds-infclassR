#include <game/gamecore.h>
#include <engine/shared/config.h>
#include <game/layers.h>
#include "gamecontext.h"

#include "bot.h"
#include "player.h"
#include "entities/character.h"

CBot::CBot(CBotEngine *pBotEngine, CPlayer *pPlayer)
{
	m_pBotEngine = pBotEngine;
	m_pPlayer = pPlayer;
	m_pGameServer = pBotEngine->GameServer();
	m_Flags = 0;
	mem_zero(&m_InputData, sizeof(m_InputData));
	m_LastData = m_InputData;

	m_SnapID = GameServer()->Server()->SnapNewID();
	m_ComputeTarget.m_Type = CTarget::TARGET_EMPTY;

	m_pPath = &(pBotEngine->m_aPaths[pPlayer->GetCID()]);
	UpdateTargetOrder();
}

CBot::~CBot()
{
	m_pPath->m_Size = 0;
	GameServer()->Server()->SnapFreeID(m_SnapID);
}

void CBot::OnReset()
{
	m_Flags = 0;
	m_pPath->m_Size = 0;
	m_ComputeTarget.m_Type = CTarget::TARGET_EMPTY;
}

void CBot::UpdateTargetOrder()
{
	const int *pPriority = &g_aBotPriority[m_pPlayer->GetCID() % 16][0];
	for(int i = 0 ; i < CTarget::NUM_TARGETS ; i++)
	{
		int j = i;
		while(j > 0 && pPriority[i] > pPriority[m_aTargetOrder[j-1]])
		{
			m_aTargetOrder[j] = m_aTargetOrder[j-1];
			j--;
		}
		m_aTargetOrder[j] = i;
	}
}

vec2 CBot::ClosestCharacter()
{
	float d = -1;
	vec2 Pos = vec2(0,0);
	for(int c = 0; c < MAX_CLIENTS; c++)
		if(c != m_pPlayer->GetCID() && GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && (d < -1 || d > distance(m_pPlayer->GetCharacter()->GetPos(),GameServer()->m_apPlayers[c]->GetCharacter()->GetPos())))
		{
			d = distance(m_pPlayer->GetCharacter()->GetPos(),GameServer()->m_apPlayers[c]->GetCharacter()->GetPos());
			Pos = GameServer()->m_apPlayers[c]->GetCharacter()->GetPos();
		}
	return Pos;
}

void CBot::GiveUpTarget()
{
	m_ComputeTarget.m_Type = CTarget::TARGET_EMPTY;
	m_ComputeTarget.m_NeedUpdate = true;
}

void CBot::OnCharacterDeath(int Victim, int Killer, int Weapon)
{
	(void) Weapon;
	(void) Killer;
	if (m_ComputeTarget.m_Type == CTarget::TARGET_PLAYER && m_ComputeTarget.m_PlayerCID == Victim)
		GiveUpTarget();

	GameServer()->SendEmoticon(m_pPlayer->GetCID(),EMOTICON_EYES);
}

CBot::CTarget CBot::GetNewTarget()
{
	CBot::CTarget Target = m_ComputeTarget;
	Target.m_NeedUpdate = Target.m_Type == CTarget::TARGET_EMPTY;

	// Player target character doesn't exist
	if(Target.m_Type == CTarget::TARGET_PLAYER && !(GameServer()->m_apPlayers[Target.m_PlayerCID] && GameServer()->m_apPlayers[Target.m_PlayerCID]->GetCharacter()))
		Target.m_NeedUpdate = true;

	// Give up on actual target after 30s
	if(Target.m_StartTick + GameServer()->Server()->TickSpeed()*30 < GameServer()->Server()->Tick())
		Target.m_NeedUpdate = true;

	// Close enough to random air target
	if(Target.m_Type == CTarget::TARGET_AIR)
	{
		float dist = distance(m_pPlayer->GetCharacter()->GetPos(), Target.m_Pos);
		if(dist < 60)
			Target.m_NeedUpdate = true;
	}

	// Close enough to the pickup
	if(Target.m_Type > CTarget::TARGET_PLAYER)
	{
		float dist = distance(m_pPlayer->GetCharacter()->GetPos(), Target.m_Pos);
		if(dist < 28)
			Target.m_NeedUpdate = true;
	}

#if 0
	// Bypass when flag game and flag close enough
	if(GameServer()->m_pController->IsFlagGame()) {
		int Team = m_pPlayer->GetTeam();
		CGameControllerCTF *pController = (CGameControllerCTF*)GameServer()->m_pController;
		CFlag **apFlags = pController->m_apFlags;
		if(apFlags[Team] && distance(m_pPlayer->GetCharacter()->GetPos(), apFlags[Team]->GetPos()) < 300)
		{
			// Retrieve missing flag
			if(!apFlags[Team]->IsAtStand() && !apFlags[Team]->GetCarrier())
			{
				Target.m_Pos = apFlags[Team]->GetPos();
				Target.m_Type = CTarget::TARGET_FLAG;
				Target.m_SubType = BTARGET_RETURN_FLAG;
				return Target;
			}
			// Target flag carrier
			if(!apFlags[Team]->IsAtStand() && apFlags[Team]->GetCarrier() && apFlags[Team]->GetCarrier()->IsAlive())
			{
				Target.m_Pos = apFlags[Team]->GetPos();
				Target.m_Type = CTarget::TARGET_PLAYER;
				Target.m_SubType = BTARGET_CHASE_CARRIER;
				Target.m_PlayerCID = apFlags[Team]->GetCarrier()->GetPlayer()->GetCID();
				return Target;
			}
		}
		if(apFlags[Team^1] && distance(m_pPlayer->GetCharacter()->GetPos(), apFlags[Team^1]->GetPos()) < 300)
		{
			// Go to enemy flagstand
			if(apFlags[Team^1]->IsAtStand())
			{
				Target.m_Pos = BotEngine()->GetFlagStandPos(Team^1);
				Target.m_Type = CTarget::TARGET_FLAG;
				Target.m_SubType = BTARGET_GRAB_FLAG;
				return Target;
			}
			// Go to base carrying flag
			if(apFlags[Team^1]->GetCarrier() == m_pPlayer->GetCharacter() && (!apFlags[Team] || apFlags[Team]->IsAtStand()))
			{
				Target.m_Pos = BotEngine()->GetFlagStandPos(Team);
				Target.m_Type = CTarget::TARGET_FLAG;
				Target.m_SubType = BTARGET_CARRY_FLAG;
				return Target;
			}
		}
	}
#endif

	if(Target.m_NeedUpdate)
	{
		Target.m_Type = CTarget::TARGET_EMPTY;
		Target.m_SubType = BTARGET_NONE;
		vec2 NewTarget;
		for(int i = 0 ; i < CTarget::NUM_TARGETS ; i++)
		{
			switch(m_aTargetOrder[i])
			{
			case CTarget::TARGET_FLAG:
#if 0
				if(GameServer()->m_pController->IsFlagGame()) {
					int Team = m_pPlayer->GetTeam();
					CGameControllerCTF *pController = (CGameControllerCTF*)GameServer()->m_pController;
					CFlag **apFlags = pController->m_apFlags;
					if(apFlags[Team])
					{
						// Retrieve missing flag
						if(!apFlags[Team]->IsAtStand() && !apFlags[Team]->GetCarrier())
						{
							Target.m_Pos = apFlags[Team]->GetPos();
							Target.m_Type = CTarget::TARGET_FLAG;
							Target.m_SubType = BTARGET_RETURN_FLAG;
							return Target;
						}
						// Target flag carrier
						if(!apFlags[Team]->IsAtStand() && apFlags[Team]->GetCarrier())
						{
							Target.m_Pos = apFlags[Team]->GetPos();
							Target.m_Type = CTarget::TARGET_PLAYER;
							Target.m_SubType = BTARGET_CHASE_CARRIER;
							Target.m_PlayerCID = apFlags[Team]->GetCarrier()->GetPlayer()->GetCID();
							return Target;
						}
					}
					if(apFlags[Team^1])
					{
						// Go to enemy flagstand
						if(apFlags[Team^1]->IsAtStand())
						{
							Target.m_Pos = BotEngine()->GetFlagStandPos(Team^1);
							Target.m_Type = CTarget::TARGET_FLAG;
							Target.m_SubType = BTARGET_GRAB_FLAG;
							return Target;
						}
						// Go to base carrying flag
						if(apFlags[Team^1]->GetCarrier() == m_pPlayer->GetCharacter() && (!apFlags[Team] || apFlags[Team]->IsAtStand()))
						{
							Target.m_Pos = BotEngine()->GetFlagStandPos(Team);
							Target.m_Type = CTarget::TARGET_FLAG;
							Target.m_SubType = BTARGET_CARRY_FLAG;
							return Target;
						}
					}
				}
				break;
			case CTarget::TARGET_ARMOR:
			case CTarget::TARGET_HEALTH:
			case CTarget::TARGET_WEAPON_SHOTGUN:
			case CTarget::TARGET_WEAPON_GRENADE:
			case CTarget::TARGET_WEAPON_LASER:
				{
					float Radius = distance(m_pPlayer->GetCharacter()->GetPos(), ClosestCharacter());
					if(NeedPickup(m_aTargetOrder[i]) && FindPickup(m_aTargetOrder[i], &Target.m_Pos, Radius))
					{
						Target.m_Type = m_aTargetOrder[i];
						return Target;
					}
				}
				break;
#endif
			case CTarget::TARGET_PLAYER:
				{
					int Team = m_pPlayer->GetTeam();
					bool ZTeam = m_pPlayer->IsActuallyZombie();
					int Count = 0;
					for(int c = 0; c < MAX_CLIENTS; c++)
						if(c != m_pPlayer->GetCID() && GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && ((GameServer()->m_apPlayers[c]->IsActuallyZombie() != ZTeam) || !GameServer()->m_pController->IsTeamplay()))
							Count++;
					if(Count)
					{
						Count = random_int(0, Count - 1) + 1;
						int c = 0;
						for(; Count; c++)
							if(c != m_pPlayer->GetCID() && GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && ((GameServer()->m_apPlayers[c]->IsActuallyZombie() != ZTeam) || !GameServer()->m_pController->IsTeamplay()))
								Count--;
						c--;
						Target.m_Pos = GameServer()->m_apPlayers[c]->GetCharacter()->GetPos();
						Target.m_Type = CTarget::TARGET_PLAYER;
						Target.m_PlayerCID = c;
						return Target;
					}
				}
				break;
			case CTarget::TARGET_AIR:
				{
					// Random destination
					int r = random_int(0, BotEngine()->GetGraph()->NumVertices() - 1);
					Target.m_Pos = BotEngine()->GetGraph()->GetVertex(r);
					Target.m_Type = CTarget::TARGET_AIR;
					return Target;
				}
			}
		}
	}

	if(Target.m_Type == CTarget::TARGET_PLAYER)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[Target.m_PlayerCID];
		if(Collision()->FastIntersectLine(Target.m_Pos, pPlayer->GetCharacter()->GetPos(),0,0))
		{
			Target.m_NeedUpdate = true;
			Target.m_Pos = pPlayer->GetCharacter()->GetPos();
		}
	}
	return Target;
}

void CBot::UpdateTarget()
{
	CBot::CTarget Target = GetNewTarget();
	// if (Target.m_SubType != BTARGET_NONE && Target.m_SubType != m_ComputeTarget.m_SubType)
	// {
	// 	dbg_msg("bot", "Change flag target type: %d -> %d", m_ComputeTarget.m_SubType, Target.m_SubType);
	// }
	if (Target.m_NeedUpdate || Target.m_Type != m_ComputeTarget.m_Type || Target.m_SubType != m_ComputeTarget.m_SubType || Target.m_PlayerCID != m_ComputeTarget.m_PlayerCID) {
		m_ComputeTarget = Target;
		m_ComputeTarget.m_NeedUpdate = true;
		m_ComputeTarget.m_StartTick = GameServer()->Server()->Tick();
	}
}

bool CBot::NeedPickup(int Type)
{
#if 0
	switch(Type)
	{
	case CTarget::TARGET_HEALTH:
		return m_pPlayer->GetCharacter()->GetHealth() < 5;
	case CTarget::TARGET_ARMOR:
		return m_pPlayer->GetCharacter()->GetArmor() < 5;
	case CTarget::TARGET_WEAPON_SHOTGUN:
		return m_pPlayer->GetCharacter()->GetAmmoCount(WEAPON_SHOTGUN) < 5;
	case CTarget::TARGET_WEAPON_GRENADE:
		return m_pPlayer->GetCharacter()->GetAmmoCount(WEAPON_GRENADE) < 5;
	case CTarget::TARGET_WEAPON_LASER:
		return m_pPlayer->GetCharacter()->GetAmmoCount(WEAPON_LASER) < 5;
	}
#endif
	return false;
}

bool CBot::FindPickup(int Type, vec2 *pPos, float Radius)
{
#if 0
	switch(Type)
	{
		case CTarget::TARGET_ARMOR:
			Type = PICKUP_ARMOR;
			break;
		case CTarget::TARGET_HEALTH:
			Type = PICKUP_HEALTH;
			break;
		case CTarget::TARGET_WEAPON_SHOTGUN:
			Type = PICKUP_SHOTGUN;
			break;
		case CTarget::TARGET_WEAPON_GRENADE:
			Type = PICKUP_GRENADE;
			break;
		case CTarget::TARGET_WEAPON_LASER:
			Type = PICKUP_LASER;
			break;
	}
	CEntity *pEnt = GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP);
	bool Found = false;
	for(;	pEnt; pEnt = pEnt->TypeNext())
	{
		CPickup *pPickup = (CPickup *) pEnt;
		if(pPickup->GetType() == Type && pPickup->IsSpawned() && Radius > distance(pPickup->GetPos(),m_pPlayer->GetCharacter()->GetPos()) )
		{
			*pPos = pPickup->GetPos();
			Radius = distance(pPickup->GetPos(),m_pPlayer->GetCharacter()->GetPos());
			Found = true;
		}
	}
	return Found;
#else
	return false;
#endif
}

bool CBot::IsGrounded()
{
	return m_pPlayer->GetCharacter()->IsGrounded();
}

void CBot::Tick()
{
	if(!m_pPlayer->GetCharacter())
		return;
	const CCharacterCore *pMe = m_pPlayer->GetCharacter()->GetCore();

	UpdateTarget();

	UpdateEdge();

	mem_zero(&m_InputData, sizeof(m_InputData));

	m_InputData.m_WantedWeapon = m_LastData.m_WantedWeapon;

	vec2 Pos = pMe->m_Pos;

	bool InSight = false;
	if(m_ComputeTarget.m_Type == CTarget::TARGET_PLAYER)
	{
		const CCharacterCore *pClosest = GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID]->GetCharacter()->GetCore();
		InSight = !Collision()->FastIntersectLine(Pos, pClosest->m_Pos, 0, 0);
		m_Target = pClosest->m_Pos - Pos;
		m_RealTarget = pClosest->m_Pos;
	}

	if(GameServer()->Config()->m_SvBotAllowMove)
		MakeChoice(InSight);

	m_RealTarget = m_Target + Pos;

	if(GameServer()->Config()->m_SvBotAllowFire && m_pPlayer->GetCharacter()->CanFire())
		HandleWeapon(InSight);

	if(GameServer()->Config()->m_SvBotAllowMove && GameServer()->Config()->m_SvBotAllowHook)
		HandleHook(InSight);

	if(m_Flags & BFLAG_LEFT)
			m_InputData.m_Direction = -1;
	if(m_Flags & BFLAG_RIGHT)
			m_InputData.m_Direction = 1;
	if(m_Flags & BFLAG_JUMP)
			m_InputData.m_Jump = 1;
	// else if(!m_InputData.m_Fire && m_Flags & BFLAG_FIRE && m_LastData.m_Fire == 0)
	// 	m_InputData.m_Fire = 1;

	// if(InSight && diffPos.y < - Close && diffVel.y < 0)
	// 	m_InputData.m_Jump = 1;

	m_InputData.m_TargetX = m_LastData.m_TargetX;
	m_InputData.m_TargetY = m_LastData.m_TargetY;
	if(m_InputData.m_Hook || m_InputData.m_Fire) {
		m_InputData.m_TargetX = m_Target.x;
		m_InputData.m_TargetY = m_Target.y;
	}


	if(!GameServer()->Config()->m_SvBotAllowMove) {
		m_InputData.m_Direction = 0;
		m_InputData.m_Jump = 0;
		m_InputData.m_Hook = 0;
	}
	if(!GameServer()->Config()->m_SvBotAllowHook)
		m_InputData.m_Hook = 0;

	m_LastData = m_InputData;
	return;
}

void CBot::HandleHook(bool SeeTarget)
{
	CCharacterCore *pMe = m_pPlayer->GetCharacter()->GetCore();

	if(!pMe)
		return;
	int CurTile = GetTile(pMe->m_Pos.x, pMe->m_Pos.y);
	if(pMe->m_HookState == HOOK_FLYING)
	{
		m_InputData.m_Hook = 1;
		return;
	}
	if(SeeTarget)
	{
		const CCharacterCore *pClosest = GameServer()->m_apPlayers[m_ComputeTarget.m_PlayerCID]->GetCharacter()->GetCore();
		float dist = distance(pClosest->m_Pos,pMe->m_Pos);
		if(pMe->m_HookState == HOOK_GRABBED && pMe->m_HookedPlayer == m_ComputeTarget.m_PlayerCID)
			m_InputData.m_Hook = 1;
		else if(!m_InputData.m_Fire)
		{
			if(dist < Tuning()->m_HookLength*0.9f)
				m_InputData.m_Hook = m_LastData.m_Hook^1;
			SeeTarget = dist < Tuning()->m_HookLength*0.9f;
		}
	}

	if(!SeeTarget)
	{
		if(pMe->m_HookState == HOOK_GRABBED && pMe->m_HookedPlayer == -1)
		{
			vec2 HookVel = normalize(pMe->m_HookPos-pMe->m_Pos)*GameServer()->Tuning()->m_HookDragAccel;

			// from gamecore;cpp
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;
			if((HookVel.x < 0 && pMe->m_Input.m_Direction < 0) || (HookVel.x > 0 && pMe->m_Input.m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			HookVel += vec2(0,1)*GameServer()->Tuning()->m_Gravity;

			vec2 Target = m_Target;
			float ps = dot(Target, HookVel);
			if(ps > 0 || (CurTile & BTILE_HOLE && m_Target.y < 0 && pMe->m_Vel.y > 0.f && pMe->m_HookTick < SERVER_TICK_SPEED + SERVER_TICK_SPEED/2))
				m_InputData.m_Hook = 1;
			if(pMe->m_HookTick > 4*SERVER_TICK_SPEED || length(pMe->m_HookPos-pMe->m_Pos) < 20.0f)
				m_InputData.m_Hook = 0;
		}
		if(pMe->m_HookState == HOOK_FLYING)
			m_InputData.m_Hook = 1;
		// do random hook
		if(!m_InputData.m_Fire && m_LastData.m_Hook == 0 && pMe->m_HookState == HOOK_IDLE && (random_prob(0.1) || (CurTile & BTILE_HOLE && random_prob(0.25) == 0)))
		{
			int NumDir = BOT_HOOK_DIRS;
			vec2 HookDir(0.0f,0.0f);
			float MaxForce = (CurTile & BTILE_HOLE) ? -10000.0f : 0;
			vec2 Target = m_Target;
			for(int i = 0 ; i < NumDir; i++)
			{
				float a = 2*i*pi / NumDir;
				vec2 dir = direction(a);
				vec2 Pos = pMe->m_Pos+dir*Tuning()->m_HookLength;

				if((Collision()->FastIntersectLine(pMe->m_Pos,Pos,&Pos,0) & (CCollision::COLFLAG_SOLID | CCollision::COLFLAG_NOHOOK)) == CCollision::COLFLAG_SOLID)
				{
					vec2 HookVel = dir*GameServer()->Tuning()->m_HookDragAccel;

					// from gamecore.cpp
					if(HookVel.y > 0)
						HookVel.y *= 0.3f;
					if((HookVel.x < 0 && pMe->m_Input.m_Direction < 0) || (HookVel.x > 0 && pMe->m_Input.m_Direction > 0))
						HookVel.x *= 0.95f;
					else
						HookVel.x *= 0.75f;

					HookVel += vec2(0,1)*GameServer()->Tuning()->m_Gravity;

					float ps = dot(Target, HookVel);
					if( ps > MaxForce)
					{
						MaxForce = ps;
						HookDir = Pos - pMe->m_Pos;
					}
				}
			}
			if(length(HookDir) > 32.f)
			{
				m_Target = HookDir;
				m_InputData.m_Hook = 1;
				// if(Collision()->CheckPoint(pMe->m_Pos+normalize(vec2(0,m_Target.y))*28) && absolute(Target.x) < 30)
				// 	Flags = (Flags & (~BFLAG_LEFT)) | BFLAG_RIGHT;
			}
		}
	}
}

void CBot::HandleWeapon(bool SeeTarget)
{
	CCharacter *pMe = m_pPlayer->GetCharacter();

	if(!pMe)
		return;

	int Team = m_pPlayer->GetTeam();
	vec2 Pos = pMe->GetCore()->m_Pos;

	CCharacterCore* apTarget[MAX_CLIENTS];
	int Count = 0;

	for(int c = 0 ; c < MAX_CLIENTS ; c++)
	{
		if(c == m_pPlayer->GetCID())
			continue;
		if(SeeTarget && c == m_ComputeTarget.m_PlayerCID)
		{
			apTarget[Count++] = apTarget[0];
			apTarget[0] =	GameServer()->m_apPlayers[c]->GetCharacter()->GetCore();
		}
		else if(GameServer()->m_apPlayers[c] && GameServer()->m_apPlayers[c]->GetCharacter() && (GameServer()->m_apPlayers[c]->GetTeam() != Team || !GameServer()->m_pController->IsTeamplay()))
			apTarget[Count++] = GameServer()->m_apPlayers[c]->GetCharacter()->GetCore();
	}
	int Weapon = -1;
	vec2 Target;
	for(int c = 0; c < Count; c++)
	{
		float ClosestRange = distance(Pos, apTarget[c]->m_Pos);
		float Close = 65.0f;
		Target = apTarget[c]->m_Pos - Pos;
		if(ClosestRange < Close)
		{
			Weapon = WEAPON_HAMMER;
		}
		else if(pMe->GetAmmoCount(WEAPON_LASER) != 0 && ClosestRange < GameServer()->Tuning()->m_LaserReach && !Collision()->FastIntersectLine(Pos, apTarget[c]->m_Pos, 0, 0))
		{
			Weapon = WEAPON_LASER;
		}
	}
	if(Weapon < 0)
	{
		int GoodDir = -1;

		vec2 aProjectilePos[BOT_HOOK_DIRS];

		const int NbLoops = 10;

		vec2 aTargetPos[MAX_CLIENTS];
		vec2 aTargetVel[MAX_CLIENTS];

		const int Weapons[] = {WEAPON_GRENADE, WEAPON_SHOTGUN, WEAPON_GUN};
		for(int j = 0 ; j < 3 ; j++)
		{
			if(!pMe->GetAmmoCount(Weapons[j]))
				continue;
			float Curvature = 0, Speed = 0, Time = 0;
			switch(Weapons[j])
			{
				case WEAPON_GRENADE:
					Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
					Speed = GameServer()->Tuning()->m_GrenadeSpeed;
					Time = GameServer()->Tuning()->m_GrenadeLifetime;
					break;

				case WEAPON_SHOTGUN:
					Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
					Speed = GameServer()->Tuning()->m_ShotgunSpeed;
					Time = GameServer()->Tuning()->m_ShotgunLifetime;
					break;

				case WEAPON_GUN:
				default:
					Curvature = GameServer()->Tuning()->m_GunCurvature;
					Speed = GameServer()->Tuning()->m_GunSpeed;
					Time = GameServer()->Tuning()->m_GunLifetime;
					break;
			}

			int DTick = (int) (Time * GameServer()->Server()->TickSpeed() / NbLoops);

			for(int c = 0; c < Count; c++)
			{
				aTargetPos[c] = apTarget[c]->m_Pos;
				aTargetVel[c] = apTarget[c]->m_Vel*DTick;
			}

			for(int i = 0 ; i < BOT_HOOK_DIRS ; i++) {
				vec2 dir = direction(2*i*pi / BOT_HOOK_DIRS);
				aProjectilePos[i] = Pos + dir*28.*0.75;
			}

			int aIsDead[BOT_HOOK_DIRS] = {0};

			for(int k = 0; k < NbLoops && GoodDir == -1; k++) {
				for(int i = 0; i < BOT_HOOK_DIRS; i++) {
					if(aIsDead[i])
						continue;
					vec2 dir = direction(2*i*pi / BOT_HOOK_DIRS);
					vec2 NextPos = CalcPos(Pos + dir*28.*0.75, dir, Curvature, Speed, (k+1) * Time / NbLoops);

					aIsDead[i] = Collision()->FastIntersectLine(aProjectilePos[i], NextPos, &NextPos, 0);
					for(int c = 0; c < Count; c++)
					{
						vec2 InterPos = closest_point_on_line(aProjectilePos[i],NextPos, aTargetPos[c]);
						if(distance(aTargetPos[c], InterPos)< 28) {
							GoodDir = i;
							break;
						}
					}
					aProjectilePos[i] = NextPos;
				}
				for(int c = 0; c < Count; c++)
				{
					Collision()->FastIntersectLine(aTargetPos[c], aTargetPos[c]+aTargetVel[c], 0, &aTargetPos[c]);
					aTargetVel[c].y += GameServer()->Tuning()->m_Gravity*DTick*DTick;
				}
			}
			if(GoodDir != -1)
			{
				Target = direction(2*GoodDir*pi / BOT_HOOK_DIRS)*50;
				Weapon = Weapons[j];
				break;
			}
		}
	}
	if(Weapon > -1)
	{
		m_InputData.m_WantedWeapon = Weapon+1;
		m_InputData.m_Fire = m_LastData.m_Fire^1;
		if(m_InputData.m_Fire)
			m_Target = Target;
	}
	else if(pMe->GetAmmoCount(WEAPON_GUN) != 10)
		m_InputData.m_WantedWeapon = WEAPON_GUN+1;

	// Accuracy
	if (GameServer()->Config()->m_SvBotAccuracyError) {
		float Angle = angle(m_Target) + (random_int(0, 64)-32)*pi / 1024.0f;
		m_Target = direction(Angle)*length(m_Target);
	}
}

void CBot::UpdateEdge()
{
	vec2 Pos = m_pPlayer->GetCharacter()->GetPos();
	if(m_ComputeTarget.m_Type == CTarget::TARGET_EMPTY)
	{
		dbg_msg("bot", "no edge");
		return;
	}
	if(m_ComputeTarget.m_NeedUpdate)
	{
		m_pPath->m_Size = 0;
		BotEngine()->GetPath(Pos, m_ComputeTarget.m_Pos, m_pPath);
		m_ComputeTarget.m_NeedUpdate = false;
	}
}

void CBot::MakeChoice(bool UseTarget)
{
	if(!UseTarget)
	{
		vec2 Pos = m_pPlayer->GetCharacter()->GetPos();

		if(m_pPath->m_Size)
		{
			double dist = BotEngine()->FarestPointOnEdge(m_pPath, Pos, &m_Target);
			if(dist >= 0)
			{
				UseTarget = true;
				m_Target -= Pos;
			}
			else
			{
				bool Ret = BotEngine()->NextPoint(Pos, m_ComputeTarget.m_Pos, &m_Target);
				if(Ret)
					m_Target -= Pos;
				else
				{
					dbg_msg("bot", "could not find any path to target");
					m_ComputeTarget.m_NeedUpdate = true;
					return;
				}
			}
		}
	}

	int Flags = 0;
	CCharacterCore *pMe = m_pPlayer->GetCharacter()->GetCore();
	CCharacterCore TempChar = *pMe;
	TempChar.m_Input = m_InputData;
	vec2 CurPos = TempChar.m_Pos;

	CCharacterCore::CParams CoreTickParams(&m_pPlayer->m_NextTuningParams);
	//~ CCharacterCore::CParams CoreTickParams(&GameWorld()->m_Core.m_Tuning);

//	if(GetPlayerClass() == PLAYERCLASS_SPIDER)
//	{
//		CoreTickParams.m_HookGrabTime = g_Config.m_InfSpiderHookTime*SERVER_TICK_SPEED;
//	}
//	if(GetPlayerClass() == PLAYERCLASS_BAT)
//	{
//		CoreTickParams.m_HookGrabTime = g_Config.m_InfBatHookTime*SERVER_TICK_SPEED;
//	}
	CoreTickParams.m_HookMode = m_pPlayer->GetCharacter()->m_HookMode;

	int CurTile = GetTile(TempChar.m_Pos.x, TempChar.m_Pos.y);
	bool Grounded = IsGrounded();

	TempChar.m_Input.m_Direction = (m_Target.x > 28.f) ? 1 : (m_Target.x < -28.f) ? -1:0;
	CWorldCore TempWorld;
	TempWorld.m_Tuning = *GameServer()->Tuning();
	TempChar.Init(&TempWorld, Collision());
	TempChar.Tick(true, &CoreTickParams);
	TempChar.Move(&CoreTickParams);
	TempChar.Quantize();

	int NextTile = GetTile(TempChar.m_Pos.x, TempChar.m_Pos.y);
	vec2 NextPos = TempChar.m_Pos;

	if(TempChar.m_Input.m_Direction > 0)
		Flags |= BFLAG_RIGHT;

	if(TempChar.m_Input.m_Direction < 0)
		Flags |= BFLAG_LEFT;

	if(m_Target.y < 0)
	{
		if(CurTile & BTILE_SAFE && NextTile & BTILE_HOLE && (Grounded || TempChar.m_Vel.y > 0))
			Flags |= BFLAG_JUMP;
		if(CurTile & BTILE_SAFE && NextTile & BTILE_SAFE)
		{
			static bool tried = false;
			if(absolute(CurPos.x - NextPos.x) < 1.0f && TempChar.m_Input.m_Direction)
			{
				if(Grounded)
				{
					Flags |= BFLAG_JUMP;
					tried = true;
				}
				else if(tried && !(TempChar.m_Jumped) && TempChar.m_Vel.y > 0)
					Flags |= BFLAG_JUMP;
				else if(tried && TempChar.m_Jumped & 2 && TempChar.m_Vel.y > 0)
					Flags ^= BFLAG_RIGHT | BFLAG_LEFT;
			}
			else
				tried = false;
		}

		if(!(pMe->m_Jumped))
		{
			vec2 Vel(pMe->m_Vel.x, minimum(pMe->m_Vel.y, 0.0f));
			if(Collision()->FastIntersectLine(pMe->m_Pos,pMe->m_Pos+Vel*10.0f,0,0) && !Collision()->FastIntersectLine(pMe->m_Pos,pMe->m_Pos+(Vel-vec2(0,TempWorld.m_Tuning.m_AirJumpImpulse))*10.0f,0,0))
				Flags |= BFLAG_JUMP;
			if(absolute(m_Target.x) < 28.f && pMe->m_Vel.y > -1.f)
				Flags |= BFLAG_JUMP;
		}
	}
	if(Flags & BFLAG_JUMP || pMe->m_Vel.y < 0)
		m_InputData.m_WantedWeapon = WEAPON_GRENADE +1;
	if(m_Target.y < -400 && pMe->m_Vel.y < 0 && absolute(m_Target.x) < 30 && Collision()->CheckPoint(pMe->m_Pos+vec2(0,50)))
	{
		Flags &= ~BFLAG_HOOK;
		Flags |= BFLAG_FIRE;
		m_Target = vec2(0,28);
	}
	else if(m_Target.y < -300 && pMe->m_Vel.y < 0 && absolute(m_Target.x) < 30 && Collision()->CheckPoint(pMe->m_Pos+vec2(32,48)))
	{
		Flags &= ~BFLAG_HOOK;
		Flags |= BFLAG_FIRE;
		m_Target = vec2(14,28);
	}
	else if(m_Target.y < -300 && pMe->m_Vel.y < 0 && absolute(m_Target.x) < 30 && Collision()->CheckPoint(pMe->m_Pos+vec2(-32,48)))
	{
		Flags &= ~BFLAG_HOOK;
		Flags |= BFLAG_FIRE;
		m_Target = vec2(-14,28);
	}
	m_Flags = Flags;
}

void CBot::Snap(int SnappingClient)
{
	if(SnappingClient == -1)
		return;

	CCharacter *pMe = m_pPlayer->GetCharacter();
	if(!pMe)
		return;

	vec2 Pos = pMe->GetCore()->m_Pos;
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(GameServer()->Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = (int)(m_RealTarget.x);
		pObj->m_Y = (int)(m_RealTarget.y);
		pObj->m_FromX = (int)Pos.x;
		pObj->m_FromY = (int)Pos.y;
		pObj->m_StartTick = GameServer()->Server()->Tick();
	}
	for(int l = 0 ; l < m_pPath->m_Size-1 ; l++)
	{
		vec2 From = m_pPath->m_pVertices[l];
		vec2 To = m_pPath->m_pVertices[l+1];
		if(BotEngine()->NetworkClipped(SnappingClient, To) && BotEngine()->NetworkClipped(SnappingClient, From))
			continue;
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(GameServer()->Server()->SnapNewItem(NETOBJTYPE_LASER, m_pPath->m_pSnapID[l], sizeof(CNetObj_Laser)));
		if(!pObj)
			return;
		pObj->m_X = (int) To.x;
		pObj->m_Y = (int) To.y;
		pObj->m_FromX = (int) From.x;
		pObj->m_FromY = (int) From.y;
		pObj->m_StartTick = GameServer()->Server()->Tick();
	}
}
