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

#include "Ufo.h"

//#include <sstream>
//#include <algorithm>

//#include <assert.h>
//#include <math.h>

//#include "../fmath.h"

#include "AlienMission.h"
#include "Craft.h"
#include "SavedGame.h"
#include "Waypoint.h"

#include "../Engine/Exception.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"


namespace OpenXcom
{

/**
 * Initializes a UFO of the specified type.
 * @param rules - pointer to RuleUfo
 */
Ufo::Ufo(RuleUfo* rules)
	:
		MovingTarget(),
		_rules(rules),
		_id(0),
		_crashId(0),
		_crashPower(0),
		_landId(0),
		_damage(0),
		_direction("STR_NORTH"),
		_altitude("STR_HIGH_UC"),
		_status(FLYING),
		_secondsRemaining(0),
		_inBattlescape(false),
		_mission(NULL),
		_trajectory(NULL),
		_trajectoryPoint(0),
		_detected(false),
		_hyperDetected(false),
		_shootingAt(0),
		_hitFrame(0),
		_terrain(""),
		_shotDownByCraftId() // kL
{
}

/**
 * Make sure our mission forgets us, and we only delete targets we own (waypoints).
 */
Ufo::~Ufo()
{
	for (std::vector<Target*>::const_iterator
			i = _followers.begin();
			i != _followers.end();
			)
	{
		Craft* const c = dynamic_cast<Craft*>(*i);
		if (c != NULL)
		{
			c->returnToBase();
			i = _followers.begin();
		}
		else
			 ++i;
	}

	if (_mission != NULL)
		_mission->decreaseLiveUfos();

	if (_dest != NULL)
	{
		const Waypoint* const wp = dynamic_cast<Waypoint*>(_dest);
		if (wp != NULL)
		{
			delete _dest;
			_dest = NULL;
		}
	}
}


/**
 * Match AlienMission based on the unique ID.
 */
class matchMissionID
	:
		public std::unary_function<const AlienMission*, bool>
{

private:
	int _id;


	public:
		/// Store ID for later comparisons.
		matchMissionID(int id)
			:
				_id(id)
		{
			/* Empty by design. */
		}

		/// Match with stored ID.
		bool operator()(const AlienMission* am) const
		{
			return am->getId() == _id;
		}
};


/**
 * Loads the UFO from a YAML file.
 * @param node		- reference a YAML node
 * @param ruleset	- reference the Ruleset (used to access trajectory data)
 * @param game		- reference the SavedGame (used to get the UFO's mission)
 */
void Ufo::load(
		const YAML::Node& node,
		const Ruleset& ruleset,
		SavedGame& game)
{
	MovingTarget::load(node);

	_id					= node["id"]				.as<int>(_id);
	_crashId			= node["crashId"]			.as<int>(_crashId);
	_crashPower			= node["crashPower"]		.as<int>(_crashPower);
	_landId				= node["landId"]			.as<int>(_landId);
	_damage				= node["damage"]			.as<int>(_damage);
	_altitude			= node["altitude"]			.as<std::string>(_altitude);
	_direction			= node["direction"]			.as<std::string>(_direction);
	_detected			= node["detected"]			.as<bool>(_detected);
	_hyperDetected		= node["hyperDetected"]		.as<bool>(_hyperDetected);
	_secondsRemaining	= node["secondsRemaining"]	.as<size_t>(_secondsRemaining);
	_inBattlescape		= node["inBattlescape"]		.as<bool>(_inBattlescape);
	_terrain			= node["terrain"]			.as<std::string>(_terrain); // kL

	double
		lon = _lon,
		lat = _lat;
	if (const YAML::Node& dest = node["dest"])
	{
		lon = dest["lon"].as<double>();
		lat = dest["lat"].as<double>();
	}

	_dest = new Waypoint();
	_dest->setLongitude(lon);
	_dest->setLatitude(lat);

	if (const YAML::Node& status = node["status"])
		_status = (UfoStatus)status.as<int>();
	else
	{
		if (_damage >= _rules->getMaxDamage())
			_status = DESTROYED;
		else if (_damage >= _rules->getMaxDamage() / 2)
			_status = CRASHED;
		else if (_altitude == "STR_GROUND")
			_status = LANDED;
		else
			_status = FLYING;
	}

	if (game.getMonthsPassed() != -1)
	{
		const int missionID = node["mission"].as<int>();
		std::vector<AlienMission*>::const_iterator found = std::find_if(
																	game.getAlienMissions().begin(),
																	game.getAlienMissions().end(),
																	matchMissionID(missionID));
		if (found == game.getAlienMissions().end())
		{
			// Corrupt save file.
			throw Exception("Unknown mission, save file is corrupt.");
		}

		_mission = *found;

		const std::string tid	= node["trajectory"].as<std::string>();
		_trajectory				= ruleset.getUfoTrajectory(tid);
		_trajectoryPoint		= node["trajectoryPoint"].as<size_t>(_trajectoryPoint);
	}

	if (_inBattlescape == true)
		setSpeed(0);
}

/**
 * Saves the UFO to a YAML file.
 * @return, YAML node
 */
YAML::Node Ufo::save(bool newBattle) const
{
	YAML::Node node = MovingTarget::save();

	node["type"]		= _rules->getType();
	node["id"]			= _id;

	if (_terrain != "")
		node["terrain"]	= _terrain; // kL

	if (_crashId != 0)
	{
		node["crashId"]		= _crashId;
		node["crashPower"]	= _crashPower;
	}
	else if (_landId != 0)
		node["landId"]	= _landId;

	node["damage"]		= _damage;
	node["altitude"]	= _altitude;
	node["direction"]	= _direction;
	node["status"]		= (int)_status;

	if (_detected == true)
		node["detected"]			= _detected;
	if (_hyperDetected == true)
		node["hyperDetected"]		= _hyperDetected;
	if (_secondsRemaining != 0)
		node["secondsRemaining"]	= _secondsRemaining;
	if (_inBattlescape == true)
		node["inBattlescape"]		= _inBattlescape;

	if (newBattle == false)
	{
		node["mission"]			= _mission->getId();
		node["trajectory"]		= _trajectory->getID();
		node["trajectoryPoint"]	= _trajectoryPoint;
	}

	return node;
}

/**
 * Saves the UFO's unique identifiers to a YAML file.
 * @return, YAML node.
 */
YAML::Node Ufo::saveId() const
{
	YAML::Node node = MovingTarget::saveId();

	node["type"]	= "STR_UFO";
	node["id"]		= _id;

	return node;
}

/**
 * Returns the ruleset for the UFO's type.
 * @return Pointer to ruleset.
 */
RuleUfo* Ufo::getRules() const
{
	return _rules;
}

/**
 * Returns the UFO's unique ID. If it's 0,
 * this UFO has never been detected.
 * @return Unique ID.
 */
int Ufo::getId() const
{
	return _id;
}

/**
 * Changes the UFO's unique ID.
 * @param id Unique ID.
 */
void Ufo::setId(int id)
{
	_id = id;
}

/**
 * Returns the UFO's unique identifying name.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::wstring Ufo::getName(Language* lang) const
{
	switch (_status)
	{
		case FLYING:
		case DESTROYED: // Destroyed also means leaving Earth.
			return lang->getString("STR_UFO_").arg(_id);
		break;
		case LANDED:
			return lang->getString("STR_LANDING_SITE_").arg(_landId);
		break;
		case CRASHED:
			return lang->getString("STR_CRASH_SITE_").arg(_crashId);
		break;

		default:
			return L"";
		break;
	}
}

/**
 * Returns the globe marker for the UFO.
 * @return Marker sprite, -1 if none or not detected
 */
int Ufo::getMarker() const
{
	if (_detected == false)
		return -1;

	switch (_status)
	{
		case Ufo::FLYING:	return 2;
		case Ufo::LANDED:	return 3;
		case Ufo::CRASHED:	return 4;

		default:			return -1;
	}
}

/**
 * Returns the amount of damage this UFO has taken.
 * @return Amount of damage.
 */
int Ufo::getDamage() const
{
	return _damage;
}

/**
 * Changes the amount of damage this UFO has taken.
 * @param damage Amount of damage.
 */
void Ufo::setDamage(int damage)
{
	_damage = damage;

	if (_damage < 0)
		_damage = 0;

	if (_damage >= _rules->getMaxDamage())
		_status = DESTROYED;
	else if (_damage >= _rules->getMaxDamage() / 2)
		_status = CRASHED;
}

/**
 * kL. Returns the ratio between the amount of damage this uFo
 * has taken and the total it can take before it's destroyed.
 * @return, Percentage of damage.
 */
int Ufo::getDamagePercent() const
{
	return static_cast<int>(
			floor((static_cast<double>(_damage) / static_cast<double>(_rules->getMaxDamage()))
			* 100.0));
}

/**
 * Returns whether this UFO has been detected by radars.
 * @return Detection status.
 */
bool Ufo::getDetected() const
{
	return _detected;
}

/**
 * Changes whether this UFO has been detected by radars.
 * @param detected Detection status.
 */
void Ufo::setDetected(bool detected)
{
	_detected = detected;
}

/**
 * Returns the amount of remaining seconds the UFO has left on the ground.
 * After this many seconds thet UFO will take off, if landed, or disappear, if
 * crashed.
 * @return Amount of seconds.
 */
size_t Ufo::getSecondsRemaining() const
{
	return _secondsRemaining;
}

/**
 * Changes the amount of remaining seconds the UFO has left on the ground.
 * After this many seconds thet UFO will take off, if landed, or disappear, if
 * crashed.
 * @param seconds Amount of seconds.
 */
void Ufo::setSecondsRemaining(size_t seconds)
{
	_secondsRemaining = seconds;
}

/**
 * Returns the current direction the UFO is heading in.
 * @return Direction.
 */
std::string Ufo::getDirection() const
{
	return _direction;
}

/**
 * Returns the current altitude of the UFO.
 * @return Altitude.
 */
std::string Ufo::getAltitude() const
{
	return _altitude;
}

/**
 * Changes the current altitude of the UFO.
 * @param altitude Altitude.
 */
void Ufo::setAltitude(const std::string& altitude)
{
	_altitude = altitude;

	if (_altitude != "STR_GROUND")
		_status = FLYING;
	else
	{
		if (isCrashed())
			_status = CRASHED;
		else
			_status = LANDED;
	}
}

/**
 * Returns if this UFO took enough damage to cause it to crash.
 * @return, crashed status
 */
bool Ufo::isCrashed() const
{
	return _damage > _rules->getMaxDamage() / 2;
}

/**
 * Returns if this UFO took enough damage to destroy it.
 * @return, destroyed status
 */
bool Ufo::isDestroyed() const
{
	return _damage >= _rules->getMaxDamage();
}

/**
 * Calculates the direction for the UFO based on the current raw speed and destination.
 */
void Ufo::calculateSpeed()
{
	MovingTarget::calculateSpeed();
	//Log(LOG_INFO) << "Ufo::calculateSpeed(), _speedLon = " << _speedLon;
	//Log(LOG_INFO) << "Ufo::calculateSpeed(), _speedLat = " << _speedLat;

	const double
		x = _speedLon,
		y = -_speedLat;

	// This section guards vs. divide-by-zero.
	if (AreSame(x, 0.0) || AreSame(y, 0.0))
	{
		if (AreSame(x, 0.0) && AreSame(y, 0.0))
			_direction = "STR_NONE_UC";
		else if (AreSame(x, 0.0))
		{
			if (y > 0.0)
				_direction = "STR_NORTH";
			else if (y < 0.0)
				_direction = "STR_SOUTH";
		}
		else if (AreSame(y, 0.0))
		{
			if (x > 0.0)
				_direction = "STR_EAST";
			else if (x < 0.0)
				_direction = "STR_WEST";
		}

		//Log(LOG_INFO) << ". . _dir = " << _direction;
		return;
	}

	double theta = atan2(y, x); // theta is radians.
	//Log(LOG_INFO) << ". . theta(rad) = " << theta;

	// Convert radians to degrees so i don't go bonkers;
	// ie. KILL IT WITH FIRE!!1@!
	// note that this is between +/- 180 deg.
	theta = theta * 180.0 / M_PI;
	//Log(LOG_INFO) << ". . theta(deg) = " << theta;

	if (157.5 < theta || theta < -157.5)
		_direction = "STR_WEST";
	else if (theta > 112.5)
		_direction = "STR_NORTH_WEST";
	else if (theta > 67.5)
		_direction = "STR_NORTH";
	else if (theta > 22.5)
		_direction = "STR_NORTH_EAST";
	else if (theta < -112.5)
		_direction = "STR_SOUTH_WEST";
	else if (theta < -67.5)
		_direction = "STR_SOUTH";
	else if (theta < -22.5)
		_direction = "STR_SOUTH_EAST";
	else
		_direction = "STR_EAST";

	//Log(LOG_INFO) << ". . _dir = " << _direction;
}

/**
 * Moves the UFO to its destination.
 */
void Ufo::think()
{
	switch (_status)
	{
		case FLYING:
			moveTarget();

			if (reachedDestination()) // Prevent further movement.
				setSpeed(0);
		break;

		case LANDED:
			assert(_secondsRemaining >= 5 && "Wrong time management.");
			_secondsRemaining -= 5;
		break;

		case CRASHED:
			if (_detected == false)
				_detected = true;

		case DESTROYED:
		default: // Do nothing
		break;
	}
}

/**
 * Gets the UFO's battlescape status.
 * @return bool
 */
bool Ufo::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Sets the UFO's battlescape status.
 * @param inbattle - true if this Ufo is in the battlescape
 */
void Ufo::setInBattlescape(const bool inbattle)
{
	if (inbattle == true)
		setSpeed(0);

	_inBattlescape = inbattle;
}

/**
 * Returns the alien race currently residing in the UFO.
 * @return, Alien race.
 */
const std::string& Ufo::getAlienRace() const
{
	return _mission->getRace();
}

/**
 * Sets a pointer to a craft that shot down this UFO.
 * @param craft - pointer to Craft
 */
void Ufo::setShotDownByCraftId(const CraftId& craft)
{
	_shotDownByCraftId = craft;
}

/**
 * Gets the pointer to the craft that shot down this UFO.
 */
CraftId Ufo::getShotDownByCraftId() const
{
	return _shotDownByCraftId;
}

/**
 * Gets a UFO's visibility to radar detection. The UFO's size and
 * altitude affect the chances of it being detected by radars.
 * @return, visibility modifier
 */
int Ufo::getVisibility() const
{
	int ret = 0;

	if (_rules->getSize() == "STR_VERY_SMALL")
		ret -= 30;
	else if (_rules->getSize() == "STR_SMALL")
		ret -= 15;
//	else if (_rules->getSize() == "STR_MEDIUM_UC")
	else if (_rules->getSize() == "STR_LARGE")
		ret -= 15;
	else if (_rules->getSize() == "STR_VERY_LARGE")
		ret -= 30;

	if (_altitude == "STR_GROUND")
//kL	ret = -30;
		ret -= 50; // kL
	else if (_altitude == "STR_VERY_LOW")
		ret -= 20;
	else if (_altitude == "STR_LOW_UC")
		ret -= 10;
//	else if (_altitude == "STR_HIGH_UC")
	else if (_altitude == "STR_VERY_HIGH")
		ret -= 10;

	//Log(LOG_INFO) << "Ufo::getVisibility() = " << ret;
	return ret;
}

/**
 * kL. Gets a UFO's detect-xCom-base ability.
 * @return, detect-a-base modifier
 */
int Ufo::getDetectors() const // kL
{
	int ret = 0;

	if (_rules->getSize() == "STR_VERY_SMALL")
		ret = -12;
	else if (_rules->getSize() == "STR_SMALL")
		ret = -8;
	else if (_rules->getSize() == "STR_MEDIUM_UC")
		ret = -5;
	else if (_rules->getSize() == "STR_LARGE")
		ret = -2;
//	else if (_rules->getSize() == "STR_VERY_LARGE")

	if (_altitude == "STR_GROUND")
		ret -= 30;
	else if (_altitude == "STR_VERY_LOW")
		ret += 17;
//	else if (_altitude == "STR_LOW_UC")
	else if (_altitude == "STR_HIGH_UC")
		ret -= 8;
	else if (_altitude == "STR_VERY_HIGH")
		ret -= 15;

	//Log(LOG_INFO) << "Ufo::getVisibility() = " << ret;
	return ret;
}

/**
 * Returns the Mission type of the UFO.
 * @return, reference to the mission type
 */
const std::string& Ufo::getMissionType() const
{
	return _mission->getType();
}

/**
 * Sets the mission information of the UFO.
 * The UFO will start at the first point of the trajectory. The actual UFO information
 * is not changed here, this only sets the information kept on behalf of the mission.
 * @param mission		- pointer to the actual mission object
 * @param trajectory	- pointer to the actual mission trajectory
 */
void Ufo::setMissionInfo(
		AlienMission* mission,
		const UfoTrajectory* trajectory)
{
	assert(!_mission && mission && trajectory);

	_mission = mission;
	_mission->increaseLiveUfos();
	_trajectoryPoint = 0;
	_trajectory = trajectory;
}

/**
 * Returns whether this UFO has been detected by hyper-wave.
 * @return Detection status.
 */
bool Ufo::getHyperDetected() const
{
	return _hyperDetected;
}

/**
 * Changes whether this UFO has been detected by hyper-wave.
 * @param hyperdetected Detection status.
 */
void Ufo::setHyperDetected(bool hyperdetected)
{
	_hyperDetected = hyperdetected;
}

/**
 * Handle destination changes, making sure to delete old waypoint destinations.
 * @param dest Pointer to the new destination.
 */
void Ufo::setDestination(Target* dest)
{
	Waypoint* old = dynamic_cast<Waypoint*>(_dest);
	MovingTarget::setDestination(dest);

	delete old;
}

/**
 *
 */
int Ufo::getShootingAt() const
{
	return _shootingAt;
}

/**
 *
 */
void Ufo::setShootingAt(int target)
{
	_shootingAt = target;
}

/**
 * Gets the UFO's landing site ID.
 */
int Ufo::getLandId() const
{
	return _landId;
}

/**
 * Sets the UFO's landing site ID.
 */
void Ufo::setLandId(int id)
{
	_landId = id;
}

/**
 * Gets the UFO's crash site ID.
 */
int Ufo::getCrashId() const
{
	return _crashId;
}

/**
 * Sets the UFO's crash site ID.
 */
void Ufo::setCrashId(int id)
{
	_crashId = id;
}

/// Gets the UFO's hit frame.
int Ufo::getHitFrame() const
{
	return _hitFrame;
}

/// Sets the UFO's hit frame.
void Ufo::setHitFrame(int frame)
{
	_hitFrame = frame;
}

/**
 * kL. Gets the UFO's powerSource explosive power factor.
 */
int Ufo::getCrashPower() const // kL
{
	return _crashPower;
}

/**
 * kL. Sets the UFO's powerSource explosive power factor.
 */
void Ufo::setCrashPower(int percent) // kL
{
	_crashPower = percent;
}

/**
 * kL. Gets a crashed or landed UFO's terrainType.
 */
std::string Ufo::getTerrain() const // kL
{
	return _terrain;
}

/**
 * kL. Sets a crashed or landed UFO's terrainType.
 */
void Ufo::setTerrain(std::string terrain) // kL
{
	_terrain = terrain;
}

}
