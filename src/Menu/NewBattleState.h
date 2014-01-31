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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_NEWBATTLESTATE_H
#define OPENXCOM_NEWBATTLESTATE_H

#include <string>
#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class Text;
class TextButton;
class Window;


/**
 * New Battle that displays a list of options to configure a standalone mission.
 */
class NewBattleState
	:
		public State
{

private:
	bool _music;
	size_t
		_selAlien,
		_selCraft,
		_selDarkness,
		_selDifficulty,
		_selItemLevel,
		_selMission,
		_selTerrain;

	Window *_window;
	Text
		*_txtAlienRace,
		*_txtCraft,
		*_txtDarkness,
		*_txtDifficulty,
		*_txtItemLevel,
		*_txtMissionType,
		*_txtTerrainType,
		*_txtTitle;
	TextButton
		*_btnAlienRace,
		*_btnCancel,
		*_btnCraft,
		*_btnDarkness,
		*_btnDifficulty,
		*_btnEquip,
		*_btnItemLevel,
		*_btnMissionType,
		*_btnOk,
		*_btnRandom,
		*_btnTerrainType;
	Craft *_craft;

	std::vector<std::string>
		_alienRaces,
		_crafts,
		_darkness,
		_difficulty,
		_itemLevels,
		_missionTypes,
		_terrainTypes;
	std::vector<int> _textures;

	///
	void updateIndex(
			size_t& index,
			std::vector<std::string>& list,
			int change);
	///
	void updateButtons();


	public:
		/// Creates the New Battle state.
		NewBattleState(Game *game);
		/// Cleans up the New Battle state.
		~NewBattleState();

		/// Resets state.
		void init();
		/// Initializes a blank savegame.
		void initSave();

		/// Handler for clicking the OK button.
		void btnOkClick(Action *action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action *action);
		/// Handler for clicking the Randomize button.
		void btnRandomClick(Action *action);
		/// Handler for clicking the Equip Craft button.
		void btnEquipClick(Action *action);
		/// Handler for clicking the Mission Type button.
		void btnMissionTypeClick(Action *action);
		/// Handler for clicking the Mission Option button.
		void btnTerrainTypeClick(Action *action);
		/// Handler for clicking the Alien Race button.
		void btnAlienRaceClick(Action *action);
		/// Handler for clicking the Difficulty button.
		void btnDifficultyClick(Action *action);
		/// Handler for clicking the Darkness button.
		void btnDarknessClick(Action *action);
		/// Handler for clicking the Craft button.
		void btnCraftClick(Action *action);
		///
		void btnItemLevelClick(Action *action);
};

}

#endif
