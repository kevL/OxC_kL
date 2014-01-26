/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
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
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Surface.h"

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
 */
BattleUnit::BattleUnit(
		Soldier* soldier,
		UnitFaction faction)
	:
		_faction(faction),
		_originalFaction(faction),
		_killedBy(faction),
		_id(0),
		_pos(Position()),
		_tile(0),
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
		_currentAIState(0),
		_visible(false),
		_cacheInvalid(true),
		_expBravery(0),
		_expReactions(0),
		_expFiring(0),
		_expThrowing(0),
		_expPsiSkill(0),
		_expMelee(0),
		_motionPoints(0),
		_kills(0),
		_hitByFire(false),
		_moraleRestored(0),
		_coverReserve(0),
		_charging(0),
		_turnsExposed(255),
		_geoscapeSoldier(soldier),
		_unitRules(0),
		_rankInt(-1),
		_turretType(-1),
		_hidingForTurn(false),
		_stopShot(false) // kL
{
	//Log(LOG_INFO) << "Create BattleUnit 1 : soldier ID = " << getId();

	_name			= soldier->getName();
	_id				= soldier->getId();
	_type			= "SOLDIER";
	_rank			= soldier->getRankString();
	_stats			= *soldier->getCurrentStats();

	_standHeight	= soldier->getRules()->getStandHeight();
	_kneelHeight	= soldier->getRules()->getKneelHeight();
	_floatHeight	= soldier->getRules()->getFloatHeight();
	_deathSound		= 0; // this one is hardcoded
	_aggroSound		= -1;
	_moveSound		= -1; // this one is hardcoded
	_intelligence	= 2;
	_aggression		= 1;
	_specab			= SPECAB_NONE;
	_armor			= soldier->getArmor();
	_stats			+= *_armor->getStats();	// armors may modify effective stats
	_loftempsSet	= _armor->getLoftempsSet();
	_gender			= soldier->getGender();
	_faceDirection	= -1;

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

	_value		= 20 + soldier->getMissions() + rankbonus;

	_tu			= _stats.tu;
	_energy		= _stats.stamina;
	_health		= _stats.health;
	_morale		= 100;
	_stunlevel	= 0;

	_currentArmor[SIDE_FRONT]	= _armor->getFrontArmor();
	_currentArmor[SIDE_LEFT]	= _armor->getSideArmor();
	_currentArmor[SIDE_RIGHT]	= _armor->getSideArmor();
	_currentArmor[SIDE_REAR]	= _armor->getRearArmor();
	_currentArmor[SIDE_UNDER]	= _armor->getUnderArmor();

	for (int i = 0; i < 6; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < 5; ++i)
		_cache[i] = 0;

	_activeHand = "STR_RIGHT_HAND";

	lastCover = Position(-1, -1, -1);
	//Log(LOG_INFO) << "Create BattleUnit 1, DONE";
}

/**
 * Initializes a BattleUnit from a Unit object.
 * @param unit Pointer to Unit object.
 * @param faction Which faction the units belongs to.
 * @param difficulty level (for stat adjustement)
 */
BattleUnit::BattleUnit(
		Unit* unit,
		UnitFaction faction,
		int id,
		Armor* armor,
		int diff)
	:
		_faction(faction),
		_originalFaction(faction),
		_killedBy(faction),
		_id(id),
		_pos(Position()),
		_tile(0),
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
		_currentAIState(0),
		_visible(false),
		_cacheInvalid(true),
		_expBravery(0),
		_expReactions(0),
		_expFiring(0),
		_expThrowing(0),
		_expPsiSkill(0),
		_expMelee(0),
		_motionPoints(0),
		_kills(0),
		_hitByFire(false),
		_moraleRestored(0),
		_coverReserve(0),
		_charging(0),
		_turnsExposed(255),
		_armor(armor),
		_geoscapeSoldier(0),
		_unitRules(unit),
		_rankInt(-1),
		_turretType(-1),
		_hidingForTurn(false),
		_stopShot(false) // kL
{
	//Log(LOG_INFO) << "Create BattleUnit 2 : alien ID = " << getId();

	_type	= unit->getType();
	_rank	= unit->getRank();
	_race	= unit->getRace();

	_stats	= *unit->getStats();
	_stats	+= *_armor->getStats();	// armors may modify effective stats
	if (faction == FACTION_HOSTILE)
		adjustStats(diff);

	_tu			= _stats.tu;
	_energy		= _stats.stamina;
	_health		= _stats.health;
	_morale		= 100;
	_stunlevel	= 0;

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
	_zombieUnit		= unit->getZombieUnit();
	_spawnUnit		= unit->getSpawnUnit();
	_value			= unit->getValue();
	_gender			= GENDER_MALE;
	_faceDirection	= -1;

	_currentArmor[SIDE_FRONT]	= _armor->getFrontArmor();
	_currentArmor[SIDE_LEFT]	= _armor->getSideArmor();
	_currentArmor[SIDE_RIGHT]	= _armor->getSideArmor();
	_currentArmor[SIDE_REAR]	= _armor->getRearArmor();
	_currentArmor[SIDE_UNDER]	= _armor->getUnderArmor();

	for (int i = 0; i < 6; ++i)
		_fatalWounds[i] = 0;
	for (int i = 0; i < 5; ++i)
		_cache[i] = 0;

	_activeHand = "STR_RIGHT_HAND";

	lastCover = Position(-1, -1, -1);
	//Log(LOG_INFO) << "Create BattleUnit 2, DONE";
}

/**
 *
 */
BattleUnit::~BattleUnit()
{
	//Log(LOG_INFO) << "Delete BattleUnit";
	for (int i = 0; i < 5; ++i)
	{
		if (_cache[i])
			delete _cache[i];
	}

//	delete _currentAIState;
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
	_stunlevel			= node["stunlevel"].as<int>(_stunlevel);
	_energy				= node["energy"].as<int>(_energy);
	_morale				= node["morale"].as<int>(_morale);
	_kneeled			= node["kneeled"].as<bool>(_kneeled);
	_floating			= node["floating"].as<bool>(_floating);
	_fire				= node["fire"].as<int>(_fire);
	_expBravery			= node["expBravery"].as<int>(_expBravery);
	_expReactions		= node["expReactions"].as<int>(_expReactions);
	_expFiring			= node["expFiring"].as<int>(_expFiring);
	_expThrowing		= node["expThrowing"].as<int>(_expThrowing);
	_expPsiSkill		= node["expPsiSkill"].as<int>(_expPsiSkill);
	_expMelee			= node["expMelee"].as<int>(_expMelee);
	_turretType			= node["turretType"].as<int>(_turretType);
	_visible			= node["visible"].as<bool>(_visible);
	_turnsExposed		= node["turnsExposed"].as<int>(_turnsExposed);
	_killedBy			= (UnitFaction)node["killedBy"].as<int>(_killedBy);
	_moraleRestored		= node["moraleRestored"].as<int>(_moraleRestored);
	_rankInt			= node["rankInt"].as<int>(_rankInt);
	_originalFaction	= (UnitFaction)node["originalFaction"].as<int>(_originalFaction);
	_kills				= node["kills"].as<int>(_kills);
	_dontReselect		= node["dontReselect"].as<bool>(_dontReselect);
	_charging			= 0;
	_specab				= (SpecialAbility)node["specab"].as<int>(_specab);
	_spawnUnit			= node["spawnUnit"].as<std::string>(_spawnUnit);
	_motionPoints		= node["motionPoints"].as<int>(0);
	_activeHand			= node["activeHand"].as<std::string>(_activeHand); // kL

	for (int i = 0; i < 5; i++)
		_currentArmor[i]	= node["armor"][i].as<int>(_currentArmor[i]);
	for (int i = 0; i < 6; i++)
		_fatalWounds[i]		= node["fatalWounds"][i].as<int>(_fatalWounds[i]);
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
	node["stunlevel"]		= _stunlevel;
	node["energy"]			= _energy;
	node["morale"]			= _morale;
	node["kneeled"]			= _kneeled;
	node["floating"]		= _floating;
	node["fire"]			= _fire;
	node["expBravery"]		= _expBravery;
	node["expReactions"]	= _expReactions;
	node["expFiring"]		= _expFiring;
	node["expThrowing"]		= _expThrowing;
	node["expPsiSkill"]		= _expPsiSkill;
	node["expMelee"]		= _expMelee;
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

	for (int i = 0; i < 5; i++)
		node["armor"].push_back(_currentArmor[i]);
	for (int i = 0; i < 6; i++)
		node["fatalWounds"].push_back(_fatalWounds[i]);

	if (getCurrentAIState()) node["AI"]			= getCurrentAIState()->save();
	if (_originalFaction != _faction)
		node["originalFaction"]					= (int)_originalFaction;
	if (_kills) node["kills"]					= _kills;
	if (_faction == FACTION_PLAYER
		&& _dontReselect) node["dontReselect"]	= _dontReselect;
	if (!_spawnUnit.empty()) node["spawnUnit"]	= _spawnUnit;

	return node;
}

/**
 * Returns the BattleUnit's unique ID.
 * @return Unique ID.
 */
int BattleUnit::getId() const
{
	return _id;
}

/**
 * Changes the BattleUnit's position.
 * @param pos position
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
 * Gets the BattleUnit's position.
 * @return position
 */
const Position& BattleUnit::getPosition() const
{
	return _pos;
}

/**
 * Gets the BattleUnit's last position.
 * @return position
 */
const Position& BattleUnit::getLastPosition() const
{
	return _lastPos;
}

/**
 * Gets the BattleUnit's destination.
 * @return destination
 */
const Position& BattleUnit::getDestination() const
{
	return _destination;
}

/**
 * Changes the BattleUnit's direction. Only used for initial unit placement.
 * kL_note: and positioning soldier when revived from unconscious status: reviveUnconsciousUnits().
 * @param direction
 */
void BattleUnit::setDirection(int direction)
{
	_direction = direction;
	_toDirection = direction;
	_directionTurret = direction;
}

/**
 * Gets the BattleUnit's (horizontal) direction.
 * @return direction
 */
int BattleUnit::getDirection() const
{
	return _direction;
}

/**
 * Changes the facedirection. Only used for strafing moves.
 * @param direction
 */
void BattleUnit::setFaceDirection(int direction)
{
	_faceDirection = direction;
}

/**
 * Gets the BattleUnit's (horizontal) face direction. Used only during strafing moves.
 * @return direction
 */
int BattleUnit::getFaceDirection() const
{
	return _faceDirection;
}

/**
 * Gets the BattleUnit's turret direction.
 * @return direction
 */
int BattleUnit::getTurretDirection() const
{
	return _directionTurret;
}

/**
 * Gets the BattleUnit's turret To direction.
 * @return toDirectionTurret
 */
int BattleUnit::getTurretToDirection() const
{
	return _toDirectionTurret;
}

/**
 * Gets the BattleUnit's vertical direction. This is when going up or down, doh!
 * @return direction
 */
int BattleUnit::getVerticalDirection() const
{
	return _verticalDirection;
}

/**
 * Gets the unit's status.
 * @return the unit's status
 */
UnitStatus BattleUnit::getStatus() const
{
	return _status;
}

/**
 * kL. Sets a unit's status.
 * @ param status, See UnitStatus enum.
 */
void BattleUnit::setStatus(int status)
{
	switch (status)
	{
		case 0: _status = STATUS_STANDING;		break;
		case 1: _status = STATUS_WALKING;		break;
		case 2: _status = STATUS_FLYING;		break;
		case 3: _status = STATUS_TURNING;		break;
		case 4: _status = STATUS_AIMING;		break;
		case 5: _status = STATUS_COLLAPSING;	break;
		case 6: _status = STATUS_DEAD;			break;
		case 7: _status = STATUS_UNCONSCIOUS;	break;
		case 8: _status = STATUS_PANICKING;		break;
		case 9: _status = STATUS_BERSERK;		break;

		default:
		break;
	}
}

/**
 * Initialises variables to start walking.
 * @param direction, Which way to walk.
 * @param destination, The position we should end up on.
 */
void BattleUnit::startWalking(
		int direction,
		const Position& destination,
		Tile* tileBelow,
		bool cache)
{
	Log(LOG_INFO) << "BattleUnit::startWalking() ID = " << getId()
					<< " _walkPhase = 0";
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
			Log(LOG_INFO) << ". STATUS_FLYING, using GravLift";
			_floating = false;
		}
		else
		{
			Log(LOG_INFO) << ". STATUS_FLYING, up.down";
			_floating = true;
		}
	}
	else if (_tile->hasNoFloor(tileBelow))
	{
		Log(LOG_INFO) << ". STATUS_FLYING, no Floor";
		_status = STATUS_FLYING;
		_floating = true;
		_kneeled = false;
		_direction = direction;
	}
	else
	{
		Log(LOG_INFO) << ". STATUS_WALKING";
		_status = STATUS_WALKING;
		_floating = false;
		_kneeled = false;
		_direction = direction;
	}
	Log(LOG_INFO) << "BattleUnit::startWalking() EXIT";
}
/*kL void BattleUnit::startWalking(int direction, const Position &destination, Tile *tileBelowMe, bool cache)
{
	if (direction >= Pathfinding::DIR_UP)
	{
		_verticalDirection = direction;
		_status = STATUS_FLYING;
	}
	else
	{
		_direction = direction;
		_status = STATUS_WALKING;
	}
	bool floorFound = false;
	if (!_tile->hasNoFloor(tileBelowMe))
	{
		floorFound = true;
	}
	if (!floorFound || direction >= Pathfinding::DIR_UP)
	{
		_status = STATUS_FLYING;
		_floating = true;
	}
	else
	{
		_floating = false;
	}

	_walkPhase = 0;
	_destination = destination;
	_lastPos = _pos;
	_cacheInvalid = cache;
	_kneeled = false;
} */

/**
 * This will increment the walking phase.
 */
void BattleUnit::keepWalking(
		Tile* tileBelow,
		bool cache)
{
	_walkPhase++;

	Log(LOG_INFO) << "BattleUnit::keepWalking() ID = " << getId()
					<< " _walkPhase = " << _walkPhase;
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
			if (_direction < 1 || 4 < _direction) // dir = 0,7,6,5 (up x3 or left)
				middle = end;
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
	{
		// we assume we reached our destination tile
		// this is actually a drawing hack, so soldiers are not overlapped by floortiles
		// kL_note: which they (large units) are half the time anyway...
		_pos = _destination;
	}

	if (_walkPhase >= end) // officially reached the destination tile
	{
		Log(LOG_INFO) << ". end -> STATUS_STANDING";

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
	Log(LOG_INFO) << "BattleUnit::keepWalking() EXIT";
}
/*kL void BattleUnit::keepWalking(Tile *tileBelowMe, bool cache)
{
	int middle, end;
	if (_verticalDirection)
	{
		middle = 4;
		end = 8;
	}
	else
	{
		// diagonal walking takes double the steps
		middle = 4 + 4 * (_direction % 2);
		end = 8 + 8 * (_direction % 2);
		if (_armor->getSize() > 1)
		{
			if (_direction < 1 || _direction > 4)
				middle = end;
			else
				middle = 1;
		}
	}
	if (!cache)
	{
		_pos = _destination;
		end = 2;
	}

	_walkPhase++;
	

	if (_walkPhase == middle)
	{
		// we assume we reached our destination tile
		// this is actually a drawing hack, so soldiers are not overlapped by floortiles
		_pos = _destination;
	}

	if (_walkPhase >= end)
	{
		if (_floating && !_tile->hasNoFloor(tileBelowMe))
		{
			_floating = false;
		}
		// we officially reached our destination tile
		_status = STATUS_STANDING;
		_walkPhase = 0;
		_verticalDirection = 0;
		if (_faceDirection >= 0) {
			// Finish strafing move facing the correct way.
			_direction = _faceDirection;
			_faceDirection = -1;
		} 

		// motion points calculation for the motion scanner blips
		if (_armor->getSize() > 1)
		{
			_motionPoints += 30;
		}
		else
		{
			// sectoids actually have less motion points
			// but instead of create yet another variable, 
			// I used the height of the unit instead (logical)
			if (getStandHeight() > 16)
				_motionPoints += 4;
			else
				_motionPoints += 3;
		}
	}

	_cacheInvalid = cache;
} */

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
 * @param point
 */
void BattleUnit::lookAt(
		const Position& point,
		bool turret)
{
	Log(LOG_INFO) << "BattleUnit::lookAt() #1";
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
			Log(LOG_INFO) << ". . . . lookAt() -> STATUS_TURNING";
			// kL_note: what about Forcing the faced direction instantly?
		}
	}
	//Log(LOG_INFO) << "BattleUnit::lookAt() #1 EXIT";
}

/**
 * Look a direction.
 * @param direction
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
		{
			_status = STATUS_TURNING;
			Log(LOG_INFO) << ". . . . lookAt() -> STATUS_TURNING";
		}
	}
	//Log(LOG_INFO) << "BattleUnit::lookAt() #2 EXIT";
}

/**
 * Advances the turning towards the target direction.
 */
void BattleUnit::turn(bool turret)
{
	Log(LOG_INFO) << "BattleUnit::turn()";
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
 * @param cache
 * @param part
 */
void BattleUnit::setCache(
		Surface* cache,
		int part)
{
	if (cache == 0)
		_cacheInvalid = true;
	else
	{
		_cache[part] = cache;
		_cacheInvalid = false;
	}
}

/**
 * Check if the unit is still cached in the Map cache.
 * When the unit changes it needs to be re-cached.
 * @param invalid
 * @return cache
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
 * Aim. (shows the right hand sprite and weapon holding)
 * @param aiming
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
 * @return direction.
 */
int BattleUnit::directionTo(const Position& point) const
{
	if (_pos == point) return 0;	// kL. safety

	double offset_x = point.x - _pos.x;
	double offset_y = point.y - _pos.y;

	double theta = atan2(-offset_y, offset_x); // radians: + = y > 0; - = y < 0;

	// divide the pie in 4 thetas, each at 1/8th before each quarter
	double m_pi_8 = M_PI / 8.0;			// a circle divided into 16 sections (rads) -> 22.5 deg
	double d = 0.0;						// kL, a bias toward cardinal directions. (0.1..0.12)
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
 * @param position, The position defines which part of armor and/or bodypart is hit.
 * @param power
 * @param type
 * @return, Damage done after adjustment
 */
int BattleUnit::damage(
		const Position& relative,
		int power,
		ItemDamageType type,
		bool ignoreArmor)
{
	Log(LOG_INFO) << "BattleUnit::damage(), ID " << getId();

	UnitSide side = SIDE_FRONT;
	UnitBodyPart bodypart = BODYPART_TORSO;

	if (power < 1)
		return 0;

	power = static_cast<int>(
					floor(static_cast<float>(power) * _armor->getDamageModifier(type)));
	Log(LOG_INFO) << "BattleUnit::damage(), type = " << (int)type
								<< " ModifiedPower " << power;

	if (type == DT_SMOKE) // smoke doesn't do real damage, but stun damage
		type = DT_STUN;

	if (!ignoreArmor)
	{
		if (relative == Position(0, 0, 0))
		{
			side = SIDE_UNDER;
		}
		else
		{
			int relativeDirection;
			const int abs_x = abs(relative.x);
			const int abs_y = abs(relative.y);
			if (abs_y > abs_x * 2)
				relativeDirection = 8 + 4 * (relative.y > 0);
			else if (abs_x > abs_y * 2)
				relativeDirection = 10 + 4 * (relative.x < 0);
			else
			{
				if (relative.x < 0)
				{
					if (relative.y > 0)
						relativeDirection = 13;
					else
						relativeDirection = 15;
				}
				else
				{
					if (relative.y > 0)
						relativeDirection = 11;
					else
						relativeDirection = 9;
				}
			}

			switch ((relativeDirection - _direction) %8)
			{
				case 0:	side = SIDE_FRONT; 									break;
				case 1:	side = RNG::percent(50)? SIDE_FRONT: SIDE_RIGHT;	break;
				case 2:	side = SIDE_RIGHT; 									break;
				case 3:	side = RNG::percent(50)? SIDE_REAR: SIDE_RIGHT;		break;
				case 4:	side = SIDE_REAR; 									break;
				case 5:	side = RNG::percent(50)? SIDE_REAR: SIDE_LEFT; 		break;
				case 6:	side = SIDE_LEFT; 									break;
				case 7:	side = RNG::percent(50)? SIDE_FRONT: SIDE_LEFT;		break;
			}

			if (relative.z > getHeight())
			{
				bodypart = BODYPART_HEAD;
			}
			else if (relative.z > 4)
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
						bodypart = (UnitBodyPart)RNG::generate(BODYPART_RIGHTLEG, BODYPART_LEFTLEG);
				}
			}
		}

		power -= getArmor(side);
	}

	if (power > 0)
	{
		if (type == DT_STUN)
			_stunlevel += power;
		else
		{
			_health -= power; // health damage
			if (_health < 0)
				_health = 0;

			if (type != DT_IN)
			{
				if (_armor->getSize() == 1) // conventional weapons can cause additional stun damage
					_stunlevel += RNG::generate(0, power / 4);

				if (isWoundable()) // fatal wounds
				{
					if (RNG::generate(0, 10) < power)
						_fatalWounds[bodypart] += RNG::generate(1, 3);

					if (_fatalWounds[bodypart])
						moraleChange(-_fatalWounds[bodypart]);
				}

				setArmor(getArmor(side) - (power / 10) - 1, side); // armor damage
			}
		}
	}

	if (power < 1)
		power = 0;
	Log(LOG_INFO) << "BattleUnit::damage() ret PenetratedPower " << power;

	return power;
}

/**
 * Do an amount of stun recovery.
 * @param power
 */
void BattleUnit::healStun(int power)
{
	_stunlevel -= power;

	if (_stunlevel < 0)
		_stunlevel = 0;
}

/**
 *
 */
int BattleUnit::getStunlevel() const
{
	return _stunlevel;
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
		setSpecialAbility(SPECAB_NONE);
		BattleUnit* newUnit = battle->convertUnit(
												this,
												_spawnUnit);
		newUnit->knockOut(battle);
	}
	else
		_stunlevel = _health;
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

	int endFrame = 3;
	if (_spawnUnit != ""
		&& _specab != SPECAB_RESPAWN) // <- is a Zombie! kL_note
	// kL_note: so.. units with a spawnUnit string but not specAb 3 set get the transformation anim.
	{
		endFrame = 18;
	}

	if (_fallPhase == endFrame)
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
		&& getStunlevel() >= getHealth())
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
		if (getStunlevel() >= getHealth())
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
 * Get the number of time units a certain action takes.
 * @param actionType
 * @param item
 * @return TUs
 */
int BattleUnit::getActionTUs(
		BattleActionType actionType,
		BattleItem* item)
{
	if (item == 0)
	{
		return 0;
	}

	int cost = 0;
	switch (actionType)
	{
		case BA_PRIME:
//kL			cost = 50; // maybe this should go in the ruleset
			cost = 45; // kL
		break;
		case BA_THROW:
//kL			cost = 25; // maybe this should go in the ruleset
			cost = 23; // kL
		break;
		case BA_AUTOSHOT:
			cost = item->getRules()->getTUAuto();
		break;
		case BA_SNAPSHOT:
			cost = item->getRules()->getTUSnap();
		break;
		case BA_STUN:
		case BA_HIT:
			cost = item->getRules()->getTUMelee();
		break;
		case BA_LAUNCH:
		case BA_AIMEDSHOT:
			cost = item->getRules()->getTUAimed();
		break;
		case BA_USE:
		case BA_MINDCONTROL:
		case BA_PANIC:
			cost = item->getRules()->getTUUse();
		break;

		default:
			cost = 0;
		break;
	}

	// if it's a percentage, apply it to unit TUs
	if (!item->getRules()->getFlatRate()
		|| actionType == BA_PRIME
		|| actionType == BA_THROW)
	{
		cost = static_cast<int>(floor(static_cast<float>(getStats()->tu * cost) / 100.f));
	}

	return cost;
}

/**
 * Spend time units if it can. Return false if it can't.
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
 * Spend energy if it can. Return false if it can't.
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
 * Set a specific number of timeunits.
 * @param tu
 */
void BattleUnit::setTimeUnits(int tu)
{
	_tu = tu;
}

/**
 * Set a specific number of energy.
 * @param energy
 */
void BattleUnit::setEnergy(int energy)
{
	_energy = energy;
}

/**
 * Set whether this unit is visible.
 * @param flag
 */
void BattleUnit::setVisible(bool flag)
{
	_visible = flag;
}

/**
 * Get whether this unit is visible.
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
 * Add this unit to a vector of spotted and/or visible units.
 * @param unit, A seen unit.
 * @return, True if the seen unit was NOT previously flagged as visible.
 * @note, xCom soldiers are always considered "visible";
 *			only aLiens go vis/Invis
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
		{
			return false;
		}
	}

	_visibleUnits.push_back(unit);

	return true;
}

/**
 * Get the pointer to the vector of visible units.
 * @return pointer to vector.
 */
std::vector<BattleUnit*>* BattleUnit::getVisibleUnits()
{
	return &_visibleUnits;
}

/**
 * Clear visible units.
 */
void BattleUnit::clearVisibleUnits()
{
	_visibleUnits.clear();
}

/**
 * Add this unit to the list of visible tiles. Returns true, or chrashes.
 * @param tile
 * @return
 */
bool BattleUnit::addToVisibleTiles(Tile* tile)
{
	_visibleTiles.push_back(tile);

	return true;
}

/**
 * Get the pointer to the vector of visible tiles.
 * @return pointer to vector.
 */
std::vector<Tile*>* BattleUnit::getVisibleTiles()
{
	return &_visibleTiles;
}

/**
 * Clear visible tiles.
 */
void BattleUnit::clearVisibleTiles()
{
	for (std::vector<Tile*>::iterator
			j = _visibleTiles.begin();
			j != _visibleTiles.end();
			++j)
	{
		(*j)->setVisible(-1);
	}

	_visibleTiles.clear();
}

/**
 * Calculate firing accuracy.
 * Formula = accuracyStat * weaponAccuracy * kneelingbonus(1.15) * one-handPenalty(0.8) * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @param actionType
 * @param item
 * @return firing Accuracy
 */
double BattleUnit::getFiringAccuracy(
		BattleActionType actionType,
		BattleItem* item)
{
	//Log(LOG_INFO) << "BattleUnit::getFiringAccuracy(), unitID " << getId() << " /  getStats()->firing" << getStats()->firing;

	if (actionType == BA_HIT) // quick out.
	{
		return static_cast<double>(item->getRules()->getAccuracyMelee()) / 100.0;
//		return static_cast<double>(item->getRules()->getAccuracyMelee()) / 100.0 * getAccuracyModifier(); // kL
	}


	double weaponAcc = 0.0;
	if (actionType == BA_AIMEDSHOT
		|| actionType == BA_LAUNCH)
	{
		weaponAcc = item->getRules()->getAccuracyAimed();
	}
	else if (actionType == BA_AUTOSHOT)
	{
		weaponAcc = item->getRules()->getAccuracyAuto();
	}
	else
	{
		weaponAcc = item->getRules()->getAccuracySnap();
	}

	double ret = static_cast<double>(getStats()->firing) / 100.0;
	ret *= weaponAcc / 100.0;

	if (_kneeled)
		ret *= 1.16;

	if (item->getRules()->isTwoHanded())
	{
		// two handed weapon, means one hand should be empty
		if (getItem("STR_RIGHT_HAND") != 0
			&& getItem("STR_LEFT_HAND") != 0)
		{
			ret *= 0.79;
		}
	}

	return ret * getAccuracyModifier();
}

/**
 * To calculate firing accuracy. Takes health and fatal wounds into account.
 * Formula = accuracyStat * woundsPenalty(% health) * critWoundsPenalty (-10%/wound)
 * @return modifier
 */
double BattleUnit::getAccuracyModifier()
{
	double ret = static_cast<double>(_health) / static_cast<double>(getStats()->health);

	int wounds = _fatalWounds[BODYPART_HEAD] + _fatalWounds[BODYPART_RIGHTARM];
	if (wounds > 9)
		wounds = 9;

	ret *= 1.0 - (0.1 * static_cast<double>(wounds));

	return ret;
}

/**
 * Calculate throwing accuracy.
 * @return throwing Accuracy
 */
double BattleUnit::getThrowingAccuracy()
{
	return (static_cast<double>(getStats()->throwing) / 100.0) * getAccuracyModifier();
}

/**
 * Set the armor value of a certain armor side.
 * @param armor Amount of armor.
 * @param side The side of the armor.
 */
void BattleUnit::setArmor(
		int armor,
		UnitSide side)
{
	if (armor < 0)
	{
		armor = 0;
	}

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
 * Get total amount of fatal wounds this unit has.
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
 * Little formula to calculate initiative/reaction score.
 * @return, Reaction score; aka INITIATIVE
 */
double BattleUnit::getInitiative()
{
	// (Reactions Stat) x (Current Time Units / Max TUs)
	return static_cast<double>(getStats()->reactions * getTimeUnits()) / static_cast<double>(getStats()->tu);
}

/**
 * Prepare for a new turn.
 */
void BattleUnit::prepareNewTurn()
{
	Log(LOG_INFO) << "BattleUnit::prepareNewTurn() ID " << getId();

	_faction = _originalFaction;
	//Log(LOG_INFO) << ". _stopShot is " << _stopShot << " setFALSE";
	//_stopShot = false;

	_unitsSpottedThisTurn.clear();

	int tuRecovery = getStats()->tu;

	float encumbrance = static_cast<float>(getStats()->strength) / static_cast<float>(getCarriedWeight());
	if (encumbrance < 1.f)
	{
		tuRecovery = static_cast<int>(encumbrance * static_cast<float>(tuRecovery));
	}

	// Each fatal wound to the left or right leg reduces the soldier's TUs by 10%.
	tuRecovery -= (tuRecovery * (_fatalWounds[BODYPART_LEFTLEG] + _fatalWounds[BODYPART_RIGHTLEG] * 10)) / 100;
	setTimeUnits(tuRecovery);

	if (!isOut()) // recover energy
	{
//kL		int enRecovery = getStats()->tu / 3;
		// kL_begin: advanced Energy recovery
		int enRecovery = getStats()->stamina;
		if (isKneeled())
			enRecovery /= 2;					// kneeled xCom
		else if (getFaction() == FACTION_PLAYER)
			enRecovery /= 3;					// xCom
		else
			enRecovery = enRecovery * 2 / 3;	// aLiens & civies
		// kL_end.

		// Each fatal wound to the body reduces the soldier's energy recovery by 10%.
		enRecovery -= (_energy * (_fatalWounds[BODYPART_TORSO] * 10)) / 100;
		_energy += enRecovery;

		if (_energy > getStats()->stamina)
			_energy = getStats()->stamina;
	}

	_health -= getFatalWounds(); // suffer from fatal wounds

	// Fire damage is also in Battlescape/BattlescapeGame::endTurn(), stand on fire tile
	// see also, Savegame/Tile::prepareNewTurn(), catch fire on fire tile
	// fire damage by hit is caused by TileEngine::explode()
	if (!_hitByFire
		&& _fire > 0) // suffer from fire
	{
		int fireDam = static_cast<int>(_armor->getDamageModifier(DT_IN) * RNG::generate(2, 6));
		Log(LOG_INFO) << ". fireDam = " << fireDam;
		_health -= fireDam;

		_fire--;
	}

	if (_health < 0)
		_health = 0;

	if (_health == 0
		&& _currentAIState) // if unit is dead, AI state should be gone
	{
		_currentAIState->exit();

		delete _currentAIState;

		_currentAIState = 0;
	}

	if (_stunlevel > 0)
		healStun(1); // recover stun 1pt/turn

	if (!isOut())
	{
		int panicChance = 100 - (2 * getMorale());
		if (RNG::percent(panicChance))
		{
			_status = STATUS_PANICKING;		// panic is either flee or freeze (determined later)
			if (RNG::percent(30))
				_status = STATUS_BERSERK;	// or shoot stuff.
		}
		else								// successfully avoided Panic
			_expBravery++;
	}

	_hitByFire = false;
	_dontReselect = false;
	_motionPoints = 0;

	Log(LOG_INFO) << "BattleUnit::prepareNewTurn() EXIT";
}

/**
 * Morale change with bounds check.
 * @param change, Can be positive or negative
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
 * Mark this unit as not reselectable.
 */
void BattleUnit::dontReselect()
{
	_dontReselect = true;
}

/**
 * Mark this unit as reselectable.
 */
void BattleUnit::allowReselect()
{
	_dontReselect = false;
}


/**
 * Check whether reselecting this unit is allowed.
 * @return bool
 */
bool BattleUnit::reselectAllowed() const
{
	return !_dontReselect;
}

/**
 * Set the amount of turns this unit is on fire. 0 = no fire.
 * @param fire : amount of turns this tile is on fire.
 */
void BattleUnit::setFire(int fire)
{
	if (_specab != SPECAB_BURNFLOOR)
		_fire = fire;
}

/**
 * Get the amount of turns this unit is on fire. 0 = no fire.
 * @return fire : amount of turns this tile is on fire.
 */
int BattleUnit::getFire() const
{
	return _fire;
}

/**
 * Get the pointer to the vector of inventory items.
 * @return pointer to vector.
 */
std::vector<BattleItem*>* BattleUnit::getInventory()
{
	return &_inventory;
}

/**
 * Let AI do their thing.
 * @param action AI action.
 */
void BattleUnit::think(BattleAction* action)
{
	checkAmmo();
	_currentAIState->think(action);
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
 * Sets the tile that unit is standing on.
 * @param tile
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
		Log(LOG_INFO) << ". STATUS_STANDING, _floating = " << _floating;
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
	else if (_tile != 0) // Ground items
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

	return 0;
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
	else if (_tile != 0) // Ground items
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

	return 0;
}

/**
 * Get the "main hand weapon" from the unit.
 * @param quickest, Whether to get the quickest weapon, default true
 * @return, Pointer to BattleItem.
 */
BattleItem* BattleUnit::getMainHandWeapon(bool quickest) const
{
	BattleItem* weaponRight = getItem("STR_RIGHT_HAND");
	BattleItem* weaponLeft = getItem("STR_LEFT_HAND");

	bool isRight = weaponRight
			&& weaponRight->getAmmoItem()
			&& weaponRight->getAmmoItem()->getAmmoQuantity();
//			&& (weaponRight->getRules()->getBattleType() == BT_FIREARM
//				|| weaponRight->getRules()->getBattleType() == BT_MELEE)
	bool isLeft = weaponLeft
			&& weaponLeft->getAmmoItem()
			&& weaponLeft->getAmmoItem()->getAmmoQuantity();
//			&& (weaponLeft->getRules()->getBattleType() == BT_FIREARM
//				|| weaponLeft->getRules()->getBattleType() == BT_MELEE);

	if (!isRight && !isLeft)
		return 0;
	else if (!isLeft && isRight)
		return weaponRight;
	else if (!isRight && isLeft)
		return weaponLeft;
	else // (isRight && isLeft).
	{
		RuleItem* rightRule = weaponRight->getRules();
		int tuRight = rightRule->getBattleType() == BT_MELEE?
									rightRule->getTUMelee()
								: rightRule->getTUSnap();

		RuleItem* leftRule = weaponLeft->getRules();
		int tuLeft = leftRule->getBattleType() == BT_MELEE?
									leftRule->getTUMelee()
								: leftRule->getTUSnap();

		if (tuRight <= tuLeft)
			return quickest? weaponRight: weaponLeft;
		else
			return quickest? weaponLeft: weaponRight;
	}

	return 0;
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
	{
		return quickest?weaponRightHand:weaponLeftHand;
	}
	else
	{
		return quickest?weaponLeftHand:weaponRightHand;
	} */

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
		{
			return *i;
		}
	}

	return 0;
}

/**
 * Check if we have ammo and reload if needed (used for AI)
 */
bool BattleUnit::checkAmmo()
{
	BattleItem* weapon = getItem("STR_RIGHT_HAND");
	if (!weapon
		|| weapon->getAmmoItem() != 0
		|| weapon->getRules()->getBattleType() == BT_MELEE
		|| getTimeUnits() < 15)
	{
		weapon = getItem("STR_LEFT_HAND");
		if (!weapon
			|| weapon->getAmmoItem() != 0
			|| weapon->getRules()->getBattleType() == BT_MELEE
			|| getTimeUnits() < 15)
		{
			return false;
		}
	}

	// we have a non-melee weapon with no ammo and 15 or more TUs - we might need to look for ammo then
	BattleItem* ammo = 0;
	bool wrong = true;

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
				wrong = false;

				break;
			}
		}

		if (!wrong) break;
	}

	if (wrong)
		return false; // didn't find any compatible ammo in inventory

	spendTimeUnits(15);
	weapon->setAmmoItem(ammo);
	ammo->moveToOwner(0);

	return true;
}

/**
 * Check if this unit is in the exit area.
 * @return Is in the exit area?
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
 * Adds qty to the psionic exp counter.
 * @param (int)qty, Amount to add.
 */
void BattleUnit::addPsiExp(int qty)
{
	_expPsiSkill++;
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
 * @return, True if the soldier was eligible for squaddie promotion.
 */
bool BattleUnit::postMissionProcedures(SavedGame* geoscape)
{
	Soldier* s = geoscape->getSoldier(_id);
	if (s == 0)
		return false;


	updateGeoscapeStats(s);

	UnitStats* stats = s->getCurrentStats();
	const UnitStats caps = s->getRules()->getStatCaps();

	int healthLoss = stats->health - _health;
	s->setWoundRecovery(RNG::generate(
							static_cast<int>((static_cast<double>(healthLoss) * 0.5)),
							static_cast<int>((static_cast<double>(healthLoss) * 1.5))));

	if (_expBravery && stats->bravery < caps.bravery)
	{
		if (_expBravery > RNG::generate(0, 10))
			stats->bravery += 10;
	}

	if (_expReactions
		&& stats->reactions < caps.reactions)
	{
		stats->reactions += improveStat(_expReactions);
	}

	if (_expFiring
		&& stats->firing < caps.firing)
	{
		stats->firing += improveStat(_expFiring);
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

	if (_expThrowing
		&& stats->throwing < caps.throwing)
	{
		stats->throwing += improveStat(_expThrowing);
	}

	if (_expBravery
		|| _expReactions
		|| _expFiring
		|| _expMelee
		|| _expPsiSkill)
	{
		if (s->getRank() == RANK_ROOKIE)
			s->promoteRank();

		int v = caps.tu - stats->tu;

		if (v > 0) stats->tu += RNG::generate(0, (v / 10) + 2) - 1;

		v = caps.health - stats->health;
		if (v > 0) stats->health += RNG::generate(0, (v / 10) + 2) - 1;

		v = caps.strength - stats->strength;
		if (v > 0) stats->strength += RNG::generate(0, (v / 10) + 2) - 1;

		v = caps.stamina - stats->stamina;
		if (v > 0) stats->stamina += RNG::generate(0, (v / 10) + 2) - 1;

		return true;
	}
	else
	{
		return false;
	}
}

/**
 * Converts an amount of experience to a stat increase.
 * @param (int)exp, Experience counter.
 * @return, Stat increase.
 */
int BattleUnit::improveStat(int exp)
{
	double tier = 4.0;

	if (exp < 11)
	{
		tier = 3.0;

		if (exp < 6)
			tier = exp > 2? 2.0: 1.0;
	}

	return static_cast<int>((tier / 2.0) + RNG::generate(0.0, tier));
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
 * @param part, The body part to heal
 * @param woundAmount, The amount of fatal wound healed
 * @param healthAmount, The amount of health to add to soldier health
 */
void BattleUnit::heal(
		int part,
		int woundAmount,
		int healthAmount)
{
	if (part < 0 || part > 5)
		return;

	if (!_fatalWounds[part])
		return;

	_fatalWounds[part] -= woundAmount;

	_health += healthAmount;
	if (_health > getStats()->health)
		_health = getStats()->health;

	moraleChange(healthAmount);
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
 * Get motion points for the motion scanner.
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
 * Get unit's name.
 * An alien's name is the translation of its race and rank.
 * hence the language pointer needed.
 * @param lang Pointer to language.
 * @return name Widecharstring of the unit's name.
 */
std::wstring BattleUnit::getName(
		Language* lang,
		bool debugAppendId) const
{
	if (_type != "SOLDIER"
		&& lang != 0)
	{
		std::wstring ret;

		if (_type.find("STR_") != std::string::npos)
			ret = lang->getString(_type);
		else
			ret = lang->getString(_race);

		if (debugAppendId)
		{
			std::wstringstream ss;
			ss << ret << L" " << _id;
			ret = ss.str();
		}

		return ret;
	}

	return _name;
}
/**
 * Gets pointer to the unit's stats.
 * @return stats Pointer to the unit's stats.
 */
UnitStats* BattleUnit::getStats()
{
	//Log(LOG_INFO) << "UnitStats* BattleUnit::getStats(), unitID = " << getId();
//	adjustStats(3);		// kL, should be gameDifficulty in there <-

	return &_stats;
}

/**
 * Get the unit's stand height.
 * @return The unit's height in voxels, when standing up.
 */
int BattleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Get the unit's kneel height.
 * @return The unit's height in voxels, when kneeling.
 */
int BattleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Get the unit's floating elevation.
 * @return The unit's elevation over the ground in voxels, when flying.
 */
int BattleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Get the unit's loft ID.
 * This is one slice only, as it is repeated over the entire height of the unit.
 * @return, The unit's line of fire template ID.
 */
int BattleUnit::getLoftemps(int entry) const
{
	return _loftempsSet.at(entry);
}

/**
 * Get the unit's value. Used for score at debriefing.
 * @return value score
 */
int BattleUnit::getValue() const
{
	return _value;
}

/**
 * Get the unit's death sound.
 * @return id.
 */
int BattleUnit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Get the unit's move sound.
 * @return id.
 */
int BattleUnit::getMoveSound() const
{
	return _moveSound;
}

/**
 * Get whether the unit is affected by fatal wounds.
 * Normally only soldiers are affected by fatal wounds.
 * @return true or false
 */
bool BattleUnit::isWoundable() const
{
	return (_type == "SOLDIER");
}

/**
 * Get whether the unit is affected by morale loss.
 * Normally only small units are affected by morale loss.
 * @return true or false
 */
bool BattleUnit::isFearable() const
{
	return (_armor->getSize() == 1);
}

/**
 * Get the unit's intelligence. Is the number of turns AI remembers a soldier's position.
 * @return intelligence 
 */
int BattleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Get the unit's aggression.
 */
int BattleUnit::getAggression() const
{
	return _aggression;
}

/**
 * Get the unit's special ability.
 */
int BattleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Set the unit's special ability.
 */
void BattleUnit::setSpecialAbility(SpecialAbility specab)
{
	_specab = specab;
}

/**
 * Get the unit that the victim is morphed into when attacked.
 * @return unit.
 */
std::string BattleUnit::getZombieUnit() const
{
	return _zombieUnit;
}

/**
 * Get the unit that is spawned when this one dies.
 * @return unit.
 */
std::string BattleUnit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Set the unit that is spawned when this one dies.
 * @return unit.
 */
void BattleUnit::setSpawnUnit(std::string spawnUnit)
{
	_spawnUnit = spawnUnit;
}

/**
 * Get the unit's rank string.
 */
std::string BattleUnit::getRankString() const
{
	return _rank;
}

// kL_begin:
/**
 * Get the unit's race string.
 */
std::string BattleUnit::getRaceString() const
{
	return _race;
}
// kL_end.

/**
 * Get the geoscape-soldier object.
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
 * Get unit type.
 */
std::string BattleUnit::getType() const
{
	return _type;
}

/**
 * Set unit's active hand.
 */
void BattleUnit::setActiveHand(const std::string& hand)
{
	if (_activeHand != hand)
		_cacheInvalid = true;

	_activeHand = hand;
}

/**
 * Get unit's active hand.
 * Must have an item in that hand. Else, switch
 * to other hand or use default @ right Hand.
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
 */
void BattleUnit::convertToFaction(UnitFaction f)
{
	_faction = f;
}

/**
 * Set health to 0 and set status dead - used when getting zombified.
 */
void BattleUnit::instaKill()
{
	_health = 0;
	_status = STATUS_DEAD;
}

/**
 * Set health to 0 and set status dead - used when getting zombified.
 */
int BattleUnit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Halve this unit's armor values (for beginner mode)
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
 * Get the faction the unit was killed by.
 * @return faction
 */
UnitFaction BattleUnit::killedBy() const
{
	return _killedBy;
}

/**
 * Set the faction the unit was killed by.
 * @param f faction
 */
void BattleUnit::killedBy(UnitFaction f)
{
	_killedBy = f;
}

/**
 * Set the units we are charging towards.
 * @param chargeTarget Charge Target
 */
void BattleUnit::setCharging(BattleUnit* chargeTarget)
{
	_charging = chargeTarget;
}

/**
 * Get the units we are charging towards.
 * @return Charge Target
 */
BattleUnit* BattleUnit::getCharging()
{
	return _charging;
}

/**
 * Get the unit's carried weight in strength units.
 * @param draggingItem
 * @return weight
 */
int BattleUnit::getCarriedWeight(BattleItem* draggingItem) const
{
	int weight = _armor->getWeight();
	for (std::vector<BattleItem*>::const_iterator
			i = _inventory.begin();
			i != _inventory.end();
			++i)
	{
		if ((*i) == draggingItem)
			continue;

		weight += (*i)->getRules()->getWeight();

		if ((*i)->getAmmoItem()
			&& (*i)->getAmmoItem() != *i)
		{
			weight += (*i)->getAmmoItem()->getRules()->getWeight();
		}
	}

	return weight;
}

/**
 * Set how long since this unit was last exposed.
 * @param (int)turns, Set # turns unit has been exposed
 */
void BattleUnit::setTurnsExposed(int turns)
{
	_turnsExposed = turns;

	if (_turnsExposed > 255) // kL
		_turnsExposed = 255; // kL
		// kL_note: should set this to -1 instead of 255.
}

/**
 * Get how long since this unit was exposed.
 * @return (int)turns, # turns unit has been exposed
 */
int BattleUnit::getTurnsExposed() const
{
	return _turnsExposed;
}

/**
 * Get this unit's original Faction.
 * @return original faction
 */
UnitFaction BattleUnit::getOriginalFaction() const
{
	return _originalFaction;
}

/**
 * Invalidate cache; call after copying object :(
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
 * kL_note: This pertains only to individual soldiers;
 * ie, what has *this* soldier spotted this turn.
 * Update: now pertains only to aLien units.
 */
std::vector<BattleUnit*> BattleUnit::getUnitsSpottedThisTurn()
{
	return _unitsSpottedThisTurn;
}

/**
 * 
 */
void BattleUnit::setRankInt(int rank)
{
	_rankInt = rank;
}

/**
 * 
 */
int BattleUnit::getRankInt() const
{
	return _rankInt;
}

/**
 * 
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
 * This function checks if a tile is in a unit's facing-quadrant, using maths.
 * @param pos, the position to check against
 * @return, what the maths decide
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
 * Common function to adjust a unit's stats according to difficulty setting.
 */
void BattleUnit::adjustStats(const int diff)
{
	// adjust the unit's stats according to the difficulty level.
	_stats.tu			+= 4 * diff * _stats.tu / 100;
	_stats.stamina		+= 4 * diff * _stats.stamina / 100;
	_stats.reactions	+= 6 * diff * _stats.reactions / 100;
	_stats.firing		= (_stats.firing + 6 * diff * _stats.firing / 100) / (diff > 0 ? 1 : 2);
	_stats.strength		+= 2 * diff * _stats.strength / 100;
	_stats.melee		+= 4 * diff * _stats.melee / 100;
	_stats.psiSkill		+= 4 * diff * _stats.psiSkill / 100;
	_stats.psiStrength	+= 4 * diff * _stats.psiStrength / 100;

	//Log(LOG_INFO) << "BattleUnit::adjustStats(), unitID = " << getId();
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.tu = " << _stats.tu;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.stamina = " << _stats.stamina;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.reactions = " << _stats.reactions;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.firing = " << _stats.firing;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.strength = " << _stats.strength;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.melee = " << _stats.melee;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.psiSkill = " << _stats.psiSkill;
	//Log(LOG_INFO) << "BattleUnit::adjustStats(), _stats.psiStrength = " << _stats.psiStrength;
}

/**
 * Did this unit already take fire damage this turn?
 * (used to avoid damaging large units multiple times.)
 */
bool BattleUnit::tookFireDamage() const
{
	return _hitByFire;
}

/**
 * Toggle the state of the fire damage tracking boolean.
 */
void BattleUnit::toggleFireDamage()
{
	_hitByFire = !_hitByFire;
}

/**
 * 
 */
void BattleUnit::setCoverReserve(int reserve)
{
	_coverReserve = reserve;
}

/**
 * 
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
	if (dir == 3) // if facing player, 1 rotation left
	{
//		Log(LOG_INFO) << ". d_init = " << dir;
		if (_spinPhase == 3 || _spinPhase == 4) // start = 0
		{
//			Log(LOG_INFO) << ". . _spinPhase = " << _spinPhase << " [ return ]";
			 _spinPhase = -1; // end.
			_status = STATUS_STANDING;

			 return;
		}
		else if (_spinPhase == 1)
			_spinPhase = 3; // 2nd CW rotation
		else if (_spinPhase == 2)
			_spinPhase = 4; // 2nd CCW rotation
	}

	if (_spinPhase == 0)
	{
		if (-1 < dir && dir < 4)
		{
			if (dir == 3)
				_spinPhase = 3; // only 1 CW rotation
			else
				_spinPhase = 1; // 1st CW rotation
		}
		else
		{
			if (dir == 3)
				_spinPhase = 4; // only 1 CCW rotation
			else
				_spinPhase = 2; // 1st CCW rotation
		}
	}

	if (_spinPhase == 1 || _spinPhase == 3)
	{
		dir++;
		if (dir == 8) dir = 0;
	}
	else
	{
		dir--;
		if (dir == -1) dir = 7;
	}

//	Log(LOG_INFO) << ". d_final = " << dir;
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
 * @return True if an inventory is available, false otherwise.
 */
void BattleUnit::setHealth(int health)
{
	_health = health;
}

/**
 * to stop a unit from firing/throwing if it spots a new opponent during turning
 */
void BattleUnit::setStopShot(bool stop)
{
	_stopShot = stop;
}

/**
 * to stop a unit from firing/throwing if it spots a new opponent during turning
 */
bool BattleUnit::getStopShot() const
{
	return _stopShot;
}
// kL_end.

/**
 * Checks if this unit can be selected. Only alive units
 * belonging to the faction can be selected.
 * @param faction, The faction to compare with.
 * @param checkReselect, Check if the unit is reselectable.
 * @param checkInventory, Check if the unit has an inventory.
 * @return, True if the unit can be selected, false otherwise.
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
 * Checks if this unit has an inventory. Large units and/or
 * terror units don't have inventories.
 * @return True if an inventory is available, false otherwise.
 */
bool BattleUnit::hasInventory() const
{
	return _armor->getSize() == 1 && _rank != "STR_LIVE_TERRORIST";
}

}
