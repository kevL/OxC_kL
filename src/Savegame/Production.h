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

#ifndef OPENXCOM_PRODUCTION_H
#define OPENXCOM_PRODUCTION_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleManufacture;
class Base;
class SavedGame;
class Ruleset;


enum ProductProgress
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
	const RuleManufacture* _rules;

	bool
		_infinite,
		_sell;
	int
		_amount,
		_engineers,
		_timeSpent;

	bool enoughMoney(SavedGame* g);
	bool enoughMaterials(Base* b);


	public:
		///
		Production(
				const RuleManufacture* rules,
				int amount);
		// kL_note: no dTor !

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
		void setTimeSpent(int done);

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
		ProductProgress step(
				Base* b,
				SavedGame* g,
				const Ruleset* r);

		///
		const RuleManufacture* getRules() const;

		///
		void startItem(
				Base* b,
				SavedGame* g);
};

}

#endif
