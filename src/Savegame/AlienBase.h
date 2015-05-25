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

#ifndef OPENXCOM_ALIENBASE_H
#define OPENXCOM_ALIENBASE_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

/**
 * Represents an alien base on the world.
 */
class AlienBase
	:
		public Target
{

private:
	bool
		_discovered,
		_inTactical;
	int _id;

	std::string
		_edit,
		_race;


	public:
		/// Creates an alien base.
		AlienBase();
		/// Cleans up the alien base.
		~AlienBase();

		/// Loads the alien base from YAML.
		void load(const YAML::Node& node);
		/// Saves the alien base to YAML.
		YAML::Node save() const;
		/// Saves the alien base's ID to YAML.
		YAML::Node saveId() const;

		/// Gets the alien base's ID.
		int getId() const;
		/// Sets the alien base's ID.
		void setId(int id);

		/// Gets the alien base's name.
		std::wstring getName(const Language* const lang) const;
		/// Gets the alien base's marker.
		int getMarker() const;

		/// Gets the alien base's alien race.
		std::string getAlienRace() const;
		/// Sets the alien base's alien race.
		void setAlienRace(const std::string& race);

		/// Returns textedit that the player has entered.
		std::string getLabel() const;
		/// Changes textedit that the player has entered.
		void setLabel(const std::string& edit);

		/// Sets the alien base's battlescape status.
		void setInBattlescape(bool inTactical);
		/// Gets the alien base's battlescape status.
		bool isInBattlescape() const;

		/// Gets the alien base's discovered status.
		bool isDiscovered() const;
		/// Sets the alien base's discovered status.
		void setDiscovered(bool discovered);
};

}

#endif
