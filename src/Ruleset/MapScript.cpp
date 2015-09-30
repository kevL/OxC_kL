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

#include "MapScript.h"

//#include <yaml-cpp/yaml.h>

#include "MapBlock.h"
#include "RuleTerrain.h"

//#include "../Engine/Exception.h"
//#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Constructs a MapScript.
 */
MapScript::MapScript()
	:
		_type(MSC_UNDEFINED),
		_sizeX(1),
		_sizeY(1),
		_sizeZ(0),
		_executionChance(100),
		_executions(1),
		_cumulativeFrequency(0),
		_label(0),
		_direction(MD_NONE),
		_tunnelData(NULL)
{}

/**
 * dTor.
 */
MapScript::~MapScript()
{
	for (std::vector<SDL_Rect*>::const_iterator
			i = _rects.begin();
			i != _rects.end();
			++i)
	{
		delete *i;
	}

	delete _tunnelData;
}

/**
 * Loads a map script command from YAML.
 * @param node - reference a YAML node
 */
void MapScript::load(const YAML::Node& node)
{
	if (const YAML::Node& mapNode = node["type"])
	{
		const std::string command = mapNode.as<std::string>("");

		if (command == "addBlock")
			_type = MSC_ADDBLOCK;
		else if (command == "addLine")
			_type = MSC_ADDLINE;
		else if (command == "addCraft")
		{
			_type = MSC_ADDCRAFT;
			_groups.push_back(1); // this is a default and can be overridden
		}
		else if (command == "addUFO")
		{
			_type = MSC_ADDUFO;
			_groups.push_back(1); // this is a default and can be overridden
		}
		else if (command == "digTunnel")
			_type = MSC_DIGTUNNEL;
		else if (command == "fillArea")
			_type = MSC_FILLAREA;
		else if (command == "checkBlock")
			_type = MSC_CHECKBLOCK;
		else if (command == "removeBlock")
			_type = MSC_REMOVE;
		else if (command == "resize")
		{
			_type = MSC_RESIZE;
			_sizeX =
			_sizeY = 0; // defaults: don't resize anything unless specified.
		}
		else
		{
			throw Exception("Unknown command: " + command);
		}
	}
	else
	{
		throw Exception("Missing command type.");
	}

	if (const YAML::Node& mapNode = node["rects"])
	{
		for (YAML::const_iterator
				i = mapNode.begin();
				i != mapNode.end();
				++i)
		{
			SDL_Rect* const rect = new SDL_Rect();
			rect->x = static_cast<Sint16>((*i)[0].as<int>()); // note: not sure if YAML can do a cast to S/Uint16's
			rect->y = static_cast<Sint16>((*i)[1].as<int>());
			rect->w = static_cast<Uint16>((*i)[2].as<int>());
			rect->h = static_cast<Uint16>((*i)[3].as<int>());

			_rects.push_back(rect);
		}
	}

	if (const YAML::Node& mapNode = node["tunnelData"])
	{
		_tunnelData = new TunnelData;
		_tunnelData->level = mapNode["level"].as<int>(0);

		if (const YAML::Node& data = mapNode["MCDReplacements"])
		{
			for (YAML::Node::const_iterator
					i = data.begin();
					i != data.end();
					++i)
			{
				MCDReplacement replacement;
				replacement.entry = (*i)["entry"].as<int>(-1);
				replacement.dataSet = (*i)["set"].as<int>(-1);

				const std::string type = (*i)["type"].as<std::string>("");
				_tunnelData->replacements[type] = replacement;
			}
		}
	}

	if (const YAML::Node& mapNode = node["conditionals"])
	{
		if (mapNode.Type() == YAML::NodeType::Sequence)
			_conditionals = mapNode.as<std::vector<int> >(_conditionals);
		else
			_conditionals.push_back(mapNode.as<int>(0));
	}

	if (const YAML::Node& mapNode = node["size"])
	{
		if (mapNode.Type() == YAML::NodeType::Sequence)
		{
			int
				entry = 0,
				*sizes[3] =
				{
					&_sizeX,
					&_sizeY,
					&_sizeZ
				};

			for (YAML::const_iterator
					i = mapNode.begin();
					i != mapNode.end();
					++i)
			{
				*sizes[entry] = (*i).as<int>(1);
				if (++entry == 3)
					break;
			}
		}
		else _sizeX = _sizeY = mapNode.as<int>(_sizeX);
	}

	if (const YAML::Node& mapNode = node["groups"])
	{
		_groups.clear();
		if (mapNode.Type() == YAML::NodeType::Sequence)
		{
			for (YAML::const_iterator
					i = mapNode.begin();
					i != mapNode.end();
					++i)
				_groups.push_back((*i).as<int>(0));
		}
		else _groups.push_back(mapNode.as<int>(0));
	}

	size_t selectionSize = _groups.size();
	if (const YAML::Node& mapNode = node["blocks"])
	{
		_groups.clear();
		if (mapNode.Type() == YAML::NodeType::Sequence)
		{
			for (YAML::const_iterator
					i = mapNode.begin();
					i != mapNode.end();
					++i)
				_blocks.push_back((*i).as<int>(0));
		}
		else _blocks.push_back(mapNode.as<int>(0));

		selectionSize = _blocks.size();
	}

	_frequencies.resize(selectionSize, 1);
	_maxUses.resize(selectionSize, -1);

	if (const YAML::Node& mapNode = node["freqs"])
	{
		if (mapNode.Type() == YAML::NodeType::Sequence)
		{
			size_t entry = 0;
			for (YAML::const_iterator
					i = mapNode.begin();
					i != mapNode.end();
					++i)
			{
				if (entry == selectionSize)
					break;

				_frequencies.at(entry++) = (*i).as<int>(1);
			}
		}
		else _frequencies.at(0) = mapNode.as<int>(1);
	}

	if (const YAML::Node& mapNode = node["maxUses"])
	{
		if (mapNode.Type() == YAML::NodeType::Sequence)
		{
			size_t entry = 0;
			for (YAML::const_iterator
					i = mapNode.begin();
					i != mapNode.end();
					++i)
			{
				if (entry == selectionSize)
					break;

				_maxUses.at(entry++) = (*i).as<int>(-1);
			}
		}
		else _maxUses.at(0) = mapNode.as<int>(-1);
	}

	if (const YAML::Node& mapNode = node["direction"])
	{
		std::string dir = mapNode.as<std::string>("");
		if (dir.length() != 0)
		{
			std::transform(
						dir.begin(),
						dir.end(),
						dir.begin(),
						::toupper);

			if (dir.substr(0, 1) == "V")
				_direction = MD_VERTICAL;
			else if (dir.substr(0, 1) == "H")
				_direction = MD_HORIZONTAL;
			else if (dir.substr(0, 1) == "B")
				_direction = MD_BOTH;
			else
			{
				throw Exception("direction must be [V]ertical, [H]orizontal, or [B]oth - found [" + dir + "]");
			}
		}
	}

	if (_direction == MD_NONE)
	{
		if (_type == MSC_DIGTUNNEL)
		{
			throw Exception("no direction defined for dig tunnel command, must be [V]ertical, [H]orizontal, or [B]oth");
		}
		else if (_type == MSC_ADDLINE)
		{
			throw Exception("no direction defined for add line command, must be [V]ertical, [H]orizontal, or [B]oth");
		}
	}

	_executionChance	= node["executionChance"]	.as<int>(_executionChance);
	_executions			= node["executions"]		.as<int>(_executions);
	_ufoType			= node["ufoType"]			.as<std::string>(_ufoType);
	_label				= std::abs(node["label"]	.as<int>(_label)); // take no chances, don't accept negative values here.
}

/**
 * Initializes all the various scratch values and such for the command.
 */
void MapScript::init()
{
	_cumulativeFrequency = 0;
	_blocksTemp.clear();
	_groupsTemp.clear();
	_frequenciesTemp.clear();
	_maxUsesTemp.clear();

	for (std::vector<int>::const_iterator
			i = _frequencies.begin();
			i != _frequencies.end();
			++i)
	{
		_cumulativeFrequency += *i;
	}

	_blocksTemp = _blocks;
	_groupsTemp = _groups;
	_frequenciesTemp = _frequencies;
	_maxUsesTemp = _maxUses;
}

/**
 * Gets a random group number from the array accounting for frequencies and max uses.
 * @note If no groups or blocks are defined this will return the default group;
 * if all the max uses are used up it will return undefined.
 * @return, group number
 */
int MapScript::getGroupNumber() // private.
{
	if (_groups.size() == 0)
		return MBT_DEFAULT;

	if (_cumulativeFrequency > 0)
	{
		int pick = RNG::generate(0,
							_cumulativeFrequency - 1);

		for (size_t
				i = 0;
				i != _groupsTemp.size();
				++i)
		{
			if (pick < _frequenciesTemp.at(i))
			{
				const int ret = _groupsTemp.at(i);

				if (_maxUsesTemp.at(i) > 0)
				{
					if (--_maxUsesTemp.at(i) == 0)
					{
						_groupsTemp.erase(_groupsTemp.begin() + i);
						_cumulativeFrequency -= _frequenciesTemp.at(i);
						_frequenciesTemp.erase(_frequenciesTemp.begin() + i);
						_maxUsesTemp.erase(_maxUsesTemp.begin() + i);
					}
				}

				return ret;
			}

			pick -= _frequenciesTemp.at(i);
		}
	}

	return MBT_UNDEFINED;
}

/**
 * Gets a random block number from the array accounting for frequencies and max uses.
 * @note If no blocks are defined it will use a group instead.
 * @return, block number
 */
int MapScript::getBlockNumber() // private.
{
	if (_cumulativeFrequency > 0)
	{
		int pick = RNG::generate(0,
							_cumulativeFrequency - 1);

		for (size_t
				i = 0;
				i != _blocksTemp.size();
				++i)
		{
			if (pick < _frequenciesTemp.at(i))
			{
				const int ret = _blocksTemp.at(i);

				if (_maxUsesTemp.at(i) > 0)
				{
					if (--_maxUsesTemp.at(i) == 0)
					{
						_blocksTemp.erase(_blocksTemp.begin() + i);
						_cumulativeFrequency -= _frequenciesTemp.at(i);
						_frequenciesTemp.erase(_frequenciesTemp.begin() + i);
						_maxUsesTemp.erase(_maxUsesTemp.begin() + i);
					}
				}

				return ret;
			}

			pick -= _frequenciesTemp.at(i);
		}
	}

	return MBT_UNDEFINED;
}

/**
 * Gets a random map block from a given terrain using either the groups or the blocks defined.
 * @param terraRule - pointer to the RuleTerrain to pick a block from
 * @return, pointer to a randomly chosen MapBlock given the options available in this MapScript
 */
MapBlock* MapScript::getNextBlock(RuleTerrain* const terraRule)
{
	if (_blocks.empty() == true)
		return terraRule->getRandomMapBlock(
										_sizeX * 10,
										_sizeY * 10,
										getGroupNumber());

	const int result = getBlockNumber();
	if (result < static_cast<int>(terraRule->getMapBlocks()->size())
		&& result != MBT_UNDEFINED)
	{
		return terraRule->getMapBlocks()->at(static_cast<size_t>(result));
	}

	return NULL;
}

/**
 * Gets the type of the UFO for the case of "setUFO".
 * @return, UFO type
 */
std::string MapScript::getUfoType()
{
	return _ufoType;
}

}
