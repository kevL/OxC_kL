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

#ifndef OPENXCOM_MAPSCRIPT_H
#define OPENXCOM_MAPSCRIPT_H

#include <vector>
#include <string>

#include <SDL_video.h>
#include <yaml-cpp/yaml.h>

#include "MapBlock.h"
#include "RuleTerrain.h"


namespace OpenXcom
{

enum MapDirection
{
	MD_NONE,		// 0
	MD_VERTICAL,	// 1
	MD_HORIZONTAL,	// 2
	MD_BOTH			// 3
};

struct MCDReplacement
{
	int
		dataSet,
		entry;
};

struct TunnelData
{
	int level;
	std::map<std::string, MCDReplacement> replacements;

	///
	MCDReplacement* getMCDReplacement(std::string type)
	{
		if (replacements.find(type) == replacements.end())
			return NULL;

		return &replacements[type];
	}
};

enum MapScriptCommand
{
	MSC_UNDEFINED = -1,	// -1
	MSC_ADDBLOCK,		//  0
	MSC_ADDLINE,		//  1
	MSC_ADDCRAFT,		//  2
	MSC_ADDUFO,			//  3
	MSC_DIGTUNNEL,		//  4
	MSC_FILLAREA,		//  5
	MSC_CHECKBLOCK,		//  6
	MSC_REMOVE,			//  7
	MSC_RESIZE			//  8
};


class MapBlock;
class RuleTerrain;


class MapScript
{
private:
	int
		_sizeX,
		_sizeY,
		_sizeZ,
		_executionChances,
		_executions,
		_cumulativeFrequency,
		_label;

	MapDirection _direction;
	MapScriptCommand _type;
	TunnelData* _tunnelData;

	std::vector<SDL_Rect*> _rects;
	std::vector<int>
		_blocks,
		_blocksTemp,
		_conditionals,
		_frequencies,
		_frequenciesTemp,
		_groups,
		_groupsTemp,
		_maxUses,
		_maxUsesTemp;

	/// Randomly generate a group from within the array.
	const int getGroupNumber();

	/// Randomly generate a block number from within the array.
	const int getBlockNumber();


	public:
		/// Constructs a MapScript.
		MapScript();
		/// Destructs this MapScript.
		~MapScript();

		/// Loads information from a ruleset file.
		void load(const YAML::Node& node);

		/// Initializes all the variables and junk for a MapScript command.
		void init();

		/// Gets what type of command this is.
		MapScriptCommand getType()
		{
			return _type;
		};

		/// Gets the rects, describing the areas this command applies to.
		const std::vector<SDL_Rect*>* getRects() const
		{
			return &_rects;
		};

		/// Gets the X size for this command.
		const int getSizeX() const
		{
			return _sizeX;
		};
		/// Gets the Y size for this command.
		const int getSizeY() const
		{
			return _sizeY;
		};
		/// Gets the Z size for this command.
		const int getSizeZ() const
		{
			return _sizeZ;
		};

			/// Get the chances of this command executing.
		const int getChancesOfExecution() const
		{
			return _executionChances;
		};

		/// Gets the label for this command.
		const int getLabel() const
		{
			return _label;
		};

		/// Gets how many times this command repeats (1 repeat means 2 executions)
		const int getExecutions() const
		{
			return _executions;
		};

		/// Gets what conditions apply to this command.
		const std::vector<int>* getConditionals() const
		{
			return &_conditionals;
		};

		/// Gets the groups vector for iteration.
		const std::vector<int>* getGroups() const
		{
			return &_groups;
		};
		/// Gets the blocks vector for iteration.
		const std::vector<int>* getBlocks() const
		{
			return &_blocks;
		};

		/// Gets the direction this command goes (for lines and tunnels).
		const MapDirection getDirection() const
		{
			return _direction;
		};

		/// Gets the mcd replacement data for tunnel replacements.
		TunnelData* getTunnelData()
		{
			return _tunnelData;
		};

		/// Randomly generate a block from within either the array of groups or blocks.
		MapBlock* getNextBlock(RuleTerrain* terrain);
};

}

#endif
