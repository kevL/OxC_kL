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

#ifndef OPENXCOM_BATTLEAISTATE_H
#define OPENXCOM_BATTLEAISTATE_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class BattleUnit;
class SavedBattleGame;

struct BattleAction;


enum AIMode
{
	AI_PATROL,	// 0
	AI_AMBUSH,	// 1
	AI_COMBAT,	// 2
	AI_ESCAPE	// 3
};


/**
 * This class is used by the BattleUnit AI.
 */
class BattleAIState
{

protected:
	SavedBattleGame* _save;
	BattleUnit* _unit;


	public:
		/// Creates a new BattleAIState linked to the SavedBattleGame and a certain BattleUnit.
		BattleAIState(
				SavedBattleGame* save,
				BattleUnit* unit);
		/// Cleans up the BattleAIState.
		virtual ~BattleAIState();

		/// Loads the AI state from YAML.
		void load(const YAML::Node& node);
		/// Saves the AI state to YAML.
		virtual YAML::Node save() const;

		/// Enters the state.
		virtual void enter();
		/// Exits the state.
		virtual void exit();
		/// Runs state functionality every AI cycle.
		virtual void think(BattleAction* action);
};

}

#endif
