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
	static bool decode(
			const Node& node,
			OpenXcom::RuleGender& rhs)
	{
		if (node.IsSequence() == false
			|| node.size() != 2)
		{
			return false;
		}

		rhs.male	= node[0].as<int>();
		rhs.female	= node[1].as<int>();

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a type of Soldier.
 * @param type - reference to type
 */
RuleSoldier::RuleSoldier(const std::string& type)
	:
		_type(type),
		_standHeight(0),
		_kneelHeight(0),
		_floatHeight(0),
		_genderRatio(),
		_costBuy(0),
		_costSalary(0)
{}

/**
 * dTor.
 */
RuleSoldier::~RuleSoldier()
{}

/**
 * Loads the Soldier ruleset from a YAML file.
 * @param node - reference the YAML node
 */
void RuleSoldier::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_requires		= node["requires"]		.as<std::vector<std::string> >(_requires);
	_armor			= node["armor"]			.as<std::string>(_armor);
	_costBuy		= node["costBuy"]		.as<int>(_costBuy);
	_costSalary		= node["costSalary"]	.as<int>(_costSalary);
	_standHeight	= node["standHeight"]	.as<int>(_standHeight);
	_kneelHeight	= node["kneelHeight"]	.as<int>(_kneelHeight);
	_floatHeight	= node["floatHeight"]	.as<int>(_floatHeight);
	_genderRatio	= node["genderRatio"]	.as<RuleGender>(_genderRatio);

	_minStats.mergeStats(node["minStats"].as<UnitStats>(_minStats));
	_maxStats.mergeStats(node["maxStats"].as<UnitStats>(_maxStats));
	_statCaps.mergeStats(node["statCaps"].as<UnitStats>(_statCaps));

//	_femaleFrequency = node["femaleFrequency"].as<int>(_femaleFrequency);
}

/**
 * Returns this Soldier type.
 * @return, type
 */
const std::string& RuleSoldier::getType() const
{
	return _type;
}

/**
 * Gets the list of research required to acquire this type of Soldier.
 * @return, reference to the list of research IDs
 */
const std::vector<std::string>& RuleSoldier::getRequirements() const
{
	return _requires;
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
 * Gets the cost of hiring this type of Soldier.
 * @return, cost
 */
int RuleSoldier::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the cost of this type of Soldier's salary for a month.
 * @return, cost
 */
int RuleSoldier::getSalaryCost() const
{
	return _costSalary;
}

/**
 * Gets the height of this type of Soldier when it's standing.
 * @return, the standing height
 */
int RuleSoldier::getStandHeight() const
{
	return _standHeight;
}

/**
 * Gets the height of this type of Soldier when it's kneeling.
 * @return, the kneeling height
 */
int RuleSoldier::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Gets the elevation of this type of Soldier when it's flying.
 * @return, the floating height
 */
int RuleSoldier::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets the armor issued to this type of Soldier by default.
 * @return, the armor name
 */
const std::string& RuleSoldier::getArmor() const
{
	return _armor;
}

/**
 * Gets the gender ratio struct.
 * @return, pointer to the RuleGender struct
 */
const RuleGender* const RuleSoldier::getGenderRatio() const
{
	return &_genderRatio;
}

/*
 * Gets the female appearance ratio.
 * @return, percent chance of a female soldier getting generated
 *
int RuleSoldier::getFemaleFrequency() const
{
	return _femaleFrequency;
} */

}
