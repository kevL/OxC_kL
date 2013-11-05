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
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _USE_MATH_DEFINES

#include <cmath>
#include <sstream>
#include <iomanip>
#include <SDL_gfxPrimitives.h>
#include "Map.h"
#include "Camera.h"
#include "BattlescapeState.h"
#include "NextTurnState.h"
#include "AbortMissionState.h"
#include "BattleState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"
#include "ProjectileFlyBState.h"
#include "ExplosionBState.h"
#include "TileEngine.h"
#include "ActionMenuState.h"
#include "UnitInfoState.h"
#include "UnitDieBState.h"
#include "InventoryState.h"
#include "AlienBAIState.h"
#include "CivilianBAIState.h"
#include "Pathfinding.h"
#include "BattlescapeGame.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Music.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Action.h"
#include "../Resource/ResourcePack.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/Bar.h"
#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/BattleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Armor.h"
#include "../Engine/Timer.h"
#include "WarningMessage.h"
#include "../Menu/PauseState.h"
#include "DebriefingState.h"
#include "../Engine/RNG.h"
#include "InfoboxState.h"
#include "MiniMapState.h"
#include "BattlescapeGenerator.h"
#include "BriefingState.h"
#include "../Geoscape/DefeatState.h"
#include "../Geoscape/VictoryState.h"
#include "../lodepng.h"
#include "../Engine/Logger.h"
#include "../Engine/CrossPlatform.h"
#include "../Menu/SaveState.h"
#include "../Menu/LoadState.h"
#include "../Interface/TurnCounter.h"	// kL


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape screen.
 * @param game Pointer to the core game.
 */
BattlescapeState::BattlescapeState(Game* game)
	:
		State(game),
		_popups()
{
//	Log(LOG_INFO) << "Create BattlescapeState";		// kL
//	game->getScreen()->setScale(1.0);

	int screenWidth = Options::getInt("baseXResolution");
	int screenHeight = Options::getInt("baseYResolution");
	int iconsWidth = 320;
	int iconsHeight = 56;

	_mouseOverIcons = false;

	// Create buttonbar - this should be on the centerbottom of the screen
	_icons				= new InteractiveSurface(
								iconsWidth,
								iconsHeight,
								(screenWidth / 2) - (iconsWidth / 2),
								screenHeight - iconsHeight);

	// Create the battlemap view
	// the actual map height is the total height minus the height of the buttonbar
	int visibleMapHeight = screenHeight - iconsHeight;

	_map				= new Map(_game, screenWidth, screenHeight, 0, 0, visibleMapHeight);

	_numLayers			= new NumberText(3, 5, _icons->getX() + 232, _icons->getY() + 6);
	_rank				= new Surface(26, 23, _icons->getX() + 107, _icons->getY() + 33);
	_kneel				= new Surface(2, 2, _icons->getX() + 115, _icons->getY() + 19);

	// Create buttons
	_btnUnitUp			= new InteractiveSurface(32, 16, _icons->getX() + 48, _icons->getY());
	_btnUnitDown		= new InteractiveSurface(32, 16, _icons->getX() + 48, _icons->getY() + 16);
	_btnMapUp			= new InteractiveSurface(32, 16, _icons->getX() + 80, _icons->getY());
	_btnMapDown			= new InteractiveSurface(32, 16, _icons->getX() + 80, _icons->getY() + 16);
	_btnShowMap			= new InteractiveSurface(32, 16, _icons->getX() + 112, _icons->getY());
	_btnKneel			= new InteractiveSurface(32, 16, _icons->getX() + 112, _icons->getY() + 16);
	_btnInventory		= new InteractiveSurface(32, 16, _icons->getX() + 144, _icons->getY());
	_btnCenter			= new InteractiveSurface(32, 16, _icons->getX() + 144, _icons->getY() + 16);
	_btnNextSoldier		= new InteractiveSurface(32, 16, _icons->getX() + 176, _icons->getY());
	_btnNextStop		= new InteractiveSurface(32, 16, _icons->getX() + 176, _icons->getY() + 16);
	_btnShowLayers		= new InteractiveSurface(32, 16, _icons->getX() + 208, _icons->getY());
	_btnHelp			= new InteractiveSurface(32, 16, _icons->getX() + 208, _icons->getY() + 16);
	_btnEndTurn			= new InteractiveSurface(32, 16, _icons->getX() + 240, _icons->getY());
	_btnAbort			= new InteractiveSurface(32, 16, _icons->getX() + 240, _icons->getY() + 16);
	_btnStats			= new InteractiveSurface(164, 23, _icons->getX() + 107, _icons->getY() + 33);
	_btnReserveNone		= new ImageButton(28, 11, _icons->getX() + 49, _icons->getY() + 33);
	_btnReserveSnap		= new ImageButton(28, 11, _icons->getX() + 78, _icons->getY() + 33);
	_btnReserveAimed	= new ImageButton(28, 11, _icons->getX() + 49, _icons->getY() + 45);
	_btnReserveAuto		= new ImageButton(28, 11, _icons->getX() + 78, _icons->getY() + 45);

	// TODO: the following two are set for TFTD, but are unavailable except via hotkey
	// until the battleScape UI layout is defined in the ruleset
	_btnReserveKneel	= new ImageButton(10, 21, _icons->getX() + 43, _icons->getY() + 34);
	_btnZeroTUs			= new ImageButton(10, 21, _icons->getX() + 97, _icons->getY() + 34);

	_btnLeftHandItem	= new InteractiveSurface(32, 48, _icons->getX() + 8, _icons->getY() + 5);
	_numAmmoLeft		= new NumberText(30, 5, _icons->getX() + 8, _icons->getY() + 4);
	_btnRightHandItem	= new InteractiveSurface(32, 48, _icons->getX() + 280, _icons->getY() + 5);
	_numAmmoRight		= new NumberText(30, 5, _icons->getX() + 280, _icons->getY() + 4);

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[i] = new InteractiveSurface(15, 12, _icons->getX() + iconsWidth - 20, _icons->getY() - 16 - (i * 13));
		_numVisibleUnit[i] = new NumberText(15, 12, _icons->getX() + iconsWidth - 14 , _icons->getY() - 12 - (i * 13));
	}

	_numVisibleUnit[9]->setX(_numVisibleUnit[9]->getX() - 2); // center number 10

	_warning	= new WarningMessage(224, 24, _icons->getX() + 48, _icons->getY() + 32);

	_btnLaunch	= new InteractiveSurface(32, 24, (int)((double)game->getScreen()->getWidth() / game->getScreen()->getXScale()) - 32, 0);
	_btnLaunch->setVisible(false);

	_btnPsi		= new InteractiveSurface(32, 24, (int)((double)game->getScreen()->getWidth() / game->getScreen()->getXScale()) - 32, 25);
	_btnPsi->setVisible(false);

	// Create soldier stats summary
	_txtName		= new Text(136, 10, _icons->getX() + 135, _icons->getY() + 32);
	_numTUSnap		= new NumberText(12, 10, _icons->getX() + 258, _icons->getY() + 34);

	_numTimeUnits	= new NumberText(15, 5, _icons->getX() + 136, _icons->getY() + 42);
	_barTimeUnits	= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 41);

	_numEnergy		= new NumberText(15, 5, _icons->getX() + 154, _icons->getY() + 42);
	_barEnergy		= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 45);

	_numHealth		= new NumberText(15, 5, _icons->getX() + 136, _icons->getY() + 50);
	_barHealth		= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 49);

	_numMorale		= new NumberText(15, 5, _icons->getX() + 154, _icons->getY() + 50);
	_barMorale		= new Bar(102, 3, _icons->getX() + 170, _icons->getY() + 53);

	_txtDebug		= new Text(300, 10, 20, 0);
	_txtTooltip		= new Text(300, 10, _icons->getX() + 2, _icons->getY() - 10);

	// kL_begin:
//	Log(LOG_INFO) << ". new TurnCounter";
	_turnCounter	= new TurnCounter(20, 5, 0, 0);		// kL
//	Log(LOG_INFO) << ". new TurnCounter DONE";

//	add(_turnCounter);
//	_turnCounter->setColor(Palette::blockOffset(9));
//	_turnCounter->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_0")->getColors());
//	getTurnCounter()->setColor(Palette::blockOffset(9));
	// kL_end.

	// Set palette
	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_4")->getColors());

	// Last 16 colors are a greyish gradient
	SDL_Color color[] =
	{
		{	140,	152,	148,	0	},
		{	132,	136,	140,	0	},
		{	116,	124,	132,	0	},
		{	108,	116,	124,	0	},
		{	92,		104,	108,	0	},
		{	84,		92,		100,	0	},
		{	76,		80,		92,		0	},
		{	56,		68,		84,		0	},
		{	48,		56,		68,		0	},
		{	40,		48,		56,		0	},
		{	32,		36,		48,		0	},
		{	24,		28,		32,		0	},
		{	16,		20,		24,		0	},
		{	8,		12,		16,		0	},
		{	3,		4,		8,		0	},
		{	3,		3,		6,		0	}
	};

	_game->setPalette(color, Palette::backPos + 16, 16);

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
	add(_btnHelp);
	add(_btnEndTurn);
	add(_btnAbort);
	add(_btnStats);
	add(_txtName);
	add(_numTUSnap);
	add(_numTimeUnits);
	add(_numEnergy);
	add(_numHealth);
	add(_numMorale);
	add(_barTimeUnits);
	add(_barEnergy);
	add(_barHealth);
	add(_barMorale);
	add(_btnReserveNone);
	add(_btnReserveSnap);
	add(_btnReserveAimed);
	add(_btnReserveAuto);
	add(_btnReserveKneel);
	add(_btnZeroTUs);
	add(_btnLeftHandItem);
	add(_numAmmoLeft);
	add(_btnRightHandItem);
	add(_numAmmoRight);

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		add(_btnVisibleUnit[i]);
		add(_numVisibleUnit[i]);
	}

	add(_warning);
	add(_txtDebug);
	add(_txtTooltip);

	add(_btnLaunch);
	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(0)->blit(_btnLaunch);
	add(_btnPsi);
	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blit(_btnPsi);


	add(_turnCounter);									// kL
//	_turnCounter->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_4")->getColors());	// kL
	_turnCounter->setColor(Palette::blockOffset(9));	// kL


	_save = _game->getSavedGame()->getSavedBattle();
	_map->init();
	_map->onMouseOver((ActionHandler)& BattlescapeState::mapOver);
	_map->onMousePress((ActionHandler)& BattlescapeState::mapPress);
	_map->onMouseClick((ActionHandler)& BattlescapeState::mapClick, 0);
	_map->onMouseIn((ActionHandler)& BattlescapeState::mapIn);

	// there is some cropping going on here, because the icons image is 320x200 while we only need the bottom of it.
	Surface* s = _game->getResourcePack()->getSurface("ICONS.PCK");
	SDL_Rect* r = s->getCrop();
	r->x = 0;
	r->y = 200 - iconsHeight;
	r->w = iconsWidth;
	r->h = iconsHeight;
	s->blit(_icons);

	_numLayers->setColor(Palette::blockOffset(1)-2);
	_numLayers->setValue(1);

	_numAmmoLeft->setColor(2);
	_numAmmoLeft->setValue(999);

	_numAmmoRight->setColor(2);
	_numAmmoRight->setValue(999);

	_icons->onMouseIn((ActionHandler)& BattlescapeState::mouseInIcons);
	_icons->onMouseOut((ActionHandler)& BattlescapeState::mouseOutIcons);

	_btnUnitUp->onMouseClick((ActionHandler)& BattlescapeState::btnUnitUpClick);
	_btnUnitUp->setTooltip("STR_UNIT_LEVEL_ABOVE");
	_btnUnitUp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnUnitUp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnUnitDown->onMouseClick((ActionHandler)& BattlescapeState::btnUnitDownClick);
	_btnUnitDown->setTooltip("STR_UNIT_LEVEL_BELOW");
	_btnUnitDown->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnUnitDown->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMapUp->onMouseClick((ActionHandler)& BattlescapeState::btnMapUpClick);
	_btnMapUp->onKeyboardPress((ActionHandler)& BattlescapeState::btnMapUpClick, (SDLKey)Options::getInt("keyBattleLevelUp"));
	_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");
	_btnMapUp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnMapUp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMapDown->onMouseClick((ActionHandler)& BattlescapeState::btnMapDownClick);
	_btnMapDown->onKeyboardPress((ActionHandler)& BattlescapeState::btnMapDownClick, (SDLKey)Options::getInt("keyBattleLevelDown"));
	_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");
	_btnMapDown->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnMapDown->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnShowMap->onMouseClick((ActionHandler)& BattlescapeState::btnShowMapClick);
	_btnShowMap->onKeyboardPress((ActionHandler)& BattlescapeState::btnShowMapClick, (SDLKey)Options::getInt("keyBattleMap"));
	_btnShowMap->setTooltip("STR_MINIMAP");
	_btnShowMap->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnShowMap->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnKneel->onMouseClick((ActionHandler)& BattlescapeState::btnKneelClick);
	_btnKneel->onKeyboardPress((ActionHandler)& BattlescapeState::btnKneelClick, (SDLKey)Options::getInt("keyBattleKneel"));
	_btnKneel->setTooltip("STR_KNEEL");
	_btnKneel->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnKneel->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnInventory->onMouseClick((ActionHandler)& BattlescapeState::btnInventoryClick);
	_btnInventory->onKeyboardPress((ActionHandler)& BattlescapeState::btnInventoryClick, (SDLKey)Options::getInt("keyBattleInventory"));
	_btnInventory->setTooltip("STR_INVENTORY");
	_btnInventory->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnInventory->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnCenter->onMouseClick((ActionHandler)& BattlescapeState::btnCenterClick);
	_btnCenter->onKeyboardPress((ActionHandler)& BattlescapeState::btnCenterClick, (SDLKey)Options::getInt("keyBattleCenterUnit"));
	_btnCenter->setTooltip("STR_CENTER_SELECTED_UNIT");
	_btnCenter->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnCenter->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//kL	_btnNextSoldier->onMouseClick((ActionHandler)& BattlescapeState::btnNextSoldierClick);
	_btnNextSoldier->onMouseClick((ActionHandler)& BattlescapeState::btnNextSoldierClick, SDL_BUTTON_LEFT);		// kL
	_btnNextSoldier->onMouseClick((ActionHandler)& BattlescapeState::btnPrevSoldierClick, SDL_BUTTON_RIGHT);	// kL
	_btnNextSoldier->onKeyboardPress((ActionHandler)& BattlescapeState::btnNextSoldierClick, (SDLKey)Options::getInt("keyBattleNextUnit"));
	_btnNextSoldier->onKeyboardPress((ActionHandler)& BattlescapeState::btnPrevSoldierClick, (SDLKey)Options::getInt("keyBattlePrevUnit"));
	_btnNextSoldier->setTooltip("STR_NEXT_UNIT");
	_btnNextSoldier->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnNextSoldier->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//kL	_btnNextStop->onMouseClick((ActionHandler)& BattlescapeState::btnNextStopClick);
	_btnNextStop->onMouseClick((ActionHandler)& BattlescapeState::btnNextStopClick, SDL_BUTTON_LEFT);		// kL
	_btnNextStop->onMouseClick((ActionHandler)& BattlescapeState::btnPrevStopClick, SDL_BUTTON_RIGHT);		// kL
	_btnNextStop->onKeyboardPress((ActionHandler)& BattlescapeState::btnNextStopClick, (SDLKey)Options::getInt("keyBattleDeselectUnit"));
	_btnNextStop->setTooltip("STR_DESELECT_UNIT");
	_btnNextStop->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnNextStop->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnShowLayers->onMouseClick((ActionHandler)& BattlescapeState::btnShowLayersClick);
	_btnShowLayers->setTooltip("STR_MULTI_LEVEL_VIEW");
	_btnShowLayers->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnShowLayers->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnHelp->onMouseClick((ActionHandler)& BattlescapeState::btnHelpClick);
	_btnHelp->onKeyboardPress((ActionHandler)& BattlescapeState::btnHelpClick, (SDLKey)Options::getInt("keyBattleOptions"));
	_btnHelp->setTooltip("STR_OPTIONS");
	_btnHelp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnHelp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnEndTurn->onMouseClick((ActionHandler)& BattlescapeState::btnEndTurnClick);
	_btnEndTurn->onKeyboardPress((ActionHandler)& BattlescapeState::btnEndTurnClick, (SDLKey)Options::getInt("keyBattleEndTurn"));
	_btnEndTurn->setTooltip("STR_END_TURN");
	_btnEndTurn->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnEndTurn->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnAbort->onMouseClick((ActionHandler)& BattlescapeState::btnAbortClick);
	_btnAbort->onKeyboardPress((ActionHandler)& BattlescapeState::btnAbortClick, (SDLKey)Options::getInt("keyBattleAbort"));
	_btnAbort->setTooltip("STR_ABORT_MISSION");
	_btnAbort->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnAbort->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnStats->onMouseClick((ActionHandler)& BattlescapeState::btnStatsClick);
	_btnStats->onKeyboardPress((ActionHandler)& BattlescapeState::btnStatsClick, (SDLKey)Options::getInt("keyBattleStats"));
	_btnStats->setTooltip("STR_UNIT_STATS");
	_btnStats->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnStats->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnLeftHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnLeftHandItemClick);
	_btnRightHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnRightHandItemClick);

	_btnReserveNone->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveNone->onKeyboardPress((ActionHandler)& BattlescapeState::btnReserveClick, (SDLKey)Options::getInt("keyBattleReserveNone"));
	_btnReserveNone->setTooltip("STR_DONT_RESERVE_TUS");
	_btnReserveNone->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveNone->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveSnap->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveSnap->onKeyboardPress((ActionHandler)& BattlescapeState::btnReserveClick, (SDLKey)Options::getInt("keyBattleReserveSnap"));
	_btnReserveSnap->setTooltip("STR_RESERVE_TUS_FOR_SNAP_SHOT");
	_btnReserveSnap->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveSnap->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveAimed->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveAimed->onKeyboardPress((ActionHandler)& BattlescapeState::btnReserveClick, (SDLKey)Options::getInt("keyBattleReserveAimed"));
	_btnReserveAimed->setTooltip("STR_RESERVE_TUS_FOR_AIMED_SHOT");
	_btnReserveAimed->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveAimed->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveAuto->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveAuto->onKeyboardPress((ActionHandler)& BattlescapeState::btnReserveClick, (SDLKey)Options::getInt("keyBattleReserveAuto"));
	_btnReserveAuto->setTooltip("STR_RESERVE_TUS_FOR_AUTO_SHOT");
	_btnReserveAuto->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveAuto->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveKneel->onMouseClick((ActionHandler)& BattlescapeState::btnReserveKneelClick);
	_btnZeroTUs->onMouseClick((ActionHandler)& BattlescapeState::btnZeroTUsClick);
	// todo: make these visible based on ruleset and assign them to their own button
	_btnReserveKneel->setVisible(false);
	_btnZeroTUs->setVisible(false);

	_btnStats->onKeyboardPress((ActionHandler)& BattlescapeState::btnReserveKneelClick, (SDLKey)Options::getInt("keyBattleReserveKneel"));
	_btnStats->onKeyboardPress((ActionHandler)& BattlescapeState::btnZeroTUsClick, (SDLKey)Options::getInt("keyBattleZeroTUs"));

	// shortcuts without a specific button
	_btnStats->onKeyboardPress((ActionHandler)& BattlescapeState::btnReloadClick, (SDLKey)Options::getInt("keyBattleReload"));
	_btnStats->onKeyboardPress((ActionHandler)& BattlescapeState::btnPersonalLightingClick, (SDLKey)Options::getInt("keyBattlePersonalLighting"));

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		std::ostringstream key, tooltip;

		key << "keyBattleCenterEnemy" << (i + 1);
		_btnVisibleUnit[i]->onMouseClick((ActionHandler)& BattlescapeState::btnVisibleUnitClick);
		_btnVisibleUnit[i]->onKeyboardPress((ActionHandler)& BattlescapeState::btnVisibleUnitClick, (SDLKey)Options::getInt(key.str()));

		tooltip << "STR_CENTER_ON_ENEMY_" << (i + 1);
		_btnVisibleUnit[i]->setTooltip(tooltip.str());
		_btnVisibleUnit[i]->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
		_btnVisibleUnit[i]->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);
		_numVisibleUnit[i]->setColor(16);
		_numVisibleUnit[i]->setValue(i+1);
	}

	_warning->setColor(Palette::blockOffset(2));
	_warning->setTextColor(Palette::blockOffset(1));

	_btnLaunch->onMouseClick((ActionHandler)& BattlescapeState::btnLaunchClick);
	_btnPsi->onMouseClick((ActionHandler)& BattlescapeState::btnPsiClick);

	_txtName->setColor(Palette::blockOffset(8));
	_txtName->setHighContrast(true);
	_numTUSnap->setColor(Palette::blockOffset(0)+4);
	_numTimeUnits->setColor(Palette::blockOffset(4));
	_numEnergy->setColor(Palette::blockOffset(1));
	_numHealth->setColor(Palette::blockOffset(2));
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

	_txtTooltip->setColor(Palette::blockOffset(0)-1);
	_txtTooltip->setHighContrast(true);

	_btnReserveNone->copy(_icons);
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

	// Set music
	_game->getResourcePack()->getRandomMusic("GMTACTIC")->play();

	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer->onTimer((StateHandler) &BattlescapeState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer->onTimer((StateHandler) &BattlescapeState::handleState);

	_battleGame = new BattlescapeGame(_save, this);

	firstInit = true;
	isMouseScrolling = false;
	isMouseScrolled = false;
	_currentTooltip = "";
}


/**
 * Deletes the battlescapestate.
 */
BattlescapeState::~BattlescapeState()
{
//	Log(LOG_INFO) << "Delete BattlescapeState";		// kL

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

	_animTimer->start();
	_gameTimer->start();

	_map->focus();
	_map->cacheUnits();
	_map->draw();
	_battleGame->init();

	kL_TurnCount = _save->getTurn();	// kL
	_turnCounter->update();				// kL
	_turnCounter->draw();				// kL

	updateSoldierInfo();

	// Update reserve settings
	_battleGame->setTUReserved(_save->getTUReserved(), true);
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
	}

	if (firstInit
		&& playableUnitSelected())
	{
		firstInit = false;

		_battleGame->setupCursor();
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());

		_btnReserveNone->setGroup(&_reserve);
		_btnReserveSnap->setGroup(&_reserve);
		_btnReserveAimed->setGroup(&_reserve);
		_btnReserveAuto->setGroup(&_reserve);
	}

	_txtTooltip->setText(L"");

	if (_save->getKneelReserved())
	{
		_btnReserveKneel->invert(0);
	}

	_battleGame->setKneelReserved(_save->getKneelReserved());
}

/**
 * Runs the timers and handles popups.
 */
void BattlescapeState::think()
{
	static bool popped = false;

	if (_gameTimer->isRunning())
	{
		if (_popups.empty())
		{
			State::think();

//			Log(LOG_INFO) << "BattlescapeState::think() -> _battlegame.think()";
			_battleGame->think();
//			Log(LOG_INFO) << "BattlescapeState::think() -> _animTimer.think()";
			_animTimer->think(this, 0);
//			Log(LOG_INFO) << "BattlescapeState::think() -> _gametimer.think()";
			_gameTimer->think(this, 0);

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
}

/**
 * Processes any mouse moving over the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapOver(Action* action)
{
	//Log(LOG_INFO) << "BattlescapeState::mapOver()";

	if (isMouseScrolling && action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if (0 == (SDL_GetMouseState(0, 0) & SDL_BUTTON(_save->getDragButton())))
		// so we missed again the mouse-release :(
		{
			// Check if we have to revoke the scrolling, because it
			// was too short in time, so it was a click
			if (!mouseMovedOverThreshold
				&& SDL_GetTicks() - mouseScrollingStartTime <= (Uint32)_save->getDragTimeTolerance())
			{
				_map->getCamera()->setMapOffset(mapOffsetBeforeMouseScrolling);
			}

			isMouseScrolled = isMouseScrolling = false;

			return;
		}

		isMouseScrolled = true;

		if (_save->isDragInverted())
		{
			// Set the mouse cursor back
			SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
			SDL_WarpMouse(xBeforeMouseScrolling, yBeforeMouseScrolling);
			SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
		}

		// Check the threshold
		totalMouseMoveX += action->getDetails()->motion.xrel;
		totalMouseMoveY += action->getDetails()->motion.yrel;
		if (!mouseMovedOverThreshold)
			mouseMovedOverThreshold = std::abs(totalMouseMoveX) > _save->getDragPixelTolerance()
										|| std::abs(totalMouseMoveY) > _save->getDragPixelTolerance();

		// Scrolling
		if (_save->isDragInverted())
		{
			_map->getCamera()->scrollXY(
				-action->getDetails()->motion.xrel,
				-action->getDetails()->motion.yrel, false);

			// We don't want to look the mouse-cursor jumping :)
			action->getDetails()->motion.x = xBeforeMouseScrolling;
			action->getDetails()->motion.y = yBeforeMouseScrolling;

			_game->getCursor()->handle(action);
		}
		else
		{
			_map->getCamera()->setMapOffset(mapOffsetBeforeMouseScrolling);
			_map->getCamera()->scrollXY(
				(int)((double)totalMouseMoveX / action->getXScale()),
				(int)((double)totalMouseMoveY / action->getYScale()), false);
		}
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

	// don't handle mouseclicks below 140, because they are in the buttons area (it overlaps with map surface)
	int my = int(action->getAbsoluteYMouse());
	int mx = int(action->getAbsoluteXMouse());
	if (my > _icons->getY()
		&& my < _icons->getY()+_icons->getHeight()
		&& mx > _icons->getX()
		&& mx < _icons->getX()+_icons->getWidth())
	{
		return;
	}

	if (Options::getInt("battleScrollType") == SCROLL_DRAG)
	{
		if (action->getDetails()->button.button == _save->getDragButton())
		{
			isMouseScrolling = true;
			isMouseScrolled = false;

			SDL_GetMouseState(&xBeforeMouseScrolling, &yBeforeMouseScrolling);

			mapOffsetBeforeMouseScrolling = _map->getCamera()->getMapOffset();
			totalMouseMoveX = 0; totalMouseMoveY = 0;
			mouseMovedOverThreshold = false;
			mouseScrollingStartTime = SDL_GetTicks();
		}
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
	if (isMouseScrolling)
	{
		if (action->getDetails()->button.button != _save->getDragButton()
			&& 0 == (SDL_GetMouseState(0, 0) & SDL_BUTTON(_save->getDragButton())))
			// so we missed again the mouse-release :(
		{
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (!mouseMovedOverThreshold
				&& SDL_GetTicks() - mouseScrollingStartTime <= (Uint32)_save->getDragTimeTolerance())
			{
				_map->getCamera()->setMapOffset(mapOffsetBeforeMouseScrolling);
			}

			isMouseScrolled = isMouseScrolling = false;
		}
	}

	// DragScroll-Button release: release mouse-scroll-mode
	if (isMouseScrolling)
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == _save->getDragButton())
		{
				isMouseScrolling = false;
		}
		else
		{
			//Log(LOG_INFO) << ". . isMouseScrolled = FALSE, return";

			return;
		}

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (!mouseMovedOverThreshold
			&& SDL_GetTicks() - mouseScrollingStartTime <= (Uint32)_save->getDragTimeTolerance())
		{
			isMouseScrolled = false;
			_map->getCamera()->setMapOffset(mapOffsetBeforeMouseScrolling);
		}

		if (isMouseScrolled)
		{
			//Log(LOG_INFO) << ". . isMouseScrolled == TRUE, return";

			return;
		}
	}

	// right-click aborts walking state
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_battleGame->cancelCurrentAction())
		{
			//Log(LOG_INFO) << ". . cancelCurrentAction()";

			return;
		}
	}

	// don't handle mouseclicks below y=144px, because they are in the buttons area (it overlaps with map surface)
	int my = int(action->getAbsoluteYMouse());
	int mx = int(action->getAbsoluteXMouse());
	if (my > _icons->getY()
		&& my < _icons->getY()+_icons->getHeight()
		&& mx > _icons->getX()
		&& mx < _icons->getX()+_icons->getWidth())
	{
		return;
	}

	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE || _battleGame->isBusy())
	{
		//Log(LOG_INFO) << ". . _map->getCursorType() == CT_NONE || _battleGame->isBusy()";

		return;
	}

	Position pos;
	_map->getSelectorPosition(&pos);
	//Log(LOG_INFO) << ". getSelectorPosition() DONE";

	if (_save->getDebugMode())
	{
		std::wstringstream ss;
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

	isMouseScrolling = false;
	_map->setButtonsPressed(SDL_BUTTON_RIGHT, false);

	//Log(LOG_INFO) << "BattlescapeState::mapIn() EXIT";
}

/**
 * Moves the selected unit up.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitUpClick(Action*)
{
	if (playableUnitSelected()
		&& _save->getPathfinding()->validateUpDown(_save->getSelectedUnit(), _save->getSelectedUnit()->getPosition(), Pathfinding::DIR_UP))
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(_save->getSelectedUnit(), Pathfinding::DIR_UP);
	}
}

/**
 * Moves the selected unit down.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitDownClick(Action*)
{
	if (playableUnitSelected()
		&& _save->getPathfinding()->validateUpDown(_save->getSelectedUnit(), _save->getSelectedUnit()->getPosition(), Pathfinding::DIR_DOWN))
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(_save->getSelectedUnit(), Pathfinding::DIR_DOWN);
	}
}

/**
 * Shows the next map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapUpClick(Action*)
{
	if (_save->getSide() == FACTION_PLAYER || _save->getDebugMode())
		_map->getCamera()->up();
}

/**
 * Shows the previous map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapDownClick(Action*)
{
	if (_save->getSide() == FACTION_PLAYER || _save->getDebugMode())
		_map->getCamera()->down();
}

/**
 * Shows the minimap.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowMapClick(Action*)
{
	// MiniMapState
	if (allowButtons())
		_game->pushState (new MiniMapState (_game, _map->getCamera(), _save));
}

/**
 * Toggles the current unit's kneel/standup status.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnKneelClick(Action*)
{
	if (allowButtons())
	{
		BattleUnit* bu = _save->getSelectedUnit();
		if (bu)
		{
//			Log(LOG_INFO) << "BattlescapeState::btnKneelClick()";
			_battleGame->kneel(bu);

			if (_battleGame->getPathfinding()->isPathPreviewed() && bu->isKneeled()) // kL: Moved up from below.
			{
				_battleGame->getPathfinding()->calculate(_battleGame->getCurrentAction()->actor, _battleGame->getCurrentAction()->target);
				_battleGame->getPathfinding()->removePreview();
				_battleGame->getPathfinding()->previewPath();
			}
		}

		// update any path preview if unit kneels
/*kL		if (_battleGame->getPathfinding()->isPathPreviewed() && bu->isKneeled())
		{
			_battleGame->getPathfinding()->calculate(_battleGame->getCurrentAction()->actor, _battleGame->getCurrentAction()->target);
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		} */
	}
}

/**
 * Goes to the soldier info screen.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnInventoryClick(Action*)
{
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

		_battleGame->cancelCurrentAction(true);

		_game->pushState(new InventoryState(_game, !_save->getDebugMode(), this));
	}
}

/**
 * Centers on the currently selected soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnCenterClick(Action*)
{
	if (playableUnitSelected())
	{
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
	}
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextSoldierClick(Action*)
{
	if (allowButtons())
		selectNextPlayerUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextStopClick(Action*)
{
	if (allowButtons())
		selectNextPlayerUnit(true, true);
}

/**
 * Selects next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPrevSoldierClick(Action*)
{
	if (allowButtons())
		selectPreviousPlayerUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPrevStopClick(Action*)
{
	if (allowButtons())
		selectPreviousPlayerUnit(true, true);
}

/**
 * Selects the next soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectNextPlayerUnit(bool checkReselect, bool setReselect, bool checkInventory)
{
	if (allowButtons())
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE)
			return;

		BattleUnit* unit = _save->selectNextPlayerUnit(checkReselect, setReselect, checkInventory);
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
 * @param setReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectPreviousPlayerUnit(bool checkReselect, bool setReselect, bool checkInventory)
{
	if (allowButtons())
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE)
			return;

		BattleUnit* unit = _save->selectPreviousPlayerUnit(checkReselect, setReselect, checkInventory);
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
		_game->pushState(new PauseState(_game, OPT_BATTLESCAPE));
}

/**
 * Requests the end of turn. This will add a 0 to the end of the state queue,
 * so all ongoing actions, like explosions are finished first before really switching turn.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnEndTurnClick(Action*)
{
	if (allowButtons())
	{
//		_turnCounter->update();						// kL

		_txtTooltip->setText(L"");
		_battleGame->requestEndTurn();
	}
}

/**
 * Aborts the game.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnAbortClick(Action*)
{
	if (allowButtons())
		_game->pushState(new AbortMissionState(_game, _save, this));
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnStatsClick(Action* action)
{
	if (playableUnitSelected())
	{
		bool b = true;
		if (SCROLL_TRIGGER == Options::getInt("battleScrollType")
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
				// To avoid handling this event as a click
				// on the stats button when the mouse is on the scroll-border
				b = false;
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

		if (b) popup(new UnitInfoState(_game, _save->getSelectedUnit(), this));
	}
}

/**
 * Shows an action popup menu. When clicked, creates the action.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnLeftHandItemClick(Action*)
{
	if (_battleGame->getCurrentAction()->type != BA_NONE) return;

	if (playableUnitSelected())
	{
		_save->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");
		_map->cacheUnits();
		_map->draw();

		BattleItem* leftHandItem = _save->getSelectedUnit()->getItem("STR_LEFT_HAND");
		handleItemClick(leftHandItem);
	}
}

/**
 * Shows an action popup menu. When clicked, create the action.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnRightHandItemClick(Action*)
{
	if (_battleGame->getCurrentAction()->type != BA_NONE) return;

	if (playableUnitSelected())
	{
		_save->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");
		_map->cacheUnits();
		_map->draw();

		BattleItem* rightHandItem = _save->getSelectedUnit()->getItem("STR_RIGHT_HAND");
		handleItemClick(rightHandItem);
	}
}

/**
 * Centers on the unit corresponding to this button.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnVisibleUnitClick(Action* action)
{
	int btnID = -1;

	// got to find out which button was pressed
	for (int i = 0; i < VISIBLE_MAX && btnID == -1; ++i)
	{
		if (action->getSender() == _btnVisibleUnit[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		_map->getCamera()->centerOnPosition(_visibleUnit[btnID]->getPosition());
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Launches the blaster bomb.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnLaunchClick(Action* action)
{
	_battleGame->launchAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Uses psionics.
 * @param action Pointer to an action.
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
void BattlescapeState::btnReserveClick(Action* action)
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
	}
}

/**
 * Reloads the weapon in hand.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReloadClick(Action*)
{
	if (playableUnitSelected()
		&& _save->getSelectedUnit()->checkAmmo())
	{
		_game->getResourcePack()->getSound("BATTLE.CAT", 17)->play();

		updateSoldierInfo();
	}
}

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
 * @return Whether a playable unit is selected.
 */
bool BattlescapeState::playableUnitSelected()
{
	return _save->getSelectedUnit() != 0 && allowButtons();
}

/**
 * Updates a soldier's name/rank/tu/energy/health/morale.
 */
/*kL void BattlescapeState::updateSoldierInfo()
{
	if (!_save->getSelectedUnit())
		return;


	BattleUnit* bu = _save->getSelectedUnit();
	Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo()" << bu->getId();

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);

		_visibleUnit[i] = 0;
	}

	bool isPlayable = playableUnitSelected();

	if (isPlayable
		&& bu->isKneeled())
	{
		drawKneelIndicator();
		_kneel->setVisible(true);
	}
	else
		_kneel->setVisible(false);


	BattleItem* rightHandItem = bu->getItem("STR_RIGHT_HAND");
	BattleItem* leftHandItem = bu->getItem("STR_LEFT_HAND");

	if (isPlayable							// kL_begin:
		&& bu->getActiveHand() != "")
	{
		int tuSnap = 0;
		if (bu->getActiveHand() == "STR_RIGHT_HAND"
			&& rightHandItem->getRules()->getBattleType() == BT_FIREARM)
		{
			tuSnap = bu->getActionTUs(BA_SNAPSHOT, rightHandItem);
		}
		else if (bu->getActiveHand() == "STR_LEFT_HAND"
			&& leftHandItem->getRules()->getBattleType() == BT_FIREARM)
		{
			tuSnap = bu->getActionTUs(BA_SNAPSHOT, leftHandItem);
		}

		_numTUSnap->setValue(tuSnap);
		_numTUSnap->setVisible(true);
	}
	else
		_numTUSnap->setVisible(false);	// kL_end.


	_rank->setVisible(isPlayable);
	_numTimeUnits->setVisible(isPlayable);
	_barTimeUnits->setVisible(isPlayable);
	_barTimeUnits->setVisible(isPlayable);
	_numEnergy->setVisible(isPlayable);
	_barEnergy->setVisible(isPlayable);
	_barEnergy->setVisible(isPlayable);
	_numHealth->setVisible(isPlayable);
	_barHealth->setVisible(isPlayable);
	_barHealth->setVisible(isPlayable);
	_numMorale->setVisible(isPlayable);
	_barMorale->setVisible(isPlayable);
	_barMorale->setVisible(isPlayable);
	_btnLeftHandItem->setVisible(isPlayable);
	_btnRightHandItem->setVisible(isPlayable);
	_numAmmoLeft->setVisible(isPlayable);
	_numAmmoRight->setVisible(isPlayable);

	if (!isPlayable)
	{
		_txtName->setText(L"");
		showPsiButton(false);

		return;
	}

	_txtName->setText(bu->getName(_game->getLanguage(), false));

	Soldier* s = _game->getSavedGame()->getSoldier(bu->getId());
	if (s != 0)
	{
		SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(s->getRankSprite())->blit(_rank);
	}
	else
	{
		_rank->clear();
	}

	_numTimeUnits->setValue(bu->getTimeUnits());
	_barTimeUnits->setMax(bu->getStats()->tu);
	_barTimeUnits->setValue(bu->getTimeUnits());
	_numEnergy->setValue(bu->getEnergy());
	_barEnergy->setMax(bu->getStats()->stamina);
	_barEnergy->setValue(bu->getEnergy());
	_numHealth->setValue(bu->getHealth());
	_barHealth->setMax(bu->getStats()->health);
	_barHealth->setValue(bu->getHealth());
	_barHealth->setValue2(bu->getStunlevel());
	_numMorale->setValue(bu->getMorale());
	_barMorale->setMax(100);
	_barMorale->setValue(bu->getMorale());

//	BattleItem* rightHandItem = bu->getItem("STR_RIGHT_HAND");
	_btnRightHandItem->clear();
	_numAmmoRight->setVisible(false);
	if (rightHandItem)
	{
		rightHandItem->getRules()->drawHandSprite(_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"), _btnRightHandItem);
		if (rightHandItem->getRules()->getBattleType() == BT_FIREARM
			&& (rightHandItem->needsAmmo() || rightHandItem->getRules()->getClipSize() > 0))
		{
			_numAmmoRight->setVisible(true);
			if (rightHandItem->getAmmoItem())
				_numAmmoRight->setValue(rightHandItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoRight->setValue(0);
		}
	}

//	BattleItem* leftHandItem = bu->getItem("STR_LEFT_HAND");
	_btnLeftHandItem->clear();
	_numAmmoLeft->setVisible(false);
	if (leftHandItem)
	{
		leftHandItem->getRules()->drawHandSprite(_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"), _btnLeftHandItem);
		if (leftHandItem->getRules()->getBattleType() == BT_FIREARM
			&& (leftHandItem->needsAmmo() || leftHandItem->getRules()->getClipSize() > 0))
		{
			_numAmmoLeft->setVisible(true);
			if (leftHandItem->getAmmoItem())
				_numAmmoLeft->setValue(leftHandItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoLeft->setValue(0);
		}
	}

	_save->getTileEngine()->calculateFOV(_save->getSelectedUnit());

	int j = 0;
	for (std::vector<BattleUnit*>::iterator
			i = bu->getVisibleUnits()->begin();
			i != bu->getVisibleUnits()->end()
				&& j < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[j]->setVisible(true);
		_numVisibleUnit[j]->setVisible(true);
		_visibleUnit[j] = (*i);

		++j;
	}

	showPsiButton(bu->getOriginalFaction() == FACTION_HOSTILE && bu->getStats()->psiSkill > 0);

	Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo() EXIT";
} */
// kL_begin:
void BattlescapeState::updateSoldierInfo()
{
	for (int i = 0; i < VISIBLE_MAX; ++i) // remove red target indicators
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);

		_visibleUnit[i] = 0;
	}

	bool isPlayable = playableUnitSelected();

	_rank->setVisible(isPlayable);
	_numTimeUnits->setVisible(isPlayable);
	_barTimeUnits->setVisible(isPlayable);
	_barTimeUnits->setVisible(isPlayable);
	_numEnergy->setVisible(isPlayable);
	_barEnergy->setVisible(isPlayable);
	_barEnergy->setVisible(isPlayable);
	_numHealth->setVisible(isPlayable);
	_barHealth->setVisible(isPlayable);
	_barHealth->setVisible(isPlayable);
	_numMorale->setVisible(isPlayable);
	_barMorale->setVisible(isPlayable);
	_barMorale->setVisible(isPlayable);
	_btnLeftHandItem->setVisible(isPlayable);
	_btnRightHandItem->setVisible(isPlayable);
	_numAmmoLeft->setVisible(isPlayable);
	_numAmmoRight->setVisible(isPlayable);

	if (!isPlayable)
	{
		_txtName->setText(L"");
		showPsiButton(false);
	}

	_kneel->setVisible(false);
	_numTUSnap->setVisible(false);

	_numAmmoRight->setVisible(false);
	_numAmmoLeft->setVisible(false);

	_rank->clear();
	_btnRightHandItem->clear();
	_btnLeftHandItem->clear();


	if (!_save->getSelectedUnit())
		return;

	BattleUnit* bu = _save->getSelectedUnit();
	Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo()" << bu->getId();

	if (isPlayable
		&& bu->isKneeled())
	{
		drawKneelIndicator();
		_kneel->setVisible(true);
	}

	BattleItem* rightHandItem = bu->getItem("STR_RIGHT_HAND");
	BattleItem* leftHandItem = bu->getItem("STR_LEFT_HAND");

	if (isPlayable
		&& bu->getActiveHand() != "")
	{
		int tuSnap = 0;
		if (bu->getActiveHand() == "STR_RIGHT_HAND"
			&& rightHandItem->getRules()->getBattleType() == BT_FIREARM)
		{
			tuSnap = bu->getActionTUs(BA_SNAPSHOT, rightHandItem);
		}
		else if (bu->getActiveHand() == "STR_LEFT_HAND"
			&& leftHandItem->getRules()->getBattleType() == BT_FIREARM)
		{
			tuSnap = bu->getActionTUs(BA_SNAPSHOT, leftHandItem);
		}

		_numTUSnap->setValue(tuSnap);
		_numTUSnap->setVisible(true);
	}


	_txtName->setText(bu->getName(_game->getLanguage(), false));

	Soldier* s = _game->getSavedGame()->getSoldier(bu->getId());
	if (s != 0)
	{
		SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(s->getRankSprite())->blit(_rank);
	}

	_numTimeUnits->setValue(bu->getTimeUnits());
	_barTimeUnits->setMax(bu->getStats()->tu);
	_barTimeUnits->setValue(bu->getTimeUnits());
	_numEnergy->setValue(bu->getEnergy());
	_barEnergy->setMax(bu->getStats()->stamina);
	_barEnergy->setValue(bu->getEnergy());
	_numHealth->setValue(bu->getHealth());
	_barHealth->setMax(bu->getStats()->health);
	_barHealth->setValue(bu->getHealth());
	_barHealth->setValue2(bu->getStunlevel());
	_numMorale->setValue(bu->getMorale());
	_barMorale->setMax(100);
	_barMorale->setValue(bu->getMorale());


	if (rightHandItem)
	{
		rightHandItem->getRules()->drawHandSprite(_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"), _btnRightHandItem);
		if (rightHandItem->getRules()->getBattleType() == BT_FIREARM
			&& (rightHandItem->needsAmmo() || rightHandItem->getRules()->getClipSize() > 0))
		{
			_numAmmoRight->setVisible(true);
			if (rightHandItem->getAmmoItem())
				_numAmmoRight->setValue(rightHandItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoRight->setValue(0);
		}
	}

	if (leftHandItem)
	{
		leftHandItem->getRules()->drawHandSprite(_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"), _btnLeftHandItem);
		if (leftHandItem->getRules()->getBattleType() == BT_FIREARM
			&& (leftHandItem->needsAmmo() || leftHandItem->getRules()->getClipSize() > 0))
		{
			_numAmmoLeft->setVisible(true);
			if (leftHandItem->getAmmoItem())
				_numAmmoLeft->setValue(leftHandItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoLeft->setValue(0);
		}
	}

	_save->getTileEngine()->calculateFOV(_save->getSelectedUnit());

	int j = 0;
	for (std::vector<BattleUnit*>::iterator
			i = bu->getVisibleUnits()->begin();
			i != bu->getVisibleUnits()->end()
				&& j < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[j]->setVisible(true);
		_numVisibleUnit[j]->setVisible(true);
		_visibleUnit[j] = (*i);

		++j;
	}

	showPsiButton(
			bu->getOriginalFaction() == FACTION_HOSTILE
				&& bu->getStats()->psiSkill > 0);

	Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo() EXIT";
}
// kL_end.

/**
 * Draws the kneel indicator.
 */
 void BattlescapeState::drawKneelIndicator()
 {
	SDL_Rect square;
	square.x = 0;
	square.y = 0;
	square.w = 2;
	square.h = 2;

	_kneel->drawRect(&square, Palette::blockOffset(5)+12); // 32=red, blockOffset(5)=browns
}

/**
 * Shifts the red colors of the visible unit buttons' backgrounds.
 */
void BattlescapeState::blinkVisibleUnitButtons()
{
	static int delta = 1, color = 32;

	SDL_Rect square1;
	square1.x = 0;
	square1.y = 0;
	square1.w = 15;
	square1.h = 12;

	SDL_Rect square2;
	square2.x = 1;
	square2.y = 1;
	square2.w = 13;
	square2.h = 10;

	for (int i = 0; i < VISIBLE_MAX;  ++i)
	{
		if (_btnVisibleUnit[i]->getVisible() == true)
		{
			_btnVisibleUnit[i]->drawRect(&square1, 15);
			_btnVisibleUnit[i]->drawRect(&square2, color);
		}
	}

	if (color == 44) delta = -2;
	if (color == 32) delta = 1;

	color += delta;
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
			popup(new ActionMenuState(_game, _battleGame->getCurrentAction(), _icons->getX(), _icons->getY() + 16));
		}
		else
		{
			warning("STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED");
		}
	}
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
	{
		_txtDebug->setText(message);
	}
}

/**
 * Shows a warning message.
 * @param message Warning message.
 */
void BattlescapeState::warning(const std::string& message)
{
	_warning->showMessage(tr(message));
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
			{
				btnNextSoldierClick(action);
			}
			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
			{
				btnPrevSoldierClick(action);
			}
		}

		if (action->getDetails()->type == SDL_KEYDOWN)
		{
			if (Options::getBool("debug"))
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
					for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i !=_save->getUnits()->end(); ++i)
					{
						if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
						{
							(*i)->instaKill();
							(*i)->getTile()->setUnit(0);
						}
					}
				}
				// f11 - voxel map dump
				else if (action->getDetails()->key.keysym.sym == SDLK_F11)
				{
					saveVoxelMap();
				}
				// f9 - ai
				else if (action->getDetails()->key.keysym.sym == SDLK_F9
					&& Options::getBool("traceAI"))
				{
					saveAIMap();
				}
			}
			// quick save and quick load
			// not works in debug mode to prevent conflict in hotkeys by default
			else if (action->getDetails()->key.keysym.sym == (SDLKey)Options::getInt("keyQuickSave")
				&& Options::getInt("autosave") == 1)
			{
				_game->pushState(new SaveState(_game, OPT_BATTLESCAPE, true));
			}
			else if (action->getDetails()->key.keysym.sym == (SDLKey)Options::getInt("keyQuickLoad")
				&& Options::getInt("autosave") == 1)
			{
				_game->pushState(new LoadState(_game, OPT_BATTLESCAPE, true));
			}

			// voxel view dump
			if (action->getDetails()->key.keysym.sym == (SDLKey)Options::getInt("keyBattleVoxelView"))
			{
				saveVoxelView();
			}
		}
	}
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
	Uint32 start = SDL_GetTicks();

	BattleUnit* unit = _save->getSelectedUnit();

	if (!unit) return;


	int w = _save->getMapSizeX();
	int h = _save->getMapSizeY();
	Position pos(unit->getPosition());

	int expMax = 0;

	SDL_Surface* img = SDL_AllocSurface(0, w * 8, h * 8, 24, 0xff, 0xff00, 0xff0000, 0);
//	Log(LOG_INFO) << "unit = " << unit->getId();
	memset(img->pixels, 0, img->pitch * img->h);

	Position tilePos(pos);

	SDL_Rect r;
	r.h = 8;
	r.w = 8;

	for (int y = 0; y < h; ++y)
	{
		tilePos.y = y;
		for (int x = 0; x < w; ++x)
		{
			tilePos.x = x;
			Tile* t = _save->getTile(tilePos);

			if (!t) continue;
			if (!t->isDiscovered(2)) continue;
		}
	}

	if (expMax < 100) expMax = 100;

	for (int y = 0; y < h; ++y)
	{
		tilePos.y = y;
		for (int x = 0; x < w; ++x)
		{
			tilePos.x = x;
			Tile* t = _save->getTile(tilePos);

			if (!t) continue;
			if (!t->isDiscovered(2)) continue;

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
				if (!t->getUnit()) SDL_FillRect(img, &r, SDL_MapRGB(img->format, 0x50, 0x50, 0x50)); // gray for blocked tile
			}

			for (int z = tilePos.z; z >= 0; --z)
			{
				Position pos(tilePos.x, tilePos.y, z);
				t = _save->getTile(pos);
				BattleUnit* wat = t->getUnit();
				if (wat)
				{
					switch (wat->getFaction())
					{
						case FACTION_HOSTILE:
							// #4080C0 is Volutar Blue
							characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'a' : 'A', 0x40, 0x80, 0xC0, 0xff);
						break;
						case FACTION_PLAYER:
							characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'x' : 'X', 255, 255, 127, 0xff);
						break;
						case FACTION_NEUTRAL:
							characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'c' : 'C', 255, 127, 127, 0xff);
						break;
					}

					break;
				}

				pos.z--;
				if (z > 0 && !t->hasNoFloor(_save->getTile(pos)))
				{
					break; // no seeing through floors
				}
			}

			if (t->getMapData(MapData::O_NORTHWALL)
				&& t->getMapData(MapData::O_NORTHWALL)->getTUCost(MT_FLY) == 255)
			{
				lineRGBA(img, r.x, r.y, r.x + r.w, r.y, 0x50, 0x50, 0x50, 255);
			}

			if (t->getMapData(MapData::O_WESTWALL)
				&& t->getMapData(MapData::O_WESTWALL)->getTUCost(MT_FLY) == 255)
			{
				lineRGBA(img, r.x, r.y, r.x, r.y + r.h, 0x50, 0x50, 0x50, 255);
			}
		}
	}

	std::ostringstream ss;

	ss.str("");
	ss << "z = " << tilePos.z;
	stringRGBA(img, 12, 12, ss.str().c_str(), 0, 0, 0, 0x7f);

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getUserFolder() << "AIExposure" << std::setfill('0') << std::setw(3) << i << ".png";

		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));


	unsigned error = lodepng::encode(ss.str(), (const unsigned char*)img->pixels, img->w, img->h, LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	SDL_FreeSurface(img);

//	Log(LOG_INFO) << "saveAIMap() completed in " << SDL_GetTicks() - start << "ms.";
}

/**
 * Saves a first-person voxel view of the battlescape.
 */
void BattlescapeState::saveVoxelView()
{
	static const unsigned char pal[30] =
	{
		0,0,0,
		224,224,224,	// ground
		192,224,255,	// west wall
		255,224,192,	// north wall
		128,255,128,	// object
		192,0,255,		// enemy unit
		0,0,0,
		255,255,255,
		224,192,0,		// xcom unit
		255,64,128		// neutral unit
	};	

	BattleUnit* bu = _save->getSelectedUnit();
	if (bu == 0) return; // no unit selected

	std::vector<Position> _trajectory;

	double ang_x,ang_y;
	bool black;
	Tile *tile;
	std::ostringstream ss;
	std::vector<unsigned char> image;
	int test;

	Position originVoxel = getBattleGame()->getTileEngine()->getSightOriginVoxel(bu);

	Position targetVoxel, hitPos;

	double dist;
	bool _debug = _save->getDebugMode();
	double dir = ((float) bu->getDirection() + 4) / 4 * M_PI;

	image.clear();

	for (int y = -256+32; y < 256+32; ++y)
	{
		ang_y = (((double)y)/640*M_PI+M_PI/2);
		for (int x = -256; x < 256; ++x)
		{
			ang_x = ((double)x/1024)*M_PI+dir;

			targetVoxel.x=originVoxel.x + (int)(-sin(ang_x)*1024*sin(ang_y));
			targetVoxel.y=originVoxel.y + (int)(cos(ang_x)*1024*sin(ang_y));
			targetVoxel.z=originVoxel.z + (int)(cos(ang_y)*1024);

			_trajectory.clear();
			test = _save->getTileEngine()->calculateLine(originVoxel, targetVoxel, false, &_trajectory, bu, true, !_debug) +1;
			black = true;
			if (test!=0 && test!=6)
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
					dist = sqrt((double)((hitPos.x-originVoxel.x)*(hitPos.x-originVoxel.x)
							+ (hitPos.y-originVoxel.y)*(hitPos.y-originVoxel.y)
							+ (hitPos.z-originVoxel.z)*(hitPos.z-originVoxel.z)) );
					black = false;
				}
			}

			if (black)
			{
				dist = 0;
			}
			else
			{
				if (dist > 1000) dist = 1000;
				if (dist < 1) dist = 1;
				dist = (1000 - (log(dist)) * 140) / 700;

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

				if (dist > 1) dist = 1;

				if (tile) dist *= (16 - (float)tile->getShade()) / 16;
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
 * Saves each layer of voxels on the bettlescape as a png.
 */
void BattlescapeState::saveVoxelMap()
{
	std::ostringstream ss;
	std::vector<unsigned char> image;
	static const unsigned char pal[30]=
	{255,255,255, 224,224,224,  128,160,255,  255,160,128, 128,255,128, 192,0,255,  255,255,255, 255,255,255,  224,192,0,  255,64,128 };

	Tile* tile;

	for (int z = 0; z < _save->getMapSizeZ()*12; ++z)
	{
		image.clear();

		for (int y = 0; y < _save->getMapSizeY()*16; ++y)
		{
			for (int x = 0; x < _save->getMapSizeX()*16; ++x)
			{
				int test = _save->getTileEngine()->voxelCheck(Position(x,y,z*2),0,0) +1;
				float dist=1;

				if (x %16==15)
				{
					dist*=0.9f;
				}

				if (y %16==15)
				{
					dist*=0.9f;
				}

				if (test == 5)
				{
					tile = _save->getTile(Position(x/16, y/16, z/12));
					if (tile->getUnit())
					{
						if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
						else if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
					}
					else
					{
						tile = _save->getTile(Position(x/16, y/16, z/12-1));
						if (tile && tile->getUnit())
						{
							if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
							else if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
						}
					}
				}

				image.push_back((int)((float)pal[test*3+0]*dist));
				image.push_back((int)((float)pal[test*3+1]*dist));
				image.push_back((int)((float)pal[test*3+2]*dist));
			}
		}

		ss.str("");
		ss << Options::getUserFolder() << "voxel" << std::setfill('0') << std::setw(2) << z << ".png";


		unsigned error = lodepng::encode(ss.str(), image, _save->getMapSizeX()*16, _save->getMapSizeY()*16, LCT_RGB);
		if (error)
		{
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
		}
	}
}

/**
 * Adds a new popup window to the queue
 * (this prevents popups from overlapping).
 * @param state Pointer to popup state.
 */
void BattlescapeState::popup(State* state)
{
	_popups.push_back(state);
}

/**
 * Finishes up the current battle, shuts down the battlescape
 * and presents the debriefing screen for the mission.
 * @param abort Was the mission aborted?
 * @param inExitArea Number of soldiers in the exit area OR number of survivors
 *		when battle finished due to either all aliens or objective being destroyed.
 */
void BattlescapeState::finishBattle(bool abort, int inExitArea)
{
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
		_game->pushState(new BriefingState(_game, 0, 0));
	}
	else
	{
		_popups.clear();
		_animTimer->stop();
		_gameTimer->stop();
		_game->popState();

		if (abort
			|| (!abort  && inExitArea == 0))
		{
			// abort was done or no player is still alive
			// this concludes to defeat when in mars or mars landing mission
			if ((_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT"
					|| _save->getMissionType() == "STR_MARS_CYDONIA_LANDING")
				&& _game->getSavedGame()->getMonthsPassed() > -1)
			{
				_game->pushState (new DefeatState(_game));
			}
			else
			{
				_game->pushState(new DebriefingState(_game));
			}
		}
		else
		{
			// no abort was done and at least a player is still alive
			// this concludes to victory when in mars mission
			if (_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT"
				&& _game->getSavedGame()->getMonthsPassed() > -1)
			{
				_game->pushState (new VictoryState(_game));
			}
			else
			{
				_game->pushState(new DebriefingState(_game));
			}
		}

		_game->getCursor()->setColor(Palette::blockOffset(15)+12);
		_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
	}
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
	isMouseScrolling = false;
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
void BattlescapeState::mouseInIcons(Action* /* action */)
{
	_mouseOverIcons = true;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 */
void BattlescapeState::mouseOutIcons(Action* /* action */)
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
 * Buttons are disabled in the middle of a shot, during the alien turn,
 * and while a player's units are panicking.
 * The save button is an exception as we want to still be able to save if something
 * goes wrong during the alien turn, and submit the save file for dissection.
 * @param allowSaving, True if the help button was clicked.
 */
bool BattlescapeState::allowButtons(bool allowSaving) const
{
	return ((allowSaving
			|| _save->getSide() == FACTION_PLAYER
			|| _save->getDebugMode())
		&& (_battleGame->getPanicHandled() || firstInit)
		&& _map->getProjectile() == 0);
}

/**
 * Reserves time units for kneeling.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReserveKneelClick(Action* action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;

		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);
		_battleGame->setKneelReserved(!_battleGame->getKneelReserved());
		_btnReserveKneel->invert(0);
	}
}

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
void BattlescapeState::txtTooltipIn(Action* action)
{
	if (allowButtons() && Options::getBool("battleTooltips"))
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtTooltip->setText(tr(_currentTooltip));
	}
}

/**
* Clears the tooltip text.
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipOut(Action* action)
{
	if (allowButtons() && Options::getBool("battleTooltips"))
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_txtTooltip->setText(L"");
		}
	}
}

/**
 * Returns the TurnCounter used by the game.
 * @return, Pointer to the TurnCounter.
 */
TurnCounter* BattlescapeState::getTurnCounter() const
{
	return _turnCounter;
}

}
