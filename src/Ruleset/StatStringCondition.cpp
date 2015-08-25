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

//#define _USE_MATH_DEFINES

#include "StatStringCondition.h"


namespace OpenXcom
{

/**
 * Creates a blank StatStringCondition.
 * @param conditionName	- reference the name of the condition
 * @param minVal		- minimum value
 * @param maxVal		- maximum value
 */
StatStringCondition::StatStringCondition(
		const std::string& conditionName,
		int minVal,
		int maxVal)
	:
		_conditionName(conditionName),
		_minVal(minVal),
		_maxVal(maxVal)
{}

/**
 * Cleans up the extra StatStringCondition.
 */
StatStringCondition::~StatStringCondition()
{}

/**
 * Gets the condition string.
 * @return, name of the associated stat
 */
std::string StatStringCondition::getConditionName()
{
	return _conditionName;
}

/**
 * Gets the minimum value for the condition (default is 0).
 * @return, the minimum value
 */
int StatStringCondition::getMinVal()
{
	return _minVal;
}

/**
 * Gets the maximum value for the condition (default is 255).
 * @return, the maximum value
 */
int StatStringCondition::getMaxVal()
{
	return _maxVal;
}

}
