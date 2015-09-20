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

#ifndef OPENXCOM_PRODUCTION_H
#define OPENXCOM_PRODUCTION_H

//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Ruleset;
class SavedGame;


enum ProductionProgress
{
	PROGRESS_NOT_COMPLETE,			// 0
	PROGRESS_COMPLETE,				// 1
	PROGRESS_NOT_ENOUGH_MONEY,		// 2
	PROGRESS_NOT_ENOUGH_MATERIALS,	// 3
	PROGRESS_MAX,					// 4
	PROGRESS_CONSTRUCTION			// 5
};


class Production
{

private:
	const RuleManufacture* _manufRule;

	bool
		_infinite,
		_sell;
	int
		_amount,
		_engineers,
		_timeSpent;

	///
	bool enoughMoney(const SavedGame* const gameSave) const;
	///
	bool enoughMaterials(Base* const base) const;


	public:
		///
		Production(
				const RuleManufacture* const manufRule,
				int amount);
		///
		~Production();

		///
		void load(const YAML::Node& node);
		///
		YAML::Node save() const;

		///
		int getAmountTotal() const;
		///
		void setAmountTotal(int amount);

		///
		bool getInfiniteAmount() const;
		///
		void setInfiniteAmount(bool infinite);

		///
		int getTimeSpent() const;
		///
		void setTimeSpent(int spent);

		///
		int getAmountProduced() const;

		///
		int getAssignedEngineers() const;
		///
		void setAssignedEngineers(int engineers);

		///
		bool getSellItems() const;
		///
		void setSellItems(bool sell);

		///
		ProductionProgress step(
				Base* const base,
				SavedGame* const gameSave,
				const Ruleset* const rules);

		///
		const RuleManufacture* getRules() const;

		///
		void startProduction(
				Base* const base,
				SavedGame* const gameSave) const;
};

}

#endif
