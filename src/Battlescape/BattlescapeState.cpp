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

#define _USE_MATH_DEFINES

#include "BattlescapeState.h"

#include <cmath>
#include <sstream>
#include <iomanip>

#include <SDL_gfxPrimitives.h>

#include "AbortMissionState.h"
#include "ActionMenuState.h"
#include "AlienBAIState.h"
#include "BattlescapeGame.h"
#include "BattlescapeGenerator.h"
#include "BattleState.h"
#include "BriefingState.h"
#include "Camera.h"
#include "CivilianBAIState.h"
#include "DebriefingState.h"
#include "ExplosionBState.h"
#include "InfoboxState.h"
#include "InventoryState.h"
#include "Map.h"
#include "MiniMapState.h"
#include "NextTurnState.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "UnitDieBState.h"
#include "UnitInfoState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"
#include "WarningMessage.h"

#include "../lodepng.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Geoscape/DefeatState.h"
#include "../Geoscape/VictoryState.h"

#include "../Interface/Bar.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TurnCounter.h" // kL

#include "../Menu/LoadGameState.h"
#include "../Menu/PauseState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/ResourcePack.h"
#include "../Resource/XcomResourcePack.h" // sza_MusicRules

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h" // kL
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h" // kL
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape screen.
 * @param game, Pointer to the core game.
 */
BattlescapeState::BattlescapeState(Game* game)
	:
		State(game),
		_popups(),
		_xBeforeMouseScrolling(0),
		_yBeforeMouseScrolling(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_mouseMovedOverThreshold(false),
	// kL_begin:
		_firstInit(true),
		_mouseOverIcons(false),
		_isMouseScrolled(false),
		_isMouseScrolling(false),
		_mouseScrollingStartTime(0)
	// kL_end.
{
	//Log(LOG_INFO) << "Create BattlescapeState";
	std::fill_n(
			_visibleUnit,
			10,
			(BattleUnit*)(0));

	int screenWidth		= Options::baseXResolution;
	int screenHeight	= Options::baseYResolution;
	int iconsWidth		= 320;
	int iconsHeight		= Map::ICON_HEIGHT;

//kL	_mouseOverIcons = false; // cTor.

	_txtBaseLabel = new Text(80, 9, screenWidth - 81, 0); // kL

	// Create buttonbar - this should be on the centerbottom of the screen
	_icons = new InteractiveSurface(
								iconsWidth,
								iconsHeight,
								(screenWidth / 2) - (iconsWidth / 2),
								screenHeight - iconsHeight);

	// Create the battlemap view
	// the actual map height is the total height minus the height of the buttonbar
	int visibleMapHeight = screenHeight - iconsHeight;

	_map = new Map(
				_game,
				screenWidth,
				screenHeight,
				0,
				0,
				visibleMapHeight);

	_numLayers	= new NumberText(3, 5, _icons->getX() + 232, _icons->getY() + 6);

	_rank		= new Surface(26, 23, _icons->getX() + 107, _icons->getY() + 33);
	_kneel		= new Surface( 2,  2, _icons->getX() + 115, _icons->getY() + 19);

	// Create buttons
	_btnUnitUp			= new InteractiveSurface(32,  16, _icons->getX() +  48, _icons->getY());
	_btnUnitDown		= new InteractiveSurface(32,  16, _icons->getX() +  48, _icons->getY() + 16);
	_btnMapUp			= new InteractiveSurface(32,  16, _icons->getX() +  80, _icons->getY());
	_btnMapDown			= new InteractiveSurface(32,  16, _icons->getX() +  80, _icons->getY() + 16);
	_btnShowMap			= new InteractiveSurface(32,  16, _icons->getX() + 112, _icons->getY());
	_btnKneel			= new InteractiveSurface(32,  16, _icons->getX() + 112, _icons->getY() + 16);
	_btnInventory		= new InteractiveSurface(32,  16, _icons->getX() + 144, _icons->getY());
	_btnCenter			= new InteractiveSurface(32,  16, _icons->getX() + 144, _icons->getY() + 16);
	_btnNextSoldier		= new InteractiveSurface(32,  16, _icons->getX() + 176, _icons->getY());
	_btnNextStop		= new InteractiveSurface(32,  16, _icons->getX() + 176, _icons->getY() + 16);
	_btnShowLayers		= new InteractiveSurface(32,  16, _icons->getX() + 208, _icons->getY());
	_btnOptions			= new InteractiveSurface(32,  16, _icons->getX() + 208, _icons->getY() + 16);
	_btnEndTurn			= new InteractiveSurface(32,  16, _icons->getX() + 240, _icons->getY());
	_btnAbort			= new InteractiveSurface(32,  16, _icons->getX() + 240, _icons->getY() + 16);
	_btnStats			= new InteractiveSurface(164, 23, _icons->getX() + 107, _icons->getY() + 33);
/*	_btnReserveNone		= new ImageButton(28, 11, _icons->getX() + 49, _icons->getY() + 33);
	_btnReserveSnap		= new ImageButton(28, 11, _icons->getX() + 78, _icons->getY() + 33);
	_btnReserveAimed	= new ImageButton(28, 11, _icons->getX() + 49, _icons->getY() + 45);
	_btnReserveAuto		= new ImageButton(28, 11, _icons->getX() + 78, _icons->getY() + 45); */
/*	_btnReserveNone		= new ImageButton(17, 11, _icons->getX() + 60, _icons->getY() + 33);
	_btnReserveSnap		= new ImageButton(17, 11, _icons->getX() + 78, _icons->getY() + 33);
	_btnReserveAimed	= new ImageButton(17, 11, _icons->getX() + 60, _icons->getY() + 45);
	_btnReserveAuto		= new ImageButton(17, 11, _icons->getX() + 78, _icons->getY() + 45);
	_btnReserveKneel	= new ImageButton(10, 23, _icons->getX() + 96, _icons->getY() + 33);
	_btnZeroTUs			= new ImageButton(10, 23, _icons->getX() + 49, _icons->getY() + 33); */

	_btnLeftHandItem	= new InteractiveSurface(32, 48, _icons->getX() +   8, _icons->getY() + 5);
	_btnRightHandItem	= new InteractiveSurface(32, 48, _icons->getX() + 280, _icons->getY() + 5);
	_numAmmoLeft		= new NumberText(30, 5, _icons->getX() +   8, _icons->getY() + 4);
	_numAmmoRight		= new NumberText(30, 5, _icons->getX() + 280, _icons->getY() + 4);

	for (int
			i = 0,
				offset_x = 0;		// kL
			i < VISIBLE_MAX;
			++i)
	{
		if (i > 9) offset_x = 20;	// kL

		_btnVisibleUnit[i] = new InteractiveSurface(
												15,
												12,
												_icons->getX() + iconsWidth - 20 - offset_x,
												_icons->getY() - 16 - (i * 13));
		_numVisibleUnit[i] = new NumberText(
										15,
										12,
										_icons->getX() + iconsWidth - 14 - offset_x,
										_icons->getY() - 12 - (i * 13));
	}

	_numVisibleUnit[9]->setX(_numVisibleUnit[9]->getX() - 2); // center number 10

	_warning	= new WarningMessage(
					224,
					24,
					_icons->getX() + 48,
					_icons->getY() + 32);

	_btnLaunch	= new InteractiveSurface(
					32,
					24,
					screenWidth - 32,
					0);
	_btnLaunch->setVisible(false);

	_btnPsi		= new InteractiveSurface(
					32,
					24,
					screenWidth - 32,
					25);
	_btnPsi->setVisible(false);

	// Create soldier stats summary
	_txtName		= new Text(136, 10, _icons->getX() + 135, _icons->getY() + 32);

//	_numTUSnap		= new NumberText(12, 10, _icons->getX() + 258, _icons->getY() + 34);
	_numTUAim		= new NumberText(8, 10, _icons->getX() + 241, _icons->getY() + 34);
	_numTUAuto		= new NumberText(8, 10, _icons->getX() + 252, _icons->getY() + 34);
	_numTUSnap		= new NumberText(8, 10, _icons->getX() + 263, _icons->getY() + 34);

	_numTimeUnits	= new NumberText(15, 5, _icons->getX() + 136, _icons->getY() + 42);
	_barTimeUnits	= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 41);

	_numEnergy		= new NumberText(15, 5, _icons->getX() + 154, _icons->getY() + 42);
	_barEnergy		= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 45);

	_numHealth		= new NumberText(15, 5, _icons->getX() + 136, _icons->getY() + 50);
	_barHealth		= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 49);

	_numMorale		= new NumberText(15, 5, _icons->getX() + 154, _icons->getY() + 50);
	_barMorale		= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 53);

	_txtDebug		= new Text(300, 10, 10, 0);
//	_txtTooltip		= new Text(300, 10, _icons->getX() + 2, _icons->getY() - 10);

	_turnCounter	= new TurnCounter(20, 5, 0, 0); // kL

	setPalette("PAL_BATTLESCAPE");

	// Fix system colors
	_game->getCursor()->setColor(Palette::blockOffset(9));
	_game->getFpsCounter()->setColor(Palette::blockOffset(9));

	add(_map);
	add(_icons);
	add(_numLayers);
	add(_rank);
	add(_kneel);
	add(_btnUnitUp);
	add(_btnUnitDown);
	add(_btnMapUp);
	add(_btnMapDown);
	add(_btnShowMap);
	add(_btnKneel);
	add(_btnInventory);
	add(_btnCenter);
	add(_btnNextSoldier);
	add(_btnNextStop);
	add(_btnShowLayers);
	add(_btnOptions);
	add(_btnEndTurn);
	add(_btnAbort);
	add(_btnStats);
	add(_txtName);
	add(_numTUAim);
	add(_numTUAuto);
	add(_numTUSnap);
	add(_numTimeUnits);
	add(_numEnergy);
	add(_numHealth);
	add(_numMorale);
	add(_barTimeUnits);
	add(_barEnergy);
	add(_barHealth);
	add(_barMorale);
/*	add(_btnReserveNone);
	add(_btnReserveSnap);
	add(_btnReserveAimed);
	add(_btnReserveAuto);
	add(_btnReserveKneel);
	add(_btnZeroTUs); */
	add(_btnLeftHandItem);
	add(_numAmmoLeft);
	add(_btnRightHandItem);
	add(_numAmmoRight);

	for (int
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		add(_btnVisibleUnit[i]);
		add(_numVisibleUnit[i]);
	}

	add(_warning);
	add(_txtDebug);
//	add(_txtTooltip);

	add(_btnLaunch);
	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(0)->blit(_btnLaunch);
	add(_btnPsi);
	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blit(_btnPsi);

	add(_turnCounter);									// kL
	_turnCounter->setColor(Palette::blockOffset(9)+1);	// kL

	_save = _game->getSavedGame()->getSavedBattle();

	// kL_begin:
	//Log(LOG_INFO) << "_txtBaseLabel BEGIN";
	add(_txtBaseLabel);
	_txtBaseLabel->setColor(Palette::blockOffset(9)+1);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	Base* base = 0;
	std::wstring baseLabel = L"";

	//Log(LOG_INFO) << ". iterate start";
	for (std::vector<Base*>::iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		//Log(LOG_INFO) << ". iterate bases";
		for (std::vector<Craft*>::iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			//Log(LOG_INFO) << ". iterate craft at base";
			if ((*j)->isInBattlescape())
			{
				//Log(LOG_INFO) << ". found craft in Battlescape";
				baseLabel = (*i)->getName(_game->getLanguage());

				break;
			}
		}

		if (baseLabel != L"")
			break;
		else if ((*i)->isInBattlescape())
		{
			//Log(LOG_INFO) << ". found base in Battlescape";
			baseLabel = (*i)->getName(_game->getLanguage());

			break;
		}
	}
	_txtBaseLabel->setText(baseLabel); // there'd better be a baseLabel ... or else. Pow! To the moon!!!
	//Log(LOG_INFO) << "_txtBaseLabel DONE";
	// kL_end.

	_map->init();
	_map->onMouseOver((ActionHandler)& BattlescapeState::mapOver);
	_map->onMousePress((ActionHandler)& BattlescapeState::mapPress);
	_map->onMouseClick((ActionHandler)& BattlescapeState::mapClick, 0);
	_map->onMouseIn((ActionHandler)& BattlescapeState::mapIn);

	// Add in custom reserve buttons
	Surface* icons = _game->getResourcePack()->getSurface("ICONS.PCK");
	Surface* tftdIcons = _game->getResourcePack()->getSurface("TFTDReserve");
	tftdIcons->setX(48);
	tftdIcons->setY(176);
	tftdIcons->blit(icons);

	// there is some cropping going on here, because the icons
	// image is 320x200 while we only need the bottom of it.
	SDL_Rect* r = icons->getCrop();
	r->x = 0;
	r->y = 200 - iconsHeight;
	r->w = iconsWidth;
	r->h = iconsHeight;
	icons->blit(_icons);

	_numLayers->setColor(Palette::blockOffset(5)+12);
	_numLayers->setValue(1);

	_numAmmoLeft->setColor(3);
	_numAmmoLeft->setValue(999);

	_numAmmoRight->setColor(3);
	_numAmmoRight->setValue(999);

	_icons->onMouseIn((ActionHandler)& BattlescapeState::mouseInIcons);
	_icons->onMouseOut((ActionHandler)& BattlescapeState::mouseOutIcons);

	_btnUnitUp->onMouseClick((ActionHandler)& BattlescapeState::btnUnitUpClick);
//	_btnUnitUp->setTooltip("STR_UNIT_LEVEL_ABOVE");
//	_btnUnitUp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnUnitUp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnUnitDown->onMouseClick((ActionHandler)& BattlescapeState::btnUnitDownClick);
//	_btnUnitDown->setTooltip("STR_UNIT_LEVEL_BELOW");
//	_btnUnitDown->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnUnitDown->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMapUp->onMouseClick((ActionHandler)& BattlescapeState::btnMapUpClick);
	_btnMapUp->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnMapUpClick,
					Options::keyBattleLevelUp);
//	_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");
//	_btnMapUp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnMapUp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMapDown->onMouseClick((ActionHandler)& BattlescapeState::btnMapDownClick);
	_btnMapDown->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnMapDownClick,
					Options::keyBattleLevelDown);
//	_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");
//	_btnMapDown->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnMapDown->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnShowMap->onMouseClick((ActionHandler)& BattlescapeState::btnShowMapClick);
	_btnShowMap->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnShowMapClick,
					Options::keyBattleMap);
//	_btnShowMap->setTooltip("STR_MINIMAP");
//	_btnShowMap->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnShowMap->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnKneel->onMouseClick((ActionHandler)& BattlescapeState::btnKneelClick);
	_btnKneel->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnKneelClick,
					Options::keyBattleKneel);
//	_btnKneel->setTooltip("STR_KNEEL");
//	_btnKneel->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnKneel->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnInventory->onMouseClick((ActionHandler)& BattlescapeState::btnInventoryClick);
	_btnInventory->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnInventoryClick,
					Options::keyBattleInventory);
//	_btnInventory->setTooltip("STR_INVENTORY");
//	_btnInventory->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnInventory->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnCenter->onMouseClick((ActionHandler)& BattlescapeState::btnCenterClick);
	_btnCenter->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnCenterClick,
					Options::keyBattleCenterUnit);
//	_btnCenter->setTooltip("STR_CENTER_SELECTED_UNIT");
//	_btnCenter->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnCenter->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//kL	_btnNextSoldier->onMouseClick((ActionHandler)& BattlescapeState::btnNextSoldierClick);
	_btnNextSoldier->onMouseClick(
					(ActionHandler)& BattlescapeState::btnNextSoldierClick,
					SDL_BUTTON_LEFT);	// kL
	_btnNextSoldier->onMouseClick(
					(ActionHandler)& BattlescapeState::btnPrevSoldierClick,
					SDL_BUTTON_RIGHT);	// kL
	_btnNextSoldier->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnNextSoldierClick,
					Options::keyBattleNextUnit);
	_btnNextSoldier->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnPrevSoldierClick,
					Options::keyBattlePrevUnit);
//	_btnNextSoldier->setTooltip("STR_NEXT_UNIT");
//	_btnNextSoldier->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnNextSoldier->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//kL	_btnNextStop->onMouseClick((ActionHandler)& BattlescapeState::btnNextStopClick);
	_btnNextStop->onMouseClick(
					(ActionHandler)& BattlescapeState::btnNextStopClick,
					SDL_BUTTON_LEFT);	// kL
	_btnNextStop->onMouseClick(
					(ActionHandler)& BattlescapeState::btnPrevStopClick,
					SDL_BUTTON_RIGHT);	// kL
	_btnNextStop->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnNextStopClick,
					Options::keyBattleDeselectUnit);
//	_btnNextStop->setTooltip("STR_DESELECT_UNIT");
//	_btnNextStop->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnNextStop->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnShowLayers->onMouseClick((ActionHandler)& BattlescapeState::btnShowLayersClick);
//	_btnShowLayers->setTooltip("STR_MULTI_LEVEL_VIEW");
//	_btnShowLayers->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnShowLayers->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnOptions->onMouseClick((ActionHandler)& BattlescapeState::btnHelpClick);
	_btnOptions->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnHelpClick,
					Options::keyBattleOptions);
//	_btnOptions->setTooltip("STR_OPTIONS");
//	_btnOptions->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnOptions->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnEndTurn->onMouseClick((ActionHandler)& BattlescapeState::btnEndTurnClick);
	_btnEndTurn->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnEndTurnClick,
					Options::keyBattleEndTurn);
//	_btnEndTurn->setTooltip("STR_END_TURN");
//	_btnEndTurn->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnEndTurn->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnAbort->onMouseClick((ActionHandler)& BattlescapeState::btnAbortClick);
	_btnAbort->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnAbortClick,
					Options::keyBattleAbort);
//	_btnAbort->setTooltip("STR_ABORT_MISSION");
//	_btnAbort->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnAbort->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnStats->onMouseClick((ActionHandler)& BattlescapeState::btnStatsClick);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnStatsClick,
					Options::keyBattleStats);
//	_btnStats->setTooltip("STR_UNIT_STATS");
//	_btnStats->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnStats->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnLeftHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnLeftHandItemClick);
	_btnLeftHandItem->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnLeftHandItemClick,
					Options::keyBattleUseLeftHand);
//	_btnLeftHandItem->setTooltip("STR_USE_LEFT_HAND");
//	_btnLeftHandItem->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnLeftHandItem->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnRightHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnRightHandItemClick);
	_btnRightHandItem->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnRightHandItemClick,
					Options::keyBattleUseRightHand);
//	_btnRightHandItem->setTooltip("STR_USE_RIGHT_HAND");
//	_btnRightHandItem->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnRightHandItem->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

/*	_btnReserveNone->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveNone->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveNone);
	_btnReserveNone->setTooltip("STR_DONT_RESERVE_TIME_UNITS");
	_btnReserveNone->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveNone->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveSnap->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveSnap->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveSnap);
	_btnReserveSnap->setTooltip("STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT");
	_btnReserveSnap->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveSnap->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveAimed->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveAimed->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveAimed);
	_btnReserveAimed->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT");
	_btnReserveAimed->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveAimed->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveAuto->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveAuto->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveAuto);
	_btnReserveAuto->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT");
	_btnReserveAuto->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveAuto->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveKneel->onMouseClick((ActionHandler)& BattlescapeState::btnReserveKneelClick);
	_btnReserveKneel->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveKneelClick,
					Options::keyBattleReserveKneel);
	_btnReserveKneel->setTooltip("STR_RESERVE_TIME_UNITS_FOR_KNEEL");
	_btnReserveKneel->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveKneel->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnZeroTUs->onMouseClick(
					(ActionHandler)& BattlescapeState::btnZeroTUsClick,
					SDL_BUTTON_RIGHT);
	_btnZeroTUs->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnZeroTUsClick,
					Options::keyBattleZeroTUs);
	_btnZeroTUs->setTooltip("STR_EXPEND_ALL_TIME_UNITS");
	_btnZeroTUs->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnZeroTUs->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut); */

	// shortcuts without a specific button
//	_btnStats->onKeyboardPress(
//				(ActionHandler)& BattlescapeState::btnReloadClick,
//				Options::keyBattleReload);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnPersonalLightingClick,
					Options::keyBattlePersonalLighting);
	_btnStats->onKeyboardPress( // kL, replaces _btnZeroUTs functionality.
					(ActionHandler)& BattlescapeState::btnZeroTUsClick,
					Options::keyBattleZeroTUs);

	SDLKey buttons[] = {Options::keyBattleCenterEnemy1,
						Options::keyBattleCenterEnemy2,
						Options::keyBattleCenterEnemy3,
						Options::keyBattleCenterEnemy4,
						Options::keyBattleCenterEnemy5,
						Options::keyBattleCenterEnemy6,
						Options::keyBattleCenterEnemy7,
						Options::keyBattleCenterEnemy8,
						Options::keyBattleCenterEnemy9,
						Options::keyBattleCenterEnemy10};
	for (int
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
//kL		std::ostringstream tooltip;

		_btnVisibleUnit[i]->onMouseClick((ActionHandler)& BattlescapeState::btnVisibleUnitClick);
		_btnVisibleUnit[i]->onKeyboardPress(
						(ActionHandler)& BattlescapeState::btnVisibleUnitClick,
						buttons[i]);

//		tooltip << "STR_CENTER_ON_ENEMY_" << (i + 1);
//		_btnVisibleUnit[i]->setTooltip(tooltip.str());
//		_btnVisibleUnit[i]->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//		_btnVisibleUnit[i]->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

		_numVisibleUnit[i]->setColor(16);
		_numVisibleUnit[i]->setValue(i + 1);
	}

	_warning->setColor(Palette::blockOffset(2));
	_warning->setTextColor(Palette::blockOffset(1));

	_btnLaunch->onMouseClick((ActionHandler)& BattlescapeState::btnLaunchClick);
	_btnPsi->onMouseClick((ActionHandler)& BattlescapeState::btnPsiClick);

	_txtName->setColor(Palette::blockOffset(8));
	_txtName->setHighContrast(true);
	_numTUAim->setColor(Palette::blockOffset(0)+7);
	_numTUAuto->setColor(Palette::blockOffset(0)+7);
	_numTUSnap->setColor(Palette::blockOffset(0)+7);
	_numTimeUnits->setColor(Palette::blockOffset(4));
	_numEnergy->setColor(Palette::blockOffset(1));
	_numHealth->setColor(Palette::blockOffset(2)+11);
	_numMorale->setColor(Palette::blockOffset(12));
	_barTimeUnits->setColor(Palette::blockOffset(4));
	_barTimeUnits->setScale(1.0);
	_barEnergy->setColor(Palette::blockOffset(1));
	_barEnergy->setScale(1.0);
	_barHealth->setColor(Palette::blockOffset(2));
	_barHealth->setColor2(Palette::blockOffset(5)+2);
	_barHealth->setScale(1.0);
	_barMorale->setColor(Palette::blockOffset(12));
	_barMorale->setScale(1.0);

	_txtDebug->setColor(Palette::blockOffset(8));
	_txtDebug->setHighContrast(true);

//	_txtTooltip->setColor(Palette::blockOffset(0)-1);
//	_txtTooltip->setHighContrast(true);

/*	_btnReserveNone->copy(_icons);
	_btnReserveNone->setColor(Palette::blockOffset(4)+3);
	_btnReserveNone->setGroup(&_reserve);

	_btnReserveSnap->copy(_icons);
	_btnReserveSnap->setColor(Palette::blockOffset(2)+3);
	_btnReserveSnap->setGroup(&_reserve);

	_btnReserveAimed->copy(_icons);
	_btnReserveAimed->setColor(Palette::blockOffset(2)+3);
	_btnReserveAimed->setGroup(&_reserve);

	_btnReserveAuto->copy(_icons);
	_btnReserveAuto->setColor(Palette::blockOffset(2)+3);
	_btnReserveAuto->setGroup(&_reserve);

	_btnReserveKneel->copy(_icons);
	_btnReserveKneel->setColor(Palette::blockOffset(2)+3);

	_btnZeroTUs->copy(_icons);
	_btnZeroTUs->setColor(Palette::blockOffset(2)+3); */

//	_game->getResourcePack()->playMusic("GMTACTIC");
	std::string terrain = game->getSavedGame()->getSavedBattle()->getTerrain(); // sza_MusicRules
	_game->getResourcePack()->getRandomMusic( // sza_MusicRules
										OpenXcom::XCOM_RESOURCE_MUSIC_GMTACTIC,
										terrain)->play();

//kL	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer = new Timer(DEFAULT_ANIM_SPEED);
	_animTimer->onTimer((StateHandler)& BattlescapeState::animate);

//kL	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer = new Timer(DEFAULT_ANIM_SPEED + 32);
	_gameTimer->onTimer((StateHandler)& BattlescapeState::handleState);

	_battleGame = new BattlescapeGame(
									_save,
									this);

//kL	_firstInit = true;			// cTor.
//kL	_isMouseScrolling = false;	// cTor.
//kL	_isMouseScrolled = false;	// cTor.
//	_currentTooltip = "";
}


/**
 * Deletes the battlescapestate.
 */
BattlescapeState::~BattlescapeState()
{
	//Log(LOG_INFO) << "Delete BattlescapeState";
	delete _animTimer;
	delete _gameTimer;
	delete _battleGame;
}

/**
 * Initilizes the battlescapestate.
 */
void BattlescapeState::init()
{
	//Log(LOG_INFO) << "BattlescapeState::init()";
	State::init();

	_animTimer->start();
	_gameTimer->start();

	_map->setFocus(true);
	_map->cacheUnits();
	_map->draw();
	_battleGame->init();

	kL_TurnCount = _save->getTurn();	// kL
	_turnCounter->update();				// kL
	_turnCounter->draw();				// kL

	updateSoldierInfo();

	// Update reserve settings
/*	_battleGame->setTUReserved(_save->getTUReserved(), true);
	switch (_save->getTUReserved())
	{
		case BA_SNAPSHOT:
			_reserve = _btnReserveSnap;
		break;
		case BA_AIMEDSHOT:
			_reserve = _btnReserveAimed;
		break;
		case BA_AUTOSHOT:
			_reserve = _btnReserveAuto;
		break;

		default:
			_reserve = _btnReserveNone;
		break;
	} */

	if (_firstInit
		&& playableUnitSelected())
	{
		_firstInit = false;

		_battleGame->setupCursor();
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());

/*		_btnReserveNone->setGroup(&_reserve);
		_btnReserveSnap->setGroup(&_reserve);
		_btnReserveAimed->setGroup(&_reserve);
		_btnReserveAuto->setGroup(&_reserve); */
	}

//	_txtTooltip->setText(L"");

/*	if (_save->getKneelReserved())
	{
		_btnReserveKneel->invert(_btnReserveKneel->getColor()+3);
	}

	_btnReserveKneel->toggle(_save->getKneelReserved());
	_battleGame->setKneelReserved(_save->getKneelReserved()); */
	//Log(LOG_INFO) << "BattlescapeState::init() EXIT";
}

/**
 * Runs the timers and handles popups.
 */
void BattlescapeState::think()
{
	//Log(LOG_INFO) << "BattlescapeState::think()";

	static bool popped = false;

	if (_gameTimer->isRunning())
	{
		if (_popups.empty())
		{
			State::think();

			//Log(LOG_INFO) << "BattlescapeState::think() -> _battlegame.think()";
			_battleGame->think();
			//Log(LOG_INFO) << "BattlescapeState::think() -> _animTimer.think()";
			_animTimer->think(this, 0);
			//Log(LOG_INFO) << "BattlescapeState::think() -> _gametimer.think()";
			_gameTimer->think(this, 0);
			//Log(LOG_INFO) << "BattlescapeState::think() -> back from think";

			if (popped)
			{
				_battleGame->handleNonTargetAction();
				popped = false;
			}
		}
		else // Handle popups
		{
			_game->pushState(*_popups.begin());
			_popups.erase(_popups.begin());
			popped = true;
		}
	}
	//Log(LOG_INFO) << "BattlescapeState::think() EXIT";
}

/**
 * Processes any mouse moving over the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapOver(Action* action)
{
	//Log(LOG_INFO) << "BattlescapeState::mapOver()";

	if (_isMouseScrolling
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if ((SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		// so we missed again the mouse-release :(
		{
			// Check if we have to revoke the scrolling, because it
			// was too short in time, so it was a click
			if (!_mouseMovedOverThreshold
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}

			_isMouseScrolled = _isMouseScrolling = false;

			return;
		}

		_isMouseScrolled = true;

		// Set the mouse cursor back ( or not )
/*kL		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_IGNORE);
		SDL_WarpMouse(
					static_cast<Uint16>(_xBeforeMouseScrolling),
					static_cast<Uint16>(_yBeforeMouseScrolling));
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_ENABLE); */

		// Check the threshold
		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);
		if (!_mouseMovedOverThreshold)
			_mouseMovedOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
									|| std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;

		// Scrolling
//kL		if (Options::dragScrollInvert)
		if (!Options::dragScrollInvert) // kL
		{
			_map->getCamera()->scrollXY(
									static_cast<int>(-action->getDetails()->motion.xrel),
									static_cast<int>(-action->getDetails()->motion.yrel),
									false);
		}
		else
		{
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			_map->getCamera()->scrollXY(
								static_cast<int>(static_cast<double>(_totalMouseMoveX) / action->getXScale()),
								static_cast<int>(static_cast<double>(_totalMouseMoveY) / action->getYScale()),
								false);
		}

		// We don't want to look the mouse-cursor jumping :)
		action->getDetails()->motion.x = static_cast<Uint16>(_xBeforeMouseScrolling);
		action->getDetails()->motion.y = static_cast<Uint16>(_yBeforeMouseScrolling);

		_game->getCursor()->handle(action);
	}
	//Log(LOG_INFO) << "BattlescapeState::mapOver() EXIT";
}

/**
 * Processes any presses on the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapPress(Action* action)
{
	//Log(LOG_INFO) << "BattlescapeState::mapPress()";
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons)
		return;

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		SDL_GetMouseState(
					&_xBeforeMouseScrolling,
					&_yBeforeMouseScrolling);

		_mapOffsetBeforeMouseScrolling = _map->getCamera()->getMapOffset();
		_totalMouseMoveX = 0;
		_totalMouseMoveY = 0;
		_mouseMovedOverThreshold = false;
		_mouseScrollingStartTime = SDL_GetTicks();
	}
	//Log(LOG_INFO) << "BattlescapeState::mapPress() EXIT";
}

/**
 * Processes any clicks on the map to command units.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapClick(Action* action)
{
	//Log(LOG_INFO) << "BattlescapeState::mapClick()";

	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now another button is used)
	if (_isMouseScrolling)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
			&& (SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
			// so we missed again the mouse-release :(
		{
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (!_mouseMovedOverThreshold
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}

			_isMouseScrolled = _isMouseScrolling = false;
		}
	}

	// DragScroll-Button release: release mouse-scroll-mode
	if (_isMouseScrolling)
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
			_isMouseScrolling = false;
		else
		{
			//Log(LOG_INFO) << ". . isMouseScrolled = FALSE, return";
			return;
		}

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (!_mouseMovedOverThreshold
			&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
		}

		if (_isMouseScrolled)
		{
			//Log(LOG_INFO) << ". . isMouseScrolled == TRUE, return";
			return;
		}
	}

	// right-click aborts walking state
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		&& _battleGame->cancelCurrentAction())
	{
			//Log(LOG_INFO) << ". . cancelCurrentAction()";
			return;
	}

	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons)
		return;

	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE
		|| _battleGame->isBusy())
	{
		//Log(LOG_INFO) << ". . _map->getCursorType() == CT_NONE || _battleGame->isBusy()";
		return;
	}

	Position pos;
	_map->getSelectorPosition(&pos);
	//Log(LOG_INFO) << ". getSelectorPosition() DONE";

	if (_save->getDebugMode())
	{
		std::wostringstream ss;
		ss << L"Clicked " << pos;
		debug(ss.str());
	}

	if (_save->getTile(pos) != 0) // don't allow to click into void
	{
		//Log(LOG_INFO) << ". . getTile(pos) != 0";
		if ((action->getDetails()->button.button == SDL_BUTTON_RIGHT
				|| (action->getDetails()->button.button == SDL_BUTTON_LEFT
					&& (SDL_GetModState() & KMOD_ALT) != 0))
			&& playableUnitSelected())
		{
			//Log(LOG_INFO) << ". . playableUnitSelected()";

			_battleGame->secondaryAction(pos);
			//Log(LOG_INFO) << ". . secondaryAction(pos) DONE";
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			//Log(LOG_INFO) << ". . playableUnit NOT Selected()";
			_battleGame->primaryAction(pos);
			//Log(LOG_INFO) << ". . primaryAction(pos) DONE";
		}
	}
	//Log(LOG_INFO) << "BattlescapeState::mapClick() EXIT";
}

/**
 * Handles mouse entering the map surface.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapIn(Action*)
{
	//Log(LOG_INFO) << "BattlescapeState::mapIn()";
	_isMouseScrolling = false;
	_map->setButtonsPressed(Options::battleDragScrollButton, false);

	//Log(LOG_INFO) << "BattlescapeState::mapIn() EXIT";
}

/**
 * Moves the selected unit up.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitUpClick(Action*)
{
	if (!playableUnitSelected())
		return;

	int valid = _save->getPathfinding()->validateUpDown(
												_save->getSelectedUnit(),
												_save->getSelectedUnit()->getPosition(),
												Pathfinding::DIR_UP);

	if (valid > 0) // gravLift or flying
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(
							_save->getSelectedUnit(),
							Pathfinding::DIR_UP);
	}
	else if (valid == -1) // kneeling
		warning("STR_ACTION_NOT_ALLOWED_KNEEL");
	else // blocked, roof
		warning("STR_ACTION_NOT_ALLOWED_ROOF");
}

/**
 * Moves the selected unit down.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitDownClick(Action*)
{
	if (!playableUnitSelected())
		return;

	int valid = _save->getPathfinding()->validateUpDown(
												_save->getSelectedUnit(),
												_save->getSelectedUnit()->getPosition(),
												Pathfinding::DIR_DOWN);

	if (valid > 0) // gravLift or flying
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(
							_save->getSelectedUnit(),
							Pathfinding::DIR_DOWN);
	}
	else // blocked, floor
		warning("STR_ACTION_NOT_ALLOWED_FLOOR");
}

/**
 * Shows the next map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapUpClick(Action*)
{
	if (_save->getSide() == FACTION_PLAYER
		|| _save->getDebugMode())
	{
		_map->getCamera()->up();
	}
}

/**
 * Shows the previous map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapDownClick(Action*)
{
	if (_save->getSide() == FACTION_PLAYER
		|| _save->getDebugMode())
	{
		_map->getCamera()->down();
	}
}

/**
 * Shows the minimap.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowMapClick(Action*)
{
	if (allowButtons())
		_game->pushState(new MiniMapState(
										_game,
										_map->getCamera(),
										_save));
}

/**
 * Toggles the current unit's kneel/standup status.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnKneelClick(Action*)
{
	if (allowButtons())
	{
		BattleUnit* bu = _save->getSelectedUnit();
		if (bu)
		{
			//Log(LOG_INFO) << "BattlescapeState::btnKneelClick()";
			if (_battleGame->kneel(bu))
			{
				updateSoldierInfo(false); // kL

				_battleGame->getTileEngine()->calculateFOV(bu->getPosition()); // kL
					// need this here, so that my newVis algorithm works without
					// false positives, or true negatives as it were, when a soldier
					// stands up and walks in one go via UnitWalkBState. Because if
					// I calculate newVis in kneel() it says 'yeh you see something'
					// but the soldier wouldn't stop - so newVis has to be calculated
					// directly in UnitWalkBState.... yet by doing it here on the
					// btn-press, the enemy visibility indicator should light up.
					//
					// Will check reactionFire in BattlescapeGame::kneel()
					// no, no it won't.
				_battleGame->getTileEngine()->checkReactionFire(bu); // kL

				if (_battleGame->getPathfinding()->isPathPreviewed())
				{
					_battleGame->getPathfinding()->calculate(
														_battleGame->getCurrentAction()->actor,
														_battleGame->getCurrentAction()->target);
					_battleGame->getPathfinding()->removePreview();
					_battleGame->getPathfinding()->previewPath();
				}
			}
		}
	}
}

/**
 * Goes to the soldier info screen.
 * Additionally resets TUs for current side in debug mode.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnInventoryClick(Action*)
{
	if (_save->getDebugMode())
	{
		for (std::vector<BattleUnit*>::iterator
				i = _save->getUnits()->begin();
				i != _save->getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == _save->getSide())
				(*i)->prepareNewTurn();

			updateSoldierInfo();
		}
	}

	if (playableUnitSelected()
		&& (_save->getSelectedUnit()->getArmor()->getSize() == 1
			|| _save->getDebugMode())
		&& (_save->getSelectedUnit()->getOriginalFaction() == FACTION_PLAYER
			|| _save->getSelectedUnit()->getRankString() != "STR_LIVE_TERRORIST"))
	{
		// clean up the waypoints
		if (_battleGame->getCurrentAction()->type == BA_LAUNCH)
		{
			_battleGame->getCurrentAction()->waypoints.clear();
			_battleGame->getMap()->getWaypoints()->clear();
			showLaunchButton(false);
		}

		_battleGame->getPathfinding()->removePreview();
		_battleGame->cancelCurrentAction(true);

		_game->pushState(new InventoryState(
										_game,
										!_save->getDebugMode(),
										this));
	}
}

/**
 * Centers on the currently selected soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnCenterClick(Action*)
{
	if (playableUnitSelected())
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextSoldierClick(Action*)
{
	if (allowButtons())
		selectNextFactionUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextStopClick(Action*)
{
	if (allowButtons())
		selectNextFactionUnit(true, true);
}

/**
 * Selects next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPrevSoldierClick(Action*)
{
	if (allowButtons())
		selectPreviousFactionUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPrevStopClick(Action*)
{
	if (allowButtons())
		selectPreviousFactionUnit(true, true);
}

/**
 * Selects the next soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setDontReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectNextFactionUnit(
		bool checkReselect,
		bool setDontReselect,
		bool checkInventory)
{
	if (allowButtons())
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE)
			return;

		BattleUnit* unit = _save->selectNextFactionUnit(
												checkReselect,
												setDontReselect,
												checkInventory);
		updateSoldierInfo();

		if (unit)
			_map->getCamera()->centerOnPosition(unit->getPosition());

		_battleGame->cancelCurrentAction();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}

/**
 * Selects the previous soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setDontReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectPreviousFactionUnit(
		bool checkReselect,
		bool setDontReselect,
		bool checkInventory)
{
	if (allowButtons())
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE)
			return;

		BattleUnit* unit = _save->selectPreviousFactionUnit(
													checkReselect,
													setDontReselect,
													checkInventory);
		updateSoldierInfo();

		if (unit)
			_map->getCamera()->centerOnPosition(unit->getPosition());

		_battleGame->cancelCurrentAction();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}
/**
 * Shows/hides all map layers.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowLayersClick(Action*)
{
	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());
}

/**
 * Shows options.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnHelpClick(Action*)
{
	if (allowButtons(true))
		_game->pushState(new PauseState(
									_game,
									OPT_BATTLESCAPE));
}

/**
 * Requests the end of turn. This will add a 0 to the end of the state queue,
 * so all ongoing actions, like explosions are finished first before really switching turn.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnEndTurnClick(Action*)
{
	//Log(LOG_INFO) << "BattlescapeState::btnEndTurnClick()";
	if (allowButtons())
	{
//		_txtTooltip->setText(L"");
		_battleGame->requestEndTurn();
	}
	//Log(LOG_INFO) << "BattlescapeState::btnEndTurnClick() EXIT";
}

/**
 * Aborts the game.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnAbortClick(Action*)
{
	if (allowButtons())
	{
		//Log(LOG_INFO) << "BattlescapeState::btnAbortClick()";
		_game->pushState(new AbortMissionState(
											_game,
											_save,
											this));
	}
	//Log(LOG_INFO) << "BattlescapeState::btnAbortClick() EXIT";
}

/**
 * Shows the selected soldier's info.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnStatsClick(Action* action)
{
	if (playableUnitSelected())
	{
		bool edge = false;
		if (SCROLL_TRIGGER == Options::battleEdgeScroll
			&& SDL_MOUSEBUTTONUP == action->getDetails()->type
			&& SDL_BUTTON_LEFT == action->getDetails()->button.button)
		{
			int posX = action->getXMouse();
			int posY = action->getYMouse();
			if ((posX < Camera::SCROLL_BORDER * action->getXScale() && posX > 0)
				|| posX > (_map->getWidth() - Camera::SCROLL_BORDER) * action->getXScale()
				|| (posY < (Camera::SCROLL_BORDER * action->getYScale()) && posY > 0)
				|| posY > (_map->getHeight() - Camera::SCROLL_BORDER) * action->getYScale())
			{
				// To avoid handling this event as a click on the stats
				// button when the mouse is on the scroll-border
				edge = true;
			}
		}

		// clean up the waypoints
		if (_battleGame->getCurrentAction()->type == BA_LAUNCH)
		{
			_battleGame->getCurrentAction()->waypoints.clear();
			_battleGame->getMap()->getWaypoints()->clear();

			showLaunchButton(false);
		}

		_battleGame->cancelCurrentAction(true);

		if (!edge)
			popup(new UnitInfoState(
								_game,
								_save->getSelectedUnit(),
								this,
								false,
								false));
	}
}

/**
 * Shows an action popup menu. When clicked, creates the action.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnLeftHandItemClick(Action*)
{
//Old	if (_battleGame->getCurrentAction()->type != BA_NONE) return;

	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
//kL		if (_battleGame->getCurrentAction()->targeting)
//kL		{
//kL			_battleGame->cancelCurrentAction();
//kL			return;
//kL		}

		_battleGame->cancelCurrentAction();

		_save->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");
		_map->cacheUnits();
		_map->draw();

		BattleItem* leftHandItem = _save->getSelectedUnit()->getItem("STR_LEFT_HAND");
		handleItemClick(leftHandItem);
	}
}

/**
 * Shows an action popup menu. When clicked, create the action.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnRightHandItemClick(Action*)
{
//Old	if (_battleGame->getCurrentAction()->type != BA_NONE) return;

	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
//kL		if (_battleGame->getCurrentAction()->targeting)
//kL		{
//kL			_battleGame->cancelCurrentAction();
//kL			return;
//kL		}

		_battleGame->cancelCurrentAction();

		_save->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");
		_map->cacheUnits();
		_map->draw();

		BattleItem* rightHandItem = _save->getSelectedUnit()->getItem("STR_RIGHT_HAND");
		handleItemClick(rightHandItem);
	}
}

/**
 * Centers on the unit corresponding to this button.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnVisibleUnitClick(Action* action)
{
	int btnID = -1;

	// got to find out which button was pressed
	for (int
			i = 0;
			i < VISIBLE_MAX
				&& btnID == -1;
			++i)
	{
		if (action->getSender() == _btnVisibleUnit[i])
			btnID = i;
	}

	if (btnID != -1)
		_map->getCamera()->centerOnPosition(_visibleUnit[btnID]->getPosition());

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Launches the blaster bomb.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnLaunchClick(Action* action)
{
	_battleGame->launchAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Uses psionics.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnPsiClick(Action* action)
{
	_battleGame->psiButtonAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Reserves time units.
 * @param action Pointer to an action.
 */
/* void BattlescapeState::btnReserveClick(Action* action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);

		if		(_reserve == _btnReserveNone)	_battleGame->setTUReserved(BA_NONE, true);
		else if (_reserve == _btnReserveSnap)	_battleGame->setTUReserved(BA_SNAPSHOT, true);
		else if (_reserve == _btnReserveAimed)	_battleGame->setTUReserved(BA_AIMEDSHOT, true);
		else if (_reserve == _btnReserveAuto)	_battleGame->setTUReserved(BA_AUTOSHOT, true);

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
} */

/**
 * Reloads the weapon in hand.
 * @param action Pointer to an action.
 */
/* void BattlescapeState::btnReloadClick(Action*)
{
	if (playableUnitSelected()
		&& _save->getSelectedUnit()->checkAmmo())
	{
		_game->getResourcePack()->getSound("BATTLE.CAT", 17)->play();

		updateSoldierInfo();
	}
} */

/**
 * Toggles soldier's personal lighting (purely cosmetic).
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPersonalLightingClick(Action*)
{
	if (allowButtons())
		_save->getTileEngine()->togglePersonalLighting();
}

/**
 * Determines whether a playable unit is selected. Normally only player side
 * units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if stats can be displayed and action buttons will work.
 * @return, True if a playable unit is selected.
 */
bool BattlescapeState::playableUnitSelected()
{
	return _save->getSelectedUnit() != 0
		&& allowButtons();
}

/**
 * Updates a unit's onScreen stats & info.
 */
void BattlescapeState::updateSoldierInfo(bool calcFoV)
{
	//Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo( " << calcFoV << " )";

	for (int // remove red target indicators
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);

		_visibleUnit[i] = 0;
	}

	_rank->clear();

	_btnRightHandItem->clear();
	_btnLeftHandItem->clear();

	_btnRightHandItem	->setVisible(false);
	_btnLeftHandItem	->setVisible(false);
	_numAmmoRight		->setVisible(false);
	_numAmmoLeft		->setVisible(false);

	_kneel		->setVisible(false);

	_numTUAim	->setVisible(false);
	_numTUAuto	->setVisible(false);
	_numTUSnap	->setVisible(false);


	bool isPlayable = playableUnitSelected(); // not aLien or civilian; ie. xCom Soldier
	//Log(LOG_INFO) << ". isPlayable = " << isPlayable;

	if (!isPlayable) // not xCom Soldier; ie. aLien or civilian
	{
		_txtName->setText(L"");

		showPsiButton(false);

		_rank			->setVisible(false);

		_numTimeUnits	->setVisible(false);
		_barTimeUnits	->setVisible(false);
		_barTimeUnits	->setVisible(false);

		_numEnergy		->setVisible(false);
		_barEnergy		->setVisible(false);
		_barEnergy		->setVisible(false);

		_numHealth		->setVisible(false);
		_barHealth		->setVisible(false);
		_barHealth		->setVisible(false);

		_numMorale		->setVisible(false);
		_barMorale		->setVisible(false);
		_barMorale		->setVisible(false);

		//Log(LOG_INFO) << ". . return";
		return;
	}
	else // not aLien or civilian; ie. xCom Soldier
	{
		_rank			->setVisible(true);

		_numTimeUnits	->setVisible(true);
		_barTimeUnits	->setVisible(true);
		_barTimeUnits	->setVisible(true);

		_numEnergy		->setVisible(true);
		_barEnergy		->setVisible(true);
		_barEnergy		->setVisible(true);

		_numHealth		->setVisible(true);
		_barHealth		->setVisible(true);
		_barHealth		->setVisible(true);

		_numMorale		->setVisible(true);
		_barMorale		->setVisible(true);
		_barMorale		->setVisible(true);
	}


	BattleUnit* selectedUnit = 0;
	if (_save->getSelectedUnit())
		selectedUnit = _save->getSelectedUnit();
		//Log(LOG_INFO) << ". . selectedUnit ID " << selectedUnit->getId();
	else // safety.
		//Log(LOG_INFO) << ". . selectedUnit = 0 return";
		return;


	if (calcFoV)
		_save->getTileEngine()->calculateFOV(selectedUnit);

	int j = 0;
	for (std::vector<BattleUnit*>::iterator
			i = selectedUnit->getVisibleUnits()->begin();
			i != selectedUnit->getVisibleUnits()->end()
				&& j < VISIBLE_MAX;
			++i,
				++j)
	{
		_btnVisibleUnit[j]->setVisible(true);
		_numVisibleUnit[j]->setVisible(true);

		_visibleUnit[j] = *i;
	}


	_txtName->setText(selectedUnit->getName(_game->getLanguage(), false));

	Soldier* s = _game->getSavedGame()->getSoldier(selectedUnit->getId());
	if (s != 0)
	{
		SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(s->getRankSprite())->blit(_rank);
	}

/*	_numTimeUnits	->setValue(selectedUnit->getTimeUnits());
	_barTimeUnits	->setMax(selectedUnit->getStats()->tu);
	_barTimeUnits	->setValue(selectedUnit->getTimeUnits());

	_numEnergy		->setValue(selectedUnit->getEnergy());
	_barEnergy		->setMax(selectedUnit->getStats()->stamina);
	_barEnergy		->setValue(selectedUnit->getEnergy());

	_numHealth		->setValue(selectedUnit->getHealth());
	_barHealth		->setMax(selectedUnit->getStats()->health);
	_barHealth		->setValue(selectedUnit->getHealth());
	_barHealth		->setValue2(selectedUnit->getStunlevel());

	_numMorale		->setValue(selectedUnit->getMorale());
	_barMorale		->setMax(100.0);
	_barMorale		->setValue(selectedUnit->getMorale()); */

	double stat = static_cast<double>(selectedUnit->getStats()->tu);
	int tu = selectedUnit->getTimeUnits();
	_numTimeUnits->setValue(static_cast<unsigned>(tu));
	_barTimeUnits->setMax(100.0);
	_barTimeUnits->setValue(ceil(
							static_cast<double>(tu) / stat * 100.0));

	stat = static_cast<double>(selectedUnit->getStats()->stamina);
	int energy = selectedUnit->getEnergy();
	_numEnergy->setValue(static_cast<unsigned>(energy));
	_barEnergy->setMax(100.0);
	_barEnergy->setValue(ceil(
							static_cast<double>(energy) / stat * 100.0));

	stat = static_cast<double>(selectedUnit->getStats()->health);
	int health = selectedUnit->getHealth();
	_numHealth->setValue(static_cast<unsigned>(health));
	_barHealth->setMax(100.0);
	_barHealth->setValue(ceil(
							static_cast<double>(health) / stat * 100.0));
	_barHealth->setValue2(ceil(
							static_cast<double>(selectedUnit->getStunlevel()) / stat * 100.0));

	int morale = selectedUnit->getMorale();
	_numMorale->setValue(static_cast<unsigned>(morale));
	_barMorale->setMax(100.0);
	_barMorale->setValue(morale);

	if (selectedUnit->isKneeled())
	{
		drawKneelIndicator();
		_kneel->setVisible(true);
	}


	BattleItem* rtItem = selectedUnit->getItem("STR_RIGHT_HAND");
	BattleItem* ltItem = selectedUnit->getItem("STR_LEFT_HAND");

	if (selectedUnit->getActiveHand() != "")
	{
		int tuAim = 0;
		int tuAuto = 0;
		int tuSnap = 0;

		if (selectedUnit->getActiveHand() == "STR_RIGHT_HAND"
			&& (rtItem->getRules()->getBattleType() == BT_FIREARM
				|| rtItem->getRules()->getBattleType() == BT_MELEE))
		{
			tuAim = selectedUnit->getActionTUs(BA_AIMEDSHOT, rtItem);
			tuAuto = selectedUnit->getActionTUs(BA_AUTOSHOT, rtItem);
			tuSnap = selectedUnit->getActionTUs(BA_SNAPSHOT, rtItem);
			if (tuAim == 0
				&& tuAuto == 0
				&& tuSnap == 0)
			{
				tuSnap = selectedUnit->getActionTUs(BA_HIT, rtItem);
			}
		}
		else if (selectedUnit->getActiveHand() == "STR_LEFT_HAND"
			&& (ltItem->getRules()->getBattleType() == BT_FIREARM
				|| ltItem->getRules()->getBattleType() == BT_MELEE))
		{
			tuAim = selectedUnit->getActionTUs(BA_AIMEDSHOT, ltItem);
			tuAuto = selectedUnit->getActionTUs(BA_AUTOSHOT, ltItem);
			tuSnap = selectedUnit->getActionTUs(BA_SNAPSHOT, ltItem);
			if (tuAim == 0
				&& tuAuto == 0
				&& tuSnap == 0)
			{
				tuSnap = selectedUnit->getActionTUs(BA_HIT, ltItem);
			}
		}

		if (tuAim)
		{
			_numTUAim->setValue(tuAim);
			_numTUAim->setVisible(true);
		}

		if (tuAuto)
		{
			_numTUAuto->setValue(tuAuto);
			_numTUAuto->setVisible(true);
		}

		if (tuSnap)
		{
			_numTUSnap->setValue(tuSnap);
			_numTUSnap->setVisible(true);
		}
	}

	if (rtItem)
	{
		rtItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_btnRightHandItem);
		_btnRightHandItem->setVisible(true);

		if (rtItem->getRules()->getBattleType() == BT_FIREARM
			&& (rtItem->needsAmmo()
				|| rtItem->getRules()->getClipSize() > 0))
		{
			_numAmmoRight->setVisible(true);
			if (rtItem->getAmmoItem())
				_numAmmoRight->setValue(rtItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoRight->setValue(0);
		}
	}

	if (ltItem)
	{
		ltItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_btnLeftHandItem);
		_btnLeftHandItem->setVisible(true);

		if (ltItem->getRules()->getBattleType() == BT_FIREARM
			&& (ltItem->needsAmmo()
				|| ltItem->getRules()->getClipSize() > 0))
		{
			_numAmmoLeft->setVisible(true);
			if (ltItem->getAmmoItem())
				_numAmmoLeft->setValue(ltItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoLeft->setValue(0);
		}
	}

	showPsiButton(
				selectedUnit->getOriginalFaction() == FACTION_HOSTILE
				&& selectedUnit->getStats()->psiSkill > 0);

	//Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo() EXIT";
}

/**
 * kL. Draws the kneel indicator.
 */
 void BattlescapeState::drawKneelIndicator() // kL
 {
	SDL_Rect square;
	square.x = 0;
	square.y = 0;
	square.w = 2;
	square.h = 2;

	_kneel->drawRect(&square, Palette::blockOffset(5)+12);
}

/**
 * Shifts the red colors of the visible unit buttons' backgrounds.
 */
void BattlescapeState::blinkVisibleUnitButtons()
{
	static int
		delta = 1,
		color = 34;

	SDL_Rect square1; // black border
	square1.x = 0;
	square1.y = 0;
	square1.w = 15;
	square1.h = 12;

	SDL_Rect square2; // inner red square
	square2.x = 1;
	square2.y = 1;
	square2.w = 13;
	square2.h = 10;

	for (int
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		if (_btnVisibleUnit[i]->getVisible() == true)
		{
			_btnVisibleUnit[i]->drawRect(&square1, 15);		// black border
			_btnVisibleUnit[i]->drawRect(&square2, color);	// inner red square
		}
	}

//kL	if (color == 44) delta = -2;
	if (color == 45) // reached darkish red
		delta = -1;
	if (color == 34) // reached lightest red
		delta = 1;

	color += delta;
}

/**
 * Animates map objects on the map, also smoke,fire, ...
 */
void BattlescapeState::animate()
{
	_map->animate(!_battleGame->isBusy());

	blinkVisibleUnitButtons();
}

/**
 * Popups a context sensitive list of actions the user can choose from.
 * Some actions result in a change of gamestate.
 * @param item Item the user clicked on (righthand/lefthand)
 */
void BattlescapeState::handleItemClick(BattleItem* item)
{
	// make sure there is an item, and the battlescape is in an idle state
	if (item
		&& !_battleGame->isBusy())
	{
		if (_game->getSavedGame()->isResearched(item->getRules()->getRequirements())
			|| _save->getSelectedUnit()->getOriginalFaction() == FACTION_HOSTILE)
		{
			_battleGame->getCurrentAction()->weapon = item;
			popup(new ActionMenuState(
									_game,
									_battleGame->getCurrentAction(),
									_icons->getX(),
									_icons->getY() + 16));
		}
		else
			warning("STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED");
	}
}

/**
 * Handles the battle game state.
 */
void BattlescapeState::handleState()
{
	_battleGame->handleState();
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void BattlescapeState::setStateInterval(Uint32 interval)
{
	_gameTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return Pointer to game.
 */
Game* BattlescapeState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the map. Some states need this info.
 * @return Pointer to map.
 */
Map* BattlescapeState::getMap() const
{
	return _map;
}

/**
 * Shows a debug message in the topleft corner.
 * @param message Debug message.
 */
void BattlescapeState::debug(const std::wstring& message)
{
	if (_save->getDebugMode())
		_txtDebug->setText(message);
}

/**
 * Shows a warning message.
 * @param message Warning message.
 */
//kL void BattlescapeState::warning(const std::string& message)
void BattlescapeState::warning( // kL
		const std::string& message,
		const bool useArg,
		const int arg)
{
	if (!useArg)										// kL
		_warning->showMessage(tr(message));
	else												// kL
		_warning->showMessage(tr(message).arg(arg));	// kL
}

/**
 * Takes care of any events from the core game engine.
 * @param action, Pointer to an action.
 */
inline void BattlescapeState::handle(Action* action)
{
	if (_game->getCursor()->getVisible()
		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		State::handle(action);

		if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
		{
			if (action->getDetails()->button.button == SDL_BUTTON_X1)
				btnNextSoldierClick(action);
			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
				btnPrevSoldierClick(action);
		}

		if (action->getDetails()->type == SDL_KEYDOWN)
		{
			if (Options::debug)
			{
				// "ctrl-d" - enable debug mode
				if (action->getDetails()->key.keysym.sym == SDLK_d
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					_save->setDebugMode();
					debug(L"Debug Mode");
				}
				// "ctrl-l" - reset tile visibility
				else if (_save->getDebugMode()
					&& action->getDetails()->key.keysym.sym == SDLK_l
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					debug(L"Resetting tile visibility");
					_save->resetTiles();
				}
				// "ctrl-k" - kill all aliens
				else if (_save->getDebugMode()
					&& action->getDetails()->key.keysym.sym == SDLK_k
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					debug(L"Influenza bacterium dispersed");
					for (std::vector<BattleUnit*>::iterator
							i = _save->getUnits()->begin();
							i !=_save->getUnits()->end();
							++i)
					{
						if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
						{
							(*i)->instaKill();
							if ((*i)->getTile())
								(*i)->getTile()->setUnit(0);
						}
					}
				}
				// f11 - voxel map dump
				else if (action->getDetails()->key.keysym.sym == SDLK_F11)
					saveVoxelMap();
				// f9 - ai
				else if (action->getDetails()->key.keysym.sym == SDLK_F9
					&& Options::traceAI)
				{
					saveAIMap();
				}
			}
			// quick save and quick load
			// not works in debug mode to prevent conflict in hotkeys by default
			else if (!_game->getSavedGame()->isIronman())
			{
				if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)
				{
					_game->pushState(new SaveGameState(
													_game,
													OPT_BATTLESCAPE,
													SAVE_QUICK));
				}
				else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)
				{
					_game->pushState(new LoadGameState(
													_game,
													OPT_BATTLESCAPE,
													SAVE_QUICK));
				}
			}

			// voxel view dump
			if (action->getDetails()->key.keysym.sym == Options::keyBattleVoxelView)
				saveVoxelView();
		}
	}
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
	// kL_note: Not used:
	Uint32 start = SDL_GetTicks();

	BattleUnit* unit = _save->getSelectedUnit();
	if (!unit)
		return;

	int
		w = _save->getMapSizeX(),
		h = _save->getMapSizeY();
	Position pos(unit->getPosition());

	int expMax = 0;

	SDL_Surface* img = SDL_AllocSurface(
									0,
									w * 8,
									h * 8,
									24,
									0xff,
									0xff00,
									0xff0000,
									0);
	memset(
		img->pixels,
		0,
		img->pitch * img->h);

	Position tilePos(pos);

	SDL_Rect r;
	r.h = 8;
	r.w = 8;

	for (int
			y = 0;
			y < h;
			++y)
	{
		tilePos.y = y;
		for (int
				x = 0;
				x < w;
				++x)
		{
			tilePos.x = x;
			Tile* t = _save->getTile(tilePos);

			if (!t)
				continue;
			if (!t->isDiscovered(2))
				continue;
		}
	}

	if (expMax < 100)
		expMax = 100;

	for (int
			y = 0;
			y < h;
			++y)
	{
		tilePos.y = y;
		for (int
				x = 0;
				x < w;
				++x)
		{
			tilePos.x = x;
			Tile* t = _save->getTile(tilePos);

			if (!t)
				continue;
			if (!t->isDiscovered(2))
				continue;

			r.x = x * r.w;
			r.y = y * r.h;

			if (t->getTUCost(MapData::O_FLOOR, MT_FLY) != 255
				&& t->getTUCost(MapData::O_OBJECT, MT_FLY) != 255)
			{
				SDL_FillRect(img, &r, SDL_MapRGB(img->format, 255, 0, 0x20));
				characterRGBA(img, r.x, r.y, '*' , 0x7f, 0x7f, 0x7f, 0x7f);
			}
			else
			{
				if (!t->getUnit())
					SDL_FillRect(img, &r, SDL_MapRGB(img->format, 0x50, 0x50, 0x50)); // gray for blocked tile
			}

			for (int
					z = tilePos.z;
					z >= 0;
					--z)
			{
				Position pos(
							tilePos.x,
							tilePos.y,
							z);

				t = _save->getTile(pos);
				BattleUnit* wat = t->getUnit();
				if (wat)
				{
					switch (wat->getFaction())
					{
						case FACTION_HOSTILE:
							// #4080C0 is Volutar Blue
							characterRGBA(
										img,
										r.x,
										r.y,
										(tilePos.z - z)? 'a': 'A',
										0x40,
										0x80,
										0xC0,
										0xff);
						break;
						case FACTION_PLAYER:
							characterRGBA(
										img,
										r.x,
										r.y,
										(tilePos.z - z)? 'x': 'X',
										255,
										255,
										127,
										0xff);
						break;
						case FACTION_NEUTRAL:
							characterRGBA(
										img,
										r.x,
										r.y,
										(tilePos.z - z)? 'c': 'C',
										255,
										127,
										127,
										0xff);
						break;
					}

					break;
				}

				pos.z--;
				if (z > 0
					&& !t->hasNoFloor(_save->getTile(pos)))
				{
					break; // no seeing through floors
				}
			}

			if (t->getMapData(MapData::O_NORTHWALL)
				&& t->getMapData(MapData::O_NORTHWALL)->getTUCost(MT_FLY) == 255)
			{
				lineRGBA(
						img,
						r.x,
						r.y,
						r.x + r.w,
						r.y,
						0x50,
						0x50,
						0x50,
						255);
			}

			if (t->getMapData(MapData::O_WESTWALL)
				&& t->getMapData(MapData::O_WESTWALL)->getTUCost(MT_FLY) == 255)
			{
				lineRGBA(
						img,
						r.x,
						r.y,
						r.x,
						r.y + r.h,
						0x50,
						0x50,
						0x50,
						255);
			}
		}
	}

	std::ostringstream ss;

	ss.str("");
	ss << "z = " << tilePos.z;
	stringRGBA(
			img,
			12,
			12,
			ss.str().c_str(),
			0,
			0,
			0,
			0x7f);

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getUserFolder() << "AIExposure" << std::setfill('0') << std::setw(3) << i << ".png";

		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));


	unsigned error = lodepng::encode(
									ss.str(),
									static_cast<const unsigned char*>(img->pixels),
									img->w,
									img->h,
									LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	SDL_FreeSurface(img);
}

/**
 * Saves a first-person voxel view of the battlescape.
 */
void BattlescapeState::saveVoxelView()
{
	static const unsigned char pal[30] =
	{
		  0,   0,   0,
		224, 224, 224,	// ground
		192, 224, 255,	// west wall
		255, 224, 192,	// north wall
		128, 255, 128,	// object
		192,   0, 255,	// enemy unit
		  0,   0,   0,
		255, 255, 255,
		224, 192,   0,	// xcom unit
		255,  64, 128	// neutral unit
	};

	BattleUnit* bu = _save->getSelectedUnit();
	if (bu == 0) // no unit selected
		return;

	bool
		_debug = _save->getDebugMode(),
		black = false;
	int test;
	double
		ang_x = 0.0,
		ang_y = 0.0,
		dist = 0.0,
		dir = static_cast<double>(bu->getDirection() + 4) / 4.0 * M_PI;

	std::ostringstream ss;

	std::vector<unsigned char> image;
	std::vector<Position> _trajectory;

	Position
		originVoxel = getBattleGame()->getTileEngine()->getSightOriginVoxel(bu),
		targetVoxel,
		hitPos;
	Tile* tile = 0;

	image.clear();

	for (int
			y = -256 + 32;
			y < 256 + 32;
			++y)
	{
		ang_y = (((double)y)/640*M_PI+M_PI/2);
//		ang_y = (static_cast<double>(y)) / 640. * M_PI + M_PI / 2.;	// kL
		for (int x = -256; x < 256; ++x)
		{
			ang_x = ((double)x/1024)*M_PI+dir;
//			ang_x = (static_cast<double>(x / 1024)) * M_PI + dir;	//kL, getting way too confusing here.

			targetVoxel.x=originVoxel.x + (int)(-sin(ang_x) * 1024 * sin(ang_y));
			targetVoxel.y=originVoxel.y + (int)(cos(ang_x) * 1024 * sin(ang_y));
			targetVoxel.z=originVoxel.z + (int)(cos(ang_y) * 1024);

			_trajectory.clear();
			test = _save->getTileEngine()->calculateLine(
													originVoxel,
													targetVoxel,
													false,
													&_trajectory,
													bu,
													true,
													!_debug)
												+ 1;
			black = true;
			if (test != 0 && test != 6)
			{
				tile = _save->getTile(Position(_trajectory.at(0).x/16, _trajectory.at(0).y/16, _trajectory.at(0).z/24));
				if (_debug
					|| (tile->isDiscovered(0) && test == 2)
					|| (tile->isDiscovered(1) && test == 3)
					|| (tile->isDiscovered(2) && (test == 1 || test == 4))
					|| test == 5)
				{
					if (test == 5)
					{
						if (tile->getUnit())
						{
							if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
							else if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
						}
						else
						{
							tile = _save->getTile(Position(_trajectory.at(0).x/16, _trajectory.at(0).y/16, _trajectory.at(0).z/24-1));
							if (tile && tile->getUnit())
							{
								if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
								else if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
							}
						}
					}

					hitPos = Position(_trajectory.at(0).x, _trajectory.at(0).y, _trajectory.at(0).z);
					dist = sqrt(static_cast<double>(
							(hitPos.x-originVoxel.x) * (hitPos.x-originVoxel.x)
								+ (hitPos.y-originVoxel.y) * (hitPos.y-originVoxel.y)
								+ (hitPos.z-originVoxel.z) * (hitPos.z-originVoxel.z)));

					black = false;
				}
			}

			if (black)
			{
				dist = 0;
			}
			else
			{
				if (dist > 1000.0) dist = 1000.0;
				if (dist < 1.0) dist = 1.0;
				dist = (1000.0 - (log(dist)) * 140.0) / 700.0;

				if (hitPos.x % 16 == 15)
				{
					dist *= 0.9;
				}

				if (hitPos.y % 16 == 15)
				{
					dist *= 0.9;
				}

				if (hitPos.z % 24 == 23)
				{
					dist *= 0.9;
				}

				if (dist > 1.0) dist = 1.0;

				if (tile) dist *= (16.0 - static_cast<double>(tile->getShade())) / 16.0;
			}

			image.push_back((int)((float)(pal[test*3+0])*dist));
			image.push_back((int)((float)(pal[test*3+1])*dist));
			image.push_back((int)((float)(pal[test*3+2])*dist));
		}
	}

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getUserFolder() << "fpslook" << std::setfill('0') << std::setw(3) << i << ".png";

		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));


	unsigned error = lodepng::encode(ss.str(), image, 512, 512, LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	return;
}

/**
 * Saves each layer of voxels on the battlescape as a png.
 */
void BattlescapeState::saveVoxelMap()
{
	std::ostringstream ss;
	std::vector<unsigned char> image;
	static const unsigned char pal[30] =
	{
		255,255,255,
		224,224,224,
		128,160,255,
		255,160,128,
		128,255,128,
		192,  0,255,
		255,255,255,
		255,255,255,
		224,192,  0,
		255, 64,128
	};

	Tile* tile;

	for (int z = 0; z < _save->getMapSizeZ() * 12; ++z)
	{
		image.clear();

		for (int y = 0; y < _save->getMapSizeY() * 16; ++y)
		{
			for (int x = 0; x < _save->getMapSizeX() * 16; ++x)
			{
				int test = _save->getTileEngine()->voxelCheck(Position(x, y, z * 2), 0, 0) + 1;
				float dist = 1;

				if (x %16 == 15)
				{
					dist *= 0.9f;
				}

				if (y %16 == 15)
				{
					dist *= 0.9f;
				}

				if (test == VOXEL_OUTOFBOUNDS)
				{
					tile = _save->getTile(Position(x / 16, y / 16, z / 12));
					if (tile->getUnit())
					{
						if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
							test = 9;
						else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
							test = 8;
					}
					else
					{
						tile = _save->getTile(Position(x / 16, y / 16, z / 12 - 1));
						if (tile
							&& tile->getUnit())
						{
							if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
								test = 9;
							else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
								test = 8;
						}
					}
				}

				image.push_back(static_cast<int>(static_cast<float>(pal[test * 3 + 0]) * dist));
				image.push_back(static_cast<int>(static_cast<float>(pal[test * 3 + 1]) * dist));
				image.push_back(static_cast<int>(static_cast<float>(pal[test * 3 + 2]) * dist));
			}
		}

		ss.str("");
		ss << Options::getUserFolder() << "voxel" << std::setfill('0') << std::setw(2) << z << ".png";

		unsigned error = lodepng::encode(
				ss.str(),
				image,
				_save->getMapSizeX() * 16,
				_save->getMapSizeY() * 16,
				LCT_RGB);

		if (error)
		{
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
		}
	}
}

/**
 * Adds a new popup window to the queue
 * (this prevents popups from overlapping).
 * @param state, Pointer to popup state.
 */
void BattlescapeState::popup(State* state)
{
	_popups.push_back(state);
}

/**
 * Finishes up the current battle, shuts down the battlescape
 * and presents the debriefing screen for the mission.
 * @param abort, Was the mission aborted?
 * @param inExitArea, Number of soldiers in the exit area OR number of survivors
 *		when battle finished due to either all aliens or objective being destroyed.
 */
void BattlescapeState::finishBattle(
		bool abort,
		int inExitArea)
{
	//Log(LOG_INFO) << "BattlescapeState::finishBattle()";
	while (!_game->isState(this))
		_game->popState();

	_game->getCursor()->setVisible(true);
	std::string nextStage = "";

	if (_save->getMissionType() != "STR_UFO_GROUND_ASSAULT"
		&& _save->getMissionType() != "STR_UFO_CRASH_RECOVERY")
	{
		nextStage = _game->getRuleset()->getDeployment(_save->getMissionType())->getNextStage();
	}

	if (nextStage != ""
		&& inExitArea)
	{
		// if there is a next mission stage + we have people in exit area OR we killed all aliens, load the next stage
		_popups.clear();
		_save->setMissionType(nextStage);

		BattlescapeGenerator bgen = BattlescapeGenerator(_game);

		bgen.setAlienRace("STR_MIXED");
		bgen.nextStage();

		_game->popState();
		_game->pushState(new BriefingState(
										_game,
										0,
										0));
	}
	else
	{
		//Log(LOG_INFO) << ". stopTimers, popState";

		_popups.clear();
		_animTimer->stop();
		_gameTimer->stop();
		_game->popState();

		if (abort
			|| (!abort
				&& inExitArea == 0))
		{
			//Log(LOG_INFO) << ". . missionAborted";

			// abort was done or no player is still alive
			// this concludes to defeat when in mars or mars landing mission
			if ((_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT"
					|| _save->getMissionType() == "STR_MARS_CYDONIA_LANDING")
				&& _game->getSavedGame()->getMonthsPassed() > -1)
			{
				_game->pushState(new DefeatState(_game));
			}
			else
			{
				//Log(LOG_INFO) << ". . . new DebriefingState";
				_game->pushState(new DebriefingState(_game));
				//Log(LOG_INFO) << ". . . new DebriefingState DONE";
			}
		}
		else
		{
			// no abort was done and at least a player is still alive
			// this concludes to victory when in mars mission
			if (_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT"
				&& _game->getSavedGame()->getMonthsPassed() > -1)
			{
				_game->pushState(new VictoryState(_game));
			}
			else
				_game->pushState(new DebriefingState(_game));
		}

		//Log(LOG_INFO) << ". . set Cursor & FPS colors";

		_game->getCursor()->setColor(Palette::blockOffset(15)+12);
		_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
	}

	//Log(LOG_INFO) << "BattlescapeState::finishBattle() EXIT";
}

/**
 * Shows the launch button.
 * @param show Show launch button?
 */
void BattlescapeState::showLaunchButton(bool show)
{
	_btnLaunch->setVisible(show);
}

/**
 * Shows the PSI button.
 * @param show Show PSI button?
 */
void BattlescapeState::showPsiButton(bool show)
{
	_btnPsi->setVisible(show);
}

/**
 * Clears mouse-scrolling state (isMouseScrolling).
 */
void BattlescapeState::clearMouseScrollingState()
{
	_isMouseScrolling = false;
}

/**
 * Returns a pointer to the battlegame, in case we need its functions.
 */
BattlescapeGame* BattlescapeState::getBattleGame()
{
	return _battleGame;
}

/**
 * Handler for the mouse moving over the icons, disabling the tile selection cube.
 */
void BattlescapeState::mouseInIcons(Action*)
{
	_mouseOverIcons = true;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 */
void BattlescapeState::mouseOutIcons(Action*)
{
	_mouseOverIcons = false;
}

/**
 * Checks if the mouse is over the icons.
 * @return True, if the mouse is over the icons.
 */
bool BattlescapeState::getMouseOverIcons() const
{
	return _mouseOverIcons;
}

/**
 * Determines whether the player is allowed to press buttons.
 * Buttons are disabled in the middle of a shot, during
 * the alien turn,and while a player's units are panicking.
 * The save button is an exception as we want to still be able to save if something
 * goes wrong during the alien turn, and submit the save file for dissection.
 * @param allowSaving, True if the help button was clicked.
 */
bool BattlescapeState::allowButtons(bool allowSaving) const
{
	return (
			(allowSaving
					|| _save->getSide() == FACTION_PLAYER
					|| _save->getDebugMode())
				&& (_battleGame->getPanicHandled()
					|| _firstInit)
				&& _map->getProjectile() == 0);
}

/**
 * Reserves time units for kneeling.
 * @param action Pointer to an action.
 */
/* void BattlescapeState::btnReserveKneelClick(Action* action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;

		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);
		_battleGame->setKneelReserved(!_battleGame->getKneelReserved());
//		_btnReserveKneel->invert(_btnReserveKneel->getColor()+3);
		_btnReserveKneel->toggle(_battleGame->getKneelReserved());

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
} */

/**
 * Removes all time units.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnZeroTUsClick(Action* action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;

		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);

		if (_battleGame->getSave()->getSelectedUnit())
		{
			_battleGame->getSave()->getSelectedUnit()->setTimeUnits(0);

			updateSoldierInfo();
		}
	}
}

/**
* Shows a tooltip for the appropriate button.
* @param action Pointer to an action.
*/
/* void BattlescapeState::txtTooltipIn(Action* action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtTooltip->setText(tr(_currentTooltip));
	}
} */

/**
* Clears the tooltip text.
* @param action Pointer to an action.
*/
/* void BattlescapeState::txtTooltipOut(Action* action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_txtTooltip->setText(L"");
		}
	}
} */

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void BattlescapeState::resize(
		int& dX,
		int& dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;

	int divisor = 1;

	switch (Options::battlescapeScale)
	{
		case SCALE_SCREEN_DIV_3:
			divisor = 3;
		break;
		case SCALE_SCREEN_DIV_2:
			divisor = 2;
		break;
		case SCALE_SCREEN:
		break;

		default:
			dX = 0;
			dY = 0;
		return;
	}

	Options::baseXResolution = std::max(
									Screen::ORIGINAL_WIDTH,
									Options::displayWidth / divisor);
	Options::baseYResolution = std::max(
									Screen::ORIGINAL_HEIGHT,
									Options::displayHeight / divisor);

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;

	_map->setWidth(Options::baseXResolution);
	_map->setHeight(Options::baseYResolution);
	_map->getCamera()->resize();
	_map->getCamera()->jumpXY(
							dX / 2,
							dY / 2);

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (*i != _map
			&& (*i) != _btnPsi
			&& *i != _btnLaunch
			&& *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map
			&& *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX);
		}
	}
}

/**
 * kL. Returns the TurnCounter used by the game.
 * @return, Pointer to the TurnCounter.
 */
TurnCounter* BattlescapeState::getTurnCounter() const
{
	return _turnCounter;
}

/**
 * kL. Toggles the icons' surfaces' visibility for Hidden Movement.
 */
void BattlescapeState::toggleIcons(bool vis)
{
	_icons->setVisible(vis);
	_numLayers->setVisible(vis);
}

/**
 * kL. Refreshes the visUnits indicators for UnitWalk/TurnBStates.
 */
void BattlescapeState::refreshVisUnits() // kL
{
	//Log(LOG_INFO) << "BattlescapeState::refreshVisUnits()";
	if (!playableUnitSelected())
		return;

	for (int // remove red target indicators
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);

		_visibleUnit[i] = 0;
	}

	BattleUnit* selectedUnit = 0;
	if (_save->getSelectedUnit())
	{
		selectedUnit = _save->getSelectedUnit();
		//Log(LOG_INFO) << ". selUnit ID " << selectedUnit->getId();
	}

	int j = 0;
	for (std::vector<BattleUnit*>::iterator
			i = selectedUnit->getVisibleUnits()->begin();
			i != selectedUnit->getVisibleUnits()->end()
				&& j < VISIBLE_MAX;
			++i,
				++j)
	{
		_btnVisibleUnit[j]->setVisible(true);
		_numVisibleUnit[j]->setVisible(true);

		_visibleUnit[j] = *i;
	}
}

}
