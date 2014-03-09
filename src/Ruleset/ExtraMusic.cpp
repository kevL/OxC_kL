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

#include "ExtraMusic.h"

namespace OpenXcom
{

/**
 * Creates a blank set of extra music data.
 */
ExtraMusic::ExtraMusic() : _media(""), _overrides(""), _extends(""), _modIndex(0)
{
}

/**
 * Cleans up the extra music set.
 */
ExtraMusic::~ExtraMusic()
{
}

/**
 * Loads the extra music set from YAML.
 * @param node YAML node.
 * @param modIndex The internal index of the associated mod.
 */
void ExtraMusic::load(const YAML::Node &node, int modIndex)
{
	_media = node["media"].as<std::string>(_media);
	if (node["overrides"])
	  _overrides = node["overrides"].as<std::string>(_overrides);
	if (node["extends"])
	  _extends = node["extends"].as<std::string>(_extends);  
	_terrains = node["terrain"].as< std::vector<std::string> >(_terrains);
	_modIndex = modIndex;
}

/**
 * Gets the mod index for this external music set.
 * @return The mod index for this external music set.
 */
int ExtraMusic::getModIndex()
{
	return _modIndex;
}

std::string ExtraMusic::getOverridden()
{
  return _overrides;
}
std::string ExtraMusic::getExtended()
{
  return _extends;
}
bool ExtraMusic::hasTerrainSpecification()
{
  return !_terrains.empty();
}
std::vector<std::string> ExtraMusic::getTerrains()
{
  return _terrains;
}
	
}
