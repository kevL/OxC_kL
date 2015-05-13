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

#include "Ufo.h"

//#include <sstream>
//#include <algorithm>
//#include <assert.h>
//#define _USE_MATH_DEFINES
//#include <math.h>
//#include "../fmath.h"

#include "AlienMission.h"
#include "Craft.h"
#include "SavedGame.h"
#include "Waypoint.h"

//#include "../Engine/Exception.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"

#include "../Geoscape/Globe.h" // Globe::GLM_UFO_*

#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"


namespace OpenXcom
{

/**
 * Initializes a UFO of the specified type.
 * @param rules - pointer to RuleUfo
 */
Ufo::Ufo(const RuleUfo* const rules)
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
		_secondsLeft(0),
		_inTactical(false),
		_mission(NULL),
		_trajectory(NULL),
		_trajectoryPoint(0),
		_detected(false),
		_hyperDetected(false),
		_shootingAt(0),
		_hitFrame(0),
		_processedIntercept(false),
		_fireCountdown(0),
		_escapeCountdown(0),
		_radius(rules->getRadius())
//		_shotDownByCraftId() // kL
{}

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
		Craft* const craft = dynamic_cast<Craft*>(*i);
		if (craft != NULL)
		{
			craft->returnToBase();
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
		explicit matchMissionID(int id)
			:
				_id(id)
		{}

		/// Match with stored ID.
		bool operator() (const AlienMission* am) const
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

	_id				= node["id"]			.as<int>(_id);
	_crashId		= node["crashId"]		.as<int>(_crashId);
	_crashPower		= node["crashPower"]	.as<int>(_crashPower);
	_landId			= node["landId"]		.as<int>(_landId);
	_damage			= node["damage"]		.as<int>(_damage);
	_altitude		= node["altitude"]		.as<std::string>(_altitude);
	_direction		= node["direction"]		.as<std::string>(_direction);
	_detected		= node["detected"]		.as<bool>(_detected);
	_hyperDetected	= node["hyperDetected"]	.as<bool>(_hyperDetected);
	_secondsLeft	= node["secondsLeft"]	.as<int>(_secondsLeft);
	_inTactical		= node["inTactical"]	.as<bool>(_inTactical);
	_terrain		= node["terrain"]		.as<std::string>(_terrain); // kL

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
		const int missionId = node["mission"].as<int>();
		std::vector<AlienMission*>::const_iterator found = std::find_if(
																	game.getAlienMissions().begin(),
																	game.getAlienMissions().end(),
																	matchMissionID(missionId));
		if (found == game.getAlienMissions().end())
		{
			throw Exception("Unknown mission, save file is corrupt.");
		}

		_mission = *found;

		const std::string tid	= node["trajectory"]		.as<std::string>();
		_trajectory				= ruleset.getUfoTrajectory(tid);
		_trajectoryPoint		= node["trajectoryPoint"]	.as<size_t>(_trajectoryPoint);
	}

	_fireCountdown		= node["fireCountdown"]		.as<int>(_fireCountdown);
	_escapeCountdown	= node["escapeCountdown"]	.as<int>(_escapeCountdown);

	if (_inTactical == true)
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

	if (_terrain.empty() == false)
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
		node["detected"]		= _detected;
	if (_hyperDetected == true)
		node["hyperDetected"]	= _hyperDetected;
	if (_secondsLeft != 0)
		node["secondsLeft"]		= _secondsLeft;
	if (_inTactical == true)
		node["inTactical"]		= _inTactical;

	if (newBattle == false)
	{
		node["mission"]			= _mission->getId();
		node["trajectory"]		= _trajectory->getID();
		node["trajectoryPoint"]	= _trajectoryPoint;
	}

	node["fireCountdown"]	= _fireCountdown;
	node["escapeCountdown"]	= _escapeCountdown;

	return node;
}

/**
 * Saves the UFO's unique identifiers to a YAML file.
 * @return, YAML node
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
 * @return, pointer to RuleUfo
 */
const RuleUfo* const Ufo::getRules() const
{
	return _rules;
}

/**
 * Changes the ruleset for the UFO's type.
 * @param rules - pointer to RuleUfo
 * @warning ONLY FOR NEW BATTLE USE!
 */
void Ufo::changeRules(const RuleUfo* const rules)
{
	_rules = rules;
}

/**
 * Changes the UFO's unique ID.
 * @param id - unique ID
 */
void Ufo::setId(int id)
{
	_id = id;
}

/**
 * Returns the UFO's unique ID.
 * If it's 0 this UFO has never been detected.
 * @return, unique ID
 */
int Ufo::getId() const
{
	return _id;
}

/**
 * Returns the UFO's unique identifying name.
 * @param lang - pointer to Language to get strings from
 * @return, full name
 */
std::wstring Ufo::getName(Language* lang) const
{
	switch (_status)
	{
		case FLYING:
		case DESTROYED: // Destroyed also means leaving Earth.
			return lang->getString("STR_UFO_").arg(_id);

		case LANDED:
			return lang->getString("STR_LANDING_SITE_").arg(_landId);

		case CRASHED:
			return lang->getString("STR_CRASH_SITE_").arg(_crashId);
	}

	return L"";
}

/**
 * Returns the globe marker for the UFO.
 * @return, marker sprite #2,3,4 (-1 if not detected)
 */
int Ufo::getMarker() const
{
	if (_detected == false)
		return -1;

	switch (_status)
	{
		case Ufo::FLYING:	return Globe::GLM_UFO_FLYING;
		case Ufo::LANDED:	return Globe::GLM_UFO_LANDED;
		case Ufo::CRASHED:	return Globe::GLM_UFO_CRASHED;
	}

	return _rules->getMarker();
}

/**
 * Changes the amount of damage this UFO has taken.
 * @param damage - amount of damage
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
 * Returns the amount of damage this UFO has taken.
 * @return, amount of damage
 */
int Ufo::getDamage() const
{
	return _damage;
}

/**
 * Returns the ratio between the amount of damage this UFO
 * has taken and the total it can take before it's destroyed.
 * @return, damage percent
 */
int Ufo::getDamagePercent() const
{
	return static_cast<int>(std::floor(
		   static_cast<double>(_damage) / static_cast<double>(_rules->getMaxDamage()) * 100.));
}

/**
 * Changes whether this UFO has been detected by radars.
 * @param detected - true if detected (default true)
 */
void Ufo::setDetected(bool detected)
{
	_detected = detected;
}

/**
 * Returns whether this UFO has been detected by radars.
 * @return, true if detected
 */
bool Ufo::getDetected() const
{
	return _detected;
}

/**
 * Changes whether this Ufo has been detected by hyper-wave.
 * @param hyperdetected - true if hyperwave-detected (default true)
 */
void Ufo::setHyperDetected(bool hyperdetected)
{
	_hyperDetected = hyperdetected;
}

/**
 * Returns whether this Ufo has been detected by hyper-wave.
 * @return, true if hyperwave-detected
 */
bool Ufo::getHyperDetected() const
{
	return _hyperDetected;
}

/**
 * Changes the amount of remaining seconds the UFO has left on the ground.
 * After this many seconds this Ufo will take off if landed or disappear if crashed.
 * @param sec - time in seconds
 */
void Ufo::setSecondsLeft(int sec)
{
	_secondsLeft = std::max(0, sec);
}

/**
 * Returns the amount of remaining seconds the UFO has left on the ground.
 * After this many seconds this Ufo will take off if landed or disappear if crashed.
 * @return, amount of seconds
 */
int Ufo::getSecondsLeft() const
{
	return _secondsLeft;
}

/**
 * Changes the current altitude of the UFO.
 * @param altitude - altitude
 */
void Ufo::setAltitude(const std::string& altitude)
{
	_altitude = altitude;

	if (_altitude != "STR_GROUND")
		_status = FLYING;
	else
	{
		if (isCrashed() == true)
			_status = CRASHED;
		else
			_status = LANDED;
	}
}

/**
 * Returns the current altitude of the UFO.
 * @return, altitude
 */
std::string Ufo::getAltitude() const
{
	return _altitude;
}

/**
 * Returns the current direction the UFO is headed.
 * @return, direction
 */
std::string Ufo::getDirection() const
{
	return _direction;
}

/**
 * Returns if this Ufo took enough damage to cause it to crash.
 * @return, true if crashed
 */
bool Ufo::isCrashed() const
{
	return _damage > _rules->getMaxDamage() / 2;
}

/**
 * Returns if this Ufo took enough damage to destroy it.
 * @return, true if destroyed
 */
bool Ufo::isDestroyed() const
{
	return _damage >= _rules->getMaxDamage();
}

/**
 * Calculates the direction for this Ufo based on the current raw speed and destination.
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
	if (AreSame(x, 0.) || AreSame(y, 0.))
	{
		if (AreSame(x, 0.) && AreSame(y, 0.))
			_direction = "STR_NONE_UC";
		else if (AreSame(x, 0.))
		{
			if (y > 0.)
				_direction = "STR_NORTH";
			else //if (y < 0.)
				_direction = "STR_SOUTH";
		}
		else if (AreSame(y, 0.))
		{
			if (x > 0.)
				_direction = "STR_EAST";
			else //if (x < 0.)
				_direction = "STR_WEST";
		}

		//Log(LOG_INFO) << ". . _dir = " << _direction;
		return;
	}

	double theta = std::atan2(y, x); // theta is radians.
	//Log(LOG_INFO) << ". . theta(rad) = " << theta;

	// Convert radians to degrees so i don't go bonkers;
	// ie. KILL IT WITH FIRE!!1@!
	// note that this is between +/- 180 deg.
	theta = theta * 180. / M_PI;
	//Log(LOG_INFO) << ". . theta(deg) = " << theta;

	if (theta > 157.5 || theta < -157.5)
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
 * Moves this Ufo to its destination.
 */
void Ufo::think()
{
	switch (_status)
	{
		case FLYING:
			moveTarget();

			if (reachedDestination() == true) // Prevent further movement.
				setSpeed(0);
		break;

		case LANDED:
			assert(_secondsLeft >= 5 && "Wrong time management.");
			_secondsLeft -= 5;
		break;

		case CRASHED:
			if (_detected == false)
				_detected = true;

//		case DESTROYED: // Do nothing
//		break;
	}
}

/**
 * Sets this Ufo's battlescape status.
 * @param inTactical - true if in battlescape
 */
void Ufo::setInBattlescape(const bool inTactical)
{
	if (inTactical == true)
		setSpeed(0);

	_inTactical = inTactical;
}

/**
 * Gets this Ufo's battlescape status.
 * @return, true if in battlescape
 */
bool Ufo::isInBattlescape() const
{
	return _inTactical;
}

/**
 * Returns the alien race currently residing in the UFO.
 * @return, address of aLien race
 */
const std::string& Ufo::getAlienRace() const
{
	return _mission->getRace();
}

/**
 * Sets a pointer to a craft that shot down this Ufo.
 * @param craft - address of CraftID (CraftId.h)
 */
void Ufo::setShotDownByCraftId(const CraftId& craft)
{
	_shotDownByCraftId = craft;
}

/**
 * Gets the pointer to the craft that shot down this Ufo.
 * @return, CraftId (CraftId.h)
 */
CraftId Ufo::getShotDownByCraftId() const
{
	return _shotDownByCraftId;
}

/**
 * Gets the scare-factor of UFOs for activity on the Graphs.
 * @return, activity points
 */
int Ufo::getVictoryPoints() const
{
	int ret;

	switch (_status)
	{
		case Ufo::LANDED:		ret = 6; break;
		case Ufo::CRASHED:		ret = 3; break;
		case Ufo::DESTROYED:	ret = 0; break;
		case Ufo::FLYING:
		default:				ret = 1;
	}

//	if (_rules->getSize() == "STR_VERY_SMALL")
	if (_rules->getSize() == "STR_SMALL")
		ret += 1;
	else if (_rules->getSize() == "STR_MEDIUM_UC")
		ret += 2;
	else if (_rules->getSize() == "STR_LARGE")
		ret += 3;
	else if (_rules->getSize() == "STR_VERY_LARGE")
		ret += 5;

//	if (_altitude == "STR_GROUND") // Status _LANDED or _CRASHED above.
	if (_altitude == "STR_VERY_LOW")
		ret += 3;
	else if (_altitude == "STR_LOW_UC")
		ret += 2;
	else if (_altitude == "STR_HIGH_UC")
		ret += 1;
//	else if (_altitude == "STR_VERY_HIGH")


	return ret;
}

/**
 * Gets this Ufo's visibility to radar detection.
 * The UFO's size and altitude affect its chances of being detected on radar.
 * @return, detection modifier
 */
int Ufo::getVisibility() const
{
	int ret = 0;

	if (_rules->getSize() == "STR_VERY_SMALL")
		ret = -30;
	else if (_rules->getSize() == "STR_SMALL")
		ret = -15;
//	else if (_rules->getSize() == "STR_MEDIUM_UC")
	else if (_rules->getSize() == "STR_LARGE")
		ret = 15;
	else if (_rules->getSize() == "STR_VERY_LARGE")
		ret = 30;

	if (_altitude == "STR_GROUND")
		ret -= 50;
	else if (_altitude == "STR_VERY_LOW")
		ret -= 20;
	else if (_altitude == "STR_LOW_UC")
		ret -= 10;
//	else if (_altitude == "STR_HIGH_UC")
	else if (_altitude == "STR_VERY_HIGH")
		ret -= 10;

	return ret;
}

/**
 * Gets this Ufo's detect-xCom-base ability.
 * @return, detect-a-base modifier
 */
int Ufo::getDetectors() const
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

	return ret;
}

/**
 * Sets the mission information of this Ufo.
 * The UFO will start at the first point of the trajectory. The actual UFO information
 * is not changed here; this only sets the information kept on behalf of the mission.
 * @param mission		- pointer to the actual mission object
 * @param trajectory	- pointer to the actual mission trajectory
 */
void Ufo::setUfoMissionInfo(
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
 * Returns the mission type of this Ufo.
 * @return, address of the mission type
 */
const std::string& Ufo::getUfoMissionType() const
{
	return _mission->getRules().getType();
}

/**
 * Handle destination changes, making sure to delete old waypoint destinations.
 * @param dest - pointer to a new destination
 */
void Ufo::setDestination(Target* dest)
{
	const Waypoint* const old = dynamic_cast<Waypoint*>(_dest);
	MovingTarget::setDestination(dest);

	delete old;
}

/**
 * Sets the intercept window this Ufo is shooting at in a dogfight.
 * @param target - interception number
 */
void Ufo::setShootingAt(const size_t target)
{
	_shootingAt = target;
}

/**
 * Gets the intercept window this Ufo is shooting at in a dogfight.
 * @return, interception number
 */
size_t Ufo::getShootingAt() const
{
	return _shootingAt;
}

/**
 * Sets this Ufo's landing site ID.
 * @param id - landing-site ID
 */
void Ufo::setLandId(int id)
{
	_landId = id;
}

/**
 * Gets this Ufo's landing site ID.
 * @return, landing-site ID
 */
int Ufo::getLandId() const
{
	return _landId;
}

/**
 * Sets this Ufo's crash site ID.
 * @param id - crash-site ID
 */
void Ufo::setCrashId(int id)
{
	_crashId = id;
}

/**
 * Gets this Ufo's crash site ID.
 * @return, crash-site ID
 */
int Ufo::getCrashId() const
{
	return _crashId;
}

/**
 * Sets this Ufo's hit frame.
 * @param frame - hit frame
 */
void Ufo::setHitFrame(int frame)
{
	_hitFrame = frame;
}

/**
 * Gets this Ufo's hit frame.
 * @return, hit frame
 */
int Ufo::getHitFrame() const
{
	return _hitFrame;
}

/**
 * Sets the countdown ticks for escaping a dogfight.
 * @param time - how many ticks until the ship attempts to escape
 */
void Ufo::setEscapeCountdown(int timeLeft)
{
	_escapeCountdown = timeLeft;
}

/**
 * Gets the remaining escape ticks for dogfights.
 * @return, how many ticks until the ship tries to leave
 */
int Ufo::getEscapeCountdown() const
{
	return _escapeCountdown;
}

/**
 * Sets the number of ticks until the ufo fires its weapon.
 * @param time - number of ticks until refire
 */
void Ufo::setFireCountdown(int timeLeft)
{
	_fireCountdown = timeLeft;
}

/**
 * Gets the number of ticks until the ufo is ready to fire.
 * @return, ticks until weapon is ready
 */
int Ufo::getFireCountdown() const
{
	return _fireCountdown;
}

/**
 * Sets a flag denoting that this ufo has had its timers decremented.
 * Prevents multiple interceptions from decrementing or resetting an already running timer.
 * This flag is reset in advance each time the geoscape processes the dogfights.
 * @param processed - true if the timers have been processed (default true)
 */
void Ufo::setEngaged(bool processed)
{
	_processedIntercept = processed;
}

/**
 * Gets if the ufo has had its timers decremented on this cycle of interception updates.
 * @return, true if this ufo has already been processed
 */
bool Ufo::getEngaged() const
{
	return _processedIntercept;
}

/**
 * Sets this Ufo's terrain type when it crashes or lands.
 * @param terrain - the terrain type
 */
void Ufo::setUfoTerrainType(const std::string& terrain)
{
	_terrain = terrain;
}

/**
 * Gets this Ufo's terrain type once it's crashed or landed.
 * @return, the terrain type
 */
std::string Ufo::getUfoTerrainType() const
{
	return _terrain;
}

/**
 * Gets the size of this Ufo.
 * @return, the size (call it radius if you want)
 */
size_t Ufo::getRadius() const
{
	return _radius;
}

}
