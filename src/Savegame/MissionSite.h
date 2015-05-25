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

#ifndef OPENXCOM_MISSIONSITE_H
#define OPENXCOM_MISSIONSITE_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

class AlienDeployment;
class RuleAlienMission;


/**
 * Represents an alien MissionSite in the world.
 */
class MissionSite
	:
		public Target
{

private:
	bool _inTactical;
	int
		_id,
		_secondsLeft,
		_texture;
	std::string
		_city,
		_race,
		_terrain;

	const AlienDeployment* _deployment;
	const RuleAlienMission* _missionRule;


	public:
		/// Creates this MissionSite.
		MissionSite(
				const RuleAlienMission* rules,
				const AlienDeployment* deployment);
		/// Cleans up this MissionSite.
		~MissionSite();

		/// Loads this MissionSite from YAML.
		void load(const YAML::Node& node);
		/// Saves this MissionSite to YAML.
		YAML::Node save() const;
		/// Saves this MissionSite's ID to YAML.
		YAML::Node saveId() const;

		/// Gets this MissionSite's ruleset.
		const RuleAlienMission* getRules() const;
		/// Gets this MissionSite's deployment.
		const AlienDeployment* getDeployment() const;

		/// Gets this MissionSite's ID.
		int getId() const;
		/// Sets this MissionSite's ID.
		void setId(const int id);

		/// Gets this MissionSite's name.
		std::wstring getName(const Language* const lang) const;

		/// Gets this MissionSite site's marker.
		int getMarker() const;

		/// Gets the seconds until this MissionSite expires.
		int getSecondsLeft() const;
		/// Sets the seconds until this MissionSite expires.
		void setSecondsLeft(int sec);

		/// Sets this MissionSite's battlescape status.
		void setInBattlescape(bool inTactical);
		/// Gets if this MissionSite is in battlescape.
		bool isInBattlescape() const;

		/// Gets this MissionSite's alien race.
		std::string getAlienRace() const;
		/// Sets this MissionSite's alien race.
		void setAlienRace(const std::string& race);

		/// Gets this MissionSite's terrainType.
		std::string getSiteTerrainType() const;
		/// Sets this MissionSite's terrainType.
		void setSiteTerrainType(const std::string& terrain);

		/// Gets this MissionSite's texture.
		int getSiteTextureInt() const;
		/// Sets this MissionSite's texture.
		void setSiteTextureInt(int texture);

		/// Gets this MissionSite's city.
		std::string getCity() const;
		/// Sets this MissionSite's city.
		void setCity(const std::string& city);
};

}

#endif
