/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPESTATE_H
#define OPENXCOM_BATTLESCAPESTATE_H

#include <string>
#include <vector>

#include "Position.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class BattlescapeGame;
class BattleItem;
class BattleUnit;
class ImageButton;
class InteractiveSurface;
class Map;
class NumberText;
class SavedBattleGame;
class Surface;
class Text;
class Timer;
class TurnCounter;		// kL
class WarningMessage;


/**
 * Battlescape screen which shows the tactical battle.
 */
class BattlescapeState
	:
		public State
{

private:
	static const int VISIBLE_MAX = 10;

	bool
		_mouseOverIcons,
		firstInit,
		isMouseScrolled,
		isMouseScrolling,
		mouseMovedOverThreshold;
	int
		totalMouseMoveX,
		totalMouseMoveY,
		xBeforeMouseScrolling,
		yBeforeMouseScrolling;
	Uint32 mouseScrollingStartTime;
	std::string _currentTooltip;

	Bar
		* _barTimeUnits,
		* _barEnergy,
		* _barHealth,
		* _barMorale;
	BattlescapeGame* _battleGame;
	BattleUnit* _visibleUnit[VISIBLE_MAX];
	InteractiveSurface
		* _icons,

		* _btnUnitUp,
		* _btnUnitDown,
		* _btnMapUp,
		* _btnMapDown,
		* _btnShowMap,
		* _btnKneel,

		* _btnInventory,
		* _btnCenter,
		* _btnNextSoldier,
		* _btnNextStop,
		* _btnShowLayers,
		* _btnHelp,

		* _btnEndTurn,
		* _btnAbort,
		* _btnStats,
		* _btnLaunch,
		* _btnPsi,

		* _btnLeftHandItem,
		* _btnRightHandItem,

		* _btnVisibleUnit[VISIBLE_MAX];
	ImageButton* _reserve;
//	ImageButton* _btnReserveNone, * _btnReserveSnap, * _btnReserveAimed, * _btnReserveAuto, * _btnReserveKneel, * _btnZeroTUs;
	Map* _map;
	NumberText
		* _numVisibleUnit[VISIBLE_MAX],

		* _numTUSnap,

		* _numTimeUnits,
		* _numEnergy,
		* _numHealth,
		* _numMorale,
		* _numLayers,
		* _numAmmoLeft,
		* _numAmmoRight;
	Position mapOffsetBeforeMouseScrolling;
	SavedBattleGame* _save;
	Surface
		* _rank,
		* _kneel;
	Text
		* _txtDebug, //* _txtTooltip;
		* _txtName;
	Timer
		* _animTimer,
		* _gameTimer;
	TurnCounter* _turnCounter; // kL
	WarningMessage* _warning;

	std::vector<State*> _popups;

	/// Popups a context sensitive list of actions the user can choose from.
	void handleItemClick(BattleItem* item);
	/// Shifts the red colors of the visible unit buttons backgrounds.
	void blinkVisibleUnitButtons();
	/// Draws the kneel indicator.
	void BattlescapeState::drawKneelIndicator();


	public:
		static const int DEFAULT_ANIM_SPEED = 87;

		/// Creates the Battlescape state.
		BattlescapeState(Game* game);
		/// Cleans up the Battlescape state.
		~BattlescapeState();

		/// Selects the next soldier.
		void selectNextFactionUnit(
				bool checkReselect = false,
				bool setDontReselect = false,
				bool checkInventory = false);
		/// Selects the previous soldier.
		void selectPreviousFactionUnit(
				bool checkReselect = false,
				bool setDontReselect = false,
				bool checkInventory = false);

		/// Initilizes the battlescapestate.
		void init();
		/// Runs the timers and handles popups.
		void think();

		/// Handler for moving mouse over the map.
		void mapOver(Action* action);
		/// Handler for pressing the map.
		void mapPress(Action* action);
		/// Handler for clicking the map.
		void mapClick(Action* action);
		/// Handler for entering with mouse to the map surface.
		void mapIn(Action* action);

		/// Handler for clicking the Unit Up button.
		void btnUnitUpClick(Action* action);
		/// Handler for clicking the Unit Down button.
		void btnUnitDownClick(Action* action);
		/// Handler for clicking the Map Up button.
		void btnMapUpClick(Action* action);
		/// Handler for clicking the Map Down button.
		void btnMapDownClick(Action* action);
		/// Handler for clicking the Show Map button.
		void btnShowMapClick(Action* action);
		/// Handler for clicking the Kneel button.
		void btnKneelClick(Action* action);
		/// Handler for clicking the Soldier button.
		void btnInventoryClick(Action* action);
		/// Handler for clicking the Center button.
		void btnCenterClick(Action* action);
		/// Handler for clicking the Next Soldier button.
		void btnNextSoldierClick(Action* action);
		/// Handler for clicking the Next Stop button.
		void btnNextStopClick(Action* action);
		/// Handler for clicking the Previous Soldier button.
		void btnPrevSoldierClick(Action* action);
		/// Handler for clicking the Previous Stop button.	// kL
		void btnPrevStopClick(Action* action);				// kL
		/// Handler for clicking the Show Layers button.
		void btnShowLayersClick(Action* action);
		/// Handler for clicking the Help button.
		void btnHelpClick(Action* action);
		/// Handler for clicking the End Turn button.
		void btnEndTurnClick(Action* action);
		/// Handler for clicking the Abort button.
		void btnAbortClick(Action* action);
		/// Handler for clicking the stats.
		void btnStatsClick(Action* action);
		/// Handler for clicking the left hand item button.
		void btnLeftHandItemClick(Action* action);
		/// Handler for clicking the right hand item button.
		void btnRightHandItemClick(Action* action);
		/// Handler for clicking a visible unit button.
		void btnVisibleUnitClick(Action* action);
		/// Handler for clicking the launch rocket button.
		void btnLaunchClick(Action* action);
		/// Handler for clicking the use psi button.
		void btnPsiClick(Action* action);
		/// Handler for clicking a reserved button.
//		void btnReserveClick(Action* action);
		/// Handler for clicking the reload button.
//		void btnReloadClick(Action* action);
		/// Handler for clicking the lighting button.
		void btnPersonalLightingClick(Action* action);

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected();
		/// Updates soldier name/rank/tu/energy/health/morale.
		void updateSoldierInfo();
		/// Animates map objects on the map, also smoke,fire, ...
		void animate();
		/// Handles the battle game state.
		void handleState();
		/// Sets the state timer interval.
		void setStateInterval(Uint32 interval);
		/// Gets game.
		Game* getGame() const;
		/// Gets map.
		Map* getMap() const;
		/// Show debug message.
		void debug(const std::wstring& message);
		/// Show warning message.
		void warning(const std::string& message);
		/// Handles keypresses.
		void handle(Action* action);
		/// Displays a popup window.
		void popup(State* state);
		/// Finishes a battle.
		void finishBattle(bool abort, int inExitArea);
		/// Show the launch button.
		void showLaunchButton(bool show);
		/// Shows the PSI button.
		void showPsiButton(bool show);
		/// Clears mouse-scrolling state.
		void clearMouseScrollingState();
		/// Returns a pointer to the battlegame, in case we need its functions.
		BattlescapeGame* getBattleGame();
		/// Saves a map as used by the AI.
		void saveAIMap();
		/// Saves each layer of voxels on the bettlescape as a png.
		void saveVoxelMap();
		/// Saves a first-person voxel view of the battlescape.
		void saveVoxelView();
		/// Handler for the mouse moving over the icons, disables the tile selection cube.
		void mouseInIcons(Action* action);
		/// Handler for the mouse going out of the icons, enabling the tile selection cube.
		void mouseOutIcons(Action* action);
		/// Checks if the mouse is over the icons.
		bool getMouseOverIcons() const;
		/// Is the player allowed to press buttons?
		bool allowButtons(bool allowSaving = false) const;
		/// Handler for clicking the reserve TUs to kneel button.
//		void btnReserveKneelClick(Action* action);
		/// Handler for clicking the expend all TUs button.
//		void btnZeroTUsClick(Action* action);
		/// Handler for showing tooltip.
//		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
//		void txtTooltipOut(Action* action);
		/// kL. Gets the TurnCounter.
		TurnCounter* getTurnCounter() const;	// kL
};

}

#endif
