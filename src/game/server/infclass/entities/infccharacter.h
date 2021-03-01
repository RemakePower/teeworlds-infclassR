#ifndef GAME_SERVER_INFCLASS_ENTITIES_CHARACTER_H
#define GAME_SERVER_INFCLASS_ENTITIES_CHARACTER_H

#include <game/server/entities/character.h>

class CGameContext;

class CInfClassCharacter : public CCharacter
{
	MACRO_ALLOC_POOL_ID()
public:
	CInfClassCharacter(CGameContext *pContext);

	CGameContext *GameContext() const { return m_pContext; }

protected:
	CGameContext *m_pContext = nullptr;
};

#endif // GAME_SERVER_INFCLASS_ENTITIES_CHARACTER_H
