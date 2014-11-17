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

#include "RuleMusic.h"


namespace OpenXcom
{

/**
 * cTor
 */
RuleMusic::RuleMusic()
	:
//		_type(""),
//		_mode(""),
		_midiIndex(-1)
//		_indexes(),
//		_files(),
//		_terrains()
{
}

/**
 * dTor
 */
RuleMusic::~RuleMusic()
{
}

/**
 *
 */
void RuleMusic::load(const YAML::Node& node)
{
	_type = node["type"].as<std::string>(_type);

	if (node["mode"])
		_mode = node["mode"].as<std::string>(_mode);
	else
		_mode = "replace";

	if (node["midiPack"])
		_midiIndex = node["midiPack"].as<int>(_midiIndex);
	else
		_midiIndex = -1;

	_terrains	= node["terrain"]	.as<std::vector<std::string> >(_terrains);
	_files		= node["files"]		.as<std::vector<std::string> >(_files);
	_indexes	= node["indexes"]	.as<std::vector<int> >(_indexes);

	if (_terrains.empty() == true)
		_terrains.push_back("");

	if (_files.empty() == true)
		_files.push_back(_type);

	if (_indexes.empty() == true)
		_indexes.push_back(_midiIndex);

	while (_indexes.size() < _files.size())
		_indexes.push_back(-1);
}

/**
 *
 */
std::string RuleMusic::getMode() const
{
	return _mode;
}

/**
 *
 */
int RuleMusic::getMidiIndex() const
{
	return _midiIndex;
}

/**
 *
 */
std::vector<std::string> RuleMusic::getTerrains() const
{
	return _terrains;
}

/**
 *
 */
std::vector<std::string> RuleMusic::getFiles() const
{
	return _files;
}

/**
 *
 */
std::vector<int> RuleMusic::getIndexes() const
{
	return _indexes;
}

}
