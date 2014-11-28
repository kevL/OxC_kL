/*
 * Copyright 2012 OpenXcom Developers.
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

#ifndef OPENXCOM_WEIGHTEDOPTIONS_H
#define OPENXCOM_WEIGHTEDOPTIONS_H

//#include <string>
//#include <map>

//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Holds pairs of relative weights and IDs.
 * It is used to store options and make a random choice between them.
 */
class WeightedOptions
{
private:
	std::map<std::string, size_t> _choices; //!< Options and weights
	size_t _totalWeight; //!< The total weight of all options.

	public:
		/// Creates an empty set.
		WeightedOptions()
			:
				_totalWeight(0)
		{
			/* Empty by design. */
		}

		/// Selects from among the items.
		const std::string choose() const;
		/// Selects the top item.
		const std::string topChoice() const;

		/// Sets an option's weight.
		void setWeight(
				const std::string& id,
				size_t weight);

		/// Gets if this choice is empty.
		bool hasNoWeight() const
		{
			return (_totalWeight == 0);
		}

		/// Removes all entries and weights.
		void clearWeights()
		{
			_totalWeight = 0;
			_choices.clear();
		}

		/// Updates the list with data from YAML.
		void load(const YAML::Node& node);
		/// Stores the list in YAML.
		YAML::Node save() const;
};

}

#endif
