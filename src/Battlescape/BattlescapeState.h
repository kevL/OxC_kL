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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BATTLESCAPESTATE_H
#define OPENXCOM_BATTLESCAPESTATE_H

//#include <string>
//#include <vector>

#include "Position.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class BattlescapeButton;
class BattlescapeGame;
class BattleItem;
class BattleUnit;
class ImageButton;
class InteractiveSurface;
class Map;
class NumberText;
class Ruleset;
class SavedBattleGame;
class SavedGame;
class Surface;
class Text;
class TextList;
class Tile;
class Timer;
//class TurnCounter;
class WarningMessage;


/**
 * Battlescape screen which shows the tactical battle.
 */
class BattlescapeState
	:
		public State
{

private:
	static const size_t
		HOTSQRS			= 20,
		TARGET_FRAMES	=  6,
		PULSE_FRAMES	= 22;

	static const Uint8
		GRAY	=   7,
		ORANGE	=  16,
		RED		=  32,
		RED_D	=  45,
		GREEN	=  48,
		BROWN_L	=  80,
		BROWN_D	=  92,
		BLUE	= 128,
		YELLOW	= 144,
		BROWN	= 160;

	bool
		_firstInit,
		_iconsHidden,
		_isKneeled,
		_isMouseScrolled,
		_isMouseScrolling,
		_isOverweight,
		_mouseOverIcons,
		_mouseOverThreshold,
		_showSoldierData;
	int
		_showConsole,
		_totalMouseMoveX,
		_totalMouseMoveY;
//		_xBeforeMouseScrolling,
//		_yBeforeMouseScrolling;
	size_t
		_fuseFrame,
		_hostileTargeterFrame;

	Uint32 _mouseScrollStartTime;

//	std::string _currentTooltip;

	Bar
		* _barTimeUnits,
		* _barEnergy,
		* _barHealth,
		* _barMorale;
	BattlescapeButton
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
		* _btnOptions,
		* _btnEndTurn,
		* _btnAbort,

		* _btnLaunch,
		* _btnPsi;

//		* _reserve;
//		* _btnReserveNone, * _btnReserveSnap, * _btnReserveAimed, * _btnReserveAuto, * _btnReserveKneel, * _btnZeroTUs;
	BattlescapeGame* _battleGame;
	BattleUnit* _hostileUnit[HOTSQRS];
	InteractiveSurface
		* _icons,
		* _btnLeftHandItem,
		* _btnRightHandItem,
		* _btnStats,
		* _btnHostileUnit[HOTSQRS],
		* _btnWounds,
		* _btnLogo;
//	ImageButton* _reserve;
//	ImageButton* _btnReserveNone, * _btnReserveSnap, * _btnReserveAimed, * _btnReserveAuto, * _btnReserveKneel, * _btnZeroTUs;
	Map* _map;
	NumberText
		* _numHostileUnit[HOTSQRS],

		* _numTUAim,
		* _numTUAuto,
		* _numTUSnap,
		* _numTULaunch,

		* _numTimeUnits,
		* _numEnergy,
		* _numHealth,
		* _numMorale,
		* _numAmmoLeft,
		* _numAmmoRight,

		* _numDir,
		* _numDirTur,
		* _numLayers,
		* _numWounds;
	Ruleset* _rules;
	SavedBattleGame* _battleSave;
	SavedGame* _gameSave;
	Surface
		* _alienMark,
		* _bigBtnBorder,
		* _iconsLayer,
		* _kneel,
		* _rank,
		* _overWeight,
		* _hostileTargeter;
	Text
		* _txtBaseLabel,
		* _txtConsole1,
		* _txtConsole2,
		* _txtControlDestroyed,
		* _txtDebug,
		* _txtMissionLabel,
		* _txtName,
		* _txtOperationTitle,
		* _txtOrder,
		* _txtRegion,
		* _txtShade,
		* _txtTerrain,
//		* _txtTooltip;
		* _txtTurn;
	TextList
		* _lstSoldierInfo,
		* _lstTileInfo;
	Timer
		* _aniTimer,
		* _tacticalTimer;
//	TurnCounter* _turnCounter;
	WarningMessage* _warning;

	std::vector<State*> _popups;

	Position _offsetPreDragScroll;
//		_cursorPosition,


	/// Prints contents of hovered Tile's inventory to screen.
	void printTileInventory();

	/// Shows primer warnings on hand-held live grenades.
	void drawFuse();
	/// Shifts the colors of the visible unit buttons' backgrounds.
	void cycleHostileHotcons();
	/// Animates a red cross icon when an injured soldier is selected.
	void flashMedic();
	/// Animates a target cursor over hostile unit when hostileUnit indicator is clicked.
	void hostileTargeter();
	/// Draws an execution explosion on the Map.
	void executionExplosion();
	/// Draws a shotgun blast explosion on the Map.
	void shotgunExplosion();

	/// Popups a context sensitive list of actions the player can choose from.
	void handAction(
			BattleItem* const item,
			bool injured = false);


	public:
//		static const Uint32 STATE_INTERVAL_STANDARD = 89; // for fast shaders - Raw, Quillez, etc.
		static const Uint32 STATE_INTERVAL_STANDARD = 76; // for slow shaders - 4xHQX & above.

		/// Creates a BattlescapeState.
		BattlescapeState();
		/// Cleans up the BattlescapeState.
		~BattlescapeState();

		/// Selects the next soldier.
		void selectNextFactionUnit(
				bool checkReselect = false,
				bool dontReselect = false,
				bool checkInventory = false);
		/// Selects the previous soldier.
		void selectPreviousFactionUnit(
				bool checkReselect = false,
				bool dontReselect = false,
				bool checkInventory = false);

		/// Initializes this BattlescapeState.
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

		/// Move the mouse back to where it started after we finish drag scrolling.
//		void stopScrolling(Action* action);

		/// Handles keypresses.
		void handle(Action* action);

		/// Handler for clicking the Unit Up button.
		void btnUnitUpClick(Action* action);
		/// Handler for clicking the Unit Down button.
		void btnUnitDownClick(Action* action);
		/// Handler for clicking the Map Up button.
		void btnMapUpClick(Action* action);
		/// Handler for clicking the Map Down button.
		void btnMapDownClick(Action* action);
		/// Sets the level on the icons' Layers button.
		void setLayerValue(int level);
		/// Handler for clicking the Show Map button.
		void btnShowMapClick(Action* action);
		/// Handler for clicking the Kneel button.
		void btnKneelClick(Action* action);
		/// Handler for clicking the Soldier button.
		void btnInventoryClick(Action* action);
		/// Handler for clicking the Center button.
		void btnCenterClick(Action* action);

		/// Forces a transparent SDL mouse-motion event.
		void refreshMousePosition() const;

		/// Handler for clicking the Next Soldier button.
		void btnNextSoldierClick(Action* action);
		/// Handler for clicking the Next Stop button.
		void btnNextStopClick(Action* action);
		/// Handler for clicking the Previous Soldier button.
		void btnPrevSoldierClick(Action* action);
		/// Handler for clicking the Previous Stop button.
		void btnPrevStopClick(Action* action);
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

		/// Handler for left-clicking the left hand item button.
		void btnLeftHandLeftClick(Action* action);
		/// Handler for right-clicking the left hand item button.
		void btnLeftHandRightClick(Action* action);
		/// Handler for left-clicking the right hand item button.
		void btnRightHandLeftClick(Action* action);
		/// Handler for right-clicking the right hand item button.
		void btnRightHandRightClick(Action* action);

		/// Handler for clicking a hostile unit button.
		void btnHostileUnitPress(Action* action);
		/// Handler for clicking the wounded unit button.
		void btnWoundedPress(Action* action);

		/// Handler for clicking the launch rocket button.
		void btnLaunchClick(Action* action);
		/// Handler for clicking the use psi button.
		void btnPsiClick(Action* action);

		/// Handler for clicking a reserved button.
//		void btnReserveClick(Action* action);
		/// Handler for clicking the reserve TUs to kneel button.
//		void btnReserveKneelClick(Action* action);

		/// Handler for clicking the reload button.
		void btnReloadClick(Action* action);
		/// Handler for clicking the expend all TUs button.
		void btnZeroTuClick(Action* action);
		/// Handler for clicking the UfoPaedia button.
		void btnUfoPaediaClick(Action* action);
		/// Handler for clicking the lighting button.
		void btnPersonalLightingClick(Action* action);
		/// Handler for toggling the console.
		void btnConsoleToggle(Action* action);
		/// Handler for showing tooltip.

//		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
//		void txtTooltipOut(Action* action);

		/// Determines whether a playable unit is selected.
		bool playableUnitSelected();

		/// Updates soldier name/rank/tu/energy/health/morale.
		void updateSoldierInfo(bool calcFoV = true);
		/// Refreshes the visUnits' surfaces' visibility for UnitWalk/TurnBStates.
		void updateHostileHotcons();

		/// Animates map objects on the map, also smoke,fire, ...
		void animate();
		/// Handles the top battle game state.
		void handleState();
		/// Sets the state timer interval.
		void setStateInterval(Uint32 interval);

		/// Gets game.
		Game* getGame() const;
		/// Gets pointer to the SavedGame.
		SavedGame* getSavedGame() const;
		/// Gets pointer to the SavedBattleGame.
		SavedBattleGame* getSavedBattleGame() const;
		/// Gets map.
		Map* getMap() const;

		/// Show debug message.
		void debug(const std::wstring& message);
		/// Show warning message.
		void warning(
				const std::string& message,
				bool useArg = false,
				int arg = 0);

		/// Displays a popup window.
		void popup(State* state);

		/// Finishes a tactical battle.
		void finishBattle(
				const bool abort,
				const int inExitArea);

		/// Shows the launch button.
		void showLaunchButton(bool show = true);
		/// Shows the PSI button.
		void showPsiButton(bool show = true);

		/// Clears mouse-scrolling state.
		void clearMouseScrollingState();

		/// Returns a pointer to the battlegame, in case we need its functions.
		BattlescapeGame* getBattleGame();

		/// Handler for the mouse moving over the icons, disables the tile selection cube.
		void mouseInIcons(Action* action);
		/// Handler for the mouse going out of the icons, enabling the tile selection cube.
		void mouseOutIcons(Action* action);
		/// Checks if the mouse is over the icons.
		bool getMouseOverIcons() const;

		/// Is the player allowed to press buttons?
		bool allowButtons(bool allowSaving = false) const;

		/// Updates the resolution settings, the window was just resized.
		void resize(
				int& dX,
				int& dY);

		/// Gets the TurnCounter.
//		TurnCounter* getTurnCounter() const;
		/// Updates the turn text.
		void updateTurn();

		/// Toggles the icons' surfaces' visibility for Hidden Movement.
		void toggleIcons(bool vis);

		/// Gets the TimeUnits field from icons.
		NumberText* getTuField() const;
		/// Gets the TimeUnits bar from icons.
		Bar* getTuBar() const;
		/// Gets the Energy field from icons.
		NumberText* getEnergyField() const;
		/// Gets the Energy bar from icons.
		Bar* getEnergyBar() const;

		/// Checks if it's okay to show a rookie's kill/stun alien icon.
		bool allowAlienMark() const;
		/// Updates experience data for the currently selected soldier.
		void updateExperienceInfo();
		/// Updates tile info for the tile under mouseover.
		void updateTileInfo(const Tile* const tile);

		/// Saves a map as used by the AI.
		void saveAIMap();
		/// Saves each layer of voxels on the bettlescape as a png.
		void saveVoxelMap();
		/// Saves a first-person voxel view of the battlescape.
		void saveVoxelView();
};

}

#endif
