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

#include "OperationPool.h"

//#include <fstream>
//#include <iostream>
//#include <sstream>

//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Language.h"
//#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Initializes a new pool with blank lists of titles.
 */
OperationPool::OperationPool()
{}

/**
 * dTor.
 */
OperationPool::~OperationPool()
{}

/**
 * Loads the pool from a YAML file.
 * @param filename - reference a YAML file
 */
void OperationPool::load(const std::string& filename)
{
	const std::string st = CrossPlatform::getDataFile("SoldierName/" + filename + ".opr");
	const YAML::Node doc = YAML::LoadFile(st);

	for (YAML::const_iterator
			i = doc["operaFirst"].begin();
			i != doc["operaFirst"].end();
			++i)
	{
		const std::wstring adj = Language::utf8ToWstr(i->as<std::string>());
		_operaFirst.push_back(adj);
	}

	for (YAML::const_iterator
			i = doc["operaLast"].begin();
			i != doc["operaLast"].end();
			++i)
	{
		const std::wstring noun = Language::utf8ToWstr(i->as<std::string>());
		_operaLast.push_back(noun);
	}
}

/**
 * Returns a new random title (adj + noun) from the lists of words contained within.
 * @return, the operation title
 */
std::wstring OperationPool::genOperation() const
{
	std::wostringstream title;

	if (_operaFirst.empty() == false)
	{
		const size_t adj = RNG::generate(
									0,
									_operaFirst.size() - 1);
		title << L"op." << _operaFirst[adj];
	}

	if (_operaLast.empty() == false)
	{
		const size_t noun = RNG::generate(
									0,
									_operaLast.size() - 1);
		title << L"." << _operaLast[noun];
	}

	return title.str();
}

}
