/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#define _USE_MATH_DEFINES

#include "BattleUnit.h"

#include <cmath>
#include <sstream>
#include <typeinfo>

#include "BattleItem.h"
#include "SavedGame.h"
#include "Soldier.h"
#include "Tile.h"

#include "../Battlescape/BattleAIState.h"
#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/Pathfinding.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h" // kL

#include "../Engine/Surface.h"

#include "../Resource/ResourcePack.h" // kL

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"


namespace OpenXcom
{

/**
 * Initializes a BattleUnit from a Soldier
 * @param soldier Pointer to the Soldier.
 * @param faction Which faction the units belongs to.
 * @param diff For VictoryPts value per death.
 */
BattleUnit::BattleUnit(
		Soldier* soldier,
		UnitFaction faction,
		int diff, // kL_add.
		BattlescapeGame* battleGame) // kL_add
	:
		_battleGame(battleGame), // kL_add
		_battleOrder(0), // kL
		_faction(faction),
		_originalFaction(faction),
		_killedBy(faction),
		_id(0),
		_pos(Position()),
		_tile(NULL),
		_lastPos(Position()),
		_direction(0),
		_toDirection(0),
		_directionTurret(0),
		_toDirectionTurret(0),
		_verticalDirection(0),
		_status(STATUS_STANDING),
		_walkPhase(0),
		_fallPhase(0),
		_spinPhase(-1),
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
		_motionPoints(0),
		_kills(0),
//kL	_hitByFire(false),
		_moraleRestored(0),
		_coverReserve(0),
		_charging(NULL),
		_turnsExposed(255),
		_geoscapeSoldier(soldier),
		_unitRules(NULL),
		_rankInt(-1),
		_turretType(-1),
		_hidingForTurn(false),
		_statistics(),
		_murdererId(0),
		_stopShot(false), // kL
		_dashing(false), // kL
		_takenExpl(false), // kL
		_race(""), // kL

		_deathSound(0), // kL, moved these here from def'ns below.
		_aggroSound(-1),
		_moveSound(-1),
		_intelligence(2),
		_aggression(1),
		_specab(SPECAB_NONE),
		_faceDirection(-1),
		_morale(100),
		_stunLevel(0),
		_type("SOLDIER"),
		_activeHand("STR_RIGHT_HAND"),
		_breathFrame(0),
		_floorAbove(false),
		_breathing(false),
		_diedByFire(false)
{
	//Log(LOG_INFO) << "Create BattleUnit 1 : soldier ID = " << getId();
	_name			= soldier->getName(true);
	_id				= soldier->getId();
//	_type			= "SOLDIER";
	_rank			= soldier->getRankString();
	_stats			= *soldier->getCurrentStats();

	_standHeight	= soldier->getRules()->getStandHeight();
	_kneelHeight	= soldier->getRules()->getKneelHeight();
	_floatHeight	= soldier->getRules()->getFloatHeight();
//	_deathSound		= 0; // this one is hardcoded
//	_aggroSound		= -1;
//	_moveSound		= -1; // this one is hardcoded
//	_intelligence	= 2;
//	_aggression		= 1;
//	_specab			= SPECAB_NONE;
	_armor			= soldier->getArmor();
	_stats			+= *_armor->getStats();	// armors may modify effective stats
	_loftempsSet	= _armor->getLoftempsSet();
	_gender			= soldier->getGender();
//	_faceDirection	= -1;

	int rankbonus = 0;
	switch (soldier->getRank())
	{
//		case RANK_SERGEANT:		rankbonus =	1;	break;
//		case RANK_CAPTAIN:		rankbonus =	3;	break;
//		case RANK_COLONEL:		rankbonus =	6;	break;
//		case RANK_COMMANDER:	rankbonus =	10;	break;
		case RANK_SERGEANT:		rankbonus =	5;	break;
		case RANK_CAPTAIN:		rankbonus =	15;	break;
		case RANK_COLONEL:		rankbonus =	30;	break;
		case RANK_COMMANDER:	rankbonus =	50;	break;

		default:				rankbonus =	0;	break;
	}

//kL	_value		= 20 + soldier->getMissions() + rankbonus;
	_value		= 20 + (soldier->getMissions() * (diff + 1)) + (rankbonus * (diff + 1)); // kL, heheheh

	_tu			= _stats.tu;
	_energy		= _stats.stamina;
	_health		= _stats.health;
//	_morale		= 100;
//	_stunLevel	= 0;

	_currentArmor[SIDE_FRONT]	= _armor->getFrontArmor();
	_currentArmor[SIDE_LEFT]	= _armor->getSideArmor();
	_currentArmor[SIDE_RIGHT]	= _armor->getSideArmor();
	_currentArmor[SIDE_REAR]	= _armor->getRearArmor();
	_currentArmor[SIDE_UNDER]	= _armor->getUnderArmor();

	for (int i = 0; i < 6; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < 5; ++i)
		_cache[i] = 0;

//	_activeHand = "STR_RIGHT_HAND";

	lastCover = Position(-1,-1,-1);
	//Log(LOG_INFO) << "Create BattleUnit 1, DONE";

	_statistics = new BattleUnitStatistics();
}

/**
 * Creates a battleUnit from a (non-Soldier) unit object.
 * @param unit			- pointer to a unit object
 * @param faction		- faction the unit belongs to
 * @param id			- the unit's unique ID
 * @param armor			- pointer to unit's armor
 * @param difficulty	- the current game's difficulty setting (for aLien stat adjustment)
 */
BattleUnit::BattleUnit(
		Unit* unit,
		UnitFaction faction,
		int id,
		Armor* armor,
		int diff,
		int month, // kL_add.
		BattlescapeGame* battleGame) // kL_add. May be NULL
	:
		_battleGame(battleGame), // kL_add
		_battleOrder(0), // kL
		_faction(faction),
		_originalFaction(faction),
		_killedBy(faction),
		_id(id),
		_pos(Position()),
		_tile(NULL),
		_lastPos(Position()),
		_direction(0),
		_toDirection(0),
		_directionTurret(0),
		_toDirectionTurret(0),
		_verticalDirection(0),
		_status(STATUS_STANDING),
		_walkPhase(0),
		_fallPhase(0),
		_spinPhase(-1),
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
		_motionPoints(0),
		_kills(0),
//kL	_hitByFire(false),
		_moraleRestored(0),
		_coverReserve(0),
		_charging(NULL),
		_turnsExposed(255),
		_armor(armor),
		_geoscapeSoldier(NULL),
		_unitRules(unit),
		_rankInt(-1),
		_turretType(-1),
		_hidingForTurn(false),
		_statistics(),
		_murdererId(0),
		_stopShot(false), // kL
		_dashing(false), // kL
		_takenExpl(false), // kL

		_morale(100), // kL, moved these here from def'ns below.
		_stunLevel(0),
		_activeHand("STR_RIGHT_HAND"),
		_breathFrame(-1),
		_floorAbove(false),
		_breathing(false),
		_diedByFire(false)
{
	//Log(LOG_INFO) << "Create BattleUnit 2 : alien ID = " << getId();
	_type	= unit->getType();
	_rank	= unit->getRank();
	_race	= unit->getRace();

	_stats	= *unit->getStats();
	_stats	+= *_armor->getStats();	// armors may modify effective stats

//	if (_originalFaction == FACTION_HOSTILE) // kL (note: overriding initialization above.)
	if (faction == FACTION_HOSTILE)
		adjustStats(
				diff,
				month); // kL_add


	_tu			= _stats.tu;
	_energy		= _stats.stamina;
	_health		= _stats.health;
//	_morale		= 100;
//	_stunLevel	= 0;

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
	_gender			= GENDER_MALE;
	_faceDirection	= -1;

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

//	_activeHand = "STR_RIGHT_HAND";

	lastCover = Position(-1,-1,-1);
	//Log(LOG_INFO) << "Create BattleUnit 2, DONE";

	_statistics = new BattleUnitStatistics();
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

	if (!getGeoscapeSoldier())
	{
		for (std::vector<BattleUnitKills*>::const_iterator
				i = _statistics->kills.begin();
				i != _statistics->kills.end();
				++i)
		{
			delete *i;
		}
	}
	delete _statistics;

	delete _currentAIState; // delete CTD
}

/**
 * Loads the unit from a YAML file.
 * @param node YAML node.
 */
void BattleUnit::load(const YAML::Node& node)
{
	_id					= node["id"].as<int>(_id);
	_faction			= _originalFaction = (UnitFaction)node["faction"].as<int>(_faction);
	_status				= (UnitStatus)node["status"].as<int>(_status);
	_pos				= node["position"].as<Position>(_pos);
	_direction			= _toDirection = node["direction"].as<int>(_direction);
	_directionTurret	= _toDirectionTurret = node["directionTurret"].as<int>(_directionTurret);
	_tu					= node["tu"].as<int>(_tu);
	_health				= node["health"].as<int>(_health);
	_stunLevel			= node["stunLevel"].as<int>(_stunLevel);
	_energy				= node["energy"].as<int>(_energy);
	_morale				= node["morale"].as<int>(_morale);
	_floating			= node["floating"].as<bool>(_floating);
	_fire				= node["fire"].as<int>(_fire);
	_turretType			= node["turretType"].as<int>(_turretType);
	_visible			= node["visible"].as<bool>(_visible);
	_turnsExposed		= node["turnsExposed"].as<int>(_turnsExposed);
	_killedBy			= (UnitFaction)node["killedBy"].as<int>(_killedBy);
	_moraleRestored		= node["moraleRestored"].as<int>(_moraleRestored);
	_rankInt			= node["rankInt"].as<int>(_rankInt);
	_originalFaction	= (UnitFaction)node["originalFaction"].as<int>(_originalFaction);
	_kills				= node["kills"].as<int>(_kills);
	_dontReselect		= node["dontReselect"].as<bool>(_dontReselect);
	_charging			= NULL;
	_specab				= (SpecialAbility)node["specab"].as<int>(_specab);
	_spawnUnit			= node["spawnUnit"].as<std::string>(_spawnUnit);
	_motionPoints		= node["motionPoints"].as<int>(0);
	_activeHand			= node["activeHand"].as<std::string>(_activeHand); // kL
//	_dashing			= node["dashing"].as<bool>(_dashing); // kL

	for (int i = 0; i < 5; i++)
		_currentArmor[i]	= node["armor"][i].as<int>(_currentArmor[i]);
	for (int i = 0; i < 6; i++)
		_fatalWounds[i]		= node["fatalWounds"][i].as<int>(_fatalWounds[i]);

	if (_originalFaction == FACTION_PLAYER) // kL_add.
	{
		_statistics->load(node["tempUnitStatistics"]);

		_battleOrder	= node["battleOrder"].as<size_t>(_battleOrder); // kL
		_kneeled		= node["kneeled"].as<bool>(_kneeled);

		_expBravery		= node["expBravery"].as<int>(_expBravery);
		_expReactions	= node["expReactions"].as<int>(_expReactions);
		_expFiring		= node["expFiring"].as<int>(_expFiring);
		_expThrowing	= node["expThrowing"].as<int>(_expThrowing);
		_expPsiSkill	= node["expPsiSkill"].as<int>(_expPsiSkill);
		_expPsiStrength	= node["expPsiStrength"].as<int>(_expPsiStrength);
		_expMelee		= node["expMelee"].as<int>(_expMelee);
	}
}

/**
 * Saves the soldier to a YAML file.
 * @return YAML node.
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
	node["specab"]			= (int)_specab;
	node["motionPoints"]	= _motionPoints;
	// could put (if not tank) here:
	node["activeHand"]		= _activeHand; // kL
//	node["dashing"]			= _dashing; // kL

	for (int i = 0; i < 5; i++)
		node["armor"].push_back(_currentArmor[i]);
	for (int i = 0; i < 6; i++)
		node["fatalWounds"].push_back(_fatalWounds[i]);

	if (getCurrentAIState())
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
	if (!_spawnUnit.empty())
		node["spawnUnit"]		= _spawnUnit;

	if (_originalFaction == FACTION_PLAYER) // kL_add.
	{
		node["tempUnitStatistics"]	= _statistics->save();

		node["battleOrder"]			= _battleOrder; // kL
		node["kneeled"]				= _kneeled;

		node["expBravery"]			= _expBravery;
		node["expReactions"]		= _expReactions;
		node["expFiring"]			= _expFiring;
		node["expThrowing"]			= _expThrowing;
		node["expPsiSkill"]			= _expPsiSkill;
		node["expPsiStrength"]		= _expPsiStrength;
		node["expMelee"]			= _expMelee;
	}

	return node;
		// kL_note: This doesn't save/load such things as
		// _visibleUnits, _unitsSpottedThisTurn, _visibleTiles;
		// AI is saved, but loaded someplace else
}

/**
 * Gets a BattleUnit's unique ID.
 * @return, the unique ID.
 */
int BattleUnit::getId() const
{
	return _id;
}

/**
 * Changes a BattleUnit's position.
 * @param pos			- new position
 * @param updateLastPos	- true to update old position
 */
void BattleUnit::setPosition(
		const Position& pos,
		bool updateLastPos)
{
	if (updateLastPos)
		_lastPos = _pos;

	_pos = pos;
}

/**
 * Gets a BattleUnit's position.
 * @return, reference to the position
 */
const Position& BattleUnit::getPosition() const
{
	return _pos;
}

/**
 * Gets a BattleUnit's last position.
 * @return, reference to the position
 */
const Position& BattleUnit::getLastPosition() const
{
	return _lastPos;
}

/**
 * Gets a BattleUnit's destination.
 * @return, reference to the destination
 */
const Position& BattleUnit::getDestination() const
{
	return _destination;
}

/**
 * Sets a BattleUnit's horizontal direction.
 * Only used for initial unit placement.
 * kL_note: and positioning soldier when revived from unconscious status: reviveUnconsciousUnits().
 * @param dir		- new horizontal direction
 * @param turret	- true to set the turret direction also
 */
void BattleUnit::setDirection(
		int dir,
		bool turret) // kL
{
	_direction			= dir;
	_toDirection		= dir;

	// kL_begin:
	if (getTurretType() == -1
		|| turret == true)
	{
		_directionTurret = dir;
	}
	// kL_end.
}

/**
 * Gets a BattleUnit's horizontal direction.
 * @return, horizontal direction
 */
int BattleUnit::getDirection() const
{
	return _direction;
}

/**
 * Sets a BattleUnit's horizontal direction (facing).
 * Only used for strafing moves.
 * @param dir - new horizontal direction (facing)
 */
void BattleUnit::setFaceDirection(int dir)
{
	_faceDirection = dir;
}

/**
 * Gets a BattleUnit's horizontal direction (facing).
 * Used only during strafing moves.
 * @return, horizontal direction (facing)
 */
int BattleUnit::getFaceDirection() const
{
	return _faceDirection;
}

/**
 * Gets a BattleUnit's turret direction.
 * @return, turret direction
 */
int BattleUnit::getTurretDirection() const
{
	return _directionTurret;
}

/**
 * kL. Sets a BattleUnit's turret direction.
 * @param dir - turret direction
 */
void BattleUnit::setTurretDirection(int dir) // kL
{
	_directionTurret = dir;
}

/**
 * Gets a BattleUnit's turret To direction.
 * @return, toDirectionTurret
 */
int BattleUnit::getTurretToDirection() const
{
	return _toDirectionTurret;
}

/**
 * Gets a BattleUnit's vertical direction.
 * This is when going up or down, doh!
 * @return, vertical direction
 */
int BattleUnit::getVerticalDirection() const
{
	return _verticalDirection;
}

/**
 * Gets a BattleUnit's status.
 * @return, status ( See UnitStatus enum. )
 */
UnitStatus BattleUnit::getStatus() const
{
	return _status;
}

/**
 * kL. Sets a unit's status.
 * @ param status, See UnitStatus enum.
 */
void BattleUnit::setStatus(int status) // kL
{
	switch (status)
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
		case 10:	_status = STATUS_DISABLED;		return; // kL

		default:
		return;
	}
}

/**
 * Initialises variables to start walking.
 * @param direction		- the direction to walk
 * @param destination	- reference to the position we should end up at
 * @param tileBelow		- pointer to the tile below destination position
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
//kL	_cacheInvalid = cache; // there's no movement here, This not needed.

	if (direction >= Pathfinding::DIR_UP)
	{
		_status = STATUS_FLYING; // controls walking sound in UnitWalkBState, what else
		_verticalDirection = direction;

		if (_tile->getMapData(MapData::O_FLOOR)
			&& _tile->getMapData(MapData::O_FLOOR)->isGravLift())
		{
			//Log(LOG_INFO) << ". STATUS_FLYING, using GravLift";
			_floating = false;
		}
		else
			//Log(LOG_INFO) << ". STATUS_FLYING, up.down";
			_floating = true;
	}
	else if (_tile->hasNoFloor(tileBelow))
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
 * @param tileBelow	- pointer to tile currently below the unit
 * @param cache		- true to refresh the unit cache / redraw the unit's sprite
 */
void BattleUnit::keepWalking(
		Tile* tileBelow,
		bool cache)
{
	_walkPhase++;

	//Log(LOG_INFO) << "BattleUnit::keepWalking() ID = " << getId() << " _walkPhase = " << _walkPhase;
	int
		middle,
		end;

	if (_verticalDirection)
	{
		middle = 4;
		end = 8;
	}
	else // diagonal walking takes double the steps
	{
		middle	= 4 + 4 * (_direction %2);
		end		= 8 + 8 * (_direction %2);

		if (_armor->getSize() > 1)
		{
			if (_direction < 1 || 5 < _direction) // dir = 0,7,6,5 (up x3 or left)
				middle = end;
			else if (_direction == 5)
				middle = 12;
			else if (_direction == 1)
				middle = 5;
			else
				middle = 1;
		}
	}

	if (!cache) // ie. not onScreen
	{
		middle = 1; // kL: Mc'd units offscreen won't move without this (tho they turn, as if to start walking)
		end = 2;
	}

	if (_walkPhase == middle)
		// we assume we reached our destination tile
		// this is actually a drawing hack, so soldiers are not overlapped by floortiles
		// kL_note: which they (large units) are half the time anyway...
		_pos = _destination;

	if (_walkPhase >= end) // officially reached the destination tile
	{
		//Log(LOG_INFO) << ". end -> STATUS_STANDING";
		_status = STATUS_STANDING;
		_walkPhase = 0;
		_verticalDirection = 0;

		if (_floating
			&& !_tile->hasNoFloor(tileBelow))
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
 * @return phase will always go from 0-7
 */
int BattleUnit::getWalkingPhase() const
{
	return _walkPhase %8;
}

/**
 * Gets the walking phase for diagonal walking.
 * @return phase, This will be 0 or 8, due to rounding ints down
 */
int BattleUnit::getDiagonalWalkingPhase() const
{
	return (_walkPhase / 8) * 8;
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

	if (turret)
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
	if (force)
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

	if (turret)
	{
		if (_directionTurret == _toDirectionTurret)
		{
			//Log(LOG_INFO) << ". . _d = _tD, abort";
//kL			abortTurn();
			_status = STATUS_STANDING; // kL

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
//kL			abortTurn();
			_status = STATUS_STANDING; // kL

			return;
		}

		delta = _toDirection - _direction;
		//Log(LOG_INFO) << ". . delta = " << delta;
	}

	if (delta != 0) // duh
	{
		if (delta > 0)
		{
			if (delta <= 4)
			{
				if (!turret)
				{
					if (_turretType > -1)
						_directionTurret++;

					_direction++;
				}
				else
					_directionTurret++;
			}
			else
			{
				if (!turret)
				{
					if (_turretType > -1)
						_directionTurret--;

					_direction--;
				}
				else
					_directionTurret--;
			}
		}
		else
		{
			if (delta > -4)
			{
				if (!turret)
				{
					if (_turretType > -1)
						_directionTurret--;

					_direction--;
				}
				else
					_directionTurret--;
			}
			else
			{
				if (!turret)
				{
					if (_turretType > -1)
						_directionTurret++;

					_direction++;
				}
				else
					_directionTurret++;
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

		if (_visible
			|| _faction == FACTION_PLAYER) // kL_note: Faction_player should *always* be _visible...
		{
			_cacheInvalid = true;
		}
	}

	if (turret)
	{
		 if (_toDirectionTurret == _directionTurret)
			//Log(LOG_INFO) << "BattleUnit::turn() " << getId() << "Turret - STATUS_STANDING (turn has ended)";
			_status = STATUS_STANDING; // we officially reached our destination
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
 */
SoldierGender BattleUnit::getGender() const
{
	return _gender;
}

/**
 * Returns the unit's faction.
 * @return Faction. (player, hostile or neutral)
 */
UnitFaction BattleUnit::getFaction() const
{
	return _faction;
}

/**
 * Sets the unit's cache flag.
 * Set to true when the unit has to be redrawn from scratch.
 * @param cache	- pointer to cache surface to use, NULL to redraw from scratch
 * @param part	- unit part to cache
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
 * When the unit needs to animate, it needs to be re-cached.
 * @param invalid	- pointer to get if the cache is invalid (kL_note:?)
 * @param part		- unit part to check
 * @return, pointer to the cache surface used
 */
Surface* BattleUnit::getCache(
		bool* invalid,
		int part) const
{
	if (part < 0)
		part = 0;

	*invalid = _cacheInvalid;

	return _cache[part];
	// kL_note: Is somebody 'too clever' again?
}

/**
 * Kneel down/stand up.
 * @param kneeled, Whether to kneel or stand up
 */
void BattleUnit::kneel(bool kneeled)
{
	//Log(LOG_INFO) << "BattleUnit::kneel()";
	_kneeled = kneeled;
	_cacheInvalid = true;
}

/**
 * Is kneeled down?
 * @return true/false
 */
bool BattleUnit::isKneeled() const
{
	return _kneeled;
}

/**
 * Is floating? A unit is floating when there is no ground under him/her.
 * @return true/false
 */
bool BattleUnit::isFloating() const
{
	return _floating;
}

/**
 * Shows the righthand sprite with weapon aiming.
 * @param aiming - true to aim, false to stand there like an idiot
 */
void BattleUnit::aim(bool aiming)
{
	if (aiming)
		_status = STATUS_AIMING;
	else
		_status = STATUS_STANDING;

	if (_visible
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

	double offset_x = point.x - _pos.x;
	double offset_y = point.y - _pos.y;

	double theta = atan2(-offset_y, offset_x); // radians: + = y > 0; - = y < 0;

	// divide the pie in 4 thetas, each at 1/8th before each quarter
	double m_pi_8 = M_PI / 8.0;			// a circle divided into 16 sections (rads) -> 22.5 deg
	double d = 0.1;						// kL, a bias toward cardinal directions. (0.1..0.12)
	double pie[4] =
	{
		M_PI - m_pi_8 - d,					// 2.7488935718910690836548129603696	-> 157.5 deg
		(M_PI * 3.0 / 4.0) - m_pi_8 + d,	// 1.9634954084936207740391521145497	-> 112.5 deg
		M_PI_2 - m_pi_8 - d,				// 1.1780972450961724644234912687298	-> 67.5 deg
		m_pi_8 + d							// 0.39269908169872415480783042290994	-> 22.5 deg
	};

	int dir = 2;
	if (theta > pie[0] || theta < -pie[0])
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
 * @return Time units.
 */
int BattleUnit::getTimeUnits() const
{
	return _tu;
}

/**
 * Returns the soldier's amount of energy.
 * @return Energy.
 */
int BattleUnit::getEnergy() const
{
	return _energy;
}

/**
 * Returns the soldier's amount of health.
 * @return Health.
 */
int BattleUnit::getHealth() const
{
	return _health;
}

/**
 * Returns the soldier's amount of morale.
 * @return Morale.
 */
int BattleUnit::getMorale() const
{
	return _morale;
}

/**
 * Do an amount of damage.
 * @param relative		- reference to a position (voxel) that defines which part of armor and/or bodypart is hit
 * @param power			- the amount of damage to inflict
 * @param type			- the type of damage being inflicted (RuleItem.h)
 * @param ignoreArmor	- true for stun & smoke damage; no armor reduction, although vulnerability is still factored
 * @return, damage done after adjustment
 */
int BattleUnit::damage(
		const Position& relative,
		int power,
		ItemDamageType type,
		bool ignoreArmor)
{
	//Log(LOG_INFO) << "BattleUnit::damage(), ID " << getId();
	UnitSide side = SIDE_FRONT;
	UnitBodyPart bodypart = BODYPART_TORSO;

	power = static_cast<int>(floor(static_cast<double>(power) * static_cast<double>(_armor->getDamageModifier(type))));
	//Log(LOG_INFO) << "BattleUnit::damage(), type = " << (int)type << " ModifiedPower " << power;

//	if (power < 1) // kL_note: this early-out messes with got-hit sFx below_
//		return 0;

	if (type == DT_SMOKE) // smoke doesn't do real damage, but stun damage
		type = DT_STUN;

	if (relative == Position(0, 0, 0))
		side = SIDE_UNDER;
	else
	{
		int relativeDir;

		const int
			abs_x = abs(relative.x),
			abs_y = abs(relative.y);

		if (abs_y > abs_x * 2)
			relativeDir = 8 + 4 * (relative.y > 0);
		else if (abs_x > abs_y * 2)
			relativeDir = 10 + 4 * (relative.x < 0);
		else
		{
			if (relative.x < 0)
			{
				if (relative.y > 0)
					relativeDir = 13;
				else
					relativeDir = 15;
			}
			else
			{
				if (relative.y > 0)
					relativeDir = 11;
				else
					relativeDir = 9;
			}
		}

		switch ((relativeDir - _direction) %8)
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

		if (relative.z > getHeight() - 4)
			bodypart = BODYPART_HEAD;
		else if (relative.z > 5)
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
					bodypart = (UnitBodyPart)RNG::generate(
														BODYPART_RIGHTLEG,
														BODYPART_LEFTLEG);
			}
		}
	}

	if (!ignoreArmor)
	{
		int armor = getArmor(side);
		setArmor(armor - (power / 10) - 1, side); // armor damage

		power -= armor; // subtract armor-before-damage from power.
	}

	if (power > 0)
	{
		if (type == DT_STUN)
			_stunLevel += power;
		else
		{
			_health -= power; // health damage
			if (_health < 1)
			{
				_health = 0;

				if (type == DT_IN)
					_diedByFire = true;
			}
			else
			{
				if (_armor->getSize() == 1		// add some stun damage to not-large units
					&& _race != "STR_ZOMBIE")	// unless it's a freakin Zombie.
				{
					_stunLevel += RNG::generate(0, power / 3); // kL_note: was, 4
				}

				if (!ignoreArmor)	// kinda funky: only wearers of armor-types-that-are
									// -resistant-to-damage-types can take fatal wounds
				{
					if (isWoundable()) // fatal wounds
					{
						if (RNG::generate(0, 10) < power) // kL: refactor this.
						{
							int wounds = RNG::generate(1, 3);
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
		&& _status != STATUS_UNCONSCIOUS
		&& type != DT_STUN)
	{
		playHitSound(); // kL
	}

	if (power < 0)
		power = 0;
	//Log(LOG_INFO) << "BattleUnit::damage() ret Penetrating Power " << power;

	return power;
}

/**
 * kL. Plays a grunt sFx when hit/damaged.
 */
void BattleUnit::playHitSound() // kL
{
	size_t sound = 0;

	if (getType() == "MALE_CIVILIAN")
		sound = RNG::generate(41, 43);
	else if (getType() == "FEMALE_CIVILIAN")
		sound = RNG::generate(44, 46);
	else if (getOriginalFaction() == FACTION_PLAYER) // getType() == "SOLDIER")
	{
		if (getGender() == GENDER_MALE)
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

	if (sound != 0)
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
 * @param power
 */
void BattleUnit::healStun(int power)
{
	_stunLevel -= power;

	if (_stunLevel < 0)
		_stunLevel = 0;
}

/**
 * Gets the amount of stun damage this unit has.
 */
int BattleUnit::getStun() const
{
	return _stunLevel;
}

/**
 * Raises a unit's stun level sufficiently so that the unit is ready to become unconscious.
 * Used when another unit falls on top of this unit.
 * Zombified units first convert to their spawn unit.
 * @param battle, Pointer to the battlescape game.
 */
void BattleUnit::knockOut(BattlescapeGame* battle)
{
	if (getArmor()->getSize() > 1) // large units die
		_health = 0;
	else if (_spawnUnit != "")
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
	_fallPhase++;

	if (_fallPhase == _armor->getDeathFrames())
	{
		_fallPhase--;

		if (_health == 0)
			//Log(LOG_INFO) << "BattleUnit::keepFalling() " << getId() << ". . STATUS_DEAD";
			_status = STATUS_DEAD;
		else
			//Log(LOG_INFO) << "BattleUnit::keepFalling() " << getId() << ". . STATUS_UNCONSCIOUS";
			_status = STATUS_UNCONSCIOUS;
	}

	_cacheInvalid = true;
}

/**
 * Returns the phase of the falling sequence.
 * @return phase
 */
int BattleUnit::getFallingPhase() const
{
	return _fallPhase;
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
	if (checkHealth
		&& getHealth() == 0)
	{
			return true;
	}
	else if (checkStun
		&& getStun() >= getHealth())
	{
			return true;
	}
	else if (_status == STATUS_DEAD
		|| _status == STATUS_UNCONSCIOUS)
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
 * Gets the number of time units a certain action takes.
 * @param actionType	- type of battle action (BattlescapeGame.h)
 * @param item			- pointer to an item for TU-cost
 * @return, TUs to perform action
 */
int BattleUnit::getActionTUs(
		BattleActionType actionType,
		BattleItem* item)
{
	if (item == NULL)
		return 0;

	return getActionTUs(
					actionType,
					item->getRules());
}

/**
 * Gets the number of time units a certain action takes.
 * @param actionType	- type of battle action (BattlescapeGame.h)
 * @param item			- pointer to an item for TU-cost
 * @return, TUs to perform action
 */
int BattleUnit::getActionTUs(
		BattleActionType actionType,
		RuleItem* rule)
{
	if (rule == NULL)
		return 0;

	int cost = 0;

	switch (actionType)
	{
		case BA_PRIME: // kL_note: Should put "tuPrime" yaml-entry in Xcom1Ruleset, under various grenade-types.
//kL		cost = 50; // maybe this should go in the ruleset
			cost = 45; // kL
		break;
		case BA_DEFUSE: // kL_add.
			cost = 15; // do this flatrate!
		break;
		case BA_THROW:
//kL		cost = 25; // maybe this should go in the ruleset
			cost = 23; // kL
		break;
		case BA_LAUNCH: // kL
			cost = rule->getTULaunch();
		break;
		case BA_AIMEDSHOT:
			cost = rule->getTUAimed();
		break;
		case BA_AUTOSHOT:
			cost = rule->getTUAuto();
		break;
		case BA_SNAPSHOT:
			cost = rule->getTUSnap();
		break;
		case BA_STUN:
		case BA_HIT:
			cost = rule->getTUMelee();
		break;
		case BA_USE:
		case BA_MINDCONTROL:
		case BA_PANIC:
			cost = rule->getTUUse();
		break;

		default:
			cost = 0;
		break;
	}

	// if it's a percentage, apply it to unit TUs
	if ((rule->getFlatRate() == false
			|| actionType == BA_PRIME
			|| actionType == BA_THROW)
		&& actionType != BA_DEFUSE) // kL
	{
		cost = static_cast<int>(floor(static_cast<double>(getStats()->tu * cost) / 100.0));
	}

	return cost;
}

/**
 * Spends time units if it can. Return false if it can't.
 * @param tu
 * @return flag if it could spend the time units or not.
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
 * @param energy
 * @return flag if it could spend the time units or not.
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
 * @param tu
 */
void BattleUnit::setTimeUnits(int tu)
{
	if (tu < 0)
		_tu = 0;
	else
		_tu = tu;

	if (_tu > getStats()->tu) // this might screw up ArmorBonus +tu
		_tu = getStats()->tu;
}

/**
 * Sets a specific number of energy.
 * @param energy
 */
void BattleUnit::setEnergy(int energy)
{
	if (energy < 0)
		_energy = 0;
	else
		_energy = energy;

	if (_energy > getStats()->stamina) // this might screw up ArmorBonus +stamina
		_energy = getStats()->stamina;
}

/**
 * Sets whether this unit is visible.
 * @param flag
 */
void BattleUnit::setVisible(bool flag)
{
	_visible = flag;
}

/**
 * Gets whether this unit is visible.
 * @return flag
 */
bool BattleUnit::getVisible() const
{
	if (getFaction() == FACTION_PLAYER)
		return true;
	else
		return _visible;
}

/**
 * Adds a unit to a vector of spotted and/or visible units (they're different).
 * xCom soldiers are always considered 'visible'; only aLiens go vis/unVis.
 * @param unit - pointer to a seen unit
 * @return, true if the seen unit was NOT previously flagged as visible
 */
bool BattleUnit::addToVisibleUnits(BattleUnit* unit)
{
	bool addUnit = true;

	for (std::vector<BattleUnit*>::iterator
			i = _unitsSpottedThisTurn.begin();
			i != _unitsSpottedThisTurn.end();
			++i)
	{
		if (dynamic_cast<BattleUnit*>(*i) == unit)
		{
			addUnit = false;

			break;
		}
	}

	if (addUnit)
		_unitsSpottedThisTurn.push_back(unit);


	for (std::vector<BattleUnit*>::iterator
			i = _visibleUnits.begin();
			i != _visibleUnits.end();
			++i)
	{
		if (dynamic_cast<BattleUnit*>(*i) == unit)
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
	for (std::vector<Tile*>::iterator
			j = _visibleTiles.begin();
			j != _visibleTiles.end();
			++j)
	{
		(*j)->setVisible(false);
	}

	_visibleTiles.clear();
}

/**
 * Calculates firing accuracy.
 * Formula = accuracyStat * weaponAccuracy * kneelingbonus(1.15) * one-handPenalty(0.8) * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param actionType
 * @param item
 * @return firing Accuracy
 */
double BattleUnit::getFiringAccuracy(
		BattleActionType actionType,
		BattleItem* item)
{
	//Log(LOG_INFO) << "BattleUnit::getFiringAccuracy() ID " << getId();
	if (actionType == BA_LAUNCH)
		return 1.0;

	double ret;

	if (actionType == BA_HIT
		|| actionType == BA_STUN)
	{
		ret = static_cast<double>(item->getRules()->getAccuracyMelee()) * getAccuracyModifier(item) / 100.0;

		if (item->getRules()->isSkillApplied())
			ret = ret * static_cast<double>(getStats()->melee) / 100.0;
	}
	else
	{
		int acu = item->getRules()->getAccuracySnap();

		if (actionType == BA_AIMEDSHOT)
			acu = item->getRules()->getAccuracyAimed();
		else if (actionType == BA_AUTOSHOT)
			acu = item->getRules()->getAccuracyAuto();

		ret = static_cast<double>(acu * getStats()->firing) / 10000.0;

		if (_kneeled)
			ret *= 1.16;

		if (item->getRules()->isTwoHanded()
			&& getItem("STR_RIGHT_HAND") != NULL
			&& getItem("STR_LEFT_HAND") != NULL)
		{
			ret *= 0.79;
		}

		ret *= getAccuracyModifier();
	}

	//Log(LOG_INFO) << ". ret = " << ret;
	return ret;
}

/**
 * Calculates throwing accuracy.
 * @return throwing Accuracy
 */
double BattleUnit::getThrowingAccuracy()
{
	return static_cast<double>(getStats()->throwing) * getAccuracyModifier() / 100.0;
}

/**
 * Calculates accuracy modifier. Takes health and fatal wounds into account.
 * Formula = accuracyStat * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param item the item we are shooting right now.
 * @return modifier
 */
double BattleUnit::getAccuracyModifier(BattleItem* item)
{
	//Log(LOG_INFO) << "BattleUnit::getAccuracyModifier()";
	double ret = static_cast<double>(_health) / static_cast<double>(getStats()->health);

	int wounds = _fatalWounds[BODYPART_HEAD];

	if (item)
	{
		if (item->getRules()->isTwoHanded())
			wounds += _fatalWounds[BODYPART_RIGHTARM] + _fatalWounds[BODYPART_LEFTARM];
		else
		{
			if (getItem("STR_RIGHT_HAND") == item)
				wounds += _fatalWounds[BODYPART_RIGHTARM];
			else
				wounds += _fatalWounds[BODYPART_LEFTARM];
		}
	}

	ret *= 1.0 - 0.1 * static_cast<double>(wounds);
	//Log(LOG_INFO) << ". ret = " << ret;

	if (ret < 0.1) // limit low @ 10%
		ret = 0.1;

	return ret;
}

/**
 * Sets the armor value of a certain armor side.
 * @param armor Amount of armor.
 * @param side The side of the armor.
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
 * Get the armor value of a certain armor side.
 * @param side The side of the armor.
 * @return Amount of armor.
 */
int BattleUnit::getArmor(UnitSide side) const
{
	return _currentArmor[side];
}

/**
 * Gets total amount of fatal wounds this unit has.
 * @return Number of fatal wounds.
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
 * @return, Reaction score; aka INITIATIVE
 */
//kL double BattleUnit::getInitiative()
double BattleUnit::getInitiative(int tuSpent) // kL
{
	// (Reactions Stat) x (Current Time Units / Max TUs)
	double ret = static_cast<double>(
						getStats()->reactions * (getTimeUnits() - tuSpent))
						/ static_cast<double>(getStats()->tu);
	ret *= getAccuracyModifier();

	return ret;
}

/**
 * Prepares unit for a new turn.
 */
void BattleUnit::prepareNewTurn()
{
	//Log(LOG_INFO) << "BattleUnit::prepareNewTurn() ID " << getId();
	_faction = _originalFaction;
	//Log(LOG_INFO) << ". _stopShot is " << _stopShot << " setFALSE";
	//_stopShot = false;

	_unitsSpottedThisTurn.clear();


	int prepTU = getStats()->tu;

	double underLoad = static_cast<double>(getStats()->strength) / static_cast<double>(getCarriedWeight());
	underLoad *= getAccuracyModifier() / 2.0 + 0.5;
	if (underLoad < 1.0)
		prepTU = static_cast<int>(static_cast<double>(prepTU) * underLoad);

	// Each fatal wound to the left or right leg reduces the soldier's TUs by 10%.
	if (_faction == FACTION_PLAYER)
		prepTU -= (prepTU * (_fatalWounds[BODYPART_LEFTLEG] + _fatalWounds[BODYPART_RIGHTLEG] * 10)) / 100;

	if (prepTU < 12)
		prepTU = 12;

	setTimeUnits(prepTU);

	if (!isOut()) // recover energy
	{
		// kL_begin: advanced Energy recovery
		int
			stamina = getStats()->stamina,
			enron = stamina;

		if (_turretType == -1) // is NOT xCom Tank (which get 4/5ths energy-recovery below_).
		{
			if (_faction == FACTION_PLAYER)
			{
				if (isKneeled())
					enron /= 2;
				else
					enron /= 3;
			}
			else // aLiens.
				enron = enron * _unitRules->getEnergyRecovery() / 100;
		}
		else // xCom tank.
			enron = enron * 4 / 5; // value in Ruleset is 100%

		enron = static_cast<int>(static_cast<double>(enron) * getAccuracyModifier());
		// kL_end.

		// Each fatal wound to the body reduces the soldier's energy recovery by 10%.
		// kL_note: Only xCom gets fatal wounds, atm.
		if (_faction == FACTION_PLAYER)
			enron -= (_energy * (_fatalWounds[BODYPART_TORSO] * 10)) / 100;

		_energy += enron;

		if (_energy > stamina)
			_energy = stamina;
	}

	if (_energy < 12)
		_energy = 12;

	_health -= getFatalWounds(); // suffer from fatal wounds

	// Fire damage is also in Battlescape/BattlescapeGame::endTurn(), stand on fire tile
	// see also, Savegame/Tile::prepareNewTurn(), catch fire on fire tile
	// fire damage by hit is caused by TileEngine::explode()
/*kL	if (//kL !_hitByFire &&
		_fire > 0) // suffer from fire
	{
		int fireDam = static_cast<int>(_armor->getDamageModifier(DT_IN) * RNG::generate(2, 6));
		//Log(LOG_INFO) << ". fireDam = " << fireDam;
		_health -= fireDam;

		_fire--;
	} */

	if (_health < 0)
		_health = 0;

	if (_health == 0 // if unit is dead, AI state should be gone
		&& _currentAIState != NULL)
	{
		_currentAIState->exit();

		delete _currentAIState;

		_currentAIState = NULL;
	}

	if (_stunLevel > 0)
		healStun(1); // recover stun 1pt/turn

	if (!isOut())
	{
		int panic = 100 - (2 * getMorale());
		if (RNG::percent(panic))
		{
			_status = STATUS_PANICKING;		// panic is either flee or freeze (determined later)

			if (RNG::percent(30))
				_status = STATUS_BERSERK;	// or shoot stuff.
		}
		else if (panic > 0)			// successfully avoided Panic
			_expBravery++;
	}

//kL	_hitByFire = false;
	_dontReselect = false;
	_motionPoints = 0;

	//Log(LOG_INFO) << "BattleUnit::prepareNewTurn() EXIT";
}

/**
 * Changes morale with bounds check.
 * @param change - can be positive or negative
 */
void BattleUnit::moraleChange(int change)
{
	if (!isFearable()
		&& change < 0) // kL
	{
		return;
	}

	//Log(LOG_INFO) << "BattleUnit::moraleChange() unitID = " << getId() << " delta = " << change ;
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
 * @return bool
 */
bool BattleUnit::reselectAllowed() const
{
	return !_dontReselect;
}

/**
 * Sets the amount of turns this unit is on fire. 0 = no fire.
 * @param fire : amount of turns this tile is on fire.
 */
void BattleUnit::setFire(int fire)
{
	if (_specab != SPECAB_BURNFLOOR)
		_fire = fire;
}

/**
 * Gets the amount of turns this unit is on fire. 0 = no fire.
 * @return fire : amount of turns this tile is on fire.
 */
int BattleUnit::getFire() const
{
	return _fire;
}

/**
 * Gets the pointer to the vector of inventory items.
 * @return pointer to vector.
 */
std::vector<BattleItem*>* BattleUnit::getInventory()
{
	return &_inventory;
}

/**
 * Lets AI do its thing.
 * @param action AI action.
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
 * Changes the current AI state.
 * @param aiState Pointer to AI state.
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
 * @return Pointer to AI state.
 */
BattleAIState* BattleUnit::getCurrentAIState() const
{
	return _currentAIState;
}

/**
 * Sets the tile that a unit is standing on.
 * @param tile		- pointer to a tile
 * @param tileBelow	- pointer to the tile below
 */
void BattleUnit::setTile(
		Tile* tile,
		Tile* tileBelow)
{
	//Log(LOG_INFO) << "BattleUnit::setTile()";
	_tile = tile;
	if (!_tile)
	{
		_floating = false;

		return;
	}

	// unit could have changed from flying to walking or vice versa
	if (_status == STATUS_WALKING
		&& _tile->hasNoFloor(tileBelow)
		&& _armor->getMovementType() == MT_FLY)
	{
		_status = STATUS_FLYING;
		_floating = true;
		//Log(LOG_INFO) << ". STATUS_WALKING, _floating = " << _floating;
	}
	else if (_status == STATUS_FLYING
		&& !_tile->hasNoFloor(tileBelow)
		&& _verticalDirection == 0)
	{
		_status = STATUS_WALKING;
		_floating = false;
		//Log(LOG_INFO) << ". STATUS_FLYING, _floating = " << _floating;
	}
/*	else if (_status == STATUS_STANDING			// kL. keeping this section in tho it was taken out
		&& _armor->getMovementType() == MT_FLY)	// when STATUS_UNCONSCIOUS below was inserted.
												// Problem: when loading a save, _floating goes TRUE!
	{
		_floating = _tile->hasNoFloor(tileBelow);
		//Log(LOG_INFO) << ". STATUS_STANDING, _floating = " << _floating;
	} */
	else if (_status == STATUS_UNCONSCIOUS) // <- kL_note: not so sure having flying unconscious soldiers is a good deal.
	{
		_floating = _armor->getMovementType() == MT_FLY
					&& _tile->hasNoFloor(tileBelow);
		//Log(LOG_INFO) << ". STATUS_UNCONSCIOUS, _floating = " << _floating;
	}
	//Log(LOG_INFO) << "BattleUnit::setTile() EXIT";
}

/**
 * Gets the unit's tile.
 * @return Tile
 */
Tile* BattleUnit::getTile() const
{
	return _tile;
}

/**
 * Checks if there's an inventory item in the specified inventory position.
 * @param slot, Inventory slot.
 * @param x, X position in slot.
 * @param y, Y position in slot.
 * @return BattleItem, Pointer to item in the slot, or NULL if none.
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
 * @param slot, Inventory slot.
 * @param x, X position in slot.
 * @param y, Y position in slot.
 * @return BattleItem, Pointer to item in the slot, or NULL if none.
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
			if ((*i)->getSlot() != 0
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
			if ((*i)->getSlot() != 0
				&& (*i)->occupiesSlot(x, y))
			{
				return *i;
			}
		}
	}

	return NULL;
}

/**
 * Get the 'main hand weapon' of a BattleUnit.
 * @param quickest - true to choose the quickest weapon (default true)
 * @return, pointer to BattleItem
 */
BattleItem* BattleUnit::getMainHandWeapon(bool quickest) const
{
	// kL_note: This gets called way too much, from somewhere, when just walking around.
	// probably AI patrol state

	//Log(LOG_INFO) << "BattleUnit::getMainHandWeapon()";
	BattleItem* rhtWeapon = getItem("STR_RIGHT_HAND");
	BattleItem* lftWeapon = getItem("STR_LEFT_HAND");

	bool isRht = rhtWeapon
				&& ((rhtWeapon->getAmmoItem()
						&& rhtWeapon->getAmmoItem()->getAmmoQuantity()
						&& rhtWeapon->getRules()->getBattleType() == BT_FIREARM)
					|| rhtWeapon->getRules()->getBattleType() == BT_MELEE);
	bool isLft = lftWeapon
				&& ((lftWeapon->getAmmoItem()
						&& lftWeapon->getAmmoItem()->getAmmoQuantity()
						&& lftWeapon->getRules()->getBattleType() == BT_FIREARM)
					|| lftWeapon->getRules()->getBattleType() == BT_MELEE);
	//Log(LOG_INFO) << ". isRht = " << isRht;
	//Log(LOG_INFO) << ". isLft = " << isLft;

	if (!isRht && !isLft)
		return NULL;
	else if (isRht && !isLft)
		return rhtWeapon;
	else if (!isRht && isLft)
		return lftWeapon;
	else //if (isRht && isLft).
	{
		//Log(LOG_INFO) << ". . isRht & isLft VALID";

		RuleItem* rhtRule = rhtWeapon->getRules();
		int rhtTU = rhtRule->getTUSnap();
		if (!rhtTU) //rhtRule->getBattleType() == BT_MELEE
			rhtTU = rhtRule->getTUMelee();

		RuleItem* lftRule = lftWeapon->getRules();
		int lftTU = lftRule->getTUSnap();
		if (!lftTU) //lftRule->getBattleType() == BT_MELEE
			lftTU = lftRule->getTUMelee();

		//Log(LOG_INFO) << ". . rhtTU = " << rhtTU;
		//Log(LOG_INFO) << ". . lftTU = " << lftTU;

		if (!rhtTU && !lftTU)
			return NULL;
		else if (rhtTU && !lftTU)
			return rhtWeapon;
		else if (!rhtTU && lftTU)
			return lftWeapon;
		else //if (rhtTU && lftTU)
		{
			if (quickest)
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
		}

/*		int rhtTU = rhtRule->getBattleType() == BT_MELEE?
									getActionTUs(BA_HIT, rhtWeapon)
								: getActionTUs(BA_SNAPSHOT, rhtWeapon);

		int lftTU = lftRule->getBattleType() == BT_MELEE?
									getActionTUs(BA_HIT, lftWeapon)
								: getActionTUs(BA_SNAPSHOT, lftWeapon); */
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
	if (tuLeftHand >= tuRightHand)
		return quickest?weaponRightHand:weaponLeftHand;
	else
		return quickest?weaponLeftHand:weaponRightHand; */

/**
 * Get a grenade from the belt (used for AI)
 * @return Pointer to item.
 */
BattleItem* BattleUnit::getGrenadeFromBelt() const
{
	// kL_begin: BattleUnit::getGrenadeFromBelt(), or hand.
	BattleItem* handgrenade = getItem("STR_RIGHT_HAND");
	if (!handgrenade
		|| handgrenade->getRules()->getBattleType() != BT_GRENADE)
	{
		handgrenade = getItem("STR_LEFT_HAND");
	}

	if (handgrenade
		&& handgrenade->getRules()->getBattleType() == BT_GRENADE)
	{
		return handgrenade;
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
 * Check if we have ammo and reload if needed (used for the AI).
 * @return, true if unit has a loaded weapon or has just loaded it.
 */
bool BattleUnit::checkAmmo()
{
	if (getTimeUnits() < 15)
		return false;

	BattleItem* weapon = getItem("STR_RIGHT_HAND");
	if (!weapon
		|| weapon->getAmmoItem() != NULL
		|| weapon->getRules()->getBattleType() == BT_MELEE)
	{
		weapon = getItem("STR_LEFT_HAND");
		if (!weapon
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

	for (std::vector<BattleItem*>::iterator
			i = getInventory()->begin();
			i != getInventory()->end();
			++i)
	{
		ammo = *i;
		for (std::vector<std::string>::iterator
				c = weapon->getRules()->getCompatibleAmmo()->begin();
				c != weapon->getRules()->getCompatibleAmmo()->end();
				++c)
		{
			if (*c == ammo->getRules()->getType())
			{
				wrongAmmo = false;

				break;
			}
		}

		if (!wrongAmmo)
			break;
	}

	if (wrongAmmo)
		return false; // didn't find any compatible ammo in inventory

	spendTimeUnits(15);
	weapon->setAmmoItem(ammo);
	ammo->moveToOwner(NULL);

	//Log(LOG_INFO) << "BattleUnit::checkAmmo() EXIT";
	return true;
}

/**
 * Check if this unit is in the exit area.
 * @param stt - type of exit tile to check for
 * @return, true if unit is in a special exit area
 */
bool BattleUnit::isInExitArea(SpecialTileType stt) const
{
	return _tile
			&& _tile->getMapData(MapData::O_FLOOR)
			&& _tile->getMapData(MapData::O_FLOOR)->getSpecialType() == stt;
}

/**
 * Gets the unit height taking into account kneeling/standing.
 * @return Unit's height.
 */
int BattleUnit::getHeight() const
{
	if (isKneeled())
		return getKneelHeight();

	return getStandHeight();
}

/**
 * Adds one to the reaction exp counter.
 */
void BattleUnit::addReactionExp()
{
	_expReactions++;
}

/**
 * Adds one to the firing exp counter.
 */
void BattleUnit::addFiringExp()
{
	_expFiring++;
}

/**
 * Adds one to the throwing exp counter.
 */
void BattleUnit::addThrowingExp()
{
	_expThrowing++;
}

/**
 * Adds qty to the psiSkill exp counter.
 * @param qty - amount to add
 */
void BattleUnit::addPsiSkillExp(int qty) // kL
{
//kL	_expPsiSkill++;
	_expPsiSkill += qty; // kL
}

/**
 * Adds qty to the psiStrength exp counter.
 * @param qty - amount to add
 */
void BattleUnit::addPsiStrengthExp(int qty) // kL
{
//kL	_expPsiStrength++;
	_expPsiStrength += qty; // kL
}

/**
 * Adds one to the melee exp counter.
 */
void BattleUnit::addMeleeExp()
{
	_expMelee++;
}

/**
 *
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
	Soldier* soldier = geoscape->getSoldier(_id);
	if (soldier == NULL)
		return false;


	updateGeoscapeStats(soldier); // missions & kills

	UnitStats* stats = soldier->getCurrentStats();
	const UnitStats caps = soldier->getRules()->getStatCaps();

	int healthLoss = stats->health - _health;
	soldier->setWoundRecovery(RNG::generate(
							static_cast<int>((static_cast<double>(healthLoss) * 0.5)),
							static_cast<int>((static_cast<double>(healthLoss) * 1.5))));

	if (_expBravery
		&& stats->bravery < caps.bravery)
	{
//kL	if (_expBravery > RNG::generate(0, 10))
		if (_expBravery > RNG::generate(0, 9)) // kL
			stats->bravery += 10;
	}

	if (_expFiring
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

	if (_expReactions
		&& stats->reactions < caps.reactions)
	{
		stats->reactions += improveStat(_expReactions);
	}

	if (_expMelee
		&& stats->melee < caps.melee)
	{
		stats->melee += improveStat(_expMelee);
	}

	if (_expPsiSkill
		&& stats->psiSkill < caps.psiSkill)
	{
		stats->psiSkill += improveStat(_expPsiSkill);
	}

	if (_expPsiStrength
		&& stats->psiStrength < caps.psiStrength)
	{
		stats->psiStrength += improveStat(_expPsiStrength);
	}

	if (_expThrowing
		&& stats->throwing < caps.throwing)
	{
		stats->throwing += improveStat(_expThrowing);
	}


	bool expPri = _expBravery
				|| _expReactions
				|| _expFiring
				|| _expMelee;

	if (expPri
		|| _expPsiSkill
		|| _expPsiStrength)
	{
		if (soldier->getRank() == RANK_ROOKIE)
			soldier->promoteRank();

		if (expPri)
		{
			// kL_note: The delta-bits seem odd... thought it should be only 1d6 or so.
			int delta = caps.tu - stats->tu;
			if (delta > 0)
				stats->tu += RNG::generate(
										0,
										(delta / 10) + 2)
									- 1;

			delta = caps.health - stats->health;
			if (delta > 0)
				stats->health += RNG::generate(
										0,
										(delta / 10) + 2)
									- 1;

			delta = caps.strength - stats->strength;
			if (delta > 0)
				stats->strength += RNG::generate(
										0,
										(delta / 10) + 2)
									- 1;

			delta = caps.stamina - stats->stamina;
			if (delta > 0)
				stats->stamina += RNG::generate(
										0,
										(delta / 10) + 2)
									- 1;
		}

		return true;
	}

	return false;
}

/**
 * Converts an amount of experience to a stat increase.
 * @param (int)exp, Experience counter.
 * @return, Stat increase.
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
 * @return the unit minimap index
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
	if (isOut(true, true))
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
		break;
	}
}

/**
 * Set the turret type. -1 is no turret.
 * @param turretType
 */
void BattleUnit::setTurretType(int turretType)
{
	_turretType = turretType;
}

/**
 * Get the turret type. -1 is no turret.
 * @return type
 */
int BattleUnit::getTurretType() const
{
	return _turretType;
}

/**
 * Get the amount of fatal wound for a body part.
 * @param part, The body part (in the range 0-5)
 * @return, The amount of fatal wound of a body part
 */
int BattleUnit::getFatalWound(int part) const
{
	if (part < 0 || part > 5)
		return 0;

	return _fatalWounds[part];
}

/**
 * Heal a fatal wound of the soldier.
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

	if (!_fatalWounds[part])
		return;

	_fatalWounds[part] -= wounds;

	_health += health;
	if (_health > getStats()->health)
		_health = getStats()->health;

	moraleChange(health);
}

/**
 * Restore soldier morale.
 */
void BattleUnit::painKillers()
{
	int lostHealth = getStats()->health - _health;
	if (lostHealth > _moraleRestored)
	{
		_morale = std::min(
						100,
						lostHealth - _moraleRestored + _morale);
		_moraleRestored = lostHealth;
	}
}

/**
 * Restore soldier energy and reduce stun level.
 * @param energy, The amount of energy to add
 * @param stun, The amount of stun level to reduce
 */
void BattleUnit::stimulant(
		int energy,
		int stun)
{
	_energy += energy;
	if (_energy > getStats()->stamina)
		_energy = getStats()->stamina;

	healStun(stun);
}

/**
 * Gets motion points for the motion scanner.
 * More points is a larger blip on the scanner.
 * @return points.
 */
int BattleUnit::getMotionPoints() const
{
	return _motionPoints;
}

/**
 * Gets the unit's armor.
 * @return, Pointer to armor.
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
 * @param lang			- pointer to language
 * @param debugAppendId	- append unit ID to name for debug purposes
 * @return, name of the unit
 */
std::wstring BattleUnit::getName(
		Language* lang,
		bool debugAppendId) const
{
	if (_type != "SOLDIER"
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
 * Gets a pointer to the unit's stats.
 * @return stats Pointer to the unit's stats.
 */
UnitStats* BattleUnit::getStats()
{
	//Log(LOG_INFO) << "UnitStats* BattleUnit::getStats(), unitID = " << getId();
//	adjustStats(3);		// kL, should be gameDifficulty in there <-

	return &_stats;
}

/**
 * Gets a unit's stand height.
 * @return The unit's height in voxels, when standing up.
 */
int BattleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets a unit's kneel height.
 * @return The unit's height in voxels, when kneeling.
 */
int BattleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets a unit's floating elevation.
 * @return The unit's elevation over the ground in voxels, when flying.
 */
int BattleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets this unit's LOFT id, one per unit tile.
 * This is one slice only, as it is repeated over the entire height of the unit;
 * that is, each tile has only one LOFT.
 * @return, The unit's Line of Fire Template id.
 */
int BattleUnit::getLoftemps(int entry) const
{
	return _loftempsSet.at(entry);
}

/**
 * Gets the unit's value. Used for score at debriefing.
 * @return value score
 */
int BattleUnit::getValue() const
{
	return _value;
}

/**
 * Gets the unit's death sound.
 * @return id.
 */
int BattleUnit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Gets the unit's move sound.
 * @return id.
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
	return _type == "SOLDIER"
			|| (Options::alienBleeding
				&& _faction != FACTION_PLAYER
				&& _armor->getSize() == 1);
}

/**
 * Gets whether the unit is affected by morale loss.
 * Normally only small units are affected by morale loss.
 * @return, true if unit can be affected by morale changes
 */
bool BattleUnit::isFearable() const
{
	return (_armor->getSize() == 1);
}

/**
 * Get the number of turns an AI unit remembers a soldier's position.
 * @return, intelligence ( # of turns )
 */
int BattleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Get the unit's aggression rating for use by the AI.
 * @return, aggression
 */
int BattleUnit::getAggression() const
{
	return _aggression;
}

/**
 * Gets a unit's special ability. ( See SPECAB_* enum )
 */
int BattleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Sets a unit's special ability. ( See SPECAB_* enum )
 * @param specab - special ability
 */
void BattleUnit::setSpecialAbility(SpecialAbility specab)
{
	_specab = specab;
}

/**
 * Gets a unit that is spawned when this one dies.
 * @return, special spawn unit ( ie. ZOMBIES!!! )
 */
std::string BattleUnit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Sets a unit that is spawned when this one dies.
 * @param spawnUnit - special unit
 */
void BattleUnit::setSpawnUnit(std::string spawnUnit)
{
	_spawnUnit = spawnUnit;
}

/**
 * Gets the unit's rank string.
 * @return, rank
 */
std::string BattleUnit::getRankString() const
{
	return _rank;
}

/**
 * kL. Gets the unit's race string.
 */
std::string BattleUnit::getRaceString() const // kL
{
	return _race;
}

/**
 * Gets the geoscape-soldier object.
 * @return, soldier
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
	_kills++;
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
 * @param hand - pointer to a hand
 */
void BattleUnit::setActiveHand(const std::string& hand)
{
	if (_activeHand != hand)
		_cacheInvalid = true;

	_activeHand = hand;
}

/**
 * Gets unit's active hand. Must have an item in that hand.
 * Else, switch to other hand or use righthand as default.
 * @return, the active hand
 */
std::string BattleUnit::getActiveHand() const
{
	// NOTE: what about Tanks? Does this work in canMakeSnap() for reaction fire???

	if (getItem(_activeHand)) // has an item in the already active Hand.
	{
		return _activeHand;
	}
	// kL_begin: BattleUnit::getActiveHand(), tinker tinker.
	else if (getItem("STR_RIGHT_HAND"))
	{
		return "STR_RIGHT_HAND";
	}
	else if (getItem("STR_LEFT_HAND"))
	{
		return "STR_LEFT_HAND";
	}

//	return "STR_RIGHT_HAND";
	return "";
	// kL_end.

/*kL	if (getItem("STR_LEFT_HAND"))
		return "STR_LEFT_HAND";

	return "STR_RIGHT_HAND"; */
}

/**
 * Converts unit to another faction (original faction is still stored).
 * @param f - faction
 */
void BattleUnit::convertToFaction(UnitFaction f)
{
	_faction = f;
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
 * Halves this unit's armor values (for beginner mode)
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
 * @return faction
 */
UnitFaction BattleUnit::killedBy() const
{
	return _killedBy;
}

/**
 * Sets the faction the unit was killed by.
 * @param f faction
 */
void BattleUnit::killedBy(UnitFaction f)
{
	_killedBy = f;
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
 * Gets the units we are charging towards.
 * @return, pointer to the charged target
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
int BattleUnit::getCarriedWeight(BattleItem* dragItem) const
{
	int weight = _armor->getWeight();
	//Log(LOG_INFO) << "wt armor = " << weight;

	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i) == dragItem)
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
 * @param turns, # turns unit has been exposed
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
 * @return, # turns unit has been exposed
 */
int BattleUnit::getTurnsExposed() const
{
	return _turnsExposed;
}

/**
 * Gets this unit's original Faction.
 * @return, original faction
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
 * @return, reference to a vector of pointers to units
 */
std::vector<BattleUnit*>& BattleUnit::getUnitsSpottedThisTurn()
{
	return _unitsSpottedThisTurn;
}

/**
 * Sets the numeric version of a unit's rank.
 * @param rank - unit rank ( 0 = lowest ) kL_note: uh doesn't aLien & xCom go opposite ways
 */
void BattleUnit::setRankInt(int rank)
{
	_rankInt = rank;
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
 * @param pos - the position to check against
 * @return, whatever the maths decide
 */
bool BattleUnit::checkViewSector(Position pos) const
{
	int dx = pos.x - _pos.x;
	int dy = _pos.y - pos.y;

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
		const int month) // kL_add.
{
	// adjust the unit's stats according to the difficulty level. Note, Throwing is not affected.
	_stats.tu			+= 4 * diff * _stats.tu / 100;
	_stats.stamina		+= 4 * diff * _stats.stamina / 100;
	_stats.reactions	+= 6 * diff * _stats.reactions / 100;
	_stats.firing		= (_stats.firing + 6 * diff * _stats.firing / 100) / (diff > 0? 1: 2);
	_stats.throwing		+= 4 * diff * _stats.throwing / 100; // kL
	_stats.melee		+= 4 * diff * _stats.melee / 100;
	_stats.strength		+= 2 * diff * _stats.strength / 100;
	_stats.psiStrength	+= 4 * diff * _stats.psiStrength / 100;
	_stats.psiSkill		+= 4 * diff * _stats.psiSkill / 100;

	// kL_begin:
	if (month > 0)
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

	if (diff == 0)		// kL, moved here from BattlescapeGenerator::addAlien() & BattlescapeGame::convertUnit()
		halveArmor();	// kL
	// kL_end.
}

/**
 * Gets if a unit already took fire damage this turn
 * (used to avoid damaging large units multiple times).
 * @return, ow it burns. It burrnsssss me!! cya.
 */
/*kL bool BattleUnit::getTookFire() const
{
	return _hitByFire;
} */

/**
 * Toggles the state of the fire-damage-tracking boolean.
 */
/*kL void BattleUnit::setTookFire()
{
	_hitByFire = !_hitByFire;
} */

/**
 *
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

// kL_begin:
/**
 * Initializes a death spin.
 * see Battlescape/UnitDieBState.cpp, cTor
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
 * see Battlescape/UnitDieBState.cpp, think()
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
 * @ return int, Tracks deathspin rotation
 */
int BattleUnit::getSpinPhase() const
{
	return _spinPhase;
}

/**
 * Sets the spinPhase of a unit.
 * @ param spinphase, The spinPhase to set
 */
void BattleUnit::setSpinPhase(int spinphase)
{
	_spinPhase = spinphase;
}

/**
 * Set a unit to STATUS_UNCONSCIOUS.
 * @ param status,
 */
void BattleUnit::knockOut()
{
	_status = STATUS_UNCONSCIOUS;
}

/**
 * Sets a unit's health level.
 */
void BattleUnit::setHealth(int health)
{
	_health = health;
}

/**
 * Stops a unit from firing/throwing if it spots a new opponent during turning.
 */
void BattleUnit::setStopShot(bool stop)
{
	_stopShot = stop;
}

/**
 * Stops a unit from firing/throwing if it spots a new opponent during turning.
 */
bool BattleUnit::getStopShot() const
{
	return _stopShot;
}

/**
 * Set a unit as dashing. *** Delete these and use BattleAction.run ****
 * ... although, that might not work so good; due to the unit 'dashing'
 * might not be the currentActor's currentBattleAction status.... Cf. Projectile
 */
void BattleUnit::setDashing(bool dash)
{
	_dashing = dash;
}

/**
 * Get if a unit is dashing.
 */
bool BattleUnit::getDashing() const
{
	return _dashing;
}

/**
 * Set a unit as having been damaged in a single explosion.
 */
void BattleUnit::setTakenExpl(bool beenhit)
{
	_takenExpl = beenhit;
}

/**
 * Get if a unit was aleady damaged in a single explosion.
 */
bool BattleUnit::getTakenExpl() const
{
	return _takenExpl;
}

/**
 * Sets the unit has having died by fire damage.
 */
/* void BattleUnit::setDiedByFire()
{
	_diedByFire = true;
} */

/**
 * Gets if the unit died by fire damage.
 */
bool BattleUnit::getDiedByFire() const
{
	return _diedByFire;
} // kL_end.

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
			&& !isOut()
			&& (!checkReselect || reselectAllowed())
			&& (!checkInventory || hasInventory());
}

/**
 * Checks if this unit has an inventory.
 * Large units and/or terror units don't have inventories.
 * @return, true if an inventory is available
 */
bool BattleUnit::hasInventory() const
{
	return _armor->getSize() == 1
		&& _rank != "STR_LIVE_TERRORIST";
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
		|| isOut())
	{
		_breathing = false;

		return;
	}

	if (!_breathing
		|| _status == STATUS_WALKING)
	{
		// deviation from original: TFTD used a static 10% chance for every animation frame,
		// instead let's use 5%, but allow morale to affect it.
		_breathing = (_status != STATUS_WALKING
					&& RNG::percent(105 - _morale));

		_breathFrame = 0;
	}

	if (_breathing)
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
 * @param floor - true if there is a floor
 */
void BattleUnit::setFloorAbove(bool floor)
{
	_floorAbove = floor;
}

/**
 * Checks if the floorAbove flag has been set.
 * @return, true if this unit is under cover
 */
bool BattleUnit::getFloorAbove()
{
	return _floorAbove;
}

/**
 * Gets the name of any melee weapon we may be carrying, or a built in one.
 * @return, the name of a melee weapon
 */
std::string BattleUnit::getMeleeWeapon()
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

	if (_unitRules != NULL) // -> do not CTD for Mc'd xCom agents. Thanks again.
		return _unitRules->getMeleeWeapon();

	return "";
}

/**
 * Get the unit's statistics.
 * @return BattleUnitStatistics statistics.
 */
BattleUnitStatistics* BattleUnit::getStatistics()
{
	return _statistics;
}

/**
 * Sets the unit murderer's id.
 * @param int murderer id.
 */
void BattleUnit::setMurdererId(int id)
{
	_murdererId = id;
}

/**
 * Gets the unit murderer's id.
 * @return int murderer id.
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
 * kL.
 */
void BattleUnit::setBattleGame(BattlescapeGame* battleGame) // kL
{
	_battleGame = battleGame;
}

}
