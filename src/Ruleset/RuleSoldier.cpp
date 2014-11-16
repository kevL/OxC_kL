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

#include "RuleSoldier.h"


namespace YAML
{

template<>
struct convert<OpenXcom::RuleGender>
{
	///
	static Node encode(const OpenXcom::RuleGender& rhs)
	{
		Node node;

		node.push_back(rhs.male);
		node.push_back(rhs.female);

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::RuleGender& rhs)
	{
		if (!node.IsSequence() || node.size() != 2)
			return false;

		rhs.male = node[0].as<int>();
		rhs.female = node[1].as<int>();

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleunit for a certain type of soldier.
 * NOTE: only type is "XCOM".
 * @param type - reference defining a type
 */
RuleSoldier::RuleSoldier(const std::string& type)
	:
		_type(type),
		_standHeight(0),
		_kneelHeight(0),
		_floatHeight(0)
{
}

/**
 * dTor.
 */
RuleSoldier::~RuleSoldier()
{
}

/**
 * Loads the unit from a YAML file.
 * @param node - reference the YAML node
 */
void RuleSoldier::load(const YAML::Node& node)
{
	_type			= node["type"]				.as<std::string>(_type);
	_minStats		.mergeStats(node["minStats"].as<UnitStats>(_minStats));
	_maxStats		.mergeStats(node["maxStats"].as<UnitStats>(_maxStats));
	_statCaps		.mergeStats(node["statCaps"].as<UnitStats>(_statCaps));
	_armor			= node["armor"]				.as<std::string>(_armor);
	_standHeight	= node["standHeight"]		.as<int>(_standHeight);
	_kneelHeight	= node["kneelHeight"]		.as<int>(_kneelHeight);
	_floatHeight	= node["floatHeight"]		.as<int>(_floatHeight);
	_genderRatio	= node["genderRatio"]		.as<RuleGender>(_genderRatio);

//	_femaleFrequency = node["femaleFrequency"].as<int>(_femaleFrequency);
}

/**
 * Returns the language string that names this unit.
 * Each unit type has a unique name.
 * @return, name
 */
std::string RuleSoldier::getType() const
{
	return _type;
}

/**
 * Gets the minimum stats for the random stats generator.
 * @return, the minimum stats
 */
UnitStats RuleSoldier::getMinStats() const
{
	return _minStats;
}

/**
 * Gets the maximum stats for the random stats generator.
 * @return, the maximum stats
 */
UnitStats RuleSoldier::getMaxStats() const
{
	return _maxStats;
}

/**
 * Gets the stat caps.
 * @return, the stat caps
 */
UnitStats RuleSoldier::getStatCaps() const
{
	return _statCaps;
}

/**
 * Gets the height of the soldier when it's standing.
 * @return, the standing height
 */
int RuleSoldier::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets the height of the soldier when it's kneeling.
 * @return, the kneeling height
 */
int RuleSoldier::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets the elevation of the soldier when it's flying.
 * @return, the floating height
 */
int RuleSoldier::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets the armor name.
 * @return, the armor name
 */
std::string RuleSoldier::getArmor() const
{
	return _armor;
}

/** kL. Gets the gender ratio struct.
 * @return, the RuleGender struct
 */
RuleGender RuleSoldier::getGenderRatio() const // kL
{
	return _genderRatio;
}

/**
 * Gets the female appearance ratio.
 * @return, percent chance of a female soldier getting generated
 */
/* int RuleSoldier::getFemaleFrequency() const
{
	return _femaleFrequency;
} */

}
