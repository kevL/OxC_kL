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

#ifndef OPENXCOM_RULESOLDIER_H
#define OPENXCOM_RULESOLDIER_H

//#include <string>

#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

struct RuleGender
{
	int
		male,
		female;
};


/**
 * Represents the creation data for a specific type of unit.
 * @note This info is copied to either Soldier for XCom soldiers or BattleUnit
 * for aliens and civilians.
 * @sa Soldier BattleUnit
 */
class RuleSoldier
{

private:
	int
		_costBuy,
		_costSalary,
		_floatHeight,
		_kneelHeight,
		_standHeight;
//		_femaleFrequency;

	std::string
		_armor,
		_type;

	UnitStats
		_minStats,
		_maxStats,
		_statCaps;
	RuleGender _genderRatio;


	public:
		/// Creates a blank unit ruleset.
		explicit RuleSoldier(const std::string& type);
		/// Cleans up the unit ruleset.
		~RuleSoldier();

		/// Loads the unit data from YAML.
		void load(const YAML::Node& node);

		/// Gets the unit's type.
		const std::string& getType() const;

		/// Gets the minimum stats for the random stats generator.
		UnitStats getMinStats() const;
		/// Gets the maximum stats for the random stats generator.
		UnitStats getMaxStats() const;
		/// Gets the stat caps.
		UnitStats getStatCaps() const;

		/// Gets the cost of this type of Soldier.
		int getBuyCost() const;
		/// Gets the monthly salary of this type of Soldier.
		int getSalaryCost() const;

		/// Gets the height of the soldier when it's standing.
		int getStandHeight() const;
		/// Gets the height of the soldier when it's kneeling.
		int getKneelHeight() const;
		/// Gets the elevation of the soldier when it's flying.
		int getFloatHeight() const;

		/// Gets the armor issued to this type of soldier.
		const std::string& getArmor() const;

		/// Gets the gender ratio struct.
		const RuleGender* const getGenderRatio() const;
		/// Gets the female appearance ratio.
//		int getFemaleFrequency() const;
};

}

#endif
