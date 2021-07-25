#ifndef GAME_SERVER_INFCLASS_ENTITIES_CHARACTER_H
#define GAME_SERVER_INFCLASS_ENTITIES_CHARACTER_H

#include <game/server/entities/character.h>

class CGameContext;
class CInfClassGameController;
class CInfClassPlayerClass;

struct WeaponFireContext
{
	int Weapon = 0;
	bool FireAccepted = false;
	int AmmoAvailable = 0;
	int AmmoConsumed = 0;
	bool NoAmmo = false;
};

class CInfClassCharacter : public CCharacter
{
	MACRO_ALLOC_POOL_ID()
public:
	CInfClassCharacter(CInfClassGameController *pGameController);
	~CInfClassCharacter() override;

	static const CInfClassCharacter *fromCharacter(const CCharacter *pCharacter);
	static CInfClassCharacter *fromCharacter(CCharacter *pCharacter);

	void OnCharacterSpawned();
	void OnCharacterInInfectionZone();
	void OnCharacterOutOfInfectionZone();

	void Tick() override;
	void SpecialSnapForClient(int SnappingClient, bool *pDoSnap) override;

	void FireWeapon() override;

	void OnWeaponFired(WeaponFireContext *pFireContext);

	void OnHammerFired(WeaponFireContext *pFireContext);
	void OnGunFired(WeaponFireContext *pFireContext);
	void OnShotgunFired(WeaponFireContext *pFireContext);
	void OnGrenadeFired(WeaponFireContext *pFireContext);
	void OnLaserFired(WeaponFireContext *pFireContext);
	void OnNinjaFired(WeaponFireContext *pFireContext);

	void OnMercGrenadeFired(WeaponFireContext *pFireContext);
	void OnMedicGrenadeFired(WeaponFireContext *pFireContext);
	void OnBiologistLaserFired(WeaponFireContext *pFireContext);

	void OpenClassChooser() override;
	void HandleMapMenu();

	void Die(int Killer, int Weapon) override;

	void SetActiveWeapon(int Weapon);
	void SetLastWeapon(int Weapon);
	void TakeAllWeapons();

	int GetCID() const;

	const CInfClassPlayerClass *GetClass() const { return m_pClass; }
	CInfClassPlayerClass *GetClass() { return m_pClass; }
	void SetClass(CInfClassPlayerClass *pClass);

	CInputCount CountFireInput() const;
	bool FireJustPressed() const;

	int GetActiveWeapon() const { return m_ActiveWeapon; }

	vec2 GetHookPos() const;
	int GetHookedPlayer() const;
	void SetHookedPlayer(int ClientID);

	vec2 Velocity() const;
	float Speed() const;

	CInfClassGameController *GameController() const { return m_pGameController; }
	CGameContext *GameContext() const;

	bool CanDie() const;

	int GetInAirTick() const { return m_InAirTick; }

	bool IsInvisible() const;
	bool IsInvincible() const; // Invincible here means "ignores all damage"
	bool HasHallucination() const;

	float WebHookLength() const;

	void CheckSuperWeaponAccess();
	void MaybeGiveStunGrenades();
	void FireSoldierBomb();
	void PlacePortal(WeaponFireContext *pFireContext);
	CPortal *FindPortalInTarget();
	void OnPortalDestroy(CPortal *pPortal);
	bool ProcessCharacterOnPortal(CPortal *pPortal, CCharacter *pCharacter);
	bool CanOpenPortals() const;

	void GiveRandomClassSelectionBonus();
	void MakeVisible();

	void LockPosition();
	void UnlockPosition();

	void ResetMovementsInput();
	int GetAmmoCount(int Weapon) const override;

protected:
	void PreCoreTick() override;
	void PostCoreTick() override;

	void UpdateTuningParam();

protected:
	CInfClassGameController *m_pGameController = nullptr;
	CInfClassPlayerClass *m_pClass = nullptr;
};

inline const CInfClassCharacter *CInfClassCharacter::fromCharacter(const CCharacter *pCharacter)
{
	return static_cast<const CInfClassCharacter *>(pCharacter);
}

inline CInfClassCharacter *CInfClassCharacter::fromCharacter(CCharacter *pCharacter)
{
	return static_cast<CInfClassCharacter *>(pCharacter);
}

#endif // GAME_SERVER_INFCLASS_ENTITIES_CHARACTER_H
