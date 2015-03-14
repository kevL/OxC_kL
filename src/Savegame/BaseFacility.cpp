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

#include "BaseFacility.h"

#include "Base.h"

#include "../Ruleset/RuleBaseFacility.h"


namespace OpenXcom
{

/**
 * Initializes a base facility of the specified type.
 * @param rules	- pointer to RuleBaseFacility
 * @param base	- pointer to the Base of this facility
 */
BaseFacility::BaseFacility(
		RuleBaseFacility* rules,
		Base* base)
	:
		_rules(rules),
		_base(base),
		_x(-1),
		_y(-1),
		_buildTime(0),
		_craft(NULL)
{}

/**
 * dTor.
 */
BaseFacility::~BaseFacility()
{}

/**
 * Loads the base facility from a YAML file.
 * @param node - reference a YAML node
 */
void BaseFacility::load(const YAML::Node& node)
{
	_x = node["x"].as<int>(_x);
	_y = node["y"].as<int>(_y);

	_buildTime = node["buildTime"].as<int>(_buildTime);
}

/**
 * Saves the base facility to a YAML file.
 * @return, YAML node
 */
YAML::Node BaseFacility::save() const
{
	YAML::Node node;

	node["type"]	= _rules->getType();
	node["x"]		= _x;
	node["y"]		= _y;

	if (_buildTime != 0)
		node["buildTime"] = _buildTime;

	return node;
}

/**
 * Returns the ruleset for this BaseFacility's type.
 * @return, pointer to RuleBaseFacility
 */
RuleBaseFacility* BaseFacility::getRules() const
{
	return _rules;
}

/**
 * Returns this BaseFacility's X position on the base grid.
 * @return, X position in grid squares
 */
int BaseFacility::getX() const
{
	return _x;
}

/**
 * Changes this BaseFacility's X position on the base grid.
 * @param x - X position in grid squares
 */
void BaseFacility::setX(int x)
{
	_x = x;
}

/**
 * Returns this BaseFacility's Y position on the base grid.
 * @return, Y position in grid squares
 */
int BaseFacility::getY() const
{
	return _y;
}

/**
 * Changes this BaseFacility's Y position on the base grid.
 * @param y - Y position in grid squares
 */
void BaseFacility::setY(int y)
{
	_y = y;
}

/**
 * Returns this BaseFacility's remaining time until it's finished building.
 * @return, time left in days (0 = complete)
 */
int BaseFacility::getBuildTime() const
{
	return _buildTime;
}

/**
 * Changes this BaseFacility's remaining time until it's finished building.
 * @param buildTime - time left in days
 */
void BaseFacility::setBuildTime(int buildTime)
{
	_buildTime = buildTime;
}

/**
 * Handles the facility building each day.
 */
void BaseFacility::build()
{
	--_buildTime;
}

/**
 * Returns if this BaseFacility is currently being used by its base.
 * @return, true if in use
 */
bool BaseFacility::inUse() const
{
	if (_buildTime != 0)
		return false;

	return (_rules->getPersonnel() > 0
			&& _base->getAvailableQuarters() - _rules->getPersonnel() < _base->getUsedQuarters())
		|| (_rules->getStorage() > 0
			&& _base->getAvailableStores() - _rules->getStorage() < _base->getUsedStores())
		|| (_rules->getLaboratories() > 0
			&& _base->getAvailableLaboratories() - _rules->getLaboratories() < _base->getUsedLaboratories())
		|| (_rules->getWorkshops() > 0
			&& _base->getAvailableWorkshops() - _rules->getWorkshops() < _base->getUsedWorkshops())
		|| (_rules->getCrafts() > 0
			&& _base->getAvailableHangars() - _rules->getCrafts() < _base->getUsedHangars())
		|| (_rules->getPsiLaboratories() > 0
			&& _base->getAvailablePsiLabs() - _rules->getPsiLaboratories() < _base->getUsedPsiLabs())
		|| (_rules->getAliens() > 0
			&& _base->getAvailableContainment() - _rules->getAliens() < _base->getUsedContainment());
}

/**
 * Gets craft used for drawing in facility.
 * @return, pointer to the Craft
 */
const Craft* const BaseFacility::getCraft() const
{
	return _craft;
}

/**
 * Sets craft used for drawing facility.
 * @param craft - pointer to the craft
 */
void BaseFacility::setCraft(const Craft* const craft)
{
	_craft = craft;
}

}
