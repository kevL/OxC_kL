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

#ifndef OPENXCOM_RULEAWARD_H
#define OPENXCOM_RULEAWARD_H

//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Represents a specific type of Award.
 * @note Contains constant info about the Award.
 */
class RuleAward
{

private:
	int _sprite;
	std::string
		_description,
		_descGeneral;

	std::map<std::string, std::vector<int> > _criteria;
	std::vector<std::map<int, std::vector<std::string> > > _killCriteria;


	public:
		/// Creates a blank Award ruleset.
		RuleAward();
		/// Cleans up the Award ruleset.
		~RuleAward();

		/// Loads Award data from YAML.
		void load(const YAML::Node& node);

		/// Gets the Award's description.
		std::string getDescription() const;
		/// Gets the Award's generic description.
		std::string getDescriptionGeneral() const;

		/// Gets the Award's sprite.
		int getSprite() const;

		/// Gets the Award's criteria.
		std::map<std::string, std::vector<int> >* getCriteria();
		/// Gets the Award's kill-related criteria.
		std::vector<std::map<int, std::vector<std::string> > >* getKillCriteria();
};

}

#endif
