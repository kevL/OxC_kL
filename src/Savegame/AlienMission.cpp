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

#include "AlienMission.h"

//#include <algorithm>
//#include <functional>
//#include <assert.h>
//#define _USE_MATH_DEFINES
//#include <math.h>
//#include "../fmath.h"

#include "AlienBase.h"
#include "Base.h"
#include "Country.h"
#include "Craft.h"
#include "MissionSite.h"
#include "Region.h"
#include "SavedGame.h"
#include "Ufo.h"
#include "Waypoint.h"

//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
//#include "../Engine/RNG.h"

#include "../Geoscape/Globe.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleCity.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTexture.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"


namespace OpenXcom
{

/**
 * Creates an AlienMission.
 * @param missionRule	- reference to RuleAlienMission
 * @param savedGame		- reference the SavedGame
 */
AlienMission::AlienMission(
		const RuleAlienMission& missionRule,
		SavedGame& savedGame)
	:
		_missionRule(missionRule),
		_savedGame(savedGame),
		_nextWave(0),
		_ufoCount(0),
		_spawnTime(0),
		_liveUfos(0),
		_uniqueID(0),
		_base(NULL)
{}

/**
 * dTor.
 */
AlienMission::~AlienMission()
{}


class matchById
	:
		public std::unary_function<const AlienBase*, bool>
{

private:
	int _id;

	public:
		/// Remember ID.
		matchById(int id)
			:
				_id(id)
		{}

		/// Match with stored ID.
		bool operator() (const AlienBase* ab) const
		{
			return ab->getId() == _id;
		}
};


/**
 * Loads the alien mission from a YAML file.
 * @param node - reference the YAML node containing data
 */
void AlienMission::load(const YAML::Node& node)
{
	_region		= node["region"]	.as<std::string>(_region);
	_race		= node["race"]		.as<std::string>(_race);
	_nextWave	= node["nextWave"]	.as<size_t>(_nextWave);
	_ufoCount	= node["ufoCount"]	.as<size_t>(_ufoCount);
	_spawnTime	= node["spawnTime"]	.as<size_t>(_spawnTime);
	_liveUfos	= node["liveUfos"]	.as<size_t>(_liveUfos);
	_uniqueID	= node["uniqueID"]	.as<int>(_uniqueID);

	if (const YAML::Node& base = node["alienBase"])
	{
		const int id = base.as<int>();
		const std::vector<AlienBase*>::const_iterator found = std::find_if(
																	_savedGame.getAlienBases()->begin(),
																	_savedGame.getAlienBases()->end(),
																	matchById(id));
		if (found == _savedGame.getAlienBases()->end())
		{
			throw Exception("Corrupted save: Invalid base for mission.");
		}

		_base = *found;
	}

}

/**
 * Saves the alien mission to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienMission::save() const
{
	YAML::Node node;

	node["type"]		= _missionRule.getType();
	node["region"]		= _region;
	node["race"]		= _race;
	node["nextWave"]	= _nextWave;
	node["ufoCount"]	= _ufoCount;
	node["spawnTime"]	= _spawnTime;
	node["liveUfos"]	= _liveUfos;
	node["uniqueID"]	= _uniqueID;

	if (_base != NULL)
		node["alienBase"] = _base->getId();

	return node;
}

/**
 * Checks if a mission is over and can be safely removed from the game.
 * A mission is over if it will spawn no more UFOs and it has no UFOs still in the game.
 * @return, true if the mission can be safely removed
 */
bool AlienMission::isOver() const
{
	if (_liveUfos == 0
		&& _nextWave == _missionRule.getWaveTotal()
		&& _missionRule.getObjective() != OBJECTIVE_INFILTRATION) // Infiltrations continue for ever. Almost.
//			|| RNG::percent(static_cast<int>(_savedGame.getDifficulty()) * 20) == false))
	{
		return true;
	}

	return false;
}


/**
 * Finds an XCOM base in this region that is marked for retaliation.
 */
class FindMarkedXCOMBase
	:
		public std::unary_function<const Base*, bool>
{

private:
	const RuleRegion& _region;

	public:
		///
		FindMarkedXCOMBase(const RuleRegion& region)
			:
				_region(region)
		{}

		///
		bool operator() (const Base* base) const
		{
			return base->getIsRetaliationTarget() == true
				&& _region.insideRegion(
									base->getLongitude(),
									base->getLatitude()) == true;
		}
};


/**
 * Advances this AlienMission.
 * @param engine	- reference the Game data
 * @param globe		- reference the Globe
 */
void AlienMission::think(
		Game& engine,
		const Globe& globe)
{
	const Ruleset& rules = *engine.getRuleset();

	if (_nextWave >= _missionRule.getWaveTotal())
		return;

	if (_spawnTime > 30)
	{
		_spawnTime -= 30;
		return;
	}

	const MissionWave& wave = _missionRule.getWave(_nextWave);
	const UfoTrajectory& trajectory = *rules.getUfoTrajectory(wave.trajectory);

	Ufo* const ufo = spawnUfo(
							rules,
							globe,
							wave,
							trajectory);

	if (ufo != NULL) // Some missions may not spawn a UFO!
		_savedGame.getUfos()->push_back(ufo);

	++_ufoCount;
	if (_ufoCount == wave.ufoTotal)
	{
		_ufoCount = 0;
		++_nextWave;
	}

	if (_missionRule.getObjective() == OBJECTIVE_INFILTRATION
		&& _nextWave == _missionRule.getWaveTotal())
	{
		// kL_note: well that's fucked:
		std::vector<Country*> eligibleCountries;

		for (std::vector<Country*>::const_iterator
				i = _savedGame.getCountries()->begin();
				i != _savedGame.getCountries()->end();
				++i)
		{
			if ((*i)->getPact() == false
				&& (*i)->getNewPact() == false
				&& rules.getRegion(_region)->insideRegion( // WARNING: The *label* of a Country must be inside its Region for aLiens to infiltrate!
													(*i)->getRules()->getLabelLongitude(),
													(*i)->getRules()->getLabelLatitude()) == true)
			{
				eligibleCountries.push_back(*i);
			}
		}

		if (eligibleCountries.empty() == false)
		{
			//Log(LOG_INFO) << "AlienMission::think(), GAAH! new Pact & aLien base";
			const size_t pick = RNG::generate(
										0,
										eligibleCountries.size() - 1);
			Country* const infiltratedCountry = eligibleCountries.at(pick);
			// kL_note: Ironically, this likely allows multiple alien
			// bases in Russia solely because of infiltrations ...!!
			if (infiltratedCountry->getType() != "STR_RUSSIA") // heh.
				infiltratedCountry->setNewPact();
		}

		spawnAlienBase(
					globe,
					rules,
					_missionRule.getSpawnZone());
		// kL_note: well that's better.

		_nextWave = 0; // Infiltrations loop for ever.
	}

	if (_missionRule.getObjective() == OBJECTIVE_BASE
		&& _nextWave == _missionRule.getWaveTotal())
	{
		spawnAlienBase(
					globe,
					rules,
					_missionRule.getSpawnZone());
	}

	if (_nextWave != _missionRule.getWaveTotal())
	{
		const int spawnTime = static_cast<int>(_missionRule.getWave(_nextWave).spawnTimer) / 30;
		_spawnTime = static_cast<size_t>((spawnTime / 2) + RNG::generate(
																		0,
																		spawnTime)) * 30;
	}
}

/**
 * This function will spawn a UFO according the the mission rules.
 * Some code is duplicated between cases, that's ok for now. It's on different
 * code paths and the function is MUCH easier to read written this way.
 * @param rules		- reference the ruleset
 * @param globe			- reference the globe, for land checks
 * @param wave			- reference the wave for the desired UFO
 * @param trajectory	- reference the rule for the desired trajectory
 * @return, pointer to the spawned UFO; if the mission does not spawn a UFO return NULL
 */
Ufo* AlienMission::spawnUfo(
		const Ruleset& rules,
		const Globe& globe,
		const MissionWave& wave,
		const UfoTrajectory& trajectory)
{
	const RuleUfo& ufoRule = *rules.getUfo(wave.ufoType);
	std::pair<double, double> pos;
	Waypoint* wp;
	Ufo* ufo;

	if (_missionRule.getObjective() == OBJECTIVE_RETALIATION)
	{
		const RuleRegion& regRule = *rules.getRegion(_region);
		const std::vector<Base*>::const_iterator xcomBase = std::find_if(
																	_savedGame.getBases()->begin(),
																	_savedGame.getBases()->end(),
																	FindMarkedXCOMBase(regRule));
		if (xcomBase != _savedGame.getBases()->end())
		{
			// Spawn a battleship straight for the XCOM base.
			const RuleUfo& battleshipRule = *rules.getUfo(_missionRule.getSpawnUfo());
			const UfoTrajectory& trajAssault = *rules.getUfoTrajectory("__RETALIATION_ASSAULT_RUN");

			ufo = new Ufo(&battleshipRule);
			ufo->setMissionInfo(
							this,
							&trajAssault);

			if (trajectory.getAltitude(0) == "STR_GROUND")
				pos = getLandPoint(
								globe,
								regRule,
								trajectory.getZone(0));
			else
				pos = regRule.getRandomPoint(trajectory.getZone(0));

			ufo->setAltitude(trajAssault.getAltitude(0));
			ufo->setSpeed(static_cast<int>(std::ceil(
						  static_cast<double>(trajAssault.getSpeedPercentage(0))
						* static_cast<double>(battleshipRule.getMaxSpeed()))));
			ufo->setLongitude(pos.first);
			ufo->setLatitude(pos.second);

			wp = new Waypoint();
			wp->setLongitude((*xcomBase)->getLongitude());
			wp->setLatitude((*xcomBase)->getLatitude());

			ufo->setDestination(wp);

			return ufo;
		}
	}
	else if (_missionRule.getObjective() == OBJECTIVE_SUPPLY)
	{
		if (wave.objective == true
			&& _base == NULL)
		{
			return NULL; // No base to supply!
		}

		// destination is always an alien base.
		ufo = new Ufo(&ufoRule);
		ufo->setMissionInfo(
						this,
						&trajectory);
		const RuleRegion& regRule = *rules.getRegion(_region);

		if (trajectory.getAltitude(0) == "STR_GROUND")
			pos = getLandPoint(
							globe,
							regRule,
							trajectory.getZone(0));
		else
			pos = regRule.getRandomPoint(trajectory.getZone(0));

		ufo->setAltitude(trajectory.getAltitude(0));
		ufo->setSpeed(static_cast<int>(std::ceil(
					  static_cast<double>(trajectory.getSpeedPercentage(0))
					* static_cast<double>(ufoRule.getMaxSpeed()))));
		ufo->setLongitude(pos.first);
		ufo->setLatitude(pos.second);

		if (trajectory.getAltitude(1) == "STR_GROUND")
		{
			if (wave.objective == true)
			{
				// Supply ships on supply missions land on bases, ignore trajectory zone.
				pos.first = _base->getLongitude();
				pos.second = _base->getLatitude();
			}
			else // Other ships can land where they want.
				pos = getLandPoint(
								globe,
								regRule,
								trajectory.getZone(1));
		}
		else
			pos = regRule.getRandomPoint(trajectory.getZone(1));

		wp = new Waypoint();
		wp->setLongitude(pos.first);
		wp->setLatitude(pos.second);

		ufo->setDestination(wp);

		return ufo;
	}

	// Spawn according to sequence.
	ufo = new Ufo(&ufoRule);
	ufo->setMissionInfo(
					this,
					&trajectory);
	const RuleRegion& regRule = *rules.getRegion(_region);

	pos = getWaypoint(
					trajectory,
					0,
					regRule);

	ufo->setAltitude(trajectory.getAltitude(0));
	if (trajectory.getAltitude(0) == "STR_GROUND")
		ufo->setSecondsLeft(trajectory.groundTimer() * 5);

	ufo->setSpeed(static_cast<int>(std::ceil(
				  static_cast<double>(trajectory.getSpeedPercentage(0))
				* static_cast<double>(ufoRule.getMaxSpeed()))));
	ufo->setLongitude(pos.first);
	ufo->setLatitude(pos.second);

	pos = getWaypoint(
				trajectory,
				1,
				regRule);

	wp = new Waypoint();
	wp->setLongitude(pos.first);
	wp->setLatitude(pos.second);

	ufo->setDestination(wp);

	return ufo;
}

/**
 * Starts this AlienMission.
 * @param countdown - countdown till next UFO (default 0)
 */
void AlienMission::start(size_t countdown)
{
	_nextWave =
	_ufoCount =
	_liveUfos = 0;

	if (countdown == 0)
	{
		const int spawnTime = static_cast<int>(_missionRule.getWave(0).spawnTimer) / 30;
		_spawnTime = static_cast<size_t>((spawnTime / 2) + RNG::generate(
																	0,
																	spawnTime)) * 30;
	}
	else
		_spawnTime = countdown;
}


/**
 * @brief Match a base from its coordinates.
 * This function object uses coordinates to match a base.
 */
class MatchBaseCoordinates
	:
		public std::unary_function<const Base*, bool>
{

private:
	double
		_lon,
		_lat;

	public:
		/// Remember the query coordinates.
		MatchBaseCoordinates(
				double lon,
				double lat)
			:
				_lon(lon),
				_lat(lat)
		{}

		/// Match with base's coordinates.
		bool operator() (const Base* base) const
		{
			return AreSame(base->getLongitude(), _lon)
				&& AreSame(base->getLatitude(), _lat);
		}
};


/**
 * This function is called when one of the mission's UFOs arrives at its current
 * destination. It takes care of sending the UFO to the next waypoint, landing
 * UFOs and marking them for removal as required. It must set the game data in a
 * way that the rest of the code understands what to do.
 * @param ufo	- reference the Ufo that reached its waypoint
 * @param rules	- reference the Ruleset
 */
void AlienMission::ufoReachedWaypoint(
		Ufo& ufo,
		const Ruleset& rules)
{
	const size_t
		curWaypoint = ufo.getTrajectoryPoint(),
		nextWaypoint = curWaypoint + 1;
	const UfoTrajectory& trajectory = ufo.getTrajectory();

	if (nextWaypoint >= trajectory.getWaypointTotal()) // UFO leaves Earth's atmosphere
	{
		ufo.setDetected(false);
		ufo.setStatus(Ufo::DESTROYED);

		return;
	}

	ufo.setAltitude(trajectory.getAltitude(nextWaypoint));
	ufo.setTrajectoryPoint(nextWaypoint);

	const RuleRegion& regRule = *rules.getRegion(_region);
	const std::pair<double, double> pos = getWaypoint(
												trajectory,
												nextWaypoint,
												regRule);

	Waypoint* const wayPoint = new Waypoint();
	wayPoint->setLongitude(pos.first);
	wayPoint->setLatitude(pos.second);
	ufo.setDestination(wayPoint);

	if (ufo.getAltitude() != "STR_GROUND")
	{
		ufo.setLandId(0);
		ufo.setSpeed(static_cast<int>(std::ceil(
					 static_cast<double>(trajectory.getSpeedPercentage(nextWaypoint))
				   * static_cast<double>(ufo.getRules()->getMaxSpeed()))));
	}
	else // UFO landed.
	{
		size_t waveCount;
		if (_nextWave != 0) // ie. not Retaliation
			waveCount = _nextWave - 1;
		else
//			if (RNG::percent(static_cast<int>(_savedGame.getDifficulty()) * 20) == false)
			waveCount = _missionRule.getWaveTotal() - 1; // Retaliation starts over!!!

		const MissionWave& wave = _missionRule.getWave(waveCount);

		if (wave.objective == true // remove UFO, replace with MissionSite.
			&& trajectory.getZone(curWaypoint) == _missionRule.getSpawnZone())
		{
			addScore(
				ufo.getLongitude(),
				ufo.getLatitude());

			ufo.setStatus(Ufo::DESTROYED);

			const MissionArea area = regRule.getMissionPoint(
															trajectory.getZone(curWaypoint),
															&ufo);
			const RuleTexture* const texRule = rules.getGlobe()->getTextureRule(area.texture);
			// uses Globe-defined textures for Deployment; what happens to Deployment-defined Deployments:
			const AlienDeployment* const deployRule = rules.getDeployment(texRule->getTextureDeployment());

			MissionSite* const missionSite = new MissionSite(
														&_missionRule,
														deployRule);
			missionSite->setLongitude(ufo.getLongitude());
			missionSite->setLatitude(ufo.getLatitude());
			missionSite->setId(_savedGame.getId(deployRule->getMarkerName()));
			missionSite->setSecondsLeft(RNG::generate(
												deployRule->getDurationMin(),
												deployRule->getDurationMax()) * 3600);
			missionSite->setAlienRace(_race);
			missionSite->setSiteTextureInt(area.texture);
			missionSite->setCity(area.site);

			_savedGame.getMissionSites()->push_back(missionSite);

			for (std::vector<Target*>::const_iterator
					i = ufo.getFollowers()->begin();
					i != ufo.getFollowers()->end();
					)
			{
				Craft* const craft = dynamic_cast<Craft*>(*i);
				if (craft != NULL)
				{
					craft->setDestination(missionSite);
					i = ufo.getFollowers()->begin();
				}
				else
					++i;
			}
		}
		else if (trajectory.getID() == "__RETALIATION_ASSAULT_RUN")	// remove UFO, replace with Base defense.
		{															// Ignore what the trajectory might say, this is a base defense.
			ufo.setDetected(false);

			const std::vector<Base*>::const_iterator xcomBase = std::find_if(
																		_savedGame.getBases()->begin(),
																		_savedGame.getBases()->end(),
																		MatchBaseCoordinates(
																						ufo.getLongitude(),
																						ufo.getLatitude()));
			if (xcomBase == _savedGame.getBases()->end())
			{
				ufo.setStatus(Ufo::DESTROYED);
				return; // Only spawn mission if the base is still there.
			}

			ufo.setDestination(*xcomBase);
		}
		else // Set timer for UFO on the ground.
		{
			ufo.setSecondsLeft(trajectory.groundTimer() * 5);

			if (ufo.getDetected() == true
				&& ufo.getLandId() == 0)
			{
				ufo.setLandId(_savedGame.getId("STR_LANDING_SITE"));
			}
		}
	}
}

/**
 * This function is called when one of the mission's UFOs is shot down (crashed or destroyed).
 * Currently the only thing that happens is delaying the next UFO in the mission sequence.
 * @param ufo - reference the Ufo that was shot down
 */
void AlienMission::ufoShotDown(const Ufo& ufo)
{
	switch (ufo.getStatus())
	{
		case Ufo::FLYING:
		case Ufo::LANDED:
			assert(0 && "Ufo seems ok!");
		break;

		case Ufo::CRASHED:
		case Ufo::DESTROYED:
			if (_nextWave != _missionRule.getWaveTotal())
				_spawnTime += 30 * (static_cast<size_t>(RNG::generate(0,48)) + 400); // delay next wave
	}
}

/**
 * This function is called when one of the mission's UFOs has finished its time on the ground.
 * It takes care of sending the UFO to the next waypoint and marking them for removal as required.
 * It must set the game data in a way that the rest of the code understands what to do.
 * @param ufo	- reference the Ufo that reached its waypoint
 */
void AlienMission::ufoLifting(Ufo& ufo)
{
	//Log(LOG_INFO) << "AlienMission::ufoLifting()";
	switch (ufo.getStatus())
	{
		case Ufo::FLYING:
			ufo.setUfoTerrainType(""); // kL, safety i guess.
			assert(0 && "Ufo is already on the air!");
		break;

		case Ufo::LANDED:
			{
				//Log(LOG_INFO) << ". mission complete, addScore() + getTrajectory()";
				// base missions only get points when they are completed.
				if (_missionRule.getPoints() > 0
					&& _missionRule.getObjective() != OBJECTIVE_BASE)
//				if (_missionRule.getType() == "STR_ALIEN_HARVEST"
//					|| _missionRule.getType() == "STR_ALIEN_ABDUCTION"
//					|| _missionRule.getType() == "STR_ALIEN_TERROR")
				{
					addScore(
						ufo.getLongitude(),
						ufo.getLatitude());
				}

				ufo.setUfoTerrainType(""); // kL
				ufo.setAltitude("STR_VERY_LOW");
				ufo.setSpeed(static_cast<int>(std::ceil(
							 static_cast<double>(ufo.getTrajectory().getSpeedPercentage(ufo.getTrajectoryPoint()))
						   * static_cast<double>(ufo.getRules()->getMaxSpeed()))));
			}
		break;

		case Ufo::CRASHED: // Mission expired
			ufo.setDetected(false);
			ufo.setStatus(Ufo::DESTROYED);
		break;

		case Ufo::DESTROYED:
			assert(0 && "UFO can't fly!");
	}
}

/**
 * The new time must be a multiple of 30 minutes and more than 0.
 * Calling this on a finished mission has no effect.
 * @param minutes - the minutes until the next UFO wave will spawn
 */
void AlienMission::setWaveCountdown(size_t minutes)
{
	assert(minutes != 0 && minutes %30 == 0);

	if (isOver() == false)
		_spawnTime = minutes;
}

/**
 * Assigns a unique ID to this mission.
 * It is an error to assign two IDs to the same mission.
 * @param id - the ID to assign
 */
void AlienMission::setId(int id)
{
	assert(_uniqueID == 0 && "Reassigning ID!");
	_uniqueID = id;
}

/**
 * Gets the unique ID of this mission.
 * @return, the unique ID assigned to this mission
 */
int AlienMission::getId() const
{
	assert(_uniqueID != 0 && "Uninitalized mission!");
	return _uniqueID;
}

/**
 * Sets the alien base associated with this mission.
 * Only the alien supply missions care about this.
 * @param base - pointer to an AlienBase
 */
void AlienMission::setAlienBase(const AlienBase* const base)
{
	_base = base;
}

/**
 * Only alien supply missions ever have a valid pointer.
 * @return, pointer to the AlienBase for this mission (possibly NULL)
 */
const AlienBase* AlienMission::getAlienBase() const
{
	return _base;
}

/**
 * Adds alien points to the country and region at the coordinates given.
 * @param lon - longitudinal coordinates to check
 * @param lat - latitudinal coordinates to check
 */
void AlienMission::addScore(
		const double lon,
		const double lat)
{
	for (std::vector<Region*>::const_iterator
			i = _savedGame.getRegions()->begin();
			i != _savedGame.getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										lon,
										lat) == true)
		{
			(*i)->addActivityAlien(_missionRule.getPoints());
			(*i)->recentActivity();

			break;
		}
	}

	for (std::vector<Country*>::const_iterator
			i = _savedGame.getCountries()->begin();
			i != _savedGame.getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(
										lon,
										lat) == true)
		{
			(*i)->addActivityAlien(_missionRule.getPoints());
			(*i)->recentActivity();

			break;
		}
	}
}

/**
 * Spawns an AlienBase.
 * @param globe	- reference the Globe, required to get access to land checks
 * @param rules	- reference the Ruleset
 * @param zone	- the mission zone required for determining the base coordinates
 */
void AlienMission::spawnAlienBase(
		const Globe& globe,
		const Ruleset& rules,
		const size_t zone)
{
	if (_savedGame.getAlienBases()->size() > (8 + static_cast<size_t>(_savedGame.getDifficulty()) * 2))
		return;

	// Once the last UFO is spawned, the aliens build their base.
	const RuleRegion& regionRules = *rules.getRegion(_region);
	const std::pair<double, double> pos = getLandPoint(
													globe,
													regionRules,
													zone);

	AlienBase* const alienBase = new AlienBase();
	alienBase->setAlienRace(_race);
	alienBase->setId(_savedGame.getId("STR_ALIEN_BASE"));
	alienBase->setLongitude(pos.first);
	alienBase->setLatitude(pos.second);
	_savedGame.getAlienBases()->push_back(alienBase);

	addScore(
		pos.first,
		pos.second);
}

/**
 * Sets the mission's region. If the region is incompatible with actually
 * carrying out an attack, use the "fallback" region as defined in the ruleset.
 * This is a slight difference from the original,
 * which just defaulted them to zone[0], North America.
 * @param region	- reference the region we want to try to set the mission to
 * @param rules		- reference the ruleset, in case we need to swap out the region
 */
void AlienMission::setRegion(
		const std::string& region,
		const Ruleset& rules)
{
	if (rules.getRegion(region)->getMissionRegion().empty() == false)
		_region = rules.getRegion(region)->getMissionRegion();
	else
		_region = region;
}

/**
 * Selects a destination based on the criteria of our trajectory and desired waypoint.
 * @param trajectory	- reference the trajectory in question
 * @param nextWaypoint	- the next logical waypoint in sequence (0 for newly spawned UFOs)
 * @param region		- reference the ruleset for the region of this mission
 * @return, pair of lon and lat coordinates based on the criteria of the trajectory
 */
std::pair<double, double> AlienMission::getWaypoint(
		const UfoTrajectory& trajectory,
		const size_t nextWaypoint,
		const RuleRegion &region)
{
	return region.getRandomPoint(trajectory.getZone(nextWaypoint));
}

/**
 * Gets a random point inside the given region zone.
 * The point will be used to land a UFO, so it HAS to be on land.
 * @param globe		- reference the Globe
 * @param region	- reference RuleRegion
 * @param zone		- zone number in the region
 * @return, a pair of doubles (lon & lat)
 */
std::pair<double, double> AlienMission::getLandPoint(
		const Globe& globe,
		const RuleRegion& region,
		const size_t zone)
{
	int tries = 0;

	std::pair<double, double> pos;
	do
	{
		pos = region.getRandomPoint(zone);
		++tries;
	}
	while (tries < 100
		&& (globe.insideLand(
						pos.first,
						pos.second) == false
			|| region.insideRegion(
						pos.first,
						pos.second) == false));

	if (tries == 100)
	{
//		Log(LOG_DEBUG) << "Region: " << region.getType()
		Log(LOG_INFO) << "Region: " << region.getType()
			<< " Longitude: " << pos.first
			<< " Latitude: " << pos.second
			<< " invalid zone: " << zone << " ufo forced to land on water!";
	}

	return pos;
}

}
