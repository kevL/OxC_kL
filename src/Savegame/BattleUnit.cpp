/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

//#define _USE_MATH_DEFINES

#include "BattleUnit.h"

//#include <cmath>
//#include <sstream>
//#include <typeinfo>

#include "BattleItem.h"
#include "SavedGame.h"
//#include "SavedBattleGame.h"
#include "Soldier.h"
#include "Tile.h"

#include "../Battlescape/BattleAIState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/Pathfinding.h"

#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Engine/Surface.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"


namespace OpenXcom
{

/**
 * Initializes a BattleUnit from a Soldier
 * @param soldier		- pointer to a geoscape Soldier
 * @param depth			- the depth of the battlefield (used to determine movement type in case of MT_FLOAT)
 * @param diff			- for VictoryPts value at death
 * @param battleGame	- pointer to the BattlescapeGame (default NULL)
 */
BattleUnit::BattleUnit(
		Soldier* soldier,
		const int depth,
		const int diff,
		BattlescapeGame* battleGame)
	:
		_geoscapeSoldier(soldier),
		_unitRules(NULL),
		_faction(FACTION_PLAYER),
		_originalFaction(FACTION_PLAYER),
		_killedBy(FACTION_PLAYER),
		_murdererId(0),
		_battleGame(battleGame),
		_rankInt(-1),
		_turretType(-1),
		_tile(NULL),
		_pos(Position()),
		_lastPos(Position()),
		_direction(0),
		_toDirection(0),
		_directionTurret(0),
		_toDirectionTurret(0),
		_verticalDirection(0),
		_faceDirection(-1),
		_status(STATUS_STANDING),
		_walkPhase(0),
		_fallPhase(0),
		_spinPhase(-1),
		_aimPhase(0),
		_kneeled(false),
		_floating(false),
		_dontReselect(false),
		_fire(0),
		_currentAIState(NULL),
		_visible(false),
		_cacheInvalid(true),
		_expBravery(0),
		_expReactions(0),
		_expFiring(0),
		_expThrowing(0),
		_expPsiSkill(0),
		_expPsiStrength(0),
		_expMelee(0),
		_kills(0),
		_motionPoints(0),
		_moraleRestored(0),
		_coverReserve(0),
		_charging(NULL),
		_turnsExposed(255),
		_hidingForTurn(false),
		_respawn(false),
		_battleOrder(0),
		_stopShot(false),
		_dashing(false),
		_takenExpl(false),
		_diedByFire(false),
		_turnDir(0),
//		_race(""),

		_deathSound(0),
		_aggroSound(-1),
		_moveSound(-1),
		_intelligence(2),
		_aggression(1),
		_specab(SPECAB_NONE),
		_morale(100),
		_stunLevel(0),
		_type("SOLDIER"),
		_activeHand("STR_RIGHT_HAND"),
		_breathFrame(0),
		_breathing(false),
		_floorAbove(false)
{
	//Log(LOG_INFO) << "Create BattleUnit 1 : soldier ID = " << getId();
	_name			= soldier->getName();
	_id				= soldier->getId();
	_rank			= soldier->getRankString();

	_stats			= *soldier->getCurrentStats();
	_armor			= soldier->getArmor();
	_stats			+= *_armor->getStats();	// armors may modify effective stats

	_loftempsSet	= _armor->getLoftempsSet();
	_movementType	= _armor->getMovementType();

	if (_movementType == MT_FLOAT)
	{
		if (depth > 0)
			_movementType = MT_FLY;
		else
			_movementType = MT_WALK;
	}

	_standHeight	= soldier->getRules()->getStandHeight();
	_kneelHeight	= soldier->getRules()->getKneelHeight();
	_floatHeight	= soldier->getRules()->getFloatHeight();

	_gender			= soldier->getGender();

	int rankbonus = 0;
	switch (soldier->getRank())
	{
		case RANK_SERGEANT:		rankbonus =	5;	break; // was 1
		case RANK_CAPTAIN:		rankbonus =	15;	break; // was 3
		case RANK_COLONEL:		rankbonus =	30;	break; // was 6
		case RANK_COMMANDER:	rankbonus =	50;	break; // was 10

		default:				rankbonus =	0;	break;
	}

//	_value		= 20 + soldier->getMissions() + rankbonus;
	_value		= 20 + (soldier->getMissions() * (diff + 1)) + (rankbonus * (diff + 1)); // kL, heheheh

	_tu			= _stats.tu;
	_energy		= _stats.stamina;
	_health		= _stats.health;

	_currentArmor[SIDE_FRONT]	= _armor->getFrontArmor();
	_currentArmor[SIDE_LEFT]	= _armor->getSideArmor();
	_currentArmor[SIDE_RIGHT]	= _armor->getSideArmor();
	_currentArmor[SIDE_REAR]	= _armor->getRearArmor();
	_currentArmor[SIDE_UNDER]	= _armor->getUnderArmor();

	for (int i = 0; i < 6; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < 5; ++i)
		_cache[i] = 0;
//	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
//		_specWeapon[i] = 0;

	lastCover = Position(-1,-1,-1);

	_statistics = new BattleUnitStatistics();
	//Log(LOG_INFO) << "Create BattleUnit 1, DONE";
}

/**
 * Creates this BattleUnit from a (non-Soldier) Unit-rule object.
 * @param unit			- pointer to Unit rule
 * @param faction		- faction the unit belongs to
 * @param id			- the unit's unique ID
 * @param armor			- pointer to unit's armor
 * @param diff			- the current game's difficulty setting (for aLien stat adjustment)
 * @param depth			- the depth of the battlefield (used to determine movement type in case of MT_FLOAT)
 * @param month			- the current month (default 0)
 * @param battleGame	- pointer to the BattlescapeGame (default NULL)
 */
BattleUnit::BattleUnit(
		Unit* unit,
		const UnitFaction faction,
		const int id,
		Armor* const armor,
		const int diff,
		const int depth,
		const int month,
		BattlescapeGame* const battleGame) // May be NULL
	:
		_unitRules(unit),
		_geoscapeSoldier(NULL),
		_id(id),
		_faction(faction),
		_originalFaction(faction),
		_killedBy(faction),
		_murdererId(0),
		_armor(armor),
		_battleGame(battleGame),
		_rankInt(-1),
		_turretType(-1),
		_pos(Position()),
		_lastPos(Position()),
		_tile(NULL),
		_direction(0),
		_toDirection(0),
		_directionTurret(0),
		_toDirectionTurret(0),
		_verticalDirection(0),
		_faceDirection(-1),
		_status(STATUS_STANDING),
		_walkPhase(0),
		_fallPhase(0),
		_spinPhase(-1),
		_aimPhase(0),
		_kneeled(false),
		_floating(false),
		_dontReselect(false),
		_fire(0),
		_currentAIState(NULL),
		_visible(false),
		_cacheInvalid(true),
		_expBravery(0),
		_expReactions(0),
		_expFiring(0),
		_expThrowing(0),
		_expPsiSkill(0),
		_expPsiStrength(0),
		_expMelee(0),
		_kills(0),
		_motionPoints(0),
		_moraleRestored(0),
		_coverReserve(0),
		_charging(NULL),
		_turnsExposed(255),
		_hidingForTurn(false),
		_respawn(false),
		_stopShot(false),
		_dashing(false),
		_takenExpl(false),
		_battleOrder(0),

		_morale(100),
		_stunLevel(0),
		_activeHand("STR_RIGHT_HAND"),
		_breathFrame(-1),
		_breathing(false),
		_floorAbove(false),
//		_gender(GENDER_MALE),
		_diedByFire(false),
		_turnDir(0),

		_statistics(NULL) // Soldier Diary
{
	//Log(LOG_INFO) << "Create BattleUnit 2 : alien ID = " << getId();
	_type	= unit->getType();
	_race	= unit->getRace();
	_rank	= unit->getRank();
	_stats	= *unit->getStats();

	if (faction == FACTION_HOSTILE)
		adjustStats(
				diff,
				month);

	_stats		+= *_armor->getStats();	// armors may modify effective stats (but not further modified by game difficulty or monthly progress)

	_tu			= _stats.tu;
	_energy		= _stats.stamina;
	_health		= _stats.health;

	_standHeight	= unit->getStandHeight();
	_kneelHeight	= unit->getKneelHeight();
	_floatHeight	= unit->getFloatHeight();
	_loftempsSet	= _armor->getLoftempsSet();
	_deathSound		= unit->getDeathSound();
	_aggroSound		= unit->getAggroSound();
	_moveSound		= unit->getMoveSound();
	_intelligence	= unit->getIntelligence();
	_aggression		= unit->getAggression();
	_specab			= (SpecialAbility)unit->getSpecialAbility();
	_spawnUnit		= unit->getSpawnUnit();
	_value			= unit->getValue();

	if (unit->isFemale() == true)
		_gender = GENDER_FEMALE;
	else
		_gender = GENDER_MALE;

	_movementType = _armor->getMovementType();
	if (_movementType == MT_FLOAT)
	{
		if (depth > 0)
			_movementType = MT_FLY;
		else
			_movementType = MT_WALK;
	}

	if (armor->getDrawingRoutine() == 14) // most aliens don't breathe per-se, that's exclusive to humanoids
		_breathFrame = 0;

	_currentArmor[SIDE_FRONT]	= _armor->getFrontArmor();
	_currentArmor[SIDE_LEFT]	= _armor->getSideArmor();
	_currentArmor[SIDE_RIGHT]	= _armor->getSideArmor();
	_currentArmor[SIDE_REAR]	= _armor->getRearArmor();
	_currentArmor[SIDE_UNDER]	= _armor->getUnderArmor();

	for (int i = 0; i < 6; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < 5; ++i)
		_cache[i] = 0;
//	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
//		_specWeapon[i] = 0;

	lastCover = Position(-1,-1,-1);

//	_statistics = new BattleUnitStatistics(); // not needed by nonSoldiers, Soldier Diary.
	//Log(LOG_INFO) << "Create BattleUnit 2, DONE";
}

/**
 * dTor.
 */
BattleUnit::~BattleUnit()
{
	//Log(LOG_INFO) << "Delete BattleUnit";
	for (int
			i = 0;
			i < 5;
			++i)
	{
		if (_cache[i])
			delete _cache[i];
	}

/*kL: Soldier Diary, not needed for nonSoldiers. Or soldiers for that matter ....
	if (getGeoscapeSoldier() == NULL)
	{
		for (std::vector<BattleUnitKills*>::const_iterator
				i = _statistics->kills.begin();
				i != _statistics->kills.end();
				++i)
		{
			delete *i;
		}
	} */
//	if (_geoscapeSoldier != NULL) // kL ... delete it anyway:
	delete _statistics;

	delete _currentAIState;
}

/**
 * Loads this BattleUnit from a YAML file.
 * @param node - reference a YAML node
 */
void BattleUnit::load(const YAML::Node& node)
{
	_id					= node["id"]										.as<int>(_id);
	_faction			= _originalFaction = (UnitFaction)node["faction"]	.as<int>(_faction);
	_status				= (UnitStatus)node["status"]						.as<int>(_status);
	_pos				= node["position"]									.as<Position>(_pos);
	_direction			= _toDirection = node["direction"]					.as<int>(_direction);
	_directionTurret	= _toDirectionTurret = node["directionTurret"]		.as<int>(_directionTurret);
	_tu					= node["tu"]										.as<int>(_tu);
	_health				= node["health"]									.as<int>(_health);
	_stunLevel			= node["stunLevel"]									.as<int>(_stunLevel);
	_energy				= node["energy"]									.as<int>(_energy);
	_morale				= node["morale"]									.as<int>(_morale);
	_floating			= node["floating"]									.as<bool>(_floating);
	_fire				= node["fire"]										.as<int>(_fire);
	_turretType			= node["turretType"]								.as<int>(_turretType);
	_visible			= node["visible"]									.as<bool>(_visible);
	_turnsExposed		= node["turnsExposed"]								.as<int>(_turnsExposed);
	_killedBy			= (UnitFaction)node["killedBy"]						.as<int>(_killedBy);
	_moraleRestored		= node["moraleRestored"]							.as<int>(_moraleRestored);
	_rankInt			= node["rankInt"]									.as<int>(_rankInt);
	_originalFaction	= (UnitFaction)node["originalFaction"]				.as<int>(_originalFaction);
	_kills				= node["kills"]										.as<int>(_kills);
	_dontReselect		= node["dontReselect"]								.as<bool>(_dontReselect);
	_charging			= NULL;
//	_specab				= (SpecialAbility)node["specab"]					.as<int>(_specab);
	_spawnUnit			= node["spawnUnit"]									.as<std::string>(_spawnUnit);
	_motionPoints		= node["motionPoints"]								.as<int>(0);
	_respawn			= node["respawn"]									.as<bool>(_respawn);
	_activeHand			= node["activeHand"]								.as<std::string>(_activeHand);

	for (int i = 0; i < 5; i++)
		_currentArmor[i]	= node["armor"][i]								.as<int>(_currentArmor[i]);
	for (int i = 0; i < 6; i++)
		_fatalWounds[i]		= node["fatalWounds"][i]						.as<int>(_fatalWounds[i]);

	if (_geoscapeSoldier != NULL)
	{
		_statistics->load(node["diaryStatistics"]);

		_battleOrder	= node["battleOrder"]								.as<size_t>(_battleOrder);
		_kneeled		= node["kneeled"]									.as<bool>(_kneeled);

		_expBravery		= node["expBravery"]								.as<int>(_expBravery);
		_expReactions	= node["expReactions"]								.as<int>(_expReactions);
		_expFiring		= node["expFiring"]									.as<int>(_expFiring);
		_expThrowing	= node["expThrowing"]								.as<int>(_expThrowing);
		_expPsiSkill	= node["expPsiSkill"]								.as<int>(_expPsiSkill);
		_expPsiStrength	= node["expPsiStrength"]							.as<int>(_expPsiStrength);
		_expMelee		= node["expMelee"]									.as<int>(_expMelee);
	}
}

/**
 * Saves this BattleUnit to a YAML file.
 * @return, YAML node
 */
YAML::Node BattleUnit::save() const
{
	YAML::Node node;

	node["id"]				= _id;
	node["faction"]			= (int)_faction;
	node["soldierId"]		= _id;
	node["genUnitType"]		= _type;
	node["genUnitArmor"]	= _armor->getType();
	node["name"]			= Language::wstrToUtf8(getName(0));
	node["status"]			= (int)_status;
	node["position"]		= _pos;
	node["direction"]		= _direction;
	node["directionTurret"]	= _directionTurret;
	node["tu"]				= _tu;
	node["health"]			= _health;
	node["stunLevel"]		= _stunLevel;
	node["energy"]			= _energy;
	node["morale"]			= _morale;
	node["floating"]		= _floating;
	node["fire"]			= _fire;
	node["turretType"]		= _turretType;
	node["visible"]			= _visible;
	node["turnsExposed"]	= _turnsExposed;
	node["rankInt"]			= _rankInt;
	node["moraleRestored"]	= _moraleRestored;
	node["killedBy"]		= (int)_killedBy;
//	node["specab"]			= (int)_specab;
	node["motionPoints"]	= _motionPoints;
	node["respawn"]			= _respawn;
	// could put (if not tank) here:
	node["activeHand"]		= _activeHand;

	for (int i = 0; i < 5; i++)
		node["armor"].push_back(_currentArmor[i]);
	for (int i = 0; i < 6; i++)
		node["fatalWounds"].push_back(_fatalWounds[i]);

	if (getCurrentAIState() != NULL)
		node["AI"]				= getCurrentAIState()->save();
	if (_originalFaction != _faction)
		node["originalFaction"]	= (int)_originalFaction;
	if (_kills)
		node["kills"]			= _kills;
	if (_faction == FACTION_PLAYER
		&& _dontReselect)
	{
		node["dontReselect"]	= _dontReselect;
	}
	if (_spawnUnit.empty() == false)
		node["spawnUnit"]		= _spawnUnit;

	if (_geoscapeSoldier != NULL)
	{
		node["diaryStatistics"]	= _statistics->save();

		node["battleOrder"]		= _battleOrder;
		node["kneeled"]			= _kneeled;

		node["expBravery"]		= _expBravery;
		node["expReactions"]	= _expReactions;
		node["expFiring"]		= _expFiring;
		node["expThrowing"]		= _expThrowing;
		node["expPsiSkill"]		= _expPsiSkill;
		node["expPsiStrength"]	= _expPsiStrength;
		node["expMelee"]		= _expMelee;
	}

	return node;
		// kL_note: This doesn't save/load such things as
		// _visibleUnits, _unitsSpottedThisTurn, _visibleTiles;
		// AI is saved, but loaded someplace else -> SavedBattleGame
}

/**
 * Gets this BattleUnit's unique ID.
 * @return, the unique ID
 */
int BattleUnit::getId() const
{
	return _id;
}

/**
 * Changes this BattleUnit's position.
 * @param pos			- new position
 * @param updateLastPos	- true to update old position (default true)
 */
void BattleUnit::setPosition(
		const Position& pos,
		bool updateLastPos)
{
	if (updateLastPos == true)
		_lastPos = _pos;

	_pos = pos;
}

/**
 * Gets this BattleUnit's position.
 * @return, reference to the position
 */
const Position& BattleUnit::getPosition() const
{
	return _pos;
}

/**
 * Gets this BattleUnit's last position.
 * @return, reference to the position
 */
const Position& BattleUnit::getLastPosition() const
{
	return _lastPos;
}

/**
 * Gets this BattleUnit's destination.
 * @return, reference to the destination
 */
const Position& BattleUnit::getDestination() const
{
	return _destination;
}

/**
 * Sets this BattleUnit's horizontal direction.
 * Only used for initial unit placement.
 * kL_note: and positioning soldier when revived from unconscious status: reviveUnconsciousUnits().
 * @param dir		- new horizontal direction
 * @param turret	- true to set the turret direction also
 */
void BattleUnit::setDirection(
		int dir,
		bool turret)
{
	_direction			= dir;
	_toDirection		= dir;

	if (_turretType == -1
		|| turret == true)
	{
		_directionTurret = dir;
	}
}

/**
 * Gets this BattleUnit's horizontal direction.
 * @return, horizontal direction
 */
int BattleUnit::getDirection() const
{
	return _direction;
}

/**
 * Sets this BattleUnit's horizontal direction (facing).
 * Only used for strafing moves.
 * @param dir - new horizontal direction (facing)
 */
void BattleUnit::setFaceDirection(int dir)
{
	_faceDirection = dir;
}

/**
 * Gets this BattleUnit's horizontal direction (facing).
 * Used only during strafing moves.
 * @return, horizontal direction (facing)
 */
int BattleUnit::getFaceDirection() const
{
	return _faceDirection;
}

/**
 * Gets this BattleUnit's turret direction.
 * @return, turret direction
 */
int BattleUnit::getTurretDirection() const
{
	return _directionTurret;
}

/**
 * Sets this BattleUnit's turret direction.
 * @param dir - turret direction
 */
void BattleUnit::setTurretDirection(int dir)
{
	_directionTurret = dir;
}

/**
 * Gets this BattleUnit's turret To direction.
 * @return, toDirectionTurret
 */
int BattleUnit::getTurretToDirection() const
{
	return _toDirectionTurret;
}

/**
 * Gets this BattleUnit's vertical direction.
 * This is when going up or down, doh!
 * @return, vertical direction
 */
int BattleUnit::getVerticalDirection() const
{
	return _verticalDirection;
}

/**
 * Gets this BattleUnit's status.
 * @return, UnitStatus enum (BattleUnit.h)
 */
UnitStatus BattleUnit::getStatus() const
{
	return _status;
}

/**
 * Sets a unit's status.
 * @param status - UnitStatus enum (BattleUnit.h)
 */
void BattleUnit::setStatus(const UnitStatus status)
{
	_status = status;
}
/*	switch (status)
	{
		case 0:		_status = STATUS_STANDING;		return;
		case 1:		_status = STATUS_WALKING;		return;
		case 2:		_status = STATUS_FLYING;		return;
		case 3:		_status = STATUS_TURNING;		return;
		case 4:		_status = STATUS_AIMING;		return;
		case 5:		_status = STATUS_COLLAPSING;	return;
		case 6:		_status = STATUS_DEAD;			return;
		case 7:		_status = STATUS_UNCONSCIOUS;	return;
		case 8:		_status = STATUS_PANICKING;		return;
		case 9:		_status = STATUS_BERSERK;		return;
		case 10:	_status = STATUS_TIME_OUT;		return;
		case 11:	_status = STATUS_DISABLED;		return;

		default:
			_status = STATUS_STANDING;
	} */

/**
 * Initialises variables to start walking.
 * @param direction		- the direction to walk
 * @param destination	- reference the Position we should end up at
 * @param tileBelow		- pointer to the Tile below destination position
 * @param cache			- true to redraw the unit's sprite ( not used. )
 */
void BattleUnit::startWalking(
		int direction,
		const Position& destination,
		Tile* tileBelow,
		bool cache)
{
	//Log(LOG_INFO) << "BattleUnit::startWalking() ID = " << getId()
	//				<< " _walkPhase = 0";
	_walkPhase = 0;
	_destination = destination;
	_lastPos = _pos;
//	_cacheInvalid = cache; // there's no movement here, This not needed.

	if (direction >= Pathfinding::DIR_UP)
	{
		_status = STATUS_FLYING; // controls walking sound in UnitWalkBState, what else
		_verticalDirection = direction;

		if (_tile->getMapData(MapData::O_FLOOR) != NULL
			&& _tile->getMapData(MapData::O_FLOOR)->isGravLift() == true)
		{
			//Log(LOG_INFO) << ". STATUS_FLYING, using GravLift";
			_floating = false;
		}
		else
		{
			//Log(LOG_INFO) << ". STATUS_FLYING, up.down";
			_floating = true;
		}
	}
	else if (_tile->hasNoFloor(tileBelow) == true)
	{
		//Log(LOG_INFO) << ". STATUS_FLYING, no Floor";
		_status = STATUS_FLYING;
		_floating = true;
		_kneeled = false;
		_direction = direction;
	}
	else
	{
		//Log(LOG_INFO) << ". STATUS_WALKING";
		_status = STATUS_WALKING;
		_floating = false;
		_kneeled = false;
		_direction = direction;
	}
	//Log(LOG_INFO) << "BattleUnit::startWalking() EXIT";
}

/**
 * This will increment the walking phase.
 * @param tileBelow	- pointer to tile currently below this unit
 * @param cache		- true to refresh the unit cache / redraw this unit's sprite
 */
void BattleUnit::keepWalking(
		const Tile* const tileBelow,
		bool cache)
{
	++_walkPhase;

	//Log(LOG_INFO) << "BattleUnit::keepWalking() ID = " << getId() << " _walkPhase = " << _walkPhase;
	int
		midPhase,
		endPhase;

	if (_verticalDirection != 0)
	{
		midPhase = 4;
		endPhase = 8;
	}
	else // diagonal walking takes double the steps
	{
		midPhase = 4 + 4 * (_direction %2);
		endPhase = 8 + 8 * (_direction %2);

		if (_armor->getSize() > 1)
		{
			if (_direction < 1 || 5 < _direction) // dir = 0,7,6,5 (up x3 or left)
				midPhase = endPhase;
			else if (_direction == 5)
				midPhase = 12;
			else if (_direction == 1)
				midPhase = 5;
			else
				midPhase = 1;
		}
	}

	if (cache == false) // ie. not onScreen
	{
		midPhase = 1; // kL: Mc'd units offscreen won't move without this (tho they turn, as if to start walking)
		endPhase = 2;
	}

	if (_walkPhase == midPhase) // we assume we reached our destination tile
		// This is actually a drawing hack, so soldiers are not overlapped by floortiles
		// kL_note: which they (large units) are half the time anyway... fixed.
		_pos = _destination;

	if (_walkPhase >= endPhase) // officially reached the destination tile
	{
		//Log(LOG_INFO) << ". end -> STATUS_STANDING";
		_status = STATUS_STANDING;
		_walkPhase = 0;
		_verticalDirection = 0;

		if (_floating == true
			&& _tile->hasNoFloor(tileBelow) == false)
		{
			_floating = false;
		}

		if (_faceDirection > -1) // finish strafing move facing the correct way.
		{
			_direction = _faceDirection;
			_faceDirection = -1;
		}

		// motion points calculation for the motion scanner blips
		if (_armor->getSize() > 1)
			_motionPoints += 30;
		else
		{
			// sectoids actually have less motion points but instead of creating
			// yet another variable, use the height of the unit instead
			if (getStandHeight() > 16)
				_motionPoints += 4;
			else
				_motionPoints += 3;
		}
	}

	_cacheInvalid = cache;
	//Log(LOG_INFO) << "BattleUnit::keepWalking() EXIT";
}

/**
 * Gets the walking phase for animation and sound.
 * @return, phase will always go from 0-7
 */
int BattleUnit::getWalkingPhase() const
{
	return _walkPhase %8;
}

/**
 * Gets the walking phase for diagonal walking.
 * @return, phase will be 0 or 8 due to rounding ints down
 */
int BattleUnit::getDiagonalWalkingPhase() const
{
	return (_walkPhase / 8) * 8;
}

/**
 * Gets the walking phase unadjusted.
 * @return, phase
 */
int BattleUnit::getTrueWalkingPhase() const
{
	return _walkPhase;
}

/**
 * Look at a point.
 * @param point		- reference to the position to look at
 * @param turret	- true to turn the turret, false to turn the unit
 */
void BattleUnit::lookAt(
		const Position& point,
		bool turret)
{
	//Log(LOG_INFO) << "BattleUnit::lookAt() #1";
	//Log(LOG_INFO) << ". . _direction = " << _direction;
	//Log(LOG_INFO) << ". . _toDirection = " << _toDirection;
	//Log(LOG_INFO) << ". . _directionTurret = " << _directionTurret;
	//Log(LOG_INFO) << ". . _toDirectionTurret = " << _toDirectionTurret;

	int dir = directionTo(point);
	//Log(LOG_INFO) << ". . lookAt() -> dir = " << dir;

	if (turret == true)
	{
		//Log(LOG_INFO) << ". . . . has turret.";
		_toDirectionTurret = dir;
		//Log(LOG_INFO) << ". . . . . . _toDirTur = " << _toDirectionTurret;
		if (_toDirectionTurret != _directionTurret)
		{
			_status = STATUS_TURNING;
			//Log(LOG_INFO) << ". . . . lookAt() -> STATUS_TURNING, turret.";
		}
	}
	else
	{
		//Log(LOG_INFO) << ". . . . NOT turret.";

		_toDirection = dir;
		//Log(LOG_INFO) << ". . . . . . _toDir = " << _toDirection;
		if (_toDirection != _direction)
		{
			_status = STATUS_TURNING;
			//Log(LOG_INFO) << ". . . . lookAt() -> STATUS_TURNING";
			// kL_note: what about Forcing the faced direction instantly?
		}
	}
	//Log(LOG_INFO) << "BattleUnit::lookAt() #1 EXIT";
}

/**
 * Look a direction.
 * @param direction	- direction to look
 * @param force		- true to instantly set the unit's direction, false to animate / visually turn to it
 */
void BattleUnit::lookAt(
		int direction,
		bool force)
{
	//Log(LOG_INFO) << "BattleUnit::lookAt() #2";
	if (force == true)
	{
		_toDirection = direction;
		_direction = direction;
	}
	else
	{
		if (direction < 0 || 7 < direction)
			return;

		_toDirection = direction;
		if (_toDirection != _direction)
			_status = STATUS_TURNING;
			//Log(LOG_INFO) << ". . . . lookAt() -> STATUS_TURNING";
	}
	//Log(LOG_INFO) << "BattleUnit::lookAt() #2 EXIT";
}

/**
 * Advances the turning towards the target direction.
 * @param turret - true to turn the turret, false to turn the unit
 */
void BattleUnit::turn(bool turret)
{
	//Log(LOG_INFO) << "BattleUnit::turn()";
	//Log(LOG_INFO) << ". . _direction = " << _direction;
	//Log(LOG_INFO) << ". . _toDirection = " << _toDirection;
	//Log(LOG_INFO) << ". . _directionTurret = " << _directionTurret;
	//Log(LOG_INFO) << ". . _toDirectionTurret = " << _toDirectionTurret;

	int delta = 0;

	if (turret == true)
	{
		if (_directionTurret == _toDirectionTurret)
		{
			//Log(LOG_INFO) << ". . _d = _tD, abort";
//			abortTurn();
			_status = STATUS_STANDING;
			return;
		}

		delta = _toDirectionTurret - _directionTurret;
		//Log(LOG_INFO) << ". . deltaTurret = " << delta;
	}
	else
	{
		if (_direction == _toDirection)
		{
			//Log(LOG_INFO) << ". . _d = _tD, abort";
//			abortTurn();
			_status = STATUS_STANDING;
			return;
		}

		delta = _toDirection - _direction;
		//Log(LOG_INFO) << ". . delta = " << delta;
	}

	if (delta != 0) // duh
	{
		if (delta > 0)
		{
			if (delta < 5
				|| _turnDir == 1)
			{
				if (turret == false)
				{
					if (_turretType > -1)
						++_directionTurret;

					++_direction;
				}
				else
					++_directionTurret;
			}
			else // > 4
			{
				if (turret == false)
				{
					if (_turretType > -1)
						--_directionTurret;

					--_direction;
				}
				else
					--_directionTurret;
			}
		}
		else
		{
			if (delta > -4
				|| _turnDir == -1)
			{
				if (turret == false)
				{
					if (_turretType > -1)
						--_directionTurret;

					--_direction;
				}
				else
					--_directionTurret;
			}
			else // < -3
			{
				if (turret == false)
				{
					if (_turretType > -1)
						++_directionTurret;

					++_direction;
				}
				else
					++_directionTurret;
			}
		}

		if (_direction < 0)
			_direction = 7;
		else if (_direction > 7)
			_direction = 0;

		if (_directionTurret < 0)
			_directionTurret = 7;
		else if (_directionTurret > 7)
			_directionTurret = 0;

		//Log(LOG_INFO) << ". . _direction = " << _direction;
		//Log(LOG_INFO) << ". . _toDirection = " << _toDirection;
		//Log(LOG_INFO) << ". . _directionTurret = " << _directionTurret;
		//Log(LOG_INFO) << ". . _toDirectionTurret = " << _toDirectionTurret;

		if (_visible == true
			|| _faction == FACTION_PLAYER) // kL_note: Faction_player should *always* be _visible...
		{
			_cacheInvalid = true;
		}
	}

	if (turret == true)
	{
		 if (_toDirectionTurret == _directionTurret)
		 {
			//Log(LOG_INFO) << "BattleUnit::turn() " << getId() << "Turret - STATUS_STANDING (turn has ended)";
			_status = STATUS_STANDING; // we officially reached our destination
		}
	}
	else if (_toDirection == _direction
		|| _status == STATUS_UNCONSCIOUS)	// kL_note: I didn't know Unconscious could turn...
											// learn something new every day.
											// It's used when reviving unconscious soldiers;
											// they need to go to STATUS_STANDING.
	{
		//Log(LOG_INFO) << "BattleUnit::turn() " << getId() << " - STATUS_STANDING (turn has ended)";
		_status = STATUS_STANDING; // we officially reached our destination
	}
	//Log(LOG_INFO) << "BattleUnit::turn() EXIT";// unitID = " << getId();
}

/**
 * Stops the turning towards the target direction.
 */
/*kL: now uses setStatus()...
void BattleUnit::abortTurn()
{
	_status = STATUS_STANDING;
} */

/**
 * Gets the soldier's gender.
 * @return, SoldierGender enum
 */
SoldierGender BattleUnit::getGender() const
{
	return _gender;
}

/**
 * Returns the unit's faction.
 * @return, UnitFaction enum (player, hostile or neutral)
 */
UnitFaction BattleUnit::getFaction() const
{
	return _faction;
}

/**
 * Sets the unit's cache flag.
 * Set to true when the unit has to be redrawn from scratch.
 * @param cache	- pointer to cache surface to use, NULL to redraw from scratch
 * @param part	- unit part to cache (default 0)
 */
void BattleUnit::setCache(
		Surface* cache,
		int part)
{
	if (cache == NULL)
		_cacheInvalid = true;
	else
	{
		_cache[part] = cache;
		_cacheInvalid = false;
	}
}

/**
 * Check if the unit is still cached in the Map cache.
 * When the unit needs to animate it needs to be re-cached.
 * @param invalid	- pointer to true if the cache is invalid
 * @param part		- unit part to check (default 0)
 * @return, pointer to the cache Surface used
 */
Surface* BattleUnit::getCache(
		bool* invalid,
		int part) const
{
	if (part < 0)
		part = 0;

	*invalid = _cacheInvalid;

	return _cache[part];
}

/**
 * Kneels or stands this unit.
 * @param kneeled - true to kneel, false to stand up
 */
void BattleUnit::kneel(bool kneel)
{
	_kneeled = kneel;
	_cacheInvalid = true;
}

/**
 * Gets if this unit is kneeling.
 * @return, true if kneeled
 */
bool BattleUnit::isKneeled() const
{
	return _kneeled;
}

/**
 * Gets if this unit is floating.
 * A unit is floating if there is no ground underneath.
 * @return, true if floating
 */
bool BattleUnit::isFloating() const
{
	return _floating;
}

/**
 * Shows the righthand sprite with weapon aiming.
 * @param aim - true to aim, false to stand there like an idiot (default true)
 */
void BattleUnit::aim(bool aim)
{
	if (aim == true)
		_status = STATUS_AIMING;
	else
		_status = STATUS_STANDING;

	if (_visible == true
		|| _faction == FACTION_PLAYER)
	{
		_cacheInvalid = true;
	}
}

/**
 * Returns the direction from this unit to a given point.
 * kL_note: This function is almost identical to TileEngine::getDirectionTo().
 * @param point - reference to a given position
 * @return, direction from here to there
 */
int BattleUnit::directionTo(const Position& point) const
{
	if (_pos == point) // kL. safety
		return 0;

	double
		offset_x = point.x - _pos.x,
		offset_y = point.y - _pos.y,

		theta = std::atan2(-offset_y, offset_x), // radians: + = y > 0; - = y < 0;

	// divide the pie in 4 thetas, each at 1/8th before each quarter
		m_pi_8 = M_PI / 8.,					// a circle divided into 16 sections (rads) -> 22.5 deg
		d = 0.1,							// kL, a bias toward cardinal directions. (0.1..0.12)
		pie[4] =
		{
			M_PI - m_pi_8 - d,				// 2.7488935718910690836548129603696	-> 157.5 deg
			M_PI * 3. / 4. - m_pi_8 + d,	// 1.9634954084936207740391521145497	-> 112.5 deg
			M_PI_2 - m_pi_8 - d,			// 1.1780972450961724644234912687298	-> 67.5 deg
			m_pi_8 + d						// 0.39269908169872415480783042290994	-> 22.5 deg
		};

	int dir = 2;
	if (pie[0] < theta || theta < -pie[0])
		dir = 6;
	else if (theta > pie[1])
		dir = 7;
	else if (theta > pie[2])
		dir = 0;
	else if (theta > pie[3])
		dir = 1;
	else if (theta < -pie[1])
		dir = 5;
	else if (theta < -pie[2])
		dir = 4;
	else if (theta < -pie[3])
		dir = 3;

	//Log(LOG_INFO) << "BattleUnit::directionTo() dir = " << dir;
	return dir;
}

/**
 * Returns the soldier's amount of time units.
 * @return, current time units
 */
int BattleUnit::getTimeUnits() const
{
	return _tu;
}

/**
 * Returns the soldier's amount of energy.
 * @return, current stamina
 */
int BattleUnit::getEnergy() const
{
	return _energy;
}

/**
 * Returns the soldier's amount of health.
 * @return, current health
 */
int BattleUnit::getHealth() const
{
	return _health;
}

/**
 * Returns the soldier's amount of morale.
 * @return, current morale
 */
int BattleUnit::getMorale() const
{
	return _morale;
}

/**
 * Do an amount of damage.
 * @param relPos		- reference a position in voxelspace that defines which part of armor and/or body gets hit
 * @param power			- the amount of damage to inflict
 * @param type			- the ItemDamageType being inflicted (RuleItem.h)
 * @param ignoreArmor	- true for stun & smoke damage; no armor reduction, although vulnerability is still factored in
 * @return, damage done to this BattleUnit after adjustments
 */
int BattleUnit::damage(
		const Position& relPos,
		int power,
		ItemDamageType type,
		const bool ignoreArmor)
{
	//Log(LOG_INFO) << "BattleUnit::damage(), ID " << getId();
	power = static_cast<int>(Round(
			static_cast<float>(power) * _armor->getDamageModifier(type)));
	//Log(LOG_INFO) << "BattleUnit::damage(), type = " << (int)type << " ModifiedPower " << power;

//	if (power < 1) // kL_note: this early-out messes with got-hit sFx below_
//		return 0;

	if (type == DT_SMOKE) // smoke doesn't do real damage, but stun damage instead.
		type = DT_STUN;

	if (type == DT_STUN)
	{
		if (power < 1)
			return 0;
	}

	UnitBodyPart bodypart = BODYPART_TORSO;

	if (power > 0
		&& ignoreArmor == false)
	{
		UnitSide side = SIDE_FRONT;

		if (relPos == Position(0, 0, 0))
			side = SIDE_UNDER;
		else
		{
			int relDir;
			const int
				abs_x = std::abs(relPos.x),
				abs_y = std::abs(relPos.y);

			if (abs_y > abs_x * 2)
				relDir = 8 + 4 * static_cast<int>(relPos.y > 0);	// hit from South (y-pos) or North (y-neg)
			else if (abs_x > abs_y * 2)
				relDir = 10 + 4 * static_cast<int>(relPos.x < 0);	// hit from East (x-pos) or West (x-neg)
			else
			{
				if (relPos.x < 0)	// hit from West (x-neg)
				{
					if (relPos.y > 0)
						relDir = 13;	// hit from SouthWest (y-pos)
					else
						relDir = 15;	// hit from NorthWest (y-neg)
				}
				else				// hit from East (x-pos)
				{
					if (relPos.y > 0)
						relDir = 11;	// hit from SouthEast (y-pos)
					else
						relDir = 9;		// hit from NorthEast (y-neg)
				}
			}

			//Log(LOG_INFO) << "BattleUnit::damage() Target was hit from DIR = " << ((relDir - _direction) %8);
			switch ((relDir - _direction) %8)
			{
				case 0:	side = SIDE_FRONT;									break;
				case 1:	side = RNG::percent(50)? SIDE_FRONT: SIDE_RIGHT;	break;
				case 2:	side = SIDE_RIGHT;									break;
				case 3:	side = RNG::percent(50)? SIDE_REAR: SIDE_RIGHT;		break;
				case 4:	side = SIDE_REAR;									break;
				case 5:	side = RNG::percent(50)? SIDE_REAR: SIDE_LEFT; 		break;
				case 6:	side = SIDE_LEFT;									break;
				case 7:	side = RNG::percent(50)? SIDE_FRONT: SIDE_LEFT;		break;
			}
			//Log(LOG_INFO) << ". side = " << (int)side;

			if (relPos.z > getHeight() - 4)
				bodypart = BODYPART_HEAD;
			else if (relPos.z > 5)
			{
				switch (side)
				{
					case SIDE_LEFT:		bodypart = BODYPART_LEFTARM;	break;
					case SIDE_RIGHT:	bodypart = BODYPART_RIGHTARM;	break;

					default:			bodypart = BODYPART_TORSO;
				}
			}
			else
			{
				switch (side)
				{
					case SIDE_LEFT: 	bodypart = BODYPART_LEFTLEG; 	break;
					case SIDE_RIGHT:	bodypart = BODYPART_RIGHTLEG; 	break;

					default:
						bodypart = static_cast<UnitBodyPart>(RNG::generate(
																		BODYPART_RIGHTLEG,
																		BODYPART_LEFTLEG));
				}
			}
			//Log(LOG_INFO) << ". bodypart = " << (int)bodypart;
		}

		const int armor = getArmor(side);
		setArmor( // armor damage
				std::max(
						0,
						armor - (power + 9) / 10), // round up.
				side);

		power -= armor; // subtract armor-before-damage from power.
	}

	if (power > 0)
	{
		if (type == DT_STUN
			&& (_geoscapeSoldier != NULL	// note that this should be obviated in the rules for Armor damage vulnerability.
				|| (_unitRules				// Rather than here
					&& _unitRules->isMechanical() == false
					&& _race != "STR_ZOMBIE")))
		{
			_stunLevel += power;
		}
		else
		{
			_health -= power; // health damage
			if (_health < 1)
			{
				_health = 0;

				if (type == DT_IN)
				{
					_diedByFire = true;
					_spawnUnit.clear();

					if (_type == "STR_ZOMBIE")
						_specab = SPECAB_EXPLODEONDEATH;
					else
						_specab = SPECAB_NONE;
				}
			}
			else
			{
				if (_geoscapeSoldier != NULL					// add some stun to xCom agents
					|| (_unitRules->isMechanical() == false	// or to non-mechanical units
						&& _race != "STR_ZOMBIE"))				// unless it's a freakin Zombie.
				{
					_stunLevel += RNG::generate(0, power / 3);
				}

				if (ignoreArmor == false)// Only wearers of armors-that-are-resistant-to-damage-type can take fatal wounds.
				{
					if (isWoundable() == true) // fatal wounds
					{
						if (RNG::generate(0, 10) < power) // kL: refactor this.
						{
							const int wounds = RNG::generate(1, 3);
							_fatalWounds[bodypart] += wounds;

							moraleChange(-wounds * 3);
						}
					}
//					setArmor(getArmor(side) - (power / 10) - 1, side); // armor damage
				}
			}
		}
	}

	// TODO: give a short "ugh" if hit causes no damage or perhaps stuns ( power must be > 0 though );
	// a longer "uuuhghghgh" if hit causes damage ... and let DieBState handle deathscreams.
//	if (!isOut(true, true)) // no sound if Stunned; deathscream handled by uh, UnitDieBState.
//		playHitSound(); // kL
	if (_health > 0
		&& _visible == true
		&& _status != STATUS_UNCONSCIOUS
		&& type != DT_STUN
		&& (_geoscapeSoldier != NULL
			|| _unitRules->isMechanical() == false))
	{
		playHitSound(); // kL
	}

	if (power < 0)
		power = 0;
	//Log(LOG_INFO) << "BattleUnit::damage() ret Penetrating Power " << power;

	return power;
}

/**
 * kL. Plays a grunt sFx when this BattleUnit gets damaged or hit.
 */
void BattleUnit::playHitSound() // kL
{
	int sound = -1;

	if (_type == "MALE_CIVILIAN")
		sound = RNG::generate(41, 43);
	else if (_type == "FEMALE_CIVILIAN")
		sound = RNG::generate(44, 46);
	else if (_originalFaction == FACTION_PLAYER) // getType() == "SOLDIER")
	{
		if (_gender == GENDER_MALE)
		{
			sound = RNG::generate(141, 151);
			//Log(LOG_INFO) << "death Male, sound = " << sound;
		}
		else // if (getGender() == GENDER_FEMALE)
		{
			sound = RNG::generate(121, 135);
			//Log(LOG_INFO) << "death Female, sound = " << sound;
		}
	}

	if (sound != -1)
		_battleGame->getResourcePack()->getSound(
											"BATTLE.CAT",
											sound)
										->play();
//	else // note: this will crash because _battleGame is currently left NULL for non-xCom & non-Civies.
//		_battleGame->getResourcePack()->getSound(
//											"BATTLE.CAT",
//											_unit->getDeathSound())
//										->play();
}

/**
 * Do an amount of stun recovery.
 * @param power - stun to recover
 */
void BattleUnit::healStun(int power)
{
	_stunLevel -= power;

	if (_stunLevel < 0)
		_stunLevel = 0;
}

/**
 * Gets the amount of stun damage this unit has.
 * @return, stun level
 */
int BattleUnit::getStun() const
{
	return _stunLevel;
}

/**
 * Raises a unit's stun level sufficiently so that the unit is ready to become unconscious.
 * Used when another unit falls on top of this unit.
 * Zombified units first convert to their spawn unit.
 * @param battle - pointer to BattlescapeGame
 */
void BattleUnit::knockOut(BattlescapeGame* battle)
{
	if (getArmor()->getSize() > 1		// large units die
		|| _unitRules->isMechanical())	// so do scout drones
	{
		_health = 0;
	}
	else if (_spawnUnit.empty() == false)
	{
//kL	setSpecialAbility(SPECAB_NONE); // do this in convertUnit()
		BattleUnit* newUnit = battle->convertUnit(
												this,
												_spawnUnit);
		newUnit->knockOut(battle); // -> STATUS_UNCONSCIOUS
	}
	else
		_stunLevel = _health;
}

/**
 * Intialises the falling sequence. Occurs after death or stunned.
 */
void BattleUnit::startFalling()
{
	_status = STATUS_COLLAPSING;
	_fallPhase = 0;
	_cacheInvalid = true;
}

/**
 * Advances the phase of falling sequence.
 */
void BattleUnit::keepFalling()
{
	if (_diedByFire == true)
		_fallPhase = _armor->getDeathFrames();
	else
		++_fallPhase;

	if (_fallPhase == _armor->getDeathFrames())
	{
		--_fallPhase;

		if (_health == 0)
		{
			//Log(LOG_INFO) << "BattleUnit::keepFalling() " << getId() << ". . STATUS_DEAD";
			_status = STATUS_DEAD;
		}
		else
		{
			//Log(LOG_INFO) << "BattleUnit::keepFalling() " << getId() << ". . STATUS_UNCONSCIOUS";
			_status = STATUS_UNCONSCIOUS;
		}
	}

	_cacheInvalid = true;
}

/**
 * Returns the phase of the falling sequence.
 * @return, phase
 */
int BattleUnit::getFallingPhase() const
{
	return _fallPhase;
}

/**
 * kL. Intialises the aiming sequence.
 */
void BattleUnit::startAiming() // kL
{
	if (_armor->getShootFrames() == 0)
		return;

	_status = STATUS_AIMING;
	_aimPhase = 0;

	if (_visible == true)
//		|| _faction == FACTION_PLAYER) // all Faction_Player is visible ...
	{
		_cacheInvalid = true;
	}
}

/**
 * kL. Advances the phase of aiming sequence.
 */
void BattleUnit::keepAiming() // kL
{
	if (++_aimPhase == _armor->getShootFrames())
	{
		--_aimPhase;
		_status = STATUS_STANDING;
	}

	_cacheInvalid = true;
}

/**
 * kL. Returns the phase of the Aiming sequence.
 * @return, aiming phase
 */
int BattleUnit::getAimingPhase() const // kL
{
	return _aimPhase;
}

/**
 * Returns whether the unit is out of combat, ie dead or unconscious.
 * A unit that is Out, cannot perform any actions,
 * and cannot be selected, but it's still a unit.
 * @note checkHealth and checkStun are early returns and therefore
 * should generally not be used to test a negation: eg, !isOut()
 * @param checkHealth, Check if unit still has health
 * @param checkStun, Check if unit is stunned
 * @return bool, True if unable to function on the battlefield
 */
bool BattleUnit::isOut(
		bool checkHealth,
		bool checkStun) const
{
	//Log(LOG_INFO) << "BattleUnit::isOut() ID = " << getId();
	if (checkHealth == true
		&& getHealth() == 0)
	{
			return true;
	}
	else if (checkStun == true
		&& getStun() >= getHealth())
	{
			return true;
	}
	else if (_status == STATUS_DEAD
		|| _status == STATUS_UNCONSCIOUS
		|| _status == STATUS_TIME_OUT)
	{
		return true;
	}

	return false;
/*	bool ret = false;

	if (checkHealth)
	{
		if (getHealth() == 0)
			ret = true;
	}

	if (checkStun)
	{
		if (getStun() >= getHealth())
			ret = true;
	}

	if (_status == STATUS_DEAD
		|| _status == STATUS_UNCONSCIOUS)
	{
		ret = true;
	}

	return ret; */
}

/**
 * Gets the number of time units a certain action takes for this BattleUnit.
 * @param actionType	- type of battle action (BattlescapeGame.h)
 * @param item			- pointer to BattleItem for TU-cost
 * @return, TUs to perform action
 */
int BattleUnit::getActionTUs(
		const BattleActionType actionType,
		const BattleItem* item) const
{
	if (item == NULL)
		return 0;

	return getActionTUs(
					actionType,
					item->getRules());
}

/**
 * Gets the number of time units a certain action takes for this BattleUnit.
 * @param actionType	- type of battle action (BattlescapeGame.h)
 * @param item			- pointer to RuleItem for TU-cost (default NULL)
 * @return, TUs to perform action
 */
int BattleUnit::getActionTUs(
		const BattleActionType actionType,
		const RuleItem* rule) const
{
//	if (rule == NULL) return 0;
	int cost = 0;

	switch (actionType)
	{
		// note: Should put "tuPrime", "tuDefuse", & "tuThrow" yaml-entry in Xcom1Ruleset, under various grenade-types etc.
		case BA_DROP:
		{
			const RuleInventory
				* const handRule = _battleGame->getRuleset()->getInventory("STR_RIGHT_HAND"), // might be leftHand Lol ...
				* const groundRule = _battleGame->getRuleset()->getInventory("STR_GROUND");
			cost = handRule->getCost(groundRule);
//			cost = 2;
		}
		break;

		case BA_DEFUSE:
			cost = 15;
		break;

		case BA_PRIME:
			cost = 45;
		break;

		case BA_THROW:
			cost = 23;
		break;

		case BA_LAUNCH:
			if (rule == NULL)
				return 0;
			cost = rule->getTULaunch();
		break;

		case BA_AIMEDSHOT:
			if (rule == NULL)
				return 0;
			cost = rule->getTUAimed();
		break;

		case BA_AUTOSHOT:
			if (rule == NULL)
				return 0;
			cost = rule->getTUAuto();
		break;

		case BA_SNAPSHOT:
			if (rule == NULL)
				return 0;
			cost = rule->getTUSnap();
		break;

		case BA_STUN:
		case BA_HIT:
			if (rule == NULL)
				return 0;
			cost = rule->getTUMelee();
		break;

		case BA_USE:
		case BA_MINDCONTROL:
		case BA_PANIC:
			if (rule == NULL)
				return 0;
			cost = rule->getTUUse();
		break;

		default:
			cost = 0;
						// problem: cost=0 can lead to infinite loop in reaction fire
						// Presently, cost=0 means 'cannot do' but conceivably an action
						// could be free, or cost=0; so really cost=-1 ought be
						// implemented, to mean 'cannot do' ......
						// (ofc this default is rather meaningless, but there is a point)
	}


	if (cost > 0
		&& ((rule != NULL
				&& rule->getFlatRate() == false) // it's a percentage, apply to TUs
			|| actionType == BA_PRIME
			|| actionType == BA_THROW)
		&& actionType != BA_DEFUSE
		&& actionType != BA_DROP)
	{
		cost = std::max(
					1,
					static_cast<int>(std::floor(static_cast<double>(getBaseStats()->tu * cost) / 100.)));
	}

	return cost;
}

/**
 * Spends time units if it can. Return false if it can't.
 * @param tu - the TU to check & spend
 * @return, true if this unit could spend the time units
 */
bool BattleUnit::spendTimeUnits(int tu)
{
	if (tu <= _tu)
	{
		_tu -= tu;
		return true;
	}

	return false;
}

/**
 * Spends energy if it can. Return false if it can't.
 * @param energy - the stamina to check & expend
 * @return, true if this unit could expend the stamina
 */
bool BattleUnit::spendEnergy(int energy)
{
	if (energy <= _energy)
	{
		_energy -= energy;
		return true;
	}

	return false;
}

/**
 * Sets a specific number of timeunits.
 * @param tu - the TU for this unit to set
 */
void BattleUnit::setTimeUnits(int tu)
{
	if (tu < 0)
		_tu = 0;
	else
		_tu = tu;

//	if (_tu > getBaseStats()->tu) // This definitely screws up TU refunds.
//		_tu = getBaseStats()->tu;
}

/**
 * Sets a specific number of energy.
 * @param energy - the energy to set for this unit
 */
void BattleUnit::setEnergy(int energy)
{
	if (energy < 0)
		_energy = 0;
	else
		_energy = energy;

	if (_energy > getBaseStats()->stamina)
		_energy = getBaseStats()->stamina;
}

/**
 * Sets whether this unit is visible to the player;
 * that is should it be drawn on the Map.
 * @param flag - true if visible (default true)
 */
void BattleUnit::setUnitVisible(bool flag)
{
	_visible = flag;
}

/**
 * Gets whether this unit is visible.
 * @return, true if visible
 */
bool BattleUnit::getUnitVisible() const
{
	if (getFaction() == FACTION_PLAYER)
		return true;
	else
		return _visible;
}

/**
 * Adds a unit to this BattleUnits vector(s) of spotted and/or visible units;
 * visible units are currently seen - unitsSpottedThisTurn are just that.
 * @note Don't confuse either of these with the 'visible-to-player' flag.
 * @param unit - pointer to a seen BattleUnit
 * @return, true if the seen unit was NOT previously flagged as a 'visibleUnit'
 */
bool BattleUnit::addToVisibleUnits(BattleUnit* unit)
{
	bool addUnit = true;

	for (std::vector<BattleUnit*>::const_iterator
			i = _unitsSpottedThisTurn.begin();
			i != _unitsSpottedThisTurn.end();
			++i)
	{
//		if (dynamic_cast<BattleUnit*>(*i) == unit)
		if (*i == unit)
		{
			addUnit = false;
			break;
		}
	}

	if (addUnit == true)
		_unitsSpottedThisTurn.push_back(unit);


	for (std::vector<BattleUnit*>::const_iterator
			i = _visibleUnits.begin();
			i != _visibleUnits.end();
			++i)
	{
//		if (dynamic_cast<BattleUnit*>(*i) == unit)
		if (*i == unit)
			return false;
	}

	_visibleUnits.push_back(unit);

	return true;
}

/**
 * Gets the pointer to a vector of visible units.
 * @return, pointer to a vector of pointers to visible units
 */
std::vector<BattleUnit*>* BattleUnit::getVisibleUnits()
{
	return &_visibleUnits;
}

/**
 * Clears visible units.
 */
void BattleUnit::clearVisibleUnits()
{
	_visibleUnits.clear();
}

/**
 * Adds a tile to the list of visible tiles.
 * @param tile - pointer to a tile to add
 * @return, true or CTD
 */
bool BattleUnit::addToVisibleTiles(Tile* tile)
{
	_visibleTiles.push_back(tile);

	return true;
}

/**
 * Gets the pointer to the vector of visible tiles.
 * @return, pointer to a vector of pointers to visible tiles
 */
std::vector<Tile*>* BattleUnit::getVisibleTiles()
{
	return &_visibleTiles;
}

/**
 * Clears visible tiles.
 */
void BattleUnit::clearVisibleTiles()
{
	for (std::vector<Tile*>::const_iterator
			j = _visibleTiles.begin();
			j != _visibleTiles.end();
			++j)
	{
		(*j)->setTileVisible(false);
	}

	_visibleTiles.clear();
}

/**
 * Calculates firing accuracy.
 * Formula = accuracyStat * weaponAccuracy * kneelingbonus(1.15) * one-handPenalty(0.8) * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param actionType	-
 * @param item			-
 * @return, firing accuracy
 */
double BattleUnit::getFiringAccuracy(
		BattleActionType actionType,
		BattleItem* item)
{
	//Log(LOG_INFO) << "BattleUnit::getFiringAccuracy() ID " << getId();
	if (actionType == BA_LAUNCH)
		return 1.;

	double ret;

	if (actionType == BA_HIT
		|| actionType == BA_STUN) // note: BA_STUN is not used in code.
	{
		ret = static_cast<double>(item->getRules()->getAccuracyMelee()) * getAccuracyModifier(item) / 100.;
		//Log(LOG_INFO) << ". weaponACU = " << item->getRules()->getAccuracyMelee() << " ret[1] = " << ret;

		if (item->getRules()->isSkillApplied() == true)
		{
			ret = ret * static_cast<double>(getBaseStats()->melee) / 100.;
			//Log(LOG_INFO) << ". meleeStat = " << getBaseStats()->melee << " ret[2] = " << ret;
		}
	}
	else
	{
		int acu = item->getRules()->getAccuracySnap();

		if (actionType == BA_AIMEDSHOT)
			acu = item->getRules()->getAccuracyAimed();
		else if (actionType == BA_AUTOSHOT)
			acu = item->getRules()->getAccuracyAuto();

		ret = static_cast<double>(acu * getBaseStats()->firing) / 10000.;

		if (_kneeled == true)
			ret *= 1.16;

		if (item->getRules()->isTwoHanded() == true
			&& getItem("STR_RIGHT_HAND") != NULL
			&& getItem("STR_LEFT_HAND") != NULL)
		{
			ret *= 0.79;
		}

		ret *= getAccuracyModifier();
	}

	//Log(LOG_INFO) << ". fire ACU ret[0] = " << ret;
	return ret;
}

/**
 * Calculates this BattleUnit's throwing accuracy.
 * @return, throwing accuracy
 */
double BattleUnit::getThrowingAccuracy()
{
	return static_cast<double>(getBaseStats()->throwing) * getAccuracyModifier() / 100.;
}

/**
 * Calculates accuracy modifier. Takes health and fatal wounds into account.
 * Formula = accuracyStat * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param item - pointer to a BattleItem (default NULL)
 * @return, modifier
 */
double BattleUnit::getAccuracyModifier(const BattleItem* const item)
{
	//Log(LOG_INFO) << "BattleUnit::getAccuracyModifier()";
	double ret = static_cast<double>(_health) / static_cast<double>(getBaseStats()->health);

	int wounds = _fatalWounds[BODYPART_HEAD];

	if (item != NULL)
	{
		if (item->getRules()->isTwoHanded() == true)
			wounds += _fatalWounds[BODYPART_RIGHTARM] + _fatalWounds[BODYPART_LEFTARM];
		else
		{
			if (item == getItem("STR_RIGHT_HAND"))
				wounds += _fatalWounds[BODYPART_RIGHTARM];
			else
				wounds += _fatalWounds[BODYPART_LEFTARM];
		}
	}

	ret *= 1. - 0.1 * static_cast<double>(wounds);

	if (ret < 0.1) // limit low @ 10%
		ret = 0.1;

	//Log(LOG_INFO) << ". . ACU modi = " << ret;
	return ret;
}

/**
 * Sets the armor value of a certain armor side.
 * @param armor	- amount of armor
 * @param side	- the side of the armor
 */
void BattleUnit::setArmor(
		int armor,
		UnitSide side)
{
	if (armor < 0)
		armor = 0;

	_currentArmor[side] = armor;
}

/**
 * Gets the armor value of a certain armor side.
 * @param side - the side of the armor
 * @return, amount of armor
 */
int BattleUnit::getArmor(UnitSide side) const
{
	return _currentArmor[side];
}

/**
 * Gets total amount of fatal wounds this BattleUnit has.
 * @return, number of fatal wounds
 */
int BattleUnit::getFatalWounds() const
{
	int wounds = 0;

	for (int
			i = 0;
			i < 6;
			++i)
	{
		wounds += _fatalWounds[i];
	}

	return wounds;
}

/**
 * Little formula that calculates initiative/reaction score.
 * Reactions Stat * Current Time Units / Max TUs
 * @param tuSpent - (default 0)
 * @return, Reaction score; aka INITIATIVE
 */
double BattleUnit::getInitiative(int tuSpent)
{
	double ret = static_cast<double>(
						getBaseStats()->reactions * (getTimeUnits() - tuSpent))
						/ static_cast<double>(getBaseStats()->tu);

	ret *= getAccuracyModifier();

	return ret;
}

/**
 * Prepares unit for a new turn.
 */
void BattleUnit::prepUnit()
{
	if (_status == STATUS_TIME_OUT)
		return;

	_faction = _originalFaction;
	_unitsSpottedThisTurn.clear();

	if (isOut() == false)
		initTU();

	// Fire damage is in Battlescape/BattlescapeGame::endTurnPhase(), standing on fire tile;
	// see also, Savegame/Tile::prepareTileTurn(), catch fire on fire tile;
	// fire damage by hit is caused by TileEngine::explode().
	if (_fire > 0)
		--_fire;

	_health -= getFatalWounds(); // suffer from fatal wounds
	if (_health < 0) _health = 0;

	if (_health == 0 // if unit is dead, AI state disappears
		&& _currentAIState != NULL)
	{
		_currentAIState->exit();
		delete _currentAIState;
		_currentAIState = NULL;
	}

	if (_stunLevel > 0 // note ... mechanical creatures should no longer be getting stunned.
		&& (_armor->getSize() == 1
			|| isOut() == false)
		&& (_geoscapeSoldier != NULL
			|| _unitRules->isMechanical() == false))
	{
		healStun(1); // recover stun 1pt/turn
	}

	if (isOut() == false)
	{
		const int panic = 100 - (2 * getMorale());
		if (RNG::percent(panic) == true)
		{
			_status = STATUS_PANICKING;		// panic is either flee or freeze (determined later)

			if (RNG::percent(30) == true)
				_status = STATUS_BERSERK;	// or shoot stuff.
		}
		else if (panic > 0					// successfully avoided Panic
			&& _geoscapeSoldier != NULL)
		{
			++_expBravery;
		}
	}

	_dontReselect = false;
	_motionPoints = 0;
}

/**
 * Calculates and resets this BattleUnit's time units and energy.
 * @param preBattle - true for pre-battle initialization (default false)
 */
void BattleUnit::initTU(bool preBattle)
{
	int initTU = _stats.tu;
	double underLoad = static_cast<double>(_stats.strength) / static_cast<double>(getCarriedWeight());
	underLoad *= getAccuracyModifier() / 2. + 0.5;
	if (underLoad < 1.)
		initTU = static_cast<int>(Round(static_cast<double>(initTU) * underLoad));

	// Each fatal wound to the left or right leg reduces a Soldier's TUs by 10%.
	if (preBattle == false
		&& _geoscapeSoldier != NULL)
	{
		initTU -= (initTU * (getFatalWound(BODYPART_LEFTLEG) + getFatalWound(BODYPART_RIGHTLEG) * 10)) / 100;
	}

	_tu = std::max(12, initTU);

	if (preBattle == false)
	{
		int initEnergy = _stats.stamina; // advanced Energy recovery
		if (_geoscapeSoldier != NULL)
		{
			if (_kneeled == true)
				initEnergy /= 2;
			else
				initEnergy /= 3;
		}
		else // aLiens & Tanks.
			initEnergy = initEnergy * _unitRules->getEnergyRecovery() / 100;

		initEnergy = static_cast<int>(Round(static_cast<double>(initEnergy) * getAccuracyModifier()));

		// Each fatal wound to the body reduces a Soldier's
		// energy recovery by 10% of his/her current energy.
		// note: only xCom Soldiers get fatal wounds, atm
		if (_geoscapeSoldier != NULL)
			initEnergy -= _energy * getFatalWound(BODYPART_TORSO) * 10 / 100;

		initEnergy += _energy;
		setEnergy(std::max(12, initEnergy));
	}
}

/**
 * Changes morale with bounds check.
 * @param change - can be positive or negative
 */
void BattleUnit::moraleChange(int change)
{
	if (isFearable() == false)
		return;

	_morale += change;

	if (_morale > 100)
		_morale = 100;
	else if (_morale < 0)
		_morale = 0;
}

/**
 * Marks this unit as not reselectable.
 */
void BattleUnit::dontReselect()
{
	_dontReselect = true;
}

/**
 * Marks this unit as reselectable.
 */
void BattleUnit::allowReselect()
{
	_dontReselect = false;
}

/**
 * Checks whether reselecting this unit is allowed.
 * @return, reselect allowed
 */
bool BattleUnit::reselectAllowed() const
{
	return !_dontReselect;
}

/**
 * Sets the amount of turns this unit is on fire.
 * @param fire - amount of turns this unit will be on fire (no fire 0)
 */
void BattleUnit::setFire(int fire)
{
//	if (_specab == SPECAB_BURNFLOOR
//		|| _specab == SPECAB_BURN_AND_EXPLODE)
	if (_specab != SPECAB_BURNFLOOR
		&& _specab != SPECAB_BURN_AND_EXPLODE)
	{
		_fire = fire;
	}
}

/**
 * Gets the amount of turns this unit is on fire.
 * @return, amount of turns this unit will be on fire (0 - no fire)
 */
int BattleUnit::getFire() const
{
	return _fire;
}

/**
 * Gets the pointer to the vector of inventory items.
 * @return, pointer to a vector of pointers to this BattleUnit's BattleItems
 */
std::vector<BattleItem*>* BattleUnit::getInventory()
{
	return &_inventory;
}

/**
 * Lets the AI do its thing.
 * @param action - current AI action
 */
void BattleUnit::think(BattleAction* action)
{
	//Log(LOG_INFO) << "BattleUnit::think()";
	//Log(LOG_INFO) << ". checkAmmo()";
	checkAmmo();
	//Log(LOG_INFO) << ". _currentAIState->think()";
	_currentAIState->think(action);
	//Log(LOG_INFO) << "BattleUnit::think() EXIT";
}

/**
 * Sets this BattleUnit's current AI state.
 * @param aiState - pointer to AI state
 */
void BattleUnit::setAIState(BattleAIState* aiState)
{
	if (_currentAIState)
	{
		_currentAIState->exit();

		delete _currentAIState;
	}

	_currentAIState = aiState;
	_currentAIState->enter();
}

/**
 * Returns the current AI state.
 * @return, pointer to this BattleUnit's AI state
 */
BattleAIState* BattleUnit::getCurrentAIState() const
{
	return _currentAIState;
}

/**
 * Sets the tile that a unit is standing on.
 * @param tile		- pointer to a Tile
 * @param tileBelow	- pointer to the Tile below
 */
void BattleUnit::setTile(
		Tile* tile,
		Tile* tileBelow)
{
	//Log(LOG_INFO) << "BattleUnit::setTile()";
	_tile = tile;
	if (_tile == NULL)
	{
		_floating = false;
		return;
	}

	// unit could have changed from flying to walking or vice versa
	if (_status == STATUS_WALKING
		&& _tile->hasNoFloor(tileBelow)
		&& _movementType == MT_FLY)
	{
		_status = STATUS_FLYING;
		_floating = true;
		//Log(LOG_INFO) << ". STATUS_WALKING, _floating = " << _floating;
	}
	else if (_status == STATUS_FLYING
		&& _tile->hasNoFloor(tileBelow) == false
		&& _verticalDirection == 0)
	{
		_status = STATUS_WALKING;
		_floating = false;
		//Log(LOG_INFO) << ". STATUS_FLYING, _floating = " << _floating;
	}
/*	else if (_status == STATUS_STANDING	// kL. keeping this section in tho it was taken out
		&& _movementType == MT_FLY)		// when STATUS_UNCONSCIOUS below was inserted.
										// Problem: when loading a save, _floating goes TRUE!
	{
		_floating = _tile->hasNoFloor(tileBelow);
		//Log(LOG_INFO) << ". STATUS_STANDING, _floating = " << _floating;
	} */
	else if (_status == STATUS_UNCONSCIOUS) // <- kL_note: not so sure having flying unconscious soldiers is a good deal.
	{
		_floating = _movementType == MT_FLY
				 && _tile->hasNoFloor(tileBelow);
		//Log(LOG_INFO) << ". STATUS_UNCONSCIOUS, _floating = " << _floating;
	}
	//Log(LOG_INFO) << "BattleUnit::setTile() EXIT";
}

/**
 * Gets this BattleUnit's current tile.
 * @return, pointer to Tile
 */
Tile* BattleUnit::getTile() const
{
	return _tile;
}

/**
 * Checks if there's an inventory item in the specified inventory position.
 * @param slot	- pointer to RuleInventory slot
 * @param x		- X position in slot (default 0)
 * @param y		- Y position in slot (default 0)
 * @return, pointer to BattleItem in slot or NULL if none
 */
BattleItem* BattleUnit::getItem(
		RuleInventory* slot,
		int x,
		int y) const
{
	if (slot->getType() != INV_GROUND) // Soldier items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getSlot() == slot
				&& (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	else if (_tile != NULL) // Ground items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _tile->getInventory()->begin();
				i != _tile->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}

	return NULL;
}

/**
 * Checks if there's an inventory item in the specified inventory position.
 * @param slot	- reference an inventory slot
 * @param x		- X position in slot (default 0)
 * @param y		- Y position in slot (default 0)
 * @return, pointer to BattleItem in slot or NULL if none
 */
BattleItem* BattleUnit::getItem(
		const std::string& slot,
		int x,
		int y) const
{
	if (slot != "STR_GROUND") // Soldier items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _inventory.begin();
				i != _inventory.end();
				++i)
		{
			if ((*i)->getSlot() != NULL
				&& (*i)->getSlot()->getId() == slot
				&& (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}
	else if (_tile != NULL) // Ground items
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _tile->getInventory()->begin();
				i != _tile->getInventory()->end();
				++i)
		{
			if ((*i)->getSlot() != NULL
				&& (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}

	return NULL;
}

/**
 * Gets the 'main hand weapon' of this BattleUnit.
 * Ought alter AI so this Returns firearms only.
 * @param quickest - true to choose the quickest weapon (default true)
 * @return, pointer to a BattleItem or NULL
 */
BattleItem* BattleUnit::getMainHandWeapon(bool quickest) const
{
	// kL_note: This gets called way too much, from somewhere, when just walking around.
	// probably AI patrol state

	//Log(LOG_INFO) << "BattleUnit::getMainHandWeapon()";
	BattleItem
		* const rhtWeapon = getItem("STR_RIGHT_HAND"),
		* const lftWeapon = getItem("STR_LEFT_HAND");

	const bool
		isRht = rhtWeapon
				&& (rhtWeapon->getRules()->getBattleType() == BT_MELEE
					|| (rhtWeapon->getRules()->getBattleType() == BT_FIREARM
						&& rhtWeapon->getAmmoItem()
						&& rhtWeapon->getAmmoItem()->getAmmoQuantity())),
		isLft = lftWeapon
				&& (lftWeapon->getRules()->getBattleType() == BT_MELEE
					|| (lftWeapon->getRules()->getBattleType() == BT_FIREARM
						&& lftWeapon->getAmmoItem()
						&& lftWeapon->getAmmoItem()->getAmmoQuantity()));
	//Log(LOG_INFO) << ". isRht = " << isRht;
	//Log(LOG_INFO) << ". isLft = " << isLft;

	if (!isRht && !isLft)
		return NULL;

	if (isRht && !isLft)
		return rhtWeapon;

	if (!isRht && isLft)
		return lftWeapon;

	//Log(LOG_INFO) << ". . isRht & isLft VALID";

	const RuleItem* rule = rhtWeapon->getRules();
	int rhtTU = rule->getTUSnap();
	if (rhtTU == 0)
		if (rhtTU = rule->getTUAuto() == 0)
			if (rhtTU = rule->getTUAimed() == 0)
				if (rhtTU = rule->getTULaunch() == 0)
					rhtTU = rule->getTUMelee();

	rule = lftWeapon->getRules();
	int lftTU = rule->getTUSnap();
	if (lftTU == 0)
		if (lftTU = rule->getTUAuto() == 0)
			if (lftTU = rule->getTUAimed() == 0)
				if (lftTU = rule->getTULaunch() == 0)
					lftTU = rule->getTUMelee();
	// note: Should probly account for 'noReaction' weapons ...

	//Log(LOG_INFO) << ". . rhtTU = " << rhtTU;
	//Log(LOG_INFO) << ". . lftTU = " << lftTU;

	if (!rhtTU && !lftTU)
		return NULL;

	if (rhtTU && !lftTU)
		return rhtWeapon;

	if (!rhtTU && lftTU)
		return lftWeapon;

	if (quickest == true) // rhtTU && lftTU
	{
		if (rhtTU <= lftTU)
			return rhtWeapon;
		else
			return lftWeapon;
	}
	else
	{
		if (rhtTU >= lftTU)
			return rhtWeapon;
		else
			return lftWeapon;
	}

	// kL_note: should exit this by setting ActiveHand.
	//Log(LOG_INFO) << "BattleUnit::getMainHandWeapon() EXIT 0, no weapon";
	return NULL;
}
/*	BattleItem *weaponRightHand = getItem("STR_RIGHT_HAND");
	BattleItem *weaponLeftHand = getItem("STR_LEFT_HAND");

	// ignore weapons without ammo (rules out grenades)
	if (!weaponRightHand || !weaponRightHand->getAmmoItem() || !weaponRightHand->getAmmoItem()->getAmmoQuantity())
		weaponRightHand = 0;
	if (!weaponLeftHand || !weaponLeftHand->getAmmoItem() || !weaponLeftHand->getAmmoItem()->getAmmoQuantity())
		weaponLeftHand = 0;

	// if there is only one weapon, it's easy:
	if (weaponRightHand && !weaponLeftHand)
		return weaponRightHand;
	else if (!weaponRightHand && weaponLeftHand)
		return weaponLeftHand;
	else if (!weaponRightHand && !weaponLeftHand)
		return 0;

	// otherwise pick the one with the least snapshot TUs
	int tuRightHand = weaponRightHand->getRules()->getTUSnap();
	int tuLeftHand = weaponLeftHand->getRules()->getTUSnap();
	// if only one weapon has snapshot, pick that one
	if (tuLeftHand <= 0 && tuRightHand > 0)
		return weaponRightHand;
	else if (tuRightHand <= 0 && tuLeftHand > 0)
		return weaponLeftHand;
	// else pick the better one
	else
	{
		if (tuLeftHand >= tuRightHand)
			return quickest?weaponRightHand:weaponLeftHand;
		else
			return quickest?weaponLeftHand:weaponRightHand;
	} */

/**
 * Get a grenade from hand or belt (used for AI).
 * @return, pointer to a grenade or NULL
 */
const BattleItem* const BattleUnit::getGrenade() const // holy crap const.
{
	// kL_begin: BattleUnit::getGrenadeFromBelt(), or hand.
	const BattleItem* grenade = getItem("STR_RIGHT_HAND");
	if (grenade == NULL
		|| grenade->getRules()->getBattleType() != BT_GRENADE)
	{
		grenade = getItem("STR_LEFT_HAND");
	}

	if (grenade != NULL
		&& grenade->getRules()->getBattleType() == BT_GRENADE)
	{
		return grenade;
	} // kL_end.

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i)->getRules()->getBattleType() == BT_GRENADE)
			return *i;
	}

	return NULL;
}

/**
 * Gets the name of any melee weapon this BattleUnit may be carrying, or a built in one.
 * @return, the name of a melee weapon
 */
std::string BattleUnit::getMeleeWeapon() const
{
	if (getItem("STR_RIGHT_HAND")
		&& getItem("STR_RIGHT_HAND")->getRules()->getBattleType() == BT_MELEE)
	{
		return getItem("STR_RIGHT_HAND")->getRules()->getType();
	}

	if (getItem("STR_LEFT_HAND")
		&& getItem("STR_LEFT_HAND")->getRules()->getBattleType() == BT_MELEE)
	{
		return getItem("STR_LEFT_HAND")->getRules()->getType();
	}

	if (_unitRules != NULL) // -> do not CTD for Mc'd xCom agents.
		return _unitRules->getMeleeWeapon();

	return "";
}
/* BattleItem *BattleUnit::getMeleeWeapon()
{
	BattleItem *melee = getItem("STR_RIGHT_HAND");
	if (melee && melee->getRules()->getBattleType() == BT_MELEE)
	{
		return melee;
	}
	melee = getItem("STR_LEFT_HAND");
	if (melee && melee->getRules()->getBattleType() == BT_MELEE)
	{
		return melee;
	}
	melee = getSpecialWeapon(BT_MELEE);
	if (melee)
	{
		return melee;
	}
	return 0;
} */

/**
 * Check if we have ammo and reload if needed (used for the AI).
 * @return, true if unit has a loaded weapon or has just loaded it.
 */
bool BattleUnit::checkAmmo()
{
	if (getTimeUnits() < 15) // should go to Ruleset.
		return false;

	BattleItem* weapon = getItem("STR_RIGHT_HAND");
	if (weapon == NULL
		|| weapon->getAmmoItem() != NULL
		|| weapon->getRules()->getBattleType() == BT_MELEE)
	{
		weapon = getItem("STR_LEFT_HAND");
		if (weapon == NULL
			|| weapon->getAmmoItem() != NULL
			|| weapon->getRules()->getBattleType() == BT_MELEE)
		{
			return false;
		}
	}

	// we have a non-melee weapon with no ammo and 15 or more TUs,
	// we might need to look for ammo then ...
	bool wrongAmmo = true;

	BattleItem* ammo = NULL;

	for (std::vector<BattleItem*>::const_iterator
			i = getInventory()->begin();
			i != getInventory()->end();
			++i)
	{
		ammo = *i;
		for (std::vector<std::string>::const_iterator
				j = weapon->getRules()->getCompatibleAmmo()->begin();
				j != weapon->getRules()->getCompatibleAmmo()->end();
				++j)
		{
			if (*j == ammo->getRules()->getType())
			{
				wrongAmmo = false;
				break;
			}
		}

		if (wrongAmmo == false)
			break;
	}

	if (wrongAmmo == true)
		return false; // didn't find any compatible ammo in inventory

	spendTimeUnits(15);
	weapon->setAmmoItem(ammo);
	ammo->moveToOwner(NULL);

	//Log(LOG_INFO) << "BattleUnit::checkAmmo() EXIT";
	return true;
}

/**
 * Check if this unit is in the exit area.
 * @param stt - type of exit tile to check for (default START_POINT)
 * @return, true if unit is in a special exit area
 */
bool BattleUnit::isInExitArea(SpecialTileType stt) const
{
	return _tile != NULL
		&& _tile->getMapData(MapData::O_FLOOR) != NULL
		&& _tile->getMapData(MapData::O_FLOOR)->getSpecialType() == stt;
}

/**
 * Gets this unit's height whether standing or kneeling.
 * @return, unit's height
 */
int BattleUnit::getHeight() const
{
	int ret;

	if (isKneeled() == true)
		ret = _kneelHeight; //getKneelHeight();
	else
		ret = _standHeight; // getStandHeight();

//	if (ret > 24)
//		ret = 24;

	return ret;
}

/**
 * Gets this unit's Firing experience.
 * @return, firing xp
 */
int BattleUnit::getExpFiring() const
{
	return _expFiring;
}

/**
 * Gets a soldier's Throwing experience.
 * @return, throwing xp
 */
int BattleUnit::getExpThrowing() const
{
	return _expThrowing;
}

/**
 * Gets a soldier's Melee experience.
 * @return, melee xp
 */
int BattleUnit::getExpMelee() const
{
	return _expMelee;
}

/**
 * Gets a soldier's Reactions experience.
 * @return, reactions xp
 */
int BattleUnit::getExpReactions() const
{
	return _expReactions;
}

/**
 * Gets a soldier's Bravery experience.
 * @return, bravery xp
 */
int BattleUnit::getExpBravery() const
{
	return _expBravery;
}

/**
 * Gets a soldier's PsiSkill experience.
 * @return, psiskill xp
 */
int BattleUnit::getExpPsiSkill() const
{
	return _expPsiSkill;
}

/**
 * Gets a soldier's PsiStrength experience.
 * @return, psistrength xp
 */
int BattleUnit::getExpPsiStrength() const
{
	return _expPsiStrength;
}

/**
 * Adds one to the reaction exp counter.
 */
void BattleUnit::addReactionExp()
{
	++_expReactions;
}

/**
 * Adds one to the firing exp counter.
 */
void BattleUnit::addFiringExp()
{
	++_expFiring;
}

/**
 * Adds one to the throwing exp counter.
 */
void BattleUnit::addThrowingExp()
{
	++_expThrowing;
}

/**
 * Adds qty to the psiSkill exp counter.
 * @param qty - amount to add (default 1)
 */
void BattleUnit::addPsiSkillExp(int qty)
{
	_expPsiSkill += qty;
}

/**
 * Adds qty to the psiStrength exp counter.
 * @param qty - amount to add (default 1)
 */
void BattleUnit::addPsiStrengthExp(int qty)
{
	_expPsiStrength += qty;
}

/**
 * Adds qty to the melee exp counter.
 * @param qty - amount to add (default 1)
 */
void BattleUnit::addMeleeExp(int qty)
{
	_expMelee += qty;
}

/**
 * Adds a mission and kill counts to Soldier's stats.
 * @param soldier - pointer to a Soldier
 */
void BattleUnit::updateGeoscapeStats(Soldier* soldier)
{
	soldier->addMissionCount();
	soldier->addKillCount(_kills);
}

/**
 * Check if unit eligible for squaddie promotion. If yes, promote the unit.
 * Increase the mission counter. Calculate the experience increases.
 * @param geoscape - pointer to geoscape save
 * @return, true if the soldier was eligible for squaddie promotion
 */
bool BattleUnit::postMissionProcedures(SavedGame* geoscape)
{
	Soldier* const soldier = geoscape->getSoldier(_id);
	if (soldier == NULL)
		return false;


	updateGeoscapeStats(soldier); // missions & kills

	UnitStats* const stats = soldier->getCurrentStats();
	const UnitStats caps = soldier->getRules()->getStatCaps();

	const int healthLoss = stats->health - _health;
	soldier->setWoundRecovery(RNG::generate(
						static_cast<int>((static_cast<double>(healthLoss) * 0.5)),
						static_cast<int>((static_cast<double>(healthLoss) * 1.5))));

	if (_expBravery != 0
		&& stats->bravery < caps.bravery)
	{
		if (_expBravery > RNG::generate(0, 8))
			stats->bravery += 10;
	}

	if (_expFiring != 0
		&& stats->firing < caps.firing)
	{
		stats->firing += improveStat(_expFiring);

		// kL_begin: add a touch of reactions if good firing .....
		if (_expFiring - 2 > 0
			&& stats->reactions < caps.reactions)
		{
			_expReactions += _expFiring / 3;
//			stats->reactions += improveStat(_expFiring - 2);
		} // kL_end.
	}

	if (_expReactions != 0
		&& stats->reactions < caps.reactions)
	{
		stats->reactions += improveStat(_expReactions);
	}

	if (_expMelee != 0
		&& stats->melee < caps.melee)
	{
		stats->melee += improveStat(_expMelee);
	}

	if (_expPsiSkill != 0
		&& stats->psiSkill < caps.psiSkill)
	{
		stats->psiSkill += improveStat(_expPsiSkill);
	}

	if (_expPsiStrength != 0
		&& stats->psiStrength < caps.psiStrength)
	{
		stats->psiStrength += improveStat(_expPsiStrength);
	}

	if (_expThrowing != 0
		&& stats->throwing < caps.throwing)
	{
		stats->throwing += improveStat(_expThrowing);
	}


	const bool expPri = _expBravery != 0
					 || _expReactions != 0
					 || _expFiring != 0
					 || _expMelee != 0;

	if (expPri == true
		|| _expPsiSkill != 0
		|| _expPsiStrength != 0)
	{
//		if (soldier->getRank() == RANK_ROOKIE)
		if (hasFirstKill() == true)
			soldier->promoteRank();

		if (expPri == true)
		{
			int delta = caps.tu - stats->tu;
			if (delta > 0)
				stats->tu += RNG::generate(
										0,
										(delta / 10) + 2) - 1;

			delta = caps.health - stats->health;
			if (delta > 0)
				stats->health += RNG::generate(
										0,
										(delta / 10) + 2) - 1;

			delta = caps.strength - stats->strength;
			if (delta > 0)
				stats->strength += RNG::generate(
										0,
										(delta / 10) + 2) - 1;

			delta = caps.stamina - stats->stamina;
			if (delta > 0)
				stats->stamina += RNG::generate(
										0,
										(delta / 10) + 2) - 1;
		}

		return true;
	}

	return false;
}

/**
 * Converts an amount of experience to a stat increase.
 * @param exp - experience count from battle mission
 * @return, stat increase
 */
int BattleUnit::improveStat(int exp)
{
	int teir = 1;

	if		(exp > 10)	teir = 4;
	else if (exp > 5)	teir = 3;
	else if (exp > 2)	teir = 2;

	return (teir / 2 + RNG::generate(0, teir));
}

/**
 * Get the unit's minimap sprite index. Used to display the unit on the minimap
 * @return, the unit minimap index
 */
int BattleUnit::getMiniMapSpriteIndex() const
{
	// minimap sprite index:
	// * 0-2   : Xcom soldier
	// * 3-5   : Alien
	// * 6-8   : Civilian
	// * 9-11  : Item
	// * 12-23 : Xcom HWP
	// * 24-35 : Alien big terror unit(cyberdisk, ...)
	if (isOut(true, true) == true)
		return 9;

	switch (getFaction())
	{
		case FACTION_HOSTILE:
			if (_armor->getSize() == 1)
				return 3;
			else
				return 24;
		break;
		case FACTION_NEUTRAL:
			return 6;
		break;

		default:
			if (_armor->getSize() == 1)
				return 0;
			else
				return 12;
	}
}

/**
 * Sets the turret type. -1 is no turret.
 * @param turretType - the turret type to set
 */
void BattleUnit::setTurretType(int turretType)
{
	_turretType = turretType;
}

/**
 * Gets the turret type. -1 is no turret.
 * @return, turret type
 */
int BattleUnit::getTurretType() const
{
	return _turretType;
}

/**
 * Gets the amount of fatal wound for a body part.
 * @param part - the body part (in the range 0-5)
 * @return, the amount of fatal wound of a body part
 */
int BattleUnit::getFatalWound(int part) const
{
	if (part < 0 || part > 5)
		return 0;

	return _fatalWounds[part];
}

/**
 * Heals a fatal wound of the soldier.
 * @param part		- the body part to heal
 * @param wounds	- the amount of fatal wound healed
 * @param health	- the amount of health to add to soldier health
 */
void BattleUnit::heal(
		int part,
		int wounds,
		int health)
{
	if (part < 0 || part > 5)
		return;

	if (_fatalWounds[part] == false)
		return;

	_fatalWounds[part] -= wounds;

	_health += health;
	if (_health > getBaseStats()->health)
		_health = getBaseStats()->health;

	moraleChange(health);
}

/**
 * Restores soldier morale.
 */
void BattleUnit::painKillers()
{
	int lostHealth = getBaseStats()->health - _health;
	if (lostHealth > _moraleRestored)
	{
		_morale = std::min(
						100,
						lostHealth - _moraleRestored + _morale);
		_moraleRestored = lostHealth;
	}
}

/**
 * Restores soldier energy and reduce stun level.
 * @param energy	- the amount of energy to add
 * @param stun		- the amount of stun level to reduce
 */
void BattleUnit::stimulant(
		int energy,
		int stun)
{
	_energy += energy;
	if (_energy > getBaseStats()->stamina)
		_energy = getBaseStats()->stamina;

	healStun(stun);
}

/**
 * Gets motion points for the motion scanner.
 * More points is a larger blip on the scanner.
 * @return, motion points
 */
int BattleUnit::getMotionPoints() const
{
	return _motionPoints;
}

/**
 * Gets the unit's armor.
 * @return, pointer to Armor
 */
Armor* BattleUnit::getArmor() const
{
	return _armor;
}

/**
 * kL. Checks if unit is wearing a PowerSuit.
 * @return, true if this unit is wearing a PowerSuit of some sort
 */
bool BattleUnit::hasPowerSuit() const // kL
{
	std::string armorType = _armor->getType();

	if (armorType == "STR_POWER_SUIT_UC"
		|| armorType == "STR_BLACK_ARMOR_UC"
		|| armorType == "STR_BLUE_ARMOR_UC"
		|| armorType == "STR_GREEN_ARMOR_UC"
		|| armorType == "STR_ORANGE_ARMOR_UC"
		|| armorType == "STR_PINK_ARMOR_UC"
		|| armorType == "STR_PURPLE_ARMOR_UC"
		|| armorType == "STR_RED_ARMOR_UC")
	{
		return true;
	}

	return false;
}

/**
 * kL. Checks if unit is wearing a FlightSuit.
 * @return, true if this unit is wearing a FlightSuit of some sort
 */
bool BattleUnit::hasFlightSuit() const // kL
{
	std::string armorType = _armor->getType();

	if (armorType == "STR_FLYING_SUIT_UC"
		|| armorType == "STR_BLACKSUIT_ARMOR_UC"
		|| armorType == "STR_BLUESUIT_ARMOR_UC"
		|| armorType == "STR_GREENSUIT_ARMOR_UC"
		|| armorType == "STR_ORANGESUIT_ARMOR_UC"
		|| armorType == "STR_PINKSUIT_ARMOR_UC"
		|| armorType == "STR_PURPLESUIT_ARMOR_UC"
		|| armorType == "STR_REDSUIT_ARMOR_UC")
	{
		return true;
	}

	return false;
}

/**
 * Gets a unit's name.
 * An aLien's name is the translation of its race and rank; hence the language pointer needed.
 * @param lang			- pointer to Language
 * @param debugAppendId	- append unit ID to name for debug purposes
 * @return, name of the unit
 */
std::wstring BattleUnit::getName(
		Language* lang,
		bool debugAppendId) const
{
	if (_geoscapeSoldier == NULL
		&& lang != NULL)
	{
		std::wstring ret;

		if (_type.find("STR_") != std::string::npos)
			ret = lang->getString(_type);
		else
			ret = lang->getString(_race);

		if (debugAppendId)
		{
			std::wostringstream ss;
			ss << ret << L" " << _id;
			ret = ss.str();
		}

		return ret;
	}

	return _name;
}

/**
 * Gets a pointer to this BattleUnit's stats.
 * NOTE: xCom Soldiers return their current statistics; aLiens & Units return rule statistics.
 * @return, pointer to UnitStats
 */
const UnitStats* BattleUnit::getBaseStats() const
{
	return &_stats;
}

/**
 * Gets a unit's stand height.
 * @return, this unit's height in voxels when standing
 */
int BattleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets a unit's kneel height.
 * @return, this unit's height in voxels when kneeling
 */
int BattleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets a unit's floating elevation.
 * @return, this unit's elevation over the ground in voxels when floating or flying
 */
int BattleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets this unit's LOFT id, one per unit tile.
 * This is one slice only, as it is repeated over the entire height of the unit;
 * that is, each tile has only one LOFT.
 * @param entry - an entry in LofTemps set
 * @return, this unit's Line of Fire Template id
 */
int BattleUnit::getLoftemps(int entry) const
{
	return _loftempsSet.at(entry);
}

/**
 * Gets this unit's value. Used for score at debriefing.
 * @return, value score
 */
int BattleUnit::getValue() const
{
	return _value;
}

/**
 * Gets this unit's death sound.
 * @return, death sound ID
 */
int BattleUnit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Gets this unit's move sound.
 * @return, move sound ID
 */
int BattleUnit::getMoveSound() const
{
	return _moveSound;
}

/**
 * Gets whether the unit is affected by fatal wounds.
 * Normally only soldiers are affected by fatal wounds.
 * @return, true if unit can be affected by wounds
 */
bool BattleUnit::isWoundable() const
{
	return _geoscapeSoldier != NULL
	   || (Options::alienBleeding
			&& _unitRules->isMechanical() == false
			&& _race != "STR_ZOMBIE");

/*	if (_geoscapeSoldier != NULL)
		return true;

	if (Options::alienBleeding
		&& _unitRules->isMechanical() == false
		&& _race != "STR_ZOMBIE")
	{
		return true;
	}

	return false; */
}

/**
 * Gets whether this unit is affected by morale loss.
 * @return, true if unit can be affected by morale changes
 */
bool BattleUnit::isFearable() const
{
	return _geoscapeSoldier != NULL
	   || (_unitRules->isMechanical() == false
			&& _race != "STR_ZOMBIE");

/*	if (_geoscapeSoldier != NULL)
		return true;

	if (_unitRules->isMechanical() == false
		&& _race != "STR_ZOMBIE")
	{
		return true;
	}

	return false; */
}

/**
 * Gets the number of turns an AI unit remembers a soldier's position.
 * @return, intelligence
 */
int BattleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Gets this unit's aggression rating for use by the AI.
 * @return, aggression
 */
int BattleUnit::getAggression() const
{
	return _aggression;
}

/**
 * Gets this unit's special ability (SpecialAbility enum).
 * @return, SpecialAbility
 */
int BattleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Sets this unit's special ability (SpecialAbility enum).
 * @param specab - SpecialAbility
 */
void BattleUnit::setSpecialAbility(const SpecialAbility specab)
{
	_specab = specab;
}

/**
 * Sets this unit to respawn or not.
 * @param respawn - true to respawn
 */
void BattleUnit::setRespawn(const bool respawn)
{
	_respawn = respawn;
}

/**
 * Gets this unit's respawn flag.
 * @return, true for respawn
 */
bool BattleUnit::getRespawn() const
{
	return _respawn;
}

/**
 * Gets unit-type that is spawned when this one dies.
 * @return, special spawn unit type (ie. ZOMBIES!!!)
 */
std::string BattleUnit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Sets a unit-type that is spawned when this one dies.
 * @param spawnUnit - reference the special unit type
 */
void BattleUnit::setSpawnUnit(const std::string& spawnUnit)
{
	_spawnUnit = spawnUnit;
}

/**
 * Gets this unit's rank string.
 * @return, rank
 */
std::string BattleUnit::getRankString() const
{
	return _rank;
}

/**
 * Gets this unit's race string.
 * @return, race
 */
std::string BattleUnit::getRaceString() const
{
	return _race;
}

/**
 * Gets the geoscape-soldier object.
 * @return, pointer to Soldier
 */
Soldier* BattleUnit::getGeoscapeSoldier() const
{
	return _geoscapeSoldier;
}

/**
 * Add a kill to the counter.
 */
void BattleUnit::addKillCount()
{
	++_kills;
}

/**
 * Gets if this is a Rookie and has made his/her first kill.
 */
bool BattleUnit::hasFirstKill() const
{
	return _rankInt == 0
		&& (_kills > 0 // redundant, but faster
			|| _statistics->hasKillOrStun());
}

/**
 * Gets unit type.
 * @return, unit type
 */
std::string BattleUnit::getType() const
{
	return _type;
}

/**
 * Sets unit's active hand.
 * @param slot - reference a handslot
 */
void BattleUnit::setActiveHand(const std::string& slot)
{
	if (_activeHand != slot)
		_cacheInvalid = true;

	_activeHand = slot;
}

/**
 * Gets unit's active hand. Must have an item in that hand.
 * Else, switch to other hand or use righthand as default.
 * @return, the active hand
 */
std::string BattleUnit::getActiveHand() const
{
	if (getItem(_activeHand)) // has an item in the already active Hand.
		return _activeHand;

	if (getItem("STR_RIGHT_HAND"))
		return "STR_RIGHT_HAND";

	if (getItem("STR_LEFT_HAND"))
		return "STR_LEFT_HAND";

	return "";
}

/**
 * Converts unit to another faction - original faction is still stored.
 * @param faction - UnitFaction
 */
void BattleUnit::convertToFaction(UnitFaction faction)
{
	_faction = faction;
}

/**
 * Sets health to 0 and set status dead - used when getting zombified, etc.
 */
void BattleUnit::instaKill()
{
	_health = 0;
	_status = STATUS_DEAD;
}

/**
 * Gets sound to play when unit aggros.
 * @return, aggro sound
 */
int BattleUnit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Halves this unit's armor values for beginner mode.
 */
void BattleUnit::halveArmor()
{
	_currentArmor[0] /= 2;
	_currentArmor[1] /= 2;
	_currentArmor[2] /= 2;
	_currentArmor[3] /= 2;
	_currentArmor[4] /= 2;
}

/**
 * Gets the faction the unit was killed by.
 * @return UnitFaction
 */
UnitFaction BattleUnit::killedBy() const
{
	return _killedBy;
}

/**
 * Sets the faction the unit was killed by.
 * @param faction - UnitFaction
 */
void BattleUnit::killedBy(UnitFaction faction)
{
	_killedBy = faction;
}

/**
 * Sets the units we are charging towards.
 * @param chargeTarget - pointer to a target
 */
void BattleUnit::setCharging(BattleUnit* chargeTarget)
{
	_charging = chargeTarget;
}

/**
 * Gets the unit this BattleUnit is charging towards.
 * @return, pointer to the charged BattleUnit
 */
BattleUnit* BattleUnit::getCharging()
{
	return _charging;
}

/**
 * Gets the unit's carried weight in strength units.
 * @param dragItem - item to ignore
 * @return, weight
 */
int BattleUnit::getCarriedWeight(const BattleItem* const dragItem) const
{
	int weight = _armor->getWeight();
	//Log(LOG_INFO) << "wt armor = " << weight;

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if (*i == dragItem)
			continue;

		weight += (*i)->getRules()->getWeight();
		//Log(LOG_INFO) << "weight = " << weight;

		if ((*i)->getAmmoItem()
			&& (*i)->getAmmoItem() != *i)
		{
			weight += (*i)->getAmmoItem()->getRules()->getWeight();
		}
	}

	//Log(LOG_INFO) << "weight[ret] = " << weight;
	return std::max(0, weight);
}

/**
 * Sets how long since this unit was last exposed.
 * Use "255" for NOT exposed.
 * @param turns - # turns this unit has been exposed
 */
void BattleUnit::setTurnsExposed(int turns)
{
	_turnsExposed = turns;

	if (_turnsExposed > 255) // kL
		_turnsExposed = 255; // kL
		// kL_note: should set this to -1 instead of 255.
		// Note, that in the .Save file, aLiens are 0
		// and notExposed xCom units are 255
}

/**
 * Gets how long since this unit was exposed.
 * @return, turns this unit has been exposed
 */
int BattleUnit::getTurnsExposed() const
{
	return _turnsExposed;
}

/**
 * Gets this unit's original Faction.
 * @return, original UnitFaction
 */
UnitFaction BattleUnit::getOriginalFaction() const
{
	return _originalFaction;
}

/**
 * Invalidates cache; call after copying object :(
 */
void BattleUnit::invalidateCache()
{
	for (int
			i = 0;
			i < 5;
			++i)
	{
		_cache[i] = 0;
	}

	_cacheInvalid = true;
}

/**
 * Gets the other units spotted this turn by this unit.
 * kL_Update: now has relevance only for aLien units.
 * @return, reference to a vector of pointers to BattleUnits
 */
std::vector<BattleUnit*>& BattleUnit::getUnitsSpottedThisTurn()
{
	return _unitsSpottedThisTurn;
}

/**
 * Sets the numeric version of a unit's rank.
 * @param ranks - unit rank ( 0 = lowest ) kL_note: uh doesn't aLien & xCom go opposite ways
 */
void BattleUnit::setRankInt(int ranks)
{
	_rankInt = ranks;
}

/**
 * Gets the numeric version of a unit's rank.
 * @return, unit rank ( 0 = lowest ) kL_note: uh doesn't aLien & xCom go opposite ways
 */
int BattleUnit::getRankInt() const
{
	return _rankInt;
}

/**
 * Derives a numeric unit rank from a string rank (for xCom/soldier units only).
 */
void BattleUnit::deriveRank()
{
	if (_faction == FACTION_PLAYER)
	{
		if		(_rank == "STR_COMMANDER")	_rankInt = 5;
		else if (_rank == "STR_COLONEL")	_rankInt = 4;
		else if (_rank == "STR_CAPTAIN")	_rankInt = 3;
		else if (_rank == "STR_SERGEANT")	_rankInt = 2;
		else if (_rank == "STR_SQUADDIE")	_rankInt = 1;
		else if (_rank == "STR_ROOKIE")		_rankInt = 0;
	}
}

/**
 * Checks if a tile is in a unit's facing-quadrant. Using maths!
 * @param pos - the Position to check against
 * @return, whatever the maths decide
 */
bool BattleUnit::checkViewSector(Position pos) const
{
	int
		dx = pos.x - _pos.x,
		dy = _pos.y - pos.y;

	switch (_direction)
	{
		case 0:
			if (dx + dy >= 0 && dy - dx >= 0)
				return true;
		break;
		case 1:
			if (dx >= 0 && dy >= 0)
				return true;
		break;
		case 2:
			if (dx + dy >= 0 && dy - dx <= 0)
				return true;
		break;
		case 3:
			if (dy <= 0 && dx >= 0)
				return true;
		break;
		case 4:
			if (dx + dy <= 0 && dy - dx <= 0)
				return true;
		break;
		case 5:
			if (dx <= 0 && dy <= 0)
				return true;
		break;
		case 6:
			if (dx + dy <= 0 && dy - dx >= 0)
				return true;
		break;
		case 7:
			if (dy >= 0 && dx <= 0)
				return true;
		break;

		default:
			return false;
		break;
	}

	return false;
}

/**
 * Adjusts a unit's stats according to difficulty setting (used by aLiens only).
 * @param diff	- the current game's difficulty setting
 * @param month	- the number of months that have progressed
 */
void BattleUnit::adjustStats(
		const int diff,
		const int month)
{
	// adjust the unit's stats according to the difficulty level. Note, Throwing is not affected.
	_stats.tu			+= 4 * diff * _stats.tu / 100;
	_stats.stamina		+= 4 * diff * _stats.stamina / 100;
	_stats.reactions	+= 6 * diff * _stats.reactions / 100;
//	_stats.firing		= (_stats.firing + 6 * diff * _stats.firing / 100) / (diff > 0? 1: 2);
	_stats.firing		+= 6 * diff * _stats.firing / 100; // see below_
	_stats.throwing		+= 4 * diff * _stats.throwing / 100; // kL
	_stats.melee		+= 4 * diff * _stats.melee / 100;
	_stats.strength		+= 2 * diff * _stats.strength / 100;
	_stats.psiStrength	+= 4 * diff * _stats.psiStrength / 100;
	_stats.psiSkill		+= 4 * diff * _stats.psiSkill / 100;

	// kL_begin:
	if (diff == 0)
	{
		halveArmor(); // moved here from BattlescapeGenerator::addAlien() & BattlescapeGame::convertUnit()
		_stats.firing /= 2; // from above^
	}

	if (month > 0) // aLiens get tuffer as game progresses:
	{
//		_stats.tu += month;
//		_stats.stamina += month;
		if (_stats.reactions > 0)	_stats.reactions	+= month;
		if (_stats.firing > 0)		_stats.firing		+= month;
		if (_stats.throwing > 0)	_stats.throwing		+= month;
		if (_stats.melee > 0)		_stats.melee		+= month;
//		_stats.strength += month;
		if (_stats.psiStrength > 0)	_stats.psiStrength	+= (month * 2);
//		if (_stats.psiSkill > 0)
//			_stats.psiSkill += month;

		_stats.health += (month / 2);
	}

	//Log(LOG_INFO) << "BattleUnit::adjustStats(), unitID = " << getId();
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.tu = " << _stats.tu;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.stamina = " << _stats.stamina;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.reactions = " << _stats.reactions;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.firing = " << _stats.firing;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.strength = " << _stats.strength;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.melee = " << _stats.melee;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.psiSkill = " << _stats.psiSkill;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.psiStrength = " << _stats.psiStrength;

	// kL_end.
}

/**
 * Sets the amount of TUs reserved for cover.
 * @param reserve - reserved time units
 */
void BattleUnit::setCoverReserve(int reserve)
{
	_coverReserve = reserve;
}

/**
 * Gets the amount of TUs reserved for cover.
 * @return, reserved time units
 */
int BattleUnit::getCoverReserve() const
{
	return _coverReserve;
}

/**
 * Initializes a death spin.
 */
void BattleUnit::initDeathSpin()
{
	//Log(LOG_INFO) << "BattleUnit::deathPirouette()" << " [target]: " << (getId());
	_status = STATUS_TURNING;
	_spinPhase = 0;
	_cacheInvalid = true;
}

/**
 * Continues a death spin.
 * _spinPhases:
				-1 = no spin
				 0 = start spin
				 1 = CW spin, 1st rotation
				 2 = CCW spin, 1st rotation
				 3 = CW spin, 2nd rotation
				 4 = CCW spin, 2nd rotation
 */
void BattleUnit::contDeathSpin()
{
	//Log(LOG_INFO) << "BattleUnit::contDeathSpin()" << " [target]: " << (getId());
	int dir = _direction;
	if (dir == 3)	// when facing player, 1 rotation left;
					// unless started facing player, in which case 2 rotations left
	{
		//Log(LOG_INFO) << ". d_init = " << dir;
/*		if (_spinPhase == 3
			|| _spinPhase == 4)
		{
			//Log(LOG_INFO) << ". . _spinPhase = " << _spinPhase << " [ return ]";
			 _spinPhase = -1; // end.
			_status = STATUS_STANDING;

			 return;
		}
		else if (_spinPhase == 1)	// CW rotation
			_spinPhase = 3;			// CW rotation 2nd
		else if (_spinPhase == 2)	// CCW rotation
			_spinPhase = 4;			// CCW rotation 2nd
*/
		if (_spinPhase == 0)		// remove this clause to use only 1 rotation when start faces player.
			_spinPhase = 2;			// CCW 2 spins.
		else if (_spinPhase == 1)	// CW rotation
			_spinPhase = 3;			// CW rotation 2nd
		else if (_spinPhase == 2)	// CCW rotation
			_spinPhase = 4;			// CCW rotation 2nd
		else if (_spinPhase == 3
			|| _spinPhase == 4)
		{
			//Log(LOG_INFO) << ". . _spinPhase = " << _spinPhase << " [ return ]";
			 _spinPhase = -1; // end.
			_status = STATUS_STANDING;

			 return;
		}
	}

	if (_spinPhase == 0) // start!
	{
		if (-1 < dir && dir < 4)
		{
			if (dir == 3)
				_spinPhase = 3; // only 1 CW rotation to go ...
			else
				_spinPhase = 1; // 1st CW rotation of 2
		}
		else
		{
			if (dir == 3)
				_spinPhase = 4; // only 1 CCW rotation to go ...
			else
				_spinPhase = 2; // 1st CCW rotation of 2
		}
	}

	if (_spinPhase == 1
		|| _spinPhase == 3)
	{
		dir++;
		if (dir == 8)
			dir = 0;
	}
	else
	{
		dir--;
		if (dir == -1)
			dir = 7;
	}

	//Log(LOG_INFO) << ". d_final = " << dir;
	setDirection(dir);
	_cacheInvalid = true;
}

/**
 * Regulates init, direction & duration of the death spin-cycle.
 * @return, deathspin rotation phase
 */
int BattleUnit::getSpinPhase() const
{
	return _spinPhase;
}

/**
 * Sets the spinphase of this unit.
 * @param spinphase - the spinphase to set
 */
void BattleUnit::setSpinPhase(int spinphase)
{
	_spinPhase = spinphase;
}

/**
 * Sets this unit to STATUS_UNCONSCIOUS.
 */
void BattleUnit::knockOut()
{
	_status = STATUS_UNCONSCIOUS;
}

/**
 * Sets this unit's health level.
 * @param health - the health to set
 */
void BattleUnit::setHealth(int health)
{
	_health = health;
}

/**
 * Stops a unit from shooting/throwing if it spots a new opponent while turning.
 * @param stop - true to stop everything and refund TU (default true)
 */
void BattleUnit::setStopShot(const bool stop)
{
	_stopShot = stop;
}

/**
 * Gets if this unit spotted a new opponent while turning + shooting/thowing.
 * @return, true if a new hostile has been seen
 */
bool BattleUnit::getStopShot() const
{
	return _stopShot;
}

/**
 * Sets this unit as dashing - reduces chance of getting hit.
 * @param dash - true to dash
 */
void BattleUnit::setDashing(bool dash)
{
	_dashing = dash;
}

/**
 * Gets if this unit is dashing.
 * @return, true if dashing
 */
bool BattleUnit::getDashing() const
{
	return _dashing;
}

/**
 * Sets this unit as having been damaged in a single explosion.
 * @param beenhit - true to not deliver any more damage from a single explosion
 */
void BattleUnit::setTakenExpl(bool beenhit)
{
	_takenExpl = beenhit;
}

/**
 * Gets if this unit was aleady damaged in a single explosion.
 * @return, true if this unit has already taken damage
 */
bool BattleUnit::getTakenExpl() const
{
	return _takenExpl;
}

/**
 * Checks if this unit can be selected.
 * Only alive units belonging to the specified faction can be selected.
 * @param faction			- the faction to compare
 * @param checkReselect		- check if the unit is reselectable
 * @param checkInventory	- check if the unit has an inventory
 * @return, true if the unit can be selected, false otherwise
 */
bool BattleUnit::isSelectable(
		UnitFaction faction,
		bool checkReselect,
		bool checkInventory) const
{
	return _faction == faction
		&& isOut() == false
		&& (checkReselect == false
			|| reselectAllowed() == true)
		&& (checkInventory == false
			|| hasInventory() == true);
}

/**
 * Checks if this unit has an inventory.
 * Large units and/or terror units don't have inventories.
 * @return, true if an inventory is available
 */
bool BattleUnit::hasInventory() const
{
/*	if (_geoscapeSoldier != NULL)
		return true;

	if (_unitRules->isMechanical() == false
		&& _rank != "STR_LIVE_TERRORIST")
	{
		return true;
	}

	return false; */
	return _geoscapeSoldier != NULL
		|| (_unitRules->isMechanical() == false
			&& _rank != "STR_LIVE_TERRORIST");
}

/**
 * If this unit is breathing, what frame should be displayed?
 * @return, frame number
 */
int BattleUnit::getBreathFrame() const
{
	if (_floorAbove)
		return 0;

	return _breathFrame;
}

/**
 * Decides if we should start producing bubbles, and/or updates which bubble frame we are on.
 */
void BattleUnit::breathe()
{
	// _breathFrame of -1 means this unit doesn't produce bubbles
	if (_breathFrame < 0
		|| isOut() == true)
	{
		_breathing = false;
		return;
	}

	if (_breathing == false
		|| _status == STATUS_WALKING)
	{
		// deviation from original: TFTD used a static 10% chance for every animation frame,
		// instead let's use 5%, but allow morale to affect it.
		_breathing = (_status != STATUS_WALKING
					&& RNG::percent(105 - _morale));

		_breathFrame = 0;
	}

	if (_breathing == true)
	{
		// advance the bubble frame
		_breathFrame++;

		// we've reached the end of the cycle, get rid of the bubbles
		if (_breathFrame > 16)
		{
			_breathFrame = 0;
			_breathing = false;
		}
	}
}

/**
 * Sets the flag for "this unit is under cover" meaning don't draw bubbles.
 * @param floorAbove - true if there is a floor
 */
void BattleUnit::setFloorAbove(const bool floorAbove)
{
	_floorAbove = floorAbove;
}

/**
 * Checks if the floorAbove flag has been set.
 * @return, true if this unit is under cover
 */
bool BattleUnit::getFloorAbove() const
{
	return _floorAbove;
}

/**
 * Gets this unit's movement type.
 * Use this instead of checking the rules of the armor.
 * @return, MovementType enum
 */
MovementType BattleUnit::getMovementType() const
{
	return _movementType;
}

/**
 * Sets this unit to "time-out" status
 * meaning they will NOT take part in the current battle.
 */
/* void BattleUnit::goToTimeOut()
{
	_status = STATUS_TIME_OUT;
} */

/**
 * Helper function used by BattleUnit::setSpecialWeapon().
 * @param save -
 * @param unit -
 * @param rule -
 * @return, pointer to BattleItem
 */
/* static inline BattleItem *createItem(SavedBattleGame *save, BattleUnit *unit, RuleItem *rule)
{
	BattleItem *item = new BattleItem(rule, save->getCurrentItemId());
	item->setOwner(unit);
	save->removeItem(item); //item outside inventory, deleted when game is shutdown.
	return item;
} */

/**
 * Sets special weapon that is handled outside inventory.
 * @param save -
 * @param rule -
 */
/* void BattleUnit::setSpecialWeapon(SavedBattleGame *save, const Ruleset *rule)
{
	RuleItem *item = 0;
	int i = 0;

	if (getUnitRules())
	{
		item = rule->getItem(getUnitRules()->getMeleeWeapon());
		if (item)
		{
			_specWeapon[i++] = createItem(save, this, item);
		}
	}
	item = rule->getItem(getArmor()->getSpecialWeapon());
	if (item)
	{
		_specWeapon[i++] = createItem(save, this, item);
	}
	if (getBaseStats()->psiSkill > 0 && getFaction() == FACTION_HOSTILE)
	{
		item = rule->getItem("ALIEN_PSI_WEAPON");
		if (item)
		{
			_specWeapon[i++] = createItem(save, this, item);
		}
	}
} */

/**
 * Gets special weapon.
 * @param type -
 * @return, pointer to BattleItem
 */
/* BattleItem *BattleUnit::getSpecialWeapon(BattleType type) const
{
	for (int i = 0; i < SPEC_WEAPON_MAX; ++i)
	{
		if (_specWeapon[i] && _specWeapon[i]->getRules()->getBattleType() == type)
		{
			return _specWeapon[i];
		}
	}
	return 0;
} */

/**
 * Get this BattleUnit's battle statistics.
 * @return, pointer to BattleUnitStatistics
 */
BattleUnitStatistics* BattleUnit::getStatistics() const
{
	return _statistics;
}

/**
 * Sets the unit murderer's ID.
 * @param id - murderer ID
 */
void BattleUnit::setMurdererId(int id)
{
	_murdererId = id;
}

/**
 * Gets the unit murderer's id.
 * @return, murderer idID
 */
int BattleUnit::getMurdererId() const
{
	return _murdererId;
}

/**
 * kL. Sets the unit's order in battle.
 * @param order - position on the craft or at the base
 */
void BattleUnit::setBattleOrder(size_t order) // kL
{
	_battleOrder = order;
}

/**
 * kL. Gets the unit's order in battle.
 * @return, position on the craft or at the base
 */
size_t BattleUnit::getBattleOrder() const // kL
{
	return _battleOrder;
}

/**
 * kL. Sets the BattleGame for this BattleUnit.
 * @param battleGame - pointer to BattleGame
 */
void BattleUnit::setBattleGame(BattlescapeGame* battleGame) // kL
{
	_battleGame = battleGame;
}

/**
 * kL. Sets this unit's parameters as down (collapsed / unconscious / dead).
 */
void BattleUnit::setDown() // kL
{
	if (_geoscapeSoldier != NULL)
	{
		_turnsExposed = 255;	// don't aggro the AI
		_kneeled = false;		// don't get hunkerdown bonus against HE detonations

		// could add or remove more, I guess .....
		// but These don't seem to affect anything:
		_floating = false;
		_dashing = false;
	}
}

/**
 * Sets this BattleUnit's turn direction when spinning 180 degrees.
 * @param turnDir - true to turn clockwise
 */
void BattleUnit::setTurnDirection(const int turnDir)
{
	_turnDir = turnDir;
}

}
