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
#ifndef OPENXCOM_EXTRAMUSIC_H
#define OPENXCOM_EXTRAMUSIC_H

#include <yaml-cpp/yaml.h>

namespace OpenXcom
{

/**
 * For adding extra music to the game.
 */
class ExtraMusic
{
private:
	std::string _media;
	std::string _overrides;
	std::string _extends;
	std::vector<std::string> _terrains;
	int _modIndex;
public:
	/// Creates a blank external music set.
	ExtraMusic();
	/// Cleans up the external music set.
	virtual ~ExtraMusic();
	/// Loads the data from yaml
	void load(const YAML::Node &node, int modIndex);
	/// get the mod index for this external music set.
	int getModIndex();
	std::string getOverridden();
	std::string getExtended();
	bool hasTerrainSpecification();
	std::vector<std::string> getTerrains();
};

}

#endif
