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

//#include <cmath>
//#include <sstream>
//#include <iomanip>

//#include <SDL_gfxPrimitives.h>

//#include "../fmath.h"
//#include "../lodepng.h"

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

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Geoscape/DefeatState.h"
#include "../Geoscape/VictoryState.h"

#include "../Interface/Bar.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
//#include "../Interface/TurnCounter.h" // kL

#include "../Menu/LoadGameState.h"
#include "../Menu/PauseState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape screen.
 */
BattlescapeState::BattlescapeState()
	:
		_savedGame(_game->getSavedGame()),
		_savedBattle(_game->getSavedGame()->getSavedBattle()),
//		_reserve(0),
		_xBeforeMouseScrolling(0),
		_yBeforeMouseScrolling(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_mouseOverThreshold(false),
		_firstInit(true),
		_mouseOverIcons(false),
		_isMouseScrolled(false),
		_isMouseScrolling(false),
		_mouseScrollingStartTime(0),
		_fuseFrame(0),
		_showConsole(2)
{
	//Log(LOG_INFO) << "Create BattlescapeState";
	std::fill_n(
			_visibleUnit,
			20,
			(BattleUnit*)(NULL));

	const int
		screenWidth			= Options::baseXResolution,
		screenHeight		= Options::baseYResolution,
		iconsWidth			= _game->getRuleset()->getInterface("battlescape")->getElement("icons")->w, // 320
		iconsHeight			= _game->getRuleset()->getInterface("battlescape")->getElement("icons")->h, // 56
		visibleMapHeight	= screenHeight - iconsHeight,
		x					= screenWidth / 2 - iconsWidth / 2,
		y					= screenHeight - iconsHeight;

	_txtBaseLabel	= new Text(80, 9, screenWidth - 81, 0);
	_txtFloor		= new Text(25, 9, screenWidth - 26, 10);

	// Create buttonbar - this should be on the centerbottom of the screen
	_icons = new InteractiveSurface(
								iconsWidth,
								iconsHeight,
								x,
								y);

	// Create the battlemap view
	// The actual map height is the total height minus the height of the buttonbar
	_map = new Map(
				_game,
				screenWidth,
				screenHeight,
				0,
				0,
				visibleMapHeight);

	_numLayers	= new NumberText(3, 5, x + 232, y + 6);
	_numDir		= new NumberText(3, 5, x + 150, y + 6);
	_numDirTur	= new NumberText(3, 5, x + 167, y + 6);

	_kneel		= new Surface( 2,  2, x + 115, y + 19);
	_rank		= new Surface(26, 23, x + 107, y + 33);
	_weight		= new Surface( 2,  2, x + 130, y + 34);

	_btnWounds	= new InteractiveSurface(14, 14, x + 5, y - 17);
	_numWounds	= new NumberText(9, 9, x, y - 20); // X gets adjusted in updateSoldierInfo()

	_btnUnitUp			= new BattlescapeButton(32,  16, x +  48, y);
	_btnUnitDown		= new BattlescapeButton(32,  16, x +  48, y + 16);
	_btnMapUp			= new BattlescapeButton(32,  16, x +  80, y);
	_btnMapDown			= new BattlescapeButton(32,  16, x +  80, y + 16);
	_btnShowMap			= new BattlescapeButton(32,  16, x + 112, y);
	_btnKneel			= new BattlescapeButton(32,  16, x + 112, y + 16);
	_btnInventory		= new BattlescapeButton(32,  16, x + 144, y);
	_btnCenter			= new BattlescapeButton(32,  16, x + 144, y + 16);
	_btnNextSoldier		= new BattlescapeButton(32,  16, x + 176, y);
	_btnNextStop		= new BattlescapeButton(32,  16, x + 176, y + 16);
	_btnShowLayers		= new BattlescapeButton(32,  16, x + 208, y);
	_btnOptions			= new BattlescapeButton(32,  16, x + 208, y + 16);
	_btnEndTurn			= new BattlescapeButton(32,  16, x + 240, y);
	_btnAbort			= new BattlescapeButton(32,  16, x + 240, y + 16);

/*	_btnReserveNone		= new BattlescapeButton(17, 11, x + 60, y + 33);
	_btnReserveSnap		= new BattlescapeButton(17, 11, x + 78, y + 33);
	_btnReserveAimed	= new BattlescapeButton(17, 11, x + 60, y + 45);
	_btnReserveAuto		= new BattlescapeButton(17, 11, x + 78, y + 45);
	_btnReserveKneel	= new BattlescapeButton(10, 23, x + 96, y + 33);
	_btnZeroTUs			= new BattlescapeButton(10, 23, x + 49, y + 33); */
	_btnStats			= new InteractiveSurface(164, 23, x + 107, y + 33);
	_btnZeroTUs			= new InteractiveSurface( 57, 23, x +  49, y + 33);

	_btnLeftHandItem	= new InteractiveSurface(32, 48, x +   8, y + 5);
	_btnRightHandItem	= new InteractiveSurface(32, 48, x + 280, y + 5);
	_numAmmoLeft		= new NumberText(30, 5, x +   8, y + 4);
	_numAmmoRight		= new NumberText(30, 5, x + 280, y + 4);

	const int
		visibleUnitX = _game->getRuleset()->getInterface("battlescape")->getElement("visibleUnits")->x,
		visibleUnitY = _game->getRuleset()->getInterface("battlescape")->getElement("visibleUnits")->y;

	for (int
			i = 0,
				offset_x = 0;
			i < VISIBLE_MAX;
			++i)
	{
		if (i > 9)
			offset_x = 15;

		_btnVisibleUnit[i] = new InteractiveSurface(
												15,
												13,
												x + iconsWidth - 21 - offset_x,
												y - 16 - (i * 13));
		_numVisibleUnit[i] = new NumberText(
										9,
										9,
										x + iconsWidth - 15 - offset_x,
										y - 12 - (i * 13));
	}

	for (int // center 10+ on buttons
			i = 9;
			i < VISIBLE_MAX;
			++i)
	{
		_numVisibleUnit[i]->setX(_numVisibleUnit[i]->getX() - 2);
	}

	_warning	= new WarningMessage(
					224,
					24,
					x + 48,
					y + 32);

	_btnLaunch	= new BattlescapeButton(
					32,
					24,
					screenWidth - 32,
					0);
	_btnPsi		= new BattlescapeButton(
					32,
					24,
					screenWidth - 32,
					25);

	_txtName		= new Text(136, 9, _icons->getX() + 135, _icons->getY() + 32);

	_numTULaunch	= new NumberText(8, 10, x + 230, y + 34);
	_numTUAim		= new NumberText(8, 10, x + 241, y + 34);
	_numTUAuto		= new NumberText(8, 10, x + 252, y + 34);
	_numTUSnap		= new NumberText(8, 10, x + 263, y + 34);

	_numTimeUnits	= new NumberText(15, 5, x + 136, y + 42);
	_barTimeUnits	= new Bar(102, 3, x + 170, y + 41);

	_numEnergy		= new NumberText(15, 5, x + 154, y + 42);
	_barEnergy		= new Bar(102, 3, x + 170, y + 45);

	_numHealth		= new NumberText(15, 5, x + 136, y + 50);
	_barHealth		= new Bar(102, 3, x + 170, y + 49);

	_numMorale		= new NumberText(15, 5, x + 154, y + 50);
	_barMorale		= new Bar(102, 3, x + 170, y + 53);

	_txtDebug		= new Text(300, 10, 150, 0);
//	_txtTooltip		= new Text(300, 10, x + 2, y - 10);

//	_turnCounter	= new TurnCounter(20, 5, 0, 0);
	_txtTerrain		= new Text(150, 9, 1, 0);
	_txtShade		= new Text(50, 9, 1, 10);
	_txtTurn		= new Text(50, 9, 1, 20);
	_lstExp			= new TextList(25, 57, 1, 37);
	_txtHasKill		= new Text(10, 9, 1, 95);

	_txtConsole1	= new Text(screenWidth / 2, y, 0, 0);
	_txtConsole2	= new Text(screenWidth / 2, y, screenWidth / 2, 0);
//	_txtConsole3	= new Text(screenWidth / 2, y, 0, 0);
//	_txtConsole4	= new Text(screenWidth / 2, y, screenWidth / 2, 0);

	_savedBattle->setPaletteByDepth(this);

	// Fix system colors
	_game->getCursor()->setColor(Palette::blockOffset(9));
	_game->getFpsCounter()->setColor(Palette::blockOffset(9));

	if (_game->getRuleset()->getInterface("battlescape")->getElement("pathfinding"))
	{
		const Element* const pathing = _game->getRuleset()->getInterface("battlescape")->getElement("pathfinding");

		Pathfinding::green = pathing->color;
		Pathfinding::yellow = pathing->color2;
		Pathfinding::red = pathing->border;
	}

	add(_map);
	_map->init();
	_map->onMouseOver((ActionHandler)& BattlescapeState::mapOver);
	_map->onMousePress((ActionHandler)& BattlescapeState::mapPress);
	_map->onMouseClick((ActionHandler)& BattlescapeState::mapClick, 0);
	_map->onMouseIn((ActionHandler)& BattlescapeState::mapIn);

	add(_icons);
	Surface* const icons = _game->getResourcePack()->getSurface("ICONS.PCK");

	// Add in custom reserve buttons
	if (_game->getResourcePack()->getSurface("TFTDReserve"))
	{
		Surface* const tftdIcons = _game->getResourcePack()->getSurface("TFTDReserve"); // 'Resources/UI/reserve.png'
		tftdIcons->setX(48);
		tftdIcons->setY(176);
		tftdIcons->blit(icons);
	}

	// there is some cropping going on here, because the icons
	// image is 320x200 while we only need the bottom of it.
	SDL_Rect* const r = icons->getCrop();
	r->x = 0;
	r->y = 200 - iconsHeight;
	r->w = iconsWidth;
	r->h = iconsHeight;
	icons->blit(_icons);

	// this is a hack to fix the single transparent pixel on TFTD's icon panel.
	if (_game->getRuleset()->getInterface("battlescape")->getElement("icons")->TFTDMode)
		_icons->setPixelColor(46, 44, 8);

	add(_rank, "rank", "battlescape", _icons);
	add(_btnWounds);
	add(_numWounds);
	add(_btnUnitUp, "buttonUnitUp", "battlescape", _icons);
	add(_btnUnitDown, "buttonUnitDown", "battlescape", _icons);
	add(_btnMapUp, "buttonMapUp", "battlescape", _icons);
	add(_btnMapDown, "buttonMapDown", "battlescape", _icons);
	add(_btnShowMap, "buttonShowMap", "battlescape", _icons);
	add(_btnKneel, "buttonKneel", "battlescape", _icons);
	add(_btnInventory, "buttonInventory", "battlescape", _icons);
	add(_btnCenter, "buttonCenter", "battlescape", _icons);
	add(_btnNextSoldier, "buttonNextSoldier", "battlescape", _icons);
	add(_btnNextStop, "buttonNextStop", "battlescape", _icons);
	add(_btnShowLayers, "buttonShowLayers", "battlescape", _icons);
	add(_numLayers, "numLayers", "battlescape", _icons);
	add(_btnOptions, "buttonHelp", "battlescape", _icons);
	add(_btnEndTurn, "buttonEndTurn", "battlescape", _icons);
	add(_btnAbort, "buttonAbort", "battlescape", _icons);
	add(_btnStats, "buttonStats", "battlescape", _icons);
	add(_numDir);
	add(_numDirTur);
	add(_kneel); // this has to go overtop _btns.
	add(_weight); // this has to go overtop _btns.
	add(_txtName, "textName", "battlescape", _icons);
	add(_numTULaunch);
	add(_numTUAim);
	add(_numTUAuto);
	add(_numTUSnap);
	add(_numTimeUnits, "numTUs", "battlescape", _icons);
	add(_numEnergy, "numEnergy", "battlescape", _icons);
	add(_numHealth, "numHealth", "battlescape", _icons);
	add(_numMorale, "numMorale", "battlescape", _icons);
	add(_barTimeUnits, "barTUs", "battlescape", _icons);
	add(_barEnergy, "barEnergy", "battlescape", _icons);
	add(_barHealth, "barHealth", "battlescape", _icons);
	add(_barMorale, "barMorale", "battlescape", _icons);
/*	add(_btnReserveNone, "buttonReserveNone", "battlescape", _icons);
	add(_btnReserveSnap, "buttonReserveSnap", "battlescape", _icons);
	add(_btnReserveAimed, "buttonReserveAimed", "battlescape", _icons);
	add(_btnReserveAuto, "buttonReserveAuto", "battlescape", _icons);
	add(_btnReserveKneel, "buttonReserveKneel", "battlescape", _icons); */
	add(_btnZeroTUs, "buttonZeroTUs", "battlescape", _icons);
	add(_btnLeftHandItem, "buttonLeftHand", "battlescape", _icons);
	add(_numAmmoLeft, "numAmmoLeft", "battlescape", _icons);
	add(_btnRightHandItem, "buttonRightHand", "battlescape", _icons);
	add(_numAmmoRight, "numAmmoRight", "battlescape", _icons);

	for (int
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		add(_btnVisibleUnit[i]);
		add(_numVisibleUnit[i]);
	}

//	add(_txtTooltip, "textTooltip", "battlescape", _icons);
	add(_txtDebug);

	add(_warning, "warning", "battlescape", _icons);
	_warning->setColor(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color2); //Palette::blockOffset(2));
	_warning->setTextColor(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color); //Palette::blockOffset(1));

	add(_btnLaunch);
	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(0)->blit(_btnLaunch);
	_btnLaunch->onMouseClick((ActionHandler)& BattlescapeState::btnLaunchClick);
	_btnLaunch->setVisible(false);

	add(_btnPsi);
	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blit(_btnPsi);
	_btnPsi->onMouseClick((ActionHandler)& BattlescapeState::btnPsiClick);
	_btnPsi->setVisible(false);

	add(_txtBaseLabel);
	_txtBaseLabel->setColor(Palette::blockOffset(9));
	_txtBaseLabel->setHighContrast();
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	std::wstring baseLabel;

	for (std::vector<Base*>::const_iterator
			i = _savedGame->getBases()->begin();
			i != _savedGame->getBases()->end()
				&& baseLabel.empty() == true;
			++i)
	{
		if ((*i)->isInBattlescape() == true)
		{
			baseLabel = (*i)->getName(_game->getLanguage());
			break;
		}

		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end()
					&& baseLabel.empty() == true;
				++j)
		{
			if ((*j)->isInBattlescape() == true)
				baseLabel = (*i)->getName(_game->getLanguage());
		}
	}
	_txtBaseLabel->setText(baseLabel); // there'd better be a baseLabel ... or else. Pow! To the moon!!!

	add(_txtFloor);
	_txtFloor->setColor(Palette::blockOffset(8)); // blue
	_txtFloor->setHighContrast();
	_txtFloor->setAlign(ALIGN_RIGHT);
	_txtFloor->setText(L"floor");
	_txtFloor->setVisible(false);

//	add(_turnCounter);
//	_turnCounter->setColor(Palette::blockOffset(8));
	add(_txtConsole1);
	add(_txtConsole2);
//	add(_txtConsole3);
//	add(_txtConsole4);

	_txtConsole1->setColor(Palette::blockOffset(8)); // blue
	_txtConsole1->setHighContrast();
	_txtConsole2->setColor(Palette::blockOffset(8));
	_txtConsole2->setHighContrast();
//	_txtConsole3->setColor(Palette::blockOffset(8));
//	_txtConsole3->setHighContrast();
//	_txtConsole4->setColor(Palette::blockOffset(8));
//	_txtConsole4->setHighContrast();

	_txtConsole1->setVisible(_showConsole > 0);
	_txtConsole2->setVisible(_showConsole > 1);
//	_txtConsole3->setVisible(_showConsole > 2);
//	_txtConsole4->setVisible(_showConsole > 3);

	add(_txtTerrain);
	add(_txtShade);
	add(_txtTurn);
	add(_lstExp);
	add(_txtHasKill);

	_txtTerrain->setColor(Palette::blockOffset(9)); // yellow
	_txtTerrain->setHighContrast();
	_txtTerrain->setText(tr("STR_TEXTURE_").arg(tr(_savedBattle->getTerrain())));

	_txtShade->setColor(Palette::blockOffset(9));
	_txtShade->setHighContrast();
	_txtShade->setText(tr("STR_SHADE_").arg(_savedBattle->getGlobalShade()));

	_txtTurn->setColor(Palette::blockOffset(9));
	_txtTurn->setHighContrast();
	_txtTurn->setText(tr("STR_TURN").arg(_savedBattle->getTurn()));

	_lstExp->setColor(Palette::blockOffset(8)); // blue
	_lstExp->setHighContrast();
	_lstExp->setColumns(2, 10, 15);

	_txtHasKill->setColor(Palette::blockOffset(9)); // yellow
	_txtHasKill->setHighContrast();

//	_numLayers->setColor(Palette::blockOffset(5)+12);
	_numLayers->setValue(1);

	_numDir->setColor(Palette::blockOffset(5)+12);
	_numDir->setValue(0);

	_numDirTur->setColor(Palette::blockOffset(5)+12);
	_numDirTur->setValue(0);

	_rank->setVisible(false);

	_kneel->drawRect(0, 0, 2, 2, Palette::blockOffset(5)+12);
	_kneel->setVisible(false);

/*	Surface* srfOverload = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(97); // 274, brown dot 2px; 97, red sq 3px
	srfOverload->setX(-1);
	srfOverload->setY(-1);
	srfOverload->blit(_weight); */
	_weight->drawRect(0, 0, 2, 2, Palette::blockOffset(8)); // (8)=blue // (6)+10=brown
	_weight->setVisible(false);

	_btnWounds->setVisible(false);
	_btnWounds->onMouseClick((ActionHandler)& BattlescapeState::btnWoundedClick);

//	_numWounds->setColor(Palette::blockOffset(0)+3); // light gray
//	_numWounds->setColor(Palette::blockOffset(0)+1); // white
	_numWounds->setColor(Palette::blockOffset(9)); // yellow
	_numWounds->setValue(0);
	_numWounds->setVisible(false);

	_numAmmoLeft->setValue(0);
	_numAmmoRight->setValue(0);

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

	_btnNextSoldier->onMouseClick(
					(ActionHandler)& BattlescapeState::btnNextSoldierClick,
					SDL_BUTTON_LEFT);
	_btnNextSoldier->onMouseClick(
					(ActionHandler)& BattlescapeState::btnPrevSoldierClick,
					SDL_BUTTON_RIGHT);
	_btnNextSoldier->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnNextSoldierClick,
					Options::keyBattleNextUnit);
	_btnNextSoldier->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnPrevSoldierClick,
					Options::keyBattlePrevUnit);
//	_btnNextSoldier->setTooltip("STR_NEXT_UNIT");
//	_btnNextSoldier->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnNextSoldier->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnNextStop->onMouseClick(
					(ActionHandler)& BattlescapeState::btnNextStopClick,
					SDL_BUTTON_LEFT);
	_btnNextStop->onMouseClick(
					(ActionHandler)& BattlescapeState::btnPrevStopClick,
					SDL_BUTTON_RIGHT);
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

//	_btnLeftHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnLeftHandItemClick);
	_btnLeftHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnLeftHandLeftClick,
					SDL_BUTTON_LEFT);
	_btnLeftHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnLeftHandRightClick,
					SDL_BUTTON_RIGHT);
	_btnLeftHandItem->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnLeftHandLeftClick,
					Options::keyBattleUseLeftHand);
//	_btnLeftHandItem->setTooltip("STR_USE_LEFT_HAND");
//	_btnLeftHandItem->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnLeftHandItem->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//	_btnRightHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnRightHandItemClick);
	_btnRightHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnRightHandLeftClick,
					SDL_BUTTON_LEFT);
	_btnRightHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnRightHandRightClick,
					SDL_BUTTON_RIGHT);
	_btnRightHandItem->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnRightHandLeftClick,
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
	_btnReserveKneel->allowToggleInversion(); */

	_btnZeroTUs->onMouseClick(
					(ActionHandler)& BattlescapeState::btnZeroTUsClick,
					SDL_BUTTON_RIGHT);
	_btnZeroTUs->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnZeroTUsClick,
					Options::keyBattleZeroTUs);
//	_btnZeroTUs->setTooltip("STR_EXPEND_ALL_TIME_UNITS");
//	_btnZeroTUs->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnZeroTUs->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);
//	_btnZeroTUs->allowClickInversion();

	// shortcuts without a specific surface button graphic.
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReloadClick,
					Options::keyBattleReload);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnPersonalLightingClick,
					Options::keyBattlePersonalLighting);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnConsoleToggle,
					Options::keyBattleConsole);


	const SDLKey buttons[] =
	{
		Options::keyBattleCenterEnemy1,
		Options::keyBattleCenterEnemy2,
		Options::keyBattleCenterEnemy3,
		Options::keyBattleCenterEnemy4,
		Options::keyBattleCenterEnemy5,
		Options::keyBattleCenterEnemy6,
		Options::keyBattleCenterEnemy7,
		Options::keyBattleCenterEnemy8,
		Options::keyBattleCenterEnemy9,
		Options::keyBattleCenterEnemy10
	};

	const Uint8 color = _game->getRuleset()->getInterface("battlescape")->getElement("visibleUnits")->color;

	for (int
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[i]->onMouseClick((ActionHandler)& BattlescapeState::btnVisibleUnitClick);
		_btnVisibleUnit[i]->onKeyboardPress(
						(ActionHandler)& BattlescapeState::btnVisibleUnitClick,
						buttons[i]);

//		std::ostringstream tooltip;
//		tooltip << "STR_CENTER_ON_ENEMY_" << (i + 1);
//		_btnVisibleUnit[i]->setTooltip(tooltip.str());
//		_btnVisibleUnit[i]->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//		_btnVisibleUnit[i]->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

		_numVisibleUnit[i]->setColor(color); // 16
		_numVisibleUnit[i]->setValue(i + 1);
	}

//	_txtName->setColor(Palette::blockOffset(8));
	_txtName->setHighContrast();

	_numTULaunch->setColor(Palette::blockOffset(0)+7);
	_numTUAim->setColor(Palette::blockOffset(0)+7);
	_numTUAuto->setColor(Palette::blockOffset(0)+7);
	_numTUSnap->setColor(Palette::blockOffset(0)+7);

//	_numTimeUnits->setColor(Palette::blockOffset(4));
//	_numEnergy->setColor(Palette::blockOffset(1));
//	_numHealth->setColor(Palette::blockOffset(2)+12);
//	_numMorale->setColor(Palette::blockOffset(12));
//	_barTimeUnits->setColor(Palette::blockOffset(4));
	_barTimeUnits->setScale();
	_barTimeUnits->setMax(100.0);
//	_barEnergy->setColor(Palette::blockOffset(1));
	_barEnergy->setScale();
	_barEnergy->setMax(100.0);
//	_barHealth->setColor(Palette::blockOffset(2));
//	_barHealth->setColor2(Palette::blockOffset(5)+2);
	_barHealth->setScale();
	_barHealth->setMax(100.0);
//	_barMorale->setColor(Palette::blockOffset(12));
	_barMorale->setScale();
	_barMorale->setMax(100.0);

	_txtDebug->setColor(Palette::blockOffset(8));
	_txtDebug->setHighContrast();

//	_txtTooltip->setColor(Palette::blockOffset(0)-1);
//	_txtTooltip->setHighContrast();

/*	_btnReserveNone->setGroup(&_reserve);
	_btnReserveSnap->setGroup(&_reserve);
	_btnReserveAimed->setGroup(&_reserve);
	_btnReserveAuto->setGroup(&_reserve); */

	std::string music = OpenXcom::res_MUSIC_TAC_BATTLE; // default/ safety.
	std::string terrain;

	const std::string mission = _savedBattle->getMissionType();
	if (mission == "STR_UFO_CRASH_RECOVERY")
	{
		music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOCRASHED;
		terrain = _savedBattle->getTerrain();
	}
	else if (mission == "STR_UFO_GROUND_ASSAULT")
	{
		music = OpenXcom::res_MUSIC_TAC_BATTLE_UFOLANDED;
		terrain = _savedBattle->getTerrain();
	}
	else if (mission == "STR_ALIEN_BASE_ASSAULT")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEASSAULT;
	else if (mission == "STR_BASE_DEFENSE")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_BASEDEFENSE;
	else if (mission == "STR_TERROR_MISSION")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_TERRORSITE;
	else if (mission == "STR_MARS_CYDONIA_LANDING")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS1;
	else if (mission == "STR_MARS_THE_FINAL_ASSAULT")
		music = OpenXcom::res_MUSIC_TAC_BATTLE_MARS2;

	_game->getResourcePack()->playMusic(
									music,
									terrain);

	_animTimer = new Timer(DEFAULT_ANIM_SPEED);
	_animTimer->onTimer((StateHandler)& BattlescapeState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED + 32);
	_gameTimer->onTimer((StateHandler)& BattlescapeState::handleState);

	_battleGame = new BattlescapeGame(
									_savedBattle,
									this);
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
	if (_savedBattle->getAmbientSound() != -1)
		_game->getResourcePack()->getSoundByDepth(
												0,
												_savedBattle->getAmbientSound())
											->loop();

	State::init();

	_animTimer->start();
	_gameTimer->start();

	_map->setFocus(true);
	_map->cacheUnits();
	_map->draw();
	_battleGame->init();

//	kL_TurnCount = _savedBattle->getTurn();	// kL
//	_turnCounter->update();					// kL
//	_turnCounter->draw();					// kL

	updateSoldierInfo();

/*	switch (_savedBattle->getTUReserved())
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

	if (_firstInit == true
		&& playableUnitSelected() == true)
	{
		_firstInit = false;

		_battleGame->setupCursor();
		_map->getCamera()->centerOnPosition(_savedBattle->getSelectedUnit()->getPosition());

/*		_btnReserveNone->setGroup(&_reserve);
		_btnReserveSnap->setGroup(&_reserve);
		_btnReserveAimed->setGroup(&_reserve);
		_btnReserveAuto->setGroup(&_reserve); */
	}

	// kL_begin:
/*	_txtFloor->setVisible(false);

	Position pos;
	_map->getSelectorPosition(&pos);

	Tile* const tile = _savedBattle->getTile(pos);
	if (tile != NULL
		&& tile->isDiscovered(2)
		&& tile->getInventory()->empty() == true)
	{
		if (tile->hasNoFloor(_savedBattle->getTile(pos + Position(0, 0,-1))) == false)
			_txtFloor->setVisible();
	} */ // kL_end. Didn't work.

//	_txtTooltip->setText(L"");

/*	if (_savedBattle->getKneelReserved())
		_btnReserveKneel->invert(_btnReserveKneel->getColor()+3);

	_btnReserveKneel->toggle(_savedBattle->getKneelReserved());
	_battleGame->setKneelReserved(_savedBattle->getKneelReserved()); */
	//Log(LOG_INFO) << "BattlescapeState::init() EXIT";
}

/**
 * Runs the timers and handles popups.
 */
void BattlescapeState::think()
{
	//Log(LOG_INFO) << "BattlescapeState::think()";
	static bool popped = false;

	if (_gameTimer->isRunning() == true)
	{
		if (_popups.empty() == true)
		{
			State::think();

			//Log(LOG_INFO) << "BattlescapeState::think() -> _battlegame.think()";
			_battleGame->think();
			//Log(LOG_INFO) << "BattlescapeState::think() -> _animTimer.think()";
			_animTimer->think(this, NULL);
			//Log(LOG_INFO) << "BattlescapeState::think() -> _gametimer.think()";
			_gameTimer->think(this, NULL);
			//Log(LOG_INFO) << "BattlescapeState::think() -> back from think";

			if (popped == true)
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
 * @param action - pointer to an Action
 */
void BattlescapeState::mapOver(Action* action)
{
	if (_isMouseScrolling == true
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if ((SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeDragScroll);
			}

			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action); // newScroll

			return;
		}

		_isMouseScrolled = true;

/*kL
		// Set the mouse cursor back ( or not )
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_IGNORE);
		SDL_WarpMouse(
					static_cast<Uint16>(_xBeforeMouseScrolling),
					static_cast<Uint16>(_yBeforeMouseScrolling));
//		SDL_WarpMouse( // newScroll
//					_game->getScreen()->getWidth() / 2,
//					_game->getScreen()->getHeight() / 2 - _map->getIconHeight() / 2);
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_ENABLE); */


		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_mouseOverThreshold == false) // check threshold
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
							   || std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;


		if (Options::battleDragScrollInvert == true) // scroll
		{
/*fenyo1
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = -(int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = -(int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = -(int) (delta2.x * action->getXScale());
				_totalMouseMoveY = -(int) (delta2.y * action->getYScale());
			} */

			_map->getCamera()->scrollXY(
									static_cast<int>(static_cast<double>(-action->getDetails()->motion.xrel) / action->getXScale()),
									static_cast<int>(static_cast<double>(-action->getDetails()->motion.yrel) / action->getYScale()),
									false);

			// We don't want to look the mouse-cursor jumping :)
//			action->getDetails()->motion.x = static_cast<Uint16>(_xBeforeMouseScrolling);
//			action->getDetails()->motion.y = static_cast<Uint16>(_yBeforeMouseScrolling);

			_map->setCursorType(CT_NONE);
		}
		else
		{
/*fenyo1
			Position delta = _map->getCamera()->getMapOffset();
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);

			int scrollX = (int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = (int)((double)_totalMouseMoveY / action->getYScale());

			Position delta2 = _map->getCamera()->getMapOffset();

			_map->getCamera()->scrollXY(scrollX, scrollY, true);

			delta2 = _map->getCamera()->getMapOffset() - delta2;
			delta = _map->getCamera()->getMapOffset() - delta;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = (int)(delta2.x * action->getXScale());
				_totalMouseMoveY = (int)(delta2.y * action->getYScale());
			} */

			_map->getCamera()->scrollXY(
								static_cast<int>(static_cast<double>(action->getDetails()->motion.xrel) * 3.62 / action->getXScale()),
								static_cast<int>(static_cast<double>(action->getDetails()->motion.yrel) * 3.62 / action->getYScale()),
								false);

			Position delta = _map->getCamera()->getMapOffset();
			delta = _map->getCamera()->getMapOffset() - delta;
			_cursorPosition.x = std::min(
									_game->getScreen()->getWidth() - static_cast<int>(Round(action->getXScale())),
									std::max(
											0,
											_cursorPosition.x + static_cast<int>(Round(static_cast<double>(delta.x) * action->getXScale()))));
			_cursorPosition.y = std::min(
									_game->getScreen()->getHeight() - static_cast<int>(Round(action->getYScale())),
									std::max(
											0,
											_cursorPosition.y + static_cast<int>(Round(static_cast<double>(delta.y) * action->getYScale()))));
/*kL
			int barWidth = _game->getScreen()->getCursorLeftBlackBand();
			int barHeight = _game->getScreen()->getCursorTopBlackBand();
			int cursorX = _cursorPosition.x + Round(delta.x * action->getXScale());
			int cursorY = _cursorPosition.y + Round(delta.y * action->getYScale());
			_cursorPosition.x = std::min(_game->getScreen()->getWidth() - barWidth - (int)(Round(action->getXScale())), std::max(barWidth, cursorX));
			_cursorPosition.y = std::min(_game->getScreen()->getHeight() - barHeight - (int)(Round(action->getYScale())), std::max(barHeight, cursorY)); */

			// We don't want to look the mouse-cursor jumping :)
//			action->getDetails()->motion.x = static_cast<Uint16>(_cursorPosition.x);
//			action->getDetails()->motion.y = static_cast<Uint16>(_cursorPosition.y);
		}

		_game->getCursor()->handle(action);
	}
	else if (_showConsole > 0 // kL_begin:
		&& _mouseOverIcons == false
		&& allowButtons() == true)
	{
		_txtConsole1->setText(L"");
		_txtConsole2->setText(L"");
//		_txtConsole3->setText(L"");
//		_txtConsole4->setText(L"");

		bool showInfo = true;
		_txtFloor->setVisible(false);

		Position pos;
		_map->getSelectorPosition(&pos);

		Tile* const tile = _savedBattle->getTile(pos);
		if (tile != NULL
			&& tile->isDiscovered(2) == true)
		{
			if (tile->getInventory()->empty() == false)
			{
				showInfo = false;

				size_t row = 0;
				std::wostringstream
					woss,	// tst
					woss1,	// Console #1
					woss2;	// Console #2
				std::wstring
					ws1,
					ws2,
					ws3;
				int qty = 1;

				for (size_t
						i = 0;
						i != tile->getInventory()->size() + 1;
						++i)
				{
					ws1 = L"> ";

					if (i < tile->getInventory()->size())
					{
						const BattleItem* const item = tile->getInventory()->at(i);

						if (item->getUnit() != NULL)
						{
							if (item->getUnit()->getType().compare(0, 11, "STR_FLOATER") == 0)
								ws1 += tr("STR_FLOATER"); // STR_FLOATER_CORPSE
							else if (item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
							{
								ws1 += item->getUnit()->getName(_game->getLanguage());

								if (item->getUnit()->getOriginalFaction() == FACTION_PLAYER)
									ws1 += L" (" + Text::formatNumber(item->getUnit()->getHealth() - item->getUnit()->getStun()) + L")";
							}
							else
								ws1 += tr(item->getRules()->getType());
						}
						else
						{
							ws1 += tr(item->getRules()->getType());

							if (item->getRules()->getBattleType() == BT_AMMO)
								ws1 += L" (" + Text::formatNumber(item->getAmmoQuantity()) + L")";
							else if (item->getRules()->getBattleType() == BT_FIREARM
								&& item->getAmmoItem() != NULL
								&& item->getAmmoItem() != item)
							{
								std::wstring ws = tr(item->getAmmoItem()->getRules()->getType());
								ws1 += L" | " + ws + L" (" + Text::formatNumber(item->getAmmoItem()->getAmmoQuantity()) + L")";
							}
							else if ((item->getRules()->getBattleType() == BT_GRENADE
									|| item->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
								&& item->getFuseTimer() > -1)
							{
								ws1 += L" (" + Text::formatNumber(item->getFuseTimer()) + L")";
							}
						}
					}

					if (i == 0)
					{
						ws3 = ws2 = ws1;
						continue;
					}

					if (ws1 == ws3)
					{
						++qty;
						continue;
					}
					else
						ws3 = ws1;

					if (qty > 1)
					{
						ws2 += L" * " + Text::formatNumber(qty);
						qty = 1;
					}

					ws2 += L"\n";
					woss << ws2;
					ws3 = ws2 = ws1;


					if (row < 26) // Console #1
					{
						if (row == 24)
						{
							woss << L"> more >";
							++row;
						}

						woss1.str(L"");
						woss1 << woss.str();

						if (row == 25)
						{
							/* Log(LOG_INFO) << "row 25";
							std::string s (ws1.begin(), ws1.end());
							Log(LOG_INFO) << ". ws1 = " << s;
							std::string t (ws2.begin(), ws2.end());
							Log(LOG_INFO) << ". ws2 = " << t;
							std::string u (ws3.begin(), ws3.end());
							Log(LOG_INFO) << ". ws3 = " << u;
							std::wstring wstr1 (woss1.str());
							std::string v (wstr1.begin(), wstr1.end());
							Log(LOG_INFO) << ". woss1 = " << v;
							std::wstring wstr2 (woss.str());
							std::string w (wstr2.begin(), wstr2.end());
							Log(LOG_INFO) << ". woss = " << w; */

							if (ws1 == L"> ")
							{
								std::wstring wstr = woss1.str();
								wstr.erase(wstr.length() - 8);
								woss1.str(L"");
								woss1 << wstr;
							}

							if (_showConsole == 1)
								break;

							woss.str(L"");
						}
					}

					if (row > 26) // Console #2
					{
						if (row == 50)
							woss << L"> more >";

						woss2.str(L"");
						woss2 << woss.str();

						if (row == 51)
						{
							if (ws1 == L"> ")
							{
								std::wstring wstr = woss2.str();
								wstr.erase(wstr.length() - 8);
								woss2.str(L"");
								woss2 << wstr;
							}

							break;
						}
					}

					++row;
				}

				_txtConsole1->setText(woss1.str());
				_txtConsole2->setText(woss2.str());
			}
			else if (tile->hasNoFloor(_savedBattle->getTile(pos + Position(0, 0,-1))) == false)
				_txtFloor->setVisible();
		}

		_txtTerrain->setVisible(showInfo);
		_txtShade->setVisible(showInfo);
		_txtTurn->setVisible(showInfo);
		_lstExp->setVisible(showInfo);
		_txtHasKill->setVisible(showInfo);
	} // kL_end.
}

/**
 * Processes any presses on the map.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapPress(Action* action)
{
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons == true)
		return;

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		SDL_GetMouseState(
					&_xBeforeMouseScrolling,
					&_yBeforeMouseScrolling);

		_mapOffsetBeforeDragScroll = _map->getCamera()->getMapOffset();

/*kL
		if (!Options::battleDragScrollInvert
			&& _cursorPosition.z == 0)
		{
			_cursorPosition.x = static_cast<int>(action->getDetails()->motion.x);
			_cursorPosition.y = static_cast<int>(action->getDetails()->motion.y);
			// the Z is irrelevant to our mouse position, but we can use it as a boolean to check if the position is set or not
			_cursorPosition.z = 1;
		} */

		_totalMouseMoveX = 0;
		_totalMouseMoveY = 0;
		_mouseOverThreshold = false;
		_mouseScrollingStartTime = SDL_GetTicks();
	}
}

/**
 * Processes any clicks.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapClick(Action* action)
{
	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now another button is used)
	if (_isMouseScrolling == true)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
			&& (SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeDragScroll);
			}

			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action); // newScroll
		}
	}

	if (_isMouseScrolling == true) // DragScroll-Button release: release mouse-scroll-mode
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
			stopScrolling(action); // newScroll
		}
		else
			return;

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (_mouseOverThreshold == false
			&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
//			_map->getCamera()->setMapOffset(_mapOffsetBeforeDragScroll); // oldScroll
			stopScrolling(action); // newScroll
		}

		if (_isMouseScrolled == true)
			return;
	}

	// right-click aborts walking state
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		&& _battleGame->cancelCurrentAction())
	{
			return;
	}

	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons == true)
		return;

	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE
		|| _battleGame->isBusy() == true)
	{
		return;
	}

	Position pos;
	_map->getSelectorPosition(&pos);

	if (_savedBattle->getDebugMode() == true)
	{
		std::wostringstream ss;
		ss << L"Clicked " << pos;
		debug(ss.str());
	}

	// don't allow to click into void
	if (_savedBattle->getTile(pos) != NULL)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
				// kL_ Taking this out so that Alt can be used for forced walking by flying soldiers.
				// We all have RMBs, it's 2014 a.d. .....
//			|| (action->getDetails()->button.button == SDL_BUTTON_LEFT
//				&& (SDL_GetModState() & KMOD_ALT) != 0))
			&& playableUnitSelected() == true)
		{
			_battleGame->secondaryAction(pos);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			_battleGame->primaryAction(pos);
	}
}

/**
 * Handles mouse entering the map surface.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapIn(Action*)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(Options::battleDragScrollButton, false);
}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * @param action - pointer to an Action
 */
void BattlescapeState::stopScrolling(Action* action)
{
/*kL
	if (Options::battleDragScrollInvert)
	{
		SDL_WarpMouse(
				static_cast<Uint16>(_xBeforeMouseScrolling),
				static_cast<Uint16>(_yBeforeMouseScrolling));
		action->setMouseAction(
				_xBeforeMouseScrolling,
				_yBeforeMouseScrolling,
				_map->getX(),
				_map->getY());

		_battleGame->setupCursor();
	}
	else
	{
		SDL_WarpMouse(
				static_cast<Uint16>(_cursorPosition.x),
				static_cast<Uint16>(_cursorPosition.y));
		action->setMouseAction(
				static_cast<int>(static_cast<double>(_cursorPosition.x) / action->getXScale()),
				static_cast<int>(static_cast<double>(_cursorPosition.y) / action->getYScale()),
				_game->getScreen()->getSurface()->getX(),
				_game->getScreen()->getSurface()->getY());

		_map->setSelectorPosition(
							static_cast<int>(static_cast<double>(_cursorPosition.x) / action->getXScale()),
							static_cast<int>(static_cast<double>(_cursorPosition.y) / action->getYScale()));
	}

	// reset our "mouse position stored" flag
	_cursorPosition.z = 0; */
}

/**
 * Takes care of any events from the core game engine.
 * @param action - pointer to an Action
 */
inline void BattlescapeState::handle(Action* action)
{
	if (_game->getCursor()->getVisible() == true
		|| (action->getDetails()->button.button == SDL_BUTTON_RIGHT
			&& (action->getDetails()->type == SDL_MOUSEBUTTONDOWN
				|| action->getDetails()->type == SDL_MOUSEBUTTONUP)))
	{
		State::handle(action);

/*kL
		if (_isMouseScrolling
			&& !Options::battleDragScrollInvert) // newScroll
		{
			_map->setSelectorPosition((_cursorPosition.x - _game->getScreen()->getCursorLeftBlackBand()) / action->getXScale(), (_cursorPosition.y - _game->getScreen()->getCursorTopBlackBand()) / action->getYScale());
//			_map->setSelectorPosition( // newScroll
//								static_cast<int>(static_cast<double>(_cursorPosition.x) / action->getXScale()),
//								static_cast<int>(static_cast<double>(_cursorPosition.y) / action->getYScale()));
//			_map->setSelectorPosition(
//								static_cast<int>(static_cast<double>(_xBeforeMouseScrolling) / action->getXScale()),
//								static_cast<int>(static_cast<double>(_yBeforeMouseScrolling) / action->getYScale()));
		} */

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
				if (action->getDetails()->key.keysym.sym == SDLK_d			// "ctrl-d" - enable debug mode
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					_savedBattle->setDebugMode();
					debug(L"Debug Mode");
				}
				else if (_savedBattle->getDebugMode()						// "ctrl-v" - reset tile visibility
					&& action->getDetails()->key.keysym.sym == SDLK_v
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					debug(L"Resetting tile visibility");
					_savedBattle->resetTiles();
				}
				else if (_savedBattle->getDebugMode()						// "ctrl-k" - kill all aliens
					&& action->getDetails()->key.keysym.sym == SDLK_k
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					debug(L"Influenza bacterium dispersed");
					for (std::vector<BattleUnit*>::iterator
							i = _savedBattle->getUnits()->begin();
							i !=_savedBattle->getUnits()->end();
							++i)
					{
						if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
						{
							(*i)->instaKill();
							if ((*i)->getTile())
								(*i)->getTile()->setUnit(NULL);
						}
					}
				}
				else if (action->getDetails()->key.keysym.sym == SDLK_F11)	// f11 - voxel map dump
					saveVoxelMap();
				else if (action->getDetails()->key.keysym.sym == SDLK_F9	// f9 - ai
					&& Options::traceAI)
				{
					saveAIMap();
				}
			}
			else if (_savedGame->isIronman() == false)						// quick save and quick load
			{
				// not works in debug mode to prevent conflict in hotkeys by default
				if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_QUICK,
													_palette));
				else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)
					_game->pushState(new LoadGameState(
													OPT_BATTLESCAPE,
													SAVE_QUICK,
													_palette));
			}

			if (action->getDetails()->key.keysym.sym == Options::keyBattleVoxelView)
				saveVoxelView();											// voxel view dump
		}
	}
}

/**
 * Moves the selected unit up.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitUpClick(Action*)
{
	if (playableUnitSelected() == false)
		return;

//	_savedBattle->getPathfinding()->setKneelCheck();
	const int valid = _savedBattle->getPathfinding()->validateUpDown(
																_savedBattle->getSelectedUnit(),
																_savedBattle->getSelectedUnit()->getPosition(),
																Pathfinding::DIR_UP);

	if (valid > 0)			// gravLift or flying
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(
							_savedBattle->getSelectedUnit(),
							Pathfinding::DIR_UP);
	}
	else if (valid == -2)	// no flight suit
		warning("STR_ACTION_NOT_ALLOWED_FLIGHT");
//	else if (valid == -1)	// kneeling
//		warning("STR_ACTION_NOT_ALLOWED_KNEEL");
	else					// valid==0 : blocked by roof
		warning("STR_ACTION_NOT_ALLOWED_ROOF");
}

/**
 * Moves the selected unit down.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitDownClick(Action*)
{
	if (playableUnitSelected() == false)
		return;

	const int valid = _savedBattle->getPathfinding()->validateUpDown(
																_savedBattle->getSelectedUnit(),
																_savedBattle->getSelectedUnit()->getPosition(),
																Pathfinding::DIR_DOWN);

	if (valid > 0)	// gravLift or flying
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(
							_savedBattle->getSelectedUnit(),
							Pathfinding::DIR_DOWN);
	}
	else			// blocked, floor
		warning("STR_ACTION_NOT_ALLOWED_FLOOR");
}

/**
 * Shows the next map layer.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapUpClick(Action*)
{
	if (_savedBattle->getSide() == FACTION_PLAYER
		|| _savedBattle->getDebugMode() == true)
	{
		_map->getCamera()->up();
	}
}

/**
 * Shows the previous map layer.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapDownClick(Action*)
{
	if (_savedBattle->getSide() == FACTION_PLAYER
		|| _savedBattle->getDebugMode() == true)
	{
		_map->getCamera()->down();
	}
}

/**
 * Shows the minimap.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnShowMapClick(Action*)
{
	if (allowButtons() == true)
		_game->pushState(new MiniMapState(
										_map->getCamera(),
										_savedBattle));
}

/**
 * Toggles the current unit's kneel/standup status.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnKneelClick(Action*)
{
	if (allowButtons() == true)
	{
		BattleUnit* const bu = _savedBattle->getSelectedUnit();
		if (bu != NULL)
		{
			//Log(LOG_INFO) << "BattlescapeState::btnKneelClick()";
			if (_battleGame->kneel(bu) == true)
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

				if (_battleGame->getPathfinding()->isPathPreviewed() == true)
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
 * Goes to the inventory screen.
 * Additionally resets TUs for current side in debug mode.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnInventoryClick(Action*)
{
	if (_savedBattle->getDebugMode())
	{
		for (std::vector<BattleUnit*>::iterator
				i = _savedBattle->getUnits()->begin();
				i != _savedBattle->getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == _savedBattle->getSide())
				(*i)->prepareUnitTurn();

			updateSoldierInfo();
		}
	}

//		&& (unit->getArmor()->getSize() == 1
//			|| _savedBattle->getDebugMode())
//		&& (unit->getOriginalFaction() == FACTION_PLAYER
//			|| unit->getRankString() != "STR_LIVE_TERRORIST"))

	if (playableUnitSelected() == true)
	{
		const BattleUnit* const unit = _savedBattle->getSelectedUnit();

		if (_savedBattle->getDebugMode()
			|| (unit->getGeoscapeSoldier() != NULL
//				|| (unit->getUnitRules() &&
				|| (unit->getUnitRules()->getMechanical() == false
					&& unit->getRankString() != "STR_LIVE_TERRORIST")))
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
											!_savedBattle->getDebugMode(),
											this));
		}
	}
}

/**
 * Centers on the currently selected soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnCenterClick(Action*)
{
	if (playableUnitSelected())
	{
		_map->getCamera()->centerOnPosition(_savedBattle->getSelectedUnit()->getPosition());
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects the next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextSoldierClick(Action*)
{
	if (allowButtons())
	{
		selectNextFactionUnit(true);
		_map->refreshSelectorPosition();
	}
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextStopClick(Action*)
{
	if (allowButtons())
		selectNextFactionUnit(true, true);
}

/**
 * Selects next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevSoldierClick(Action*)
{
	if (allowButtons())
		selectPreviousFactionUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevStopClick(Action*)
{
	if (allowButtons() == true)
	{
		selectPreviousFactionUnit(true, true);
		_map->refreshSelectorPosition();
	}
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
	if (allowButtons() == true)
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE)
			return;

		BattleUnit* unit = _savedBattle->selectNextFactionUnit(
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
	if (allowButtons() == true)
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE)
			return;

		BattleUnit* unit = _savedBattle->selectPreviousFactionUnit(
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
 * @param action - pointer to an Action
 */
void BattlescapeState::btnShowLayersClick(Action*)
{
	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());
}

/**
 * Shows options.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnHelpClick(Action*)
{
	if (allowButtons(true))
		_game->pushState(new PauseState(OPT_BATTLESCAPE));
}

/**
 * Requests the end of turn. This will add a 0 to the end of the state queue,
 * so all ongoing actions, like explosions are finished first before really switching turn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnEndTurnClick(Action*)
{
	//Log(LOG_INFO) << "BattlescapeState::btnEndTurnClick()";
	if (allowButtons() == true)
	{
//		_txtTooltip->setText(L"");
		_battleGame->requestEndTurn();
	}
	//Log(LOG_INFO) << "BattlescapeState::btnEndTurnClick() EXIT";
}

/**
 * Aborts the mission.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnAbortClick(Action*)
{
	if (allowButtons() == true)
	{
		//Log(LOG_INFO) << "BattlescapeState::btnAbortClick()";
		_game->pushState(new AbortMissionState(
											_savedBattle,
											this));
	}
	//Log(LOG_INFO) << "BattlescapeState::btnAbortClick() EXIT";
}

/**
 * Shows the selected soldier's info.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnStatsClick(Action* action)
{
	if (playableUnitSelected() == true)
	{
		bool edge = false;

		if (SCROLL_TRIGGER == Options::battleEdgeScroll
			&& SDL_MOUSEBUTTONUP == action->getDetails()->type
			&& SDL_BUTTON_LEFT == action->getDetails()->button.button)
		{
			int posX = action->getXMouse();
			int posY = action->getYMouse();
			if ((posX < Camera::SCROLL_BORDER * action->getXScale()
					&& posX > 0)
				|| posX > (_map->getWidth() - Camera::SCROLL_BORDER) * action->getXScale()
				|| (posY < (Camera::SCROLL_BORDER * action->getYScale())
						&& posY > 0)
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

		if (edge == false)
			popup(new UnitInfoState(
								_savedBattle->getSelectedUnit(),
								this,
								false,
								false));
	}
}

/**
 * Shows an action popup menu. When clicked, creates the action.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLeftHandLeftClick(Action*)
{
	if (playableUnitSelected() == true)
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
		_savedBattle->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");

		_map->cacheUnits();
		_map->draw();

		BattleItem* leftHandItem = _savedBattle->getSelectedUnit()->getItem("STR_LEFT_HAND");
		handleItemClick(leftHandItem);
	}
}

/**
 * Sets left hand as Active Hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLeftHandRightClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelCurrentAction(true);

		_savedBattle->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");
		updateSoldierInfo(false);

		_map->cacheUnits();
		_map->draw();
	}
}

/**
 * Shows an action popup menu. When clicked, creates the action.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnRightHandLeftClick(Action*)
{
	if (playableUnitSelected() == true)
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
		_savedBattle->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");

		_map->cacheUnits();
		_map->draw();

		BattleItem* rightHandItem = _savedBattle->getSelectedUnit()->getItem("STR_RIGHT_HAND");
		handleItemClick(rightHandItem);
	}
}

/**
 * Sets right hand as Active Hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnRightHandRightClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelCurrentAction(true);

		_savedBattle->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");
		updateSoldierInfo(false);

		_map->cacheUnits();
		_map->draw();
	}
}

/**
 * Centers on the unit corresponding to this button.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnVisibleUnitClick(Action* action)
{
	int btnID = -1;

	for (int // got to find out which button was pressed
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
 * Centers on the currently wounded soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnWoundedClick(Action* action)
{
	if (playableUnitSelected() == true)
		_map->getCamera()->centerOnPosition(_savedBattle->getSelectedUnit()->getPosition());

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Launches the blaster bomb.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLaunchClick(Action* action)
{
	_battleGame->launchAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Uses psionics.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPsiClick(Action* action)
{
	_battleGame->psiButtonAction();
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Reserves time units.
 * @param action - pointer to an Action
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

		if		(_reserve == _btnReserveNone)	_battleGame->setTUReserved(BA_NONE);
		else if (_reserve == _btnReserveSnap)	_battleGame->setTUReserved(BA_SNAPSHOT);
		else if (_reserve == _btnReserveAimed)	_battleGame->setTUReserved(BA_AIMEDSHOT);
		else if (_reserve == _btnReserveAuto)	_battleGame->setTUReserved(BA_AUTOSHOT);

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
} */

/**
 * Reserves time units for kneeling.
 * @param action - pointer to an Action
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
 * Reloads the weapon in hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnReloadClick(Action*)
{
	if (playableUnitSelected() == true
		&& _savedBattle->getSelectedUnit()->checkAmmo() == true)
	{
		_game->getResourcePack()->getSoundByDepth(
												_savedBattle->getDepth(),
												ResourcePack::ITEM_RELOAD)
											->play(
												-1,
												getMap()->getSoundAngle(_savedBattle->getSelectedUnit()->getPosition()));

		updateSoldierInfo();
	}
}

/**
 * Removes all time units.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnZeroTUsClick(Action* action)
{
	if (allowButtons() == true)
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;

		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);

		if (_battleGame->getSave()->getSelectedUnit() != NULL)
		{
			_battleGame->getSave()->getSelectedUnit()->setTimeUnits(0);
			updateSoldierInfo();
		}
	}
}

/**
 * Toggles soldier's personal lighting (purely cosmetic).
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPersonalLightingClick(Action*)
{
	if (allowButtons() == true)
		_savedBattle->getTileEngine()->togglePersonalLighting();
}

/**
 * kL. Handler for toggling the console.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnConsoleToggle(Action*) // kL
{
	if (allowButtons() == true)
	{
		if (_showConsole == 0)
		{
			_showConsole = 1;

			_txtConsole2->setText(L"");
//			_txtConsole3->setText(L"");
//			_txtConsole4->setText(L"");
		}
		else if (_showConsole == 1)
		{
			_showConsole = 2;

//			_txtConsole3->setText(L"");
//			_txtConsole4->setText(L"");
		}
		else if (_showConsole == 2)
		{
//			_showConsole = 3;
			_showConsole = 0;

			_txtConsole1->setText(L"");
			_txtConsole2->setText(L"");
//			_txtConsole4->setText(L"");

			_txtTerrain->setVisible();
			_txtShade->setVisible();
			_txtTurn->setVisible();
			_lstExp->setVisible();
			_txtHasKill->setVisible();
		}
/*		else if (_showConsole == 3)
		{
			_showConsole = 4;

			_txtConsole1->setText(L"");
			_txtConsole2->setText(L"");
		}
		else if (_showConsole == 4)
		{
			_showConsole = 0;

			_txtConsole1->setText(L"");
			_txtConsole2->setText(L"");
			_txtConsole3->setText(L"");
			_txtConsole4->setText(L"");

			_txtTerrain->setVisible();
			_txtShade->setVisible();
			_txtTurn->setVisible();
			_lstExp->setVisible();
			_txtHasKill->setVisible();
		} */

		_txtConsole1->setVisible(_showConsole > 0);
		_txtConsole2->setVisible(_showConsole > 1);
/*		_txtConsole1->setVisible(0 < _showConsole && _showConsole < 3);
		_txtConsole2->setVisible(1 < _showConsole && _showConsole < 3);
		_txtConsole3->setVisible(2 < _showConsole);
		_txtConsole4->setVisible(3 < _showConsole); */
	}
}

/**
* Shows a tooltip for the appropriate button.
* @param action - pointer to an Action
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
* @param action - pointer to an Action
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
 * Determines whether a playable unit is selected. Normally only player side
 * units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if stats can be displayed and action buttons will work.
 * @return, true if a playable unit is selected
 */
bool BattlescapeState::playableUnitSelected()
{
	return _savedBattle->getSelectedUnit() != NULL
		&& allowButtons();
}

/**
 * Updates a unit's onScreen stats & info.
 * @param calcFoV - true to run calculateFOV() for the unit
 */
void BattlescapeState::updateSoldierInfo(bool calcFoV)
{
	for (int // remove red target indicators
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);

		_visibleUnit[i] = NULL;
	}

	_rank->clear();

	_btnRightHandItem->clear();
	_btnLeftHandItem->clear();

	_btnRightHandItem	->setVisible(false);
	_btnLeftHandItem	->setVisible(false);
	_numAmmoRight		->setVisible(false);
	_numAmmoLeft		->setVisible(false);

	_kneel		->setVisible(false);
	_weight		->setVisible(false);
	_numDir		->setVisible(false);
	_numDirTur	->setVisible(false);

	_numTULaunch->setVisible(false);
	_numTUAim	->setVisible(false);
	_numTUAuto	->setVisible(false);
	_numTUSnap	->setVisible(false);

	_numWounds	->setVisible(false);
	_btnWounds	->setVisible(false);


	if (playableUnitSelected() == false) // not xCom Soldier; ie. aLien or civilian turn
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

		return;
	}
	else // not aLien or civilian; ie. xCom Soldier
	{
		_rank			->setVisible();

		_numTimeUnits	->setVisible();
		_barTimeUnits	->setVisible();
		_barTimeUnits	->setVisible();

		_numEnergy		->setVisible();
		_barEnergy		->setVisible();
		_barEnergy		->setVisible();

		_numHealth		->setVisible();
		_barHealth		->setVisible();
		_barHealth		->setVisible();

		_numMorale		->setVisible();
		_barMorale		->setVisible();
		_barMorale		->setVisible();
	}


	BattleUnit* const selectedUnit = _savedBattle->getSelectedUnit();

	if (selectedUnit == NULL)
		return;


	if (calcFoV == true)
		_savedBattle->getTileEngine()->calculateFOV(selectedUnit);

	int j = 0;
	for (std::vector<BattleUnit*>::const_iterator
			i = selectedUnit->getVisibleUnits()->begin();
			i != selectedUnit->getVisibleUnits()->end()
				&& j < VISIBLE_MAX;
			++i,
				++j)
	{
		_btnVisibleUnit[j]->setVisible();
		_numVisibleUnit[j]->setVisible();

		_visibleUnit[j] = *i;
	}


	_txtName->setText(selectedUnit->getName(
										_game->getLanguage(),
										false));

//	Soldier* soldier = _savedGame->getSoldier(selectedUnit->getId());
	const Soldier* const soldier = selectedUnit->getGeoscapeSoldier();
	if (soldier != NULL)
	{
/*		SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(soldier->getRankSprite())->blit(_rank); */
		SurfaceSet* const texture = _game->getResourcePack()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(20 + soldier->getRank())->blit(_rank);

		if (selectedUnit->isKneeled() == true)
		{
//			drawKneelIndicator();
//			_kneel->drawRect(0, 0, 2, 2, Palette::blockOffset(5)+12);
			_kneel->setVisible();
		}
	}

	const int strength = static_cast<int>(Round(
						 static_cast<double>(selectedUnit->getBaseStats()->strength) * (selectedUnit->getAccuracyModifier() / 2. + 0.5)));
	if (selectedUnit->getCarriedWeight() > strength)
		_weight->setVisible();

	_numDir->setValue(selectedUnit->getDirection());
	_numDir->setVisible();

	if (selectedUnit->getTurretType() != -1)
	{
		_numDirTur->setValue(selectedUnit->getTurretDirection());
		_numDirTur->setVisible();
	}

/*	int wounds = selectedUnit->getFatalWounds();
	_btnWounds->setVisible(wounds > 0);
	_numWounds->setVisible(wounds > 0);
	_numWounds->setValue(static_cast<unsigned>(wounds));
	if (wounds > 9)
		_numWounds->setX(_icons->getX() + 8);
	else
		_numWounds->setX(_icons->getX() + 10); */

	const int wounds = selectedUnit->getFatalWounds();
	if (wounds > 0)
	{
//		SurfaceSet* srtStatus = _game->getResourcePack()->getSurfaceSet("StatusIcons");
		Surface* srtStatus = _game->getResourcePack()->getSurface("RANK_ROOKIE");
		if (srtStatus != NULL)
		{
			//Log(LOG_INFO) << ". srtStatus is VALID";
//			srtStatus->getFrame(4)->blit(_btnWounds); // red heart icon
			srtStatus->blit(_btnWounds); // red heart icon
			_btnWounds->setVisible();
		}

//		if (wounds > 9)
//			_numWounds->setX(_btnWounds->getX() + 4);
//		else
//			_numWounds->setX(_btnWounds->getX() + 6);
		_numWounds->setX(_btnWounds->getX() + 7);

		_numWounds->setValue(static_cast<unsigned>(wounds));
		_numWounds->setVisible();
	}


	double stat = static_cast<double>(selectedUnit->getBaseStats()->tu);
	const int tu = selectedUnit->getTimeUnits();
	_numTimeUnits->setValue(static_cast<unsigned>(tu));
	_barTimeUnits->setValue(ceil(
							static_cast<double>(tu) / stat * 100.0));

	stat = static_cast<double>(selectedUnit->getBaseStats()->stamina);
	const int energy = selectedUnit->getEnergy();
	_numEnergy->setValue(static_cast<unsigned>(energy));
	_barEnergy->setValue(ceil(
							static_cast<double>(energy) / stat * 100.0));

	stat = static_cast<double>(selectedUnit->getBaseStats()->health);
	const int health = selectedUnit->getHealth();
	_numHealth->setValue(static_cast<unsigned>(health));
	_barHealth->setValue(ceil(
							static_cast<double>(health) / stat * 100.0));
	_barHealth->setValue2(ceil(
							static_cast<double>(selectedUnit->getStun()) / stat * 100.0));

	const int morale = selectedUnit->getMorale();
	_numMorale->setValue(static_cast<unsigned>(morale));
	_barMorale->setValue(morale);


	BattleItem
		* const rtItem = selectedUnit->getItem("STR_RIGHT_HAND"),
		* const ltItem = selectedUnit->getItem("STR_LEFT_HAND");

	std::string activeHand = selectedUnit->getActiveHand();
	if (activeHand != "")
	{
		int
			tuLaunch = 0,
			tuAim = 0,
			tuAuto = 0,
			tuSnap = 0;

		if (activeHand == "STR_RIGHT_HAND"
			&& (rtItem->getRules()->getBattleType() == BT_FIREARM
				|| rtItem->getRules()->getBattleType() == BT_MELEE))
		{
			tuLaunch = selectedUnit->getActionTUs(BA_LAUNCH, rtItem);
			tuAim = selectedUnit->getActionTUs(BA_AIMEDSHOT, rtItem);
			tuAuto = selectedUnit->getActionTUs(BA_AUTOSHOT, rtItem);
			tuSnap = selectedUnit->getActionTUs(BA_SNAPSHOT, rtItem);
			if (tuLaunch == 0
				&& tuAim == 0
				&& tuAuto == 0
				&& tuSnap == 0)
			{
				tuSnap = selectedUnit->getActionTUs(BA_HIT, rtItem);
			}
		}
		else if (activeHand == "STR_LEFT_HAND"
			&& (ltItem->getRules()->getBattleType() == BT_FIREARM
				|| ltItem->getRules()->getBattleType() == BT_MELEE))
		{
			tuLaunch = selectedUnit->getActionTUs(BA_LAUNCH, ltItem);
			tuAim = selectedUnit->getActionTUs(BA_AIMEDSHOT, ltItem);
			tuAuto = selectedUnit->getActionTUs(BA_AUTOSHOT, ltItem);
			tuSnap = selectedUnit->getActionTUs(BA_SNAPSHOT, ltItem);
			if (tuLaunch == 0
				&& tuAim == 0
				&& tuAuto == 0
				&& tuSnap == 0)
			{
				tuSnap = selectedUnit->getActionTUs(BA_HIT, ltItem);
			}
		}

		if (tuLaunch)
		{
			_numTULaunch->setValue(tuLaunch);
			_numTULaunch->setVisible();
		}

		if (tuAim)
		{
			_numTUAim->setValue(tuAim);
			_numTUAim->setVisible();
		}

		if (tuAuto)
		{
			_numTUAuto->setValue(tuAuto);
			_numTUAuto->setVisible();
		}

		if (tuSnap)
		{
			_numTUSnap->setValue(tuSnap);
			_numTUSnap->setVisible();
		}
	}

	if (rtItem)
	{
		rtItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_btnRightHandItem);
		_btnRightHandItem->setVisible();

		if (rtItem->getRules()->getBattleType() == BT_FIREARM
			&& (rtItem->needsAmmo()
				|| rtItem->getRules()->getClipSize() > 0))
		{
			_numAmmoRight->setVisible();
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
		_btnLeftHandItem->setVisible();

		if (ltItem->getRules()->getBattleType() == BT_FIREARM
			&& (ltItem->needsAmmo()
				|| ltItem->getRules()->getClipSize() > 0))
		{
			_numAmmoLeft->setVisible();
			if (ltItem->getAmmoItem())
				_numAmmoLeft->setValue(ltItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoLeft->setValue(0);
		}
	}

	showPsiButton(
				selectedUnit->getOriginalFaction() == FACTION_HOSTILE
				&& selectedUnit->getBaseStats()->psiSkill > 0);

	//Log(LOG_INFO) << "BattlescapeState::updateSoldierInfo() EXIT";
}

/**
 * kL. Draws the kneel indicator.
 */
/* void BattlescapeState::drawKneelIndicator() // kL
{
//	SDL_Rect square;
//	square.x = 0;
//	square.y = 0;
//	square.w = 2;
//	square.h = 2;

//	_kneel->drawRect(&square, Palette::blockOffset(5)+12);
	_kneel->drawRect(0, 0, 2, 2, Palette::blockOffset(5)+12);
} */

/**
 * kL. Draws the fatal wounds indicator.
 */
/* void BattlescapeState::drawWoundIndicator() // kL
{
	_btnWounds->drawRect(0, 0, 15, 12, 15);		// black border
	_btnWounds->drawRect(1, 1, 13, 10, color);	// inner red square
} */

/**
 * Shifts the red colors of the visible unit buttons' backgrounds.
 * kL: Also blinks the fatal wounds indicator.
 */
void BattlescapeState::blinkVisibleUnitButtons()
{
	static int
		delta = 1,
		color = Palette::blockOffset(2)+2;

/*	SDL_Rect square1; // black border
	square1.x = 0;
	square1.y = 0;
	square1.w = 15;
	square1.h = 12;

	SDL_Rect square2; // inner red square
	square2.x = 1;
	square2.y = 1;
	square2.w = 13;
	square2.h = 10; */

	for (int
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		if (_btnVisibleUnit[i]->getVisible())
		{
//			_btnVisibleUnit[i]->drawRect(&square1, 15);		// black border
//			_btnVisibleUnit[i]->drawRect(&square2, color);	// inner red square
			_btnVisibleUnit[i]->drawRect(0, 0, 15, 13, 15);		// black border
			_btnVisibleUnit[i]->drawRect(1, 1, 13, 11, color);	// inner red square
		}
	}

/*	if (_btnWounds->getVisible())
	{
		_btnWounds->drawRect(0, 0, 15, 13, 15);		// black border
		_btnWounds->drawRect(1, 1, 13, 11, color);	// inner red square
			// color + Palette::blockOffset(10));	// inner purple square
	} */


	if (color == Palette::blockOffset(2)+13) // reached darkish red
		delta = -1;
	else if (color == Palette::blockOffset(2)+2) // reached lightish red
		delta = 1;

	color += delta;
}

/**
 * Animates map objects on the map, also smoke,fire, ...
 */
void BattlescapeState::animate()
{
	_map->animate(_battleGame->isBusy() == false);

	blinkVisibleUnitButtons();
	drawFuse();
	flashMedic();
}

/**
 * Popups a context sensitive list of actions the user can choose from.
 * Some actions result in a change of gamestate.
 * @param item - pointer to BattleItem the user clicked on (righthand/lefthand)
 */
void BattlescapeState::handleItemClick(BattleItem* item)
{
	if (item != NULL						// make sure there is an item
		&& _battleGame->isBusy() == false)	// and the battlescape is in an idle state
	{
//kL	if (_savedGame->isResearched(item->getRules()->getRequirements())
//kL		|| _savedBattle->getSelectedUnit()->getOriginalFaction() == FACTION_HOSTILE)
//kL_note: do that in ActionMenu, to allow throwing artefacts.
//		{
		_battleGame->getCurrentAction()->weapon = item;
		popup(new ActionMenuState(
								_battleGame->getCurrentAction(),
								_icons->getX(),
								_icons->getY() + 16));
//		}
//kL	else warning("STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED");
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
 * @param interval - an interval in ms
 */
void BattlescapeState::setStateInterval(Uint32 interval)
{
	_gameTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return, pointer to Game
 */
Game* BattlescapeState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the map. Some states need this info.
 * @return, pointer to Map
 */
Map* BattlescapeState::getMap() const
{
	return _map;
}

/**
 * Shows a debug message in the topleft corner.
 * @param message - reference a debug message
 */
void BattlescapeState::debug(const std::wstring& message)
{
	if (_savedBattle->getDebugMode())
		_txtDebug->setText(message);
}

/**
 * Shows a warning message.
 * @param message	- reference a message, usually a warning
 * @param useArg	- true to add an argument to the message (default false)
 * @param arg		- the argument to add as an integer (default -1)
 */
void BattlescapeState::warning(
		const std::string& message,
		const bool useArg,
		const int arg)
{
	if (useArg == false)
		_warning->showMessage(tr(message));
	else
		_warning->showMessage(tr(message).arg(arg));
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
//kL	Uint32 start = SDL_GetTicks(); // kL_note: Not used.

	BattleUnit* unit = _savedBattle->getSelectedUnit();
	if (unit == NULL)
		return;

	int
		w = _savedBattle->getMapSizeX(),
		h = _savedBattle->getMapSizeY();
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
			Tile* t = _savedBattle->getTile(tilePos);

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
			Tile* t = _savedBattle->getTile(tilePos);

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

				t = _savedBattle->getTile(pos);
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
					&& !t->hasNoFloor(_savedBattle->getTile(pos)))
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

	BattleUnit* bu = _savedBattle->getSelectedUnit();
	if (bu == NULL) // no unit selected
		return;

	bool
		_debug = _savedBattle->getDebugMode(),
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
	Tile* tile = NULL;

	image.clear();

	for (int
			y = -256 + 32;
			y < 256 + 32;
			++y)
	{
		ang_y = (((double)y) / 640 * M_PI + M_PI / 2);
//		ang_y = (static_cast<double>(y)) / 640. * M_PI + M_PI / 2.;	// kL
		for (int x = -256; x < 256; ++x)
		{
			ang_x = ((double)x / 1024) * M_PI + dir;
//			ang_x = (static_cast<double>(x / 1024)) * M_PI + dir;	//kL, getting way too confusing here.

			targetVoxel.x = originVoxel.x + (int)(-sin(ang_x) * 1024 * sin(ang_y));
			targetVoxel.y = originVoxel.y + (int)(cos(ang_x) * 1024 * sin(ang_y));
			targetVoxel.z = originVoxel.z + (int)(cos(ang_y) * 1024);

			_trajectory.clear();
			test = _savedBattle->getTileEngine()->calculateLine(
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
				tile = _savedBattle->getTile(Position(
											_trajectory.at(0).x / 16,
											_trajectory.at(0).y / 16,
											_trajectory.at(0).z / 24));
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
							if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
								test = 9;
							else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
								test = 8;
						}
						else
						{
							tile = _savedBattle->getTile(Position(
														_trajectory.at(0).x / 16,
														_trajectory.at(0).y / 16,
														_trajectory.at(0).z / 24 - 1));
							if (tile && tile->getUnit())
							{
								if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
									test = 9;
								else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
									test = 8;
							}
						}
					}

					hitPos = Position(
									_trajectory.at(0).x,
									_trajectory.at(0).y,
									_trajectory.at(0).z);
					dist = sqrt(static_cast<double>(
								  (hitPos.x - originVoxel.x) * (hitPos.x - originVoxel.x)
								+ (hitPos.y - originVoxel.y) * (hitPos.y - originVoxel.y)
								+ (hitPos.z - originVoxel.z) * (hitPos.z - originVoxel.z)));

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

			image.push_back((int)((float)(pal[test * 3 + 0]) * dist));
			image.push_back((int)((float)(pal[test * 3 + 1]) * dist));
			image.push_back((int)((float)(pal[test * 3 + 2]) * dist));
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

	for (int z = 0; z < _savedBattle->getMapSizeZ() * 12; ++z)
	{
		image.clear();

		for (int y = 0; y < _savedBattle->getMapSizeY() * 16; ++y)
		{
			for (int x = 0; x < _savedBattle->getMapSizeX() * 16; ++x)
			{
				int test = _savedBattle->getTileEngine()->voxelCheck(Position(x, y, z * 2), 0, 0) + 1;
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
					tile = _savedBattle->getTile(Position(x / 16, y / 16, z / 12));
					if (tile->getUnit())
					{
						if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
							test = 9;
						else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
							test = 8;
					}
					else
					{
						tile = _savedBattle->getTile(Position(x / 16, y / 16, z / 12 - 1));
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
				_savedBattle->getMapSizeX() * 16,
				_savedBattle->getMapSizeY() * 16,
				LCT_RGB);

		if (error)
		{
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
		}
	}
}

/**
 * Adds a new popup window to the queue (this prevents popups from overlapping).
 * @param state - pointer to popup State
 */
void BattlescapeState::popup(State* state)
{
	_popups.push_back(state);
}

/**
 * Finishes up the current battle, shuts down the battlescape
 * and presents the debriefing screen for the mission.
 * @param abort			- true if the mission was aborted
 * @param inExitArea	- number of soldiers in the exit area OR number of
 *							survivors when battle finished due to either all
 *							aliens or objective being destroyed
 */
void BattlescapeState::finishBattle(
		const bool abort,
		const int inExitArea)
{
	while (_game->isState(this) == false)
		_game->popState();

	_game->getCursor()->setVisible();

	if (_savedBattle->getAmbientSound() != -1)
		_game->getResourcePack()->getSoundByDepth(
												0,
												_savedBattle->getAmbientSound())
											->stopLoop();

	_game->getResourcePack()->fadeMusic(_game, 975);


	const std::string mission = _savedBattle->getMissionType();

	std::string nextStage;
	if (mission != "STR_UFO_GROUND_ASSAULT"
		&& mission != "STR_UFO_CRASH_RECOVERY")
	{
		nextStage = _game->getRuleset()->getDeployment(mission)->getNextStage();
	}

	if (nextStage.empty() == false	// if there is a next mission stage, and
		&& inExitArea > 0)			// there are soldiers in Exit_Area OR all aLiens are dead, Load the Next Stage!!!
	{
/*		std::string nextStageRace = _game->getRuleset()->getDeployment(mission)->getNextStageRace();

		for (std::vector<TerrorSite*>::const_iterator
				ts = _savedGame->getTerrorSites()->begin();
				ts != _savedGame->getTerrorSites()->end()
					&& nextStageRace.empty() == true;
				++ts)
		{
			if ((*ts)->isInBattlescape() == true)
				nextStageRace = (*ts)->getAlienRace();
		}

		for (std::vector<AlienBase*>::const_iterator
				ab = _savedGame->getAlienBases()->begin();
				ab != _savedGame->getAlienBases()->end()
					&& nextStageRace.empty() == true;
				++ab)
		{
			if ((*ab)->isInBattlescape() == true)
				nextStageRace = (*ab)->getAlienRace();
		}

		if (nextStageRace.empty() == true)
			nextStageRace = "STR_MIXED";
		else if (_game->getRuleset()->getAlienRace(nextStageRace) == NULL)
		{
			throw Exception(nextStageRace + " race not found.");
		} */

		_popups.clear();
		_savedBattle->setMissionType(nextStage);

		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
//		bgen.setAlienRace("STR_MIXED");
//		bgen.setAlienRace(nextStageRace);
		bgen.nextStage();

		_game->popState();
		_game->pushState(new BriefingState());
	}
	else
	{
		_popups.clear();
		_animTimer->stop();
		_gameTimer->stop();
		_game->popState();

		if (abort == true
			|| inExitArea == 0)
		{
			// abort was done or no player is still alive
			// this concludes to defeat when in mars or mars landing mission
			if ((mission == "STR_MARS_THE_FINAL_ASSAULT"
					|| mission == "STR_MARS_CYDONIA_LANDING")
				&& _savedGame->getMonthsPassed() > -1)
			{
				_game->pushState(new DefeatState());
			}
			else
				_game->pushState(new DebriefingState());
		}
		else
		{
			// no abort was done and at least a player is still alive
			// this concludes to victory when in mars mission
			if (mission == "STR_MARS_THE_FINAL_ASSAULT"
				&& _savedGame->getMonthsPassed() > -1)
			{
				_game->pushState(new VictoryState());
			}
			else
				_game->pushState(new DebriefingState());
		}

		_game->getCursor()->setColor(Palette::blockOffset(15)+12);
		_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
	}
}

/**
 * Shows the launch button.
 * @param show - true to show launch button
 */
void BattlescapeState::showLaunchButton(bool show)
{
	_btnLaunch->setVisible(show);
}

/**
 * Shows the PSI button.
 * @param show - true to show PSI button
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
 * @return, pointer to BattlescapeGame
 */
BattlescapeGame* BattlescapeState::getBattleGame()
{
	return _battleGame;
}

/**
 * Handler for the mouse moving over the icons, disabling the tile selection cube.
 * @param action - pointer to an Action
 */
void BattlescapeState::mouseInIcons(Action*)
{
	_mouseOverIcons = true;

	_txtConsole1->setText(L"");
	_txtConsole2->setText(L"");
//	_txtConsole3->setText(L"");
//	_txtConsole4->setText(L"");

	_txtTerrain->setVisible();
	_txtShade->setVisible();
	_txtTurn->setVisible();
	_lstExp->setVisible();
	_txtHasKill->setVisible();

	_txtFloor->setVisible(false);
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 * @param action - pointer to an Action
 */
void BattlescapeState::mouseOutIcons(Action*)
{
	_mouseOverIcons = false;
}

/**
 * Checks if the mouse is over the icons.
 * @return, true if the mouse is over the icons
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
 * @param allowSaving - true if the help button was clicked
 * @return, true if the player can still press buttons
 */
bool BattlescapeState::allowButtons(bool allowSaving) const
{
	return (
			(allowSaving
					|| _savedBattle->getSide() == FACTION_PLAYER
					|| _savedBattle->getDebugMode())
				&& (_battleGame->getPanicHandled()
					|| _firstInit)
				&& _map->getProjectile() == NULL);
}

/**
 * Updates the scale.
 * @param dX - reference the delta of X
 * @param dY - referenne the delta of Y
 */
void BattlescapeState::resize(
		int& dX,
		int& dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;

	int divisor = 1;
	double pixelRatioY = 1.0;

	if (Options::nonSquarePixelRatio)
		pixelRatioY = 1.2;

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
									static_cast<int>(static_cast<double>(Options::displayHeight) / pixelRatioY / static_cast<double>(divisor)));

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
			&& *i != _btnPsi
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
 * kL. Returns the TurnCounter used by the Battlescape.
 * @return, pointer to TurnCounter
 */
/* TurnCounter* BattlescapeState::getTurnCounter() const
{
	return _turnCounter;
} */

/**
 * kL. Updates the turn text.
 */
void BattlescapeState::updateTurn() // kL
{
	_txtTurn->setText(tr("STR_TURN").arg(_savedBattle->getTurn()));

/*	std::wostringstream woStr;
	woStr << L"turn ";
	woStr << _savedBattle->getTurn();
	_txtTurn->setText(woStr.str()); */
}

/**
 * kL. Toggles the icons' surfaces' visibility for Hidden Movement.
 * @param vis - true to show show icons
 */
void BattlescapeState::toggleIcons(bool vis)
{
	_icons->setVisible(vis);
	_numLayers->setVisible(vis);

	_btnUnitUp->setVisible(vis);
	_btnUnitDown->setVisible(vis);
	_btnMapUp->setVisible(vis);
	_btnMapDown->setVisible(vis);
	_btnShowMap->setVisible(vis);
	_btnKneel->setVisible(vis);
	_btnInventory->setVisible(vis);
	_btnCenter->setVisible(vis);
	_btnNextSoldier->setVisible(vis);
	_btnNextStop->setVisible(vis);
	_btnShowLayers->setVisible(vis);
	_btnOptions->setVisible(vis);
	_btnEndTurn->setVisible(vis);
	_btnAbort->setVisible(vis);

	_lstExp->setVisible(vis);
	_txtHasKill->setVisible(vis);

	if (vis == false)
		_txtFloor->setVisible(vis);
}

/**
 * kL. Refreshes the visUnits indicators for UnitWalk/TurnBStates.
 */
void BattlescapeState::refreshVisUnits() // kL
{
	//Log(LOG_INFO) << "BattlescapeState::refreshVisUnits()";
	if (playableUnitSelected() == false)
		return;

	for (int // remove red target indicators
			i = 0;
			i < VISIBLE_MAX;
			++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);

		_visibleUnit[i] = NULL;
	}

	BattleUnit* selectedUnit = NULL;
	if (_savedBattle->getSelectedUnit())
	{
		selectedUnit = _savedBattle->getSelectedUnit();
		//Log(LOG_INFO) << ". selUnit ID " << selectedUnit->getId();
	}

	int j = 0;
	for (std::vector<BattleUnit*>::const_iterator
			i = selectedUnit->getVisibleUnits()->begin();
			i != selectedUnit->getVisibleUnits()->end()
				&& j < VISIBLE_MAX;
			++i,
				++j)
	{
		_btnVisibleUnit[j]->setVisible();
		_numVisibleUnit[j]->setVisible();

		_visibleUnit[j] = *i;
	}
}

/**
 * Shows primer warnings on all live grenades.
 */
void BattlescapeState::drawFuse()
{
	const BattleUnit* const selectedUnit = _savedBattle->getSelectedUnit();
	if (selectedUnit == NULL)
		return;


	const int pulse[22] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3};

	if (_fuseFrame == 22)
		_fuseFrame = 0;

	Surface* const srf = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9);

	const BattleItem* item = selectedUnit->getItem("STR_LEFT_HAND");
	if (item != NULL
		&& ((item->getRules()->getBattleType() == BT_GRENADE
				|| item->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
			&& item->getFuseTimer() > -1))
	{
		_btnLeftHandItem->lock();
		srf->blitNShade(
					_btnLeftHandItem,
					_btnLeftHandItem->getX(),
					_btnLeftHandItem->getY(),
					pulse[_fuseFrame],
					false,
					3); // red
		_btnLeftHandItem->unlock();
	}

	item = selectedUnit->getItem("STR_RIGHT_HAND");
	if (item != NULL
		&& ((item->getRules()->getBattleType() == BT_GRENADE
				|| item->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
			&& item->getFuseTimer() > -1))
	{
		_btnRightHandItem->lock();
		srf->blitNShade(
					_btnRightHandItem,
					_btnRightHandItem->getX(),
					_btnRightHandItem->getY(),
					pulse[_fuseFrame],
					false,
					3); // red
		_btnRightHandItem->unlock();
	}

	_fuseFrame++;
}

/**
 * kL. Gets the TimeUnits field from icons.
 * Note: these are for use in UnitWalkBState to update info when soldier walks.
 * @return, pointer to time units NumberText
 */
NumberText* BattlescapeState::getTimeUnitsField() const // kL
{
	return _numTimeUnits;
}

/**
 * kL. Gets the TimeUnits bar from icons.
 * @return, pointer to time units Bar
 */
Bar* BattlescapeState::getTimeUnitsBar() const // kL
{
	return _barTimeUnits;
}

/**
 * kL. Gets the Energy field from icons.
 * @return, pointer to stamina NumberText
 */
NumberText* BattlescapeState::getEnergyField() const // kL
{
	return _numEnergy;
}

/**
 * kL. Gets the Energy bar from icons.
 * @return, pointer to stamina Bar
 */
Bar* BattlescapeState::getEnergyBar() const // kL
{
	return _barEnergy;
}

/**
 * kL. Updates experience data for the currently selected soldier.
 */
void BattlescapeState::updateExpData() // kL
{
	_lstExp->clearList();
	_txtHasKill->setText(L"");

	const BattleUnit* const unit = _savedBattle->getSelectedUnit();
	if (unit == NULL
		|| unit->getGeoscapeSoldier() == NULL)
	{
		return;
	}

	if (unit->hasFirstKill() == true)
		_txtHasKill->setText(L"+");


	std::vector<std::wstring> xpType;
	xpType.push_back(L"f ");
	xpType.push_back(L"t ");
	xpType.push_back(L"m ");
	xpType.push_back(L"r ");
	xpType.push_back(L"b ");
	xpType.push_back(L"p ");
	xpType.push_back(L"p ");

	const int xp[] =
	{
		unit->getExpFiring(),
		unit->getExpThrowing(),
		unit->getExpMelee(),
		unit->getExpReactions(),
		unit->getExpBravery(),
		unit->getExpPsiSkill(),
		unit->getExpPsiStrength()
	};

	for (int
			i = 0;
			i < 7;
			++i)
	{
		_lstExp->addRow(
					2,
					xpType.at(i).c_str(),
					Text::formatNumber(xp[i]).c_str());

		if (xp[i] > 10)
			_lstExp->setCellColor(i, 1, Palette::blockOffset(0));	// white
		else if (xp[i] > 5)
			_lstExp->setCellColor(i, 1, Palette::blockOffset(10));	// brown
		else if (xp[i] > 2)
			_lstExp->setCellColor(i, 1, Palette::blockOffset(1));	// orange
		else if (xp[i] > 0)
			_lstExp->setCellColor(i, 1, Palette::blockOffset(3));	// green
	}
}

/**
 * kL. Animates a red cross icon when an injured soldier is selected.
 */
void BattlescapeState::flashMedic() // kL
{
	const BattleUnit* const selectedUnit = _savedBattle->getSelectedUnit();
	if (selectedUnit != NULL
		&& selectedUnit->getFatalWounds() > 0)
	{
		static int phase; // init's only once, to 0

		Surface* const srfCross = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(11); // gray cross

		_btnWounds->lock();
		srfCross->blitNShade(
						_btnWounds,
						_btnWounds->getX() + 2,
						_btnWounds->getY() + 1,
						phase,
						false,
						3); // red
		_btnWounds->unlock();

		_numWounds->setColor(Palette::blockOffset(9) + phase); // yellow


		phase += 2;
		if (phase == 16)
			phase = 0;
	}
}

}
