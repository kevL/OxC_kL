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

#include "BattlescapeState.h"

//#define _USE_MATH_DEFINES
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
//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
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
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape screen.
 */
BattlescapeState::BattlescapeState()
	:
		_gameSave(_game->getSavedGame()),
		_battleSave(_game->getSavedGame()->getSavedBattle()),
		_rules(_game->getRuleset()),
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
		_showConsole(2),
		_visUnitTargetFrame(0),
		_showSoldierData(false),
		_iconsHidden(false),
		_isOverweight(false),
		_isKneeled(false)
{
	//Log(LOG_INFO) << "Create BattlescapeState";
	const int
		screenWidth			= Options::baseXResolution,
		screenHeight		= Options::baseYResolution,
		iconsWidth			= _rules->getInterface("battlescape")->getElement("icons")->w, // 320
		iconsHeight			= _rules->getInterface("battlescape")->getElement("icons")->h, // 56
		visibleMapHeight	= screenHeight - iconsHeight,
		x					= screenWidth / 2 - iconsWidth / 2,
		y					= screenHeight - iconsHeight;

	_txtBaseLabel			= new Text(120, 9, screenWidth - 121, 0);
	_txtRegion				= new Text(120, 9, screenWidth - 121, 10);
	_lstTileInfo			= new TextList(18, 33, screenWidth - 19, 70);
	_txtControlDestroyed	= new Text(iconsWidth, 9, x, y - 20);
	_txtMissionLabel		= new Text(iconsWidth, 9, x, y - 10);
	_txtOperationTitle		= new Text(screenWidth, 17, 0, 2);

	// Create buttonbar - this should appear at the bottom-center of the screen
	_icons		= new InteractiveSurface(
									iconsWidth,
									iconsHeight,
									x,y);
	_iconsLayer	= new Surface(32, 16, x + 208, y);

	// Create the battlemap view
	// The actual map height is the total height minus the height of the buttonbar
	_map = new Map(
				_game,
				screenWidth,
				screenHeight,
				0,0,
				visibleMapHeight);

	_numLayers	= new NumberText(3, 5, x + 232, y + 6);
//	_numLayers	= new NumberText(3, 5, x + 223, y + 6);
	_numDir		= new NumberText(3, 5, x + 150, y + 6);
	_numDirTur	= new NumberText(3, 5, x + 167, y + 6);

	_kneel		= new Surface( 2,  2, x + 115, y + 19);
	_rank		= new Surface(26, 23, x + 107, y + 33);
	_overWeight	= new Surface( 2,  2, x + 130, y + 34);

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
//	_numFuseLeft		= new NumberText(15, 5, x +  16, y + 4);
//	_numFuseRight		= new NumberText(15, 5, x + 288, y + 4);

//	const int
//		visibleUnitX = _rules->getInterface("battlescape")->getElement("visibleUnits")->x,
//		visibleUnitY = _rules->getInterface("battlescape")->getElement("visibleUnits")->y;

	_visUnitTarget = new Surface(32, 40, screenWidth / 2 - 16, visibleMapHeight / 2);

	std::fill_n(
			_visibleUnit,
			INDICATORS,
			static_cast<BattleUnit*>(NULL));

	int offset_x = 0;
	for (size_t
			i = 0;
			i != INDICATORS;
			++i)
	{
		if (i > 9)
			offset_x = 15;

		_btnVisibleUnit[i] = new InteractiveSurface(
												15,13,
												x + iconsWidth - 21 - offset_x,
												y - 16 - (static_cast<int>(i) * 13));
		_numVisibleUnit[i] = new NumberText(
										9,9,
										x + iconsWidth - 15 - offset_x,
										y - 12 - (static_cast<int>(i) * 13));
	}

	for (size_t // center 10+ on buttons
			i = 9;
			i != INDICATORS;
			++i)
	{
		_numVisibleUnit[i]->setX(_numVisibleUnit[i]->getX() - 2);
	}

	_warning	= new WarningMessage(
					224,24,
					x + 48,
					y + 32);

	_btnLaunch	= new BattlescapeButton(
					32,24,
					screenWidth - 32,
					20);
	_btnPsi		= new BattlescapeButton(
					32,24,
					screenWidth - 32,
					45);

	_txtName		= new Text(136, 9, x + 135, y + 32);

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

//	_txtDebug		= new Text(300, 10, 150, 0);
	_txtDebug		= new Text(145, 9, screenWidth - 145, screenHeight - 9);
//	_txtTooltip		= new Text(300, 10, x + 2, y - 10);

//	_turnCounter	= new TurnCounter(20, 5, 0, 0);
	_txtTerrain		= new Text(150, 9, 1,  0);
	_txtShade		= new Text( 50, 9, 1, 10);
	_txtTurn		= new Text( 50, 9, 1, 20);

	_txtOrder		= new Text(55, 9, 1, 37);
	_lstSoldierInfo	= new TextList(25, 57, 1, 47);
//	_txtHasKill		= new Text(10, 9, 1, 105);
	_alienMark		= new Surface(9, 11, 1, 105);

	_txtConsole1	= new Text(screenWidth / 2, y, 0, 0);
	_txtConsole2	= new Text(screenWidth / 2, y, screenWidth / 2, 0);
//	_txtConsole3	= new Text(screenWidth / 2, y, 0, 0);
//	_txtConsole4	= new Text(screenWidth / 2, y, screenWidth / 2, 0);

//	_battleSave->setPaletteByDepth(this);
	setPalette("PAL_BATTLESCAPE");

	if (_rules->getInterface("battlescape")->getElement("pathfinding"))
	{
		const Element* const pathing = _rules->getInterface("battlescape")->getElement("pathfinding");
		Pathfinding::green = static_cast<Uint8>(pathing->color);
		Pathfinding::yellow = static_cast<Uint8>(pathing->color2);
		Pathfinding::red = static_cast<Uint8>(pathing->border);
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
	if (_game->getResourcePack()->getSurface("TFTDReserve") != NULL)
	{
		Surface* const tftdIcons = _game->getResourcePack()->getSurface("TFTDReserve"); // 'Resources/UI/reserve.png'
		tftdIcons->setX(48);
		tftdIcons->setY(176);
		tftdIcons->blit(icons);
	}

	// there is some cropping going on here, because the icons
	// image is 320x200 while we only need the bottom of it.
	SDL_Rect* const rect = icons->getCrop();
	rect->x = 0;
	rect->y = 200 - static_cast<Sint16>(iconsHeight);
	rect->w = static_cast<Uint16>(iconsWidth);
	rect->h = static_cast<Uint16>(iconsHeight);
	icons->blit(_icons);

	// this is a hack to fix the single transparent pixel on TFTD's icon panel.
//	if (_rules->getInterface("battlescape")->getElement("icons")->TFTDMode)
//		_icons->setPixelColor(46, 44, 8);

	add(_rank,				"rank",					"battlescape", _icons);
	add(_btnWounds);
	add(_numWounds);
	add(_btnUnitUp,			"buttonUnitUp",			"battlescape", _icons); // note: these are not registered in Interfaces.rul
	add(_btnUnitDown,		"buttonUnitDown",		"battlescape", _icons);
	add(_btnMapUp,			"buttonMapUp",			"battlescape", _icons);
	add(_btnMapDown,		"buttonMapDown",		"battlescape", _icons);
	add(_btnShowMap,		"buttonShowMap",		"battlescape", _icons);
	add(_btnKneel,			"buttonKneel",			"battlescape", _icons);
	add(_btnInventory,		"buttonInventory",		"battlescape", _icons);
	add(_btnCenter,			"buttonCenter",			"battlescape", _icons);
	add(_btnNextSoldier,	"buttonNextSoldier",	"battlescape", _icons);
	add(_btnNextStop,		"buttonNextStop",		"battlescape", _icons);
	add(_btnShowLayers,		"buttonShowLayers",		"battlescape", _icons);
	add(_btnOptions,		"buttonHelp",			"battlescape", _icons);
	add(_btnEndTurn,		"buttonEndTurn",		"battlescape", _icons);
	add(_btnAbort,			"buttonAbort",			"battlescape", _icons);
	add(_btnStats,			"buttonStats",			"battlescape", _icons);
	add(_numDir);
	add(_numDirTur);
	add(_iconsLayer);														// goes overtop _btns
	add(_numLayers,			"numLayers",			"battlescape", _icons);	// goes overtop _iconsLayer
	add(_kneel);															// goes overtop _btns
	add(_overWeight);														// goes overtop _rank
	add(_txtName,			"textName",				"battlescape", _icons);
	add(_numTULaunch);
	add(_numTUAim);
	add(_numTUAuto);
	add(_numTUSnap);
	add(_numTimeUnits,		"numTUs",				"battlescape", _icons);
	add(_numEnergy,			"numEnergy",			"battlescape", _icons);
	add(_numHealth,			"numHealth",			"battlescape", _icons);
	add(_numMorale,			"numMorale",			"battlescape", _icons);
	add(_barTimeUnits,		"barTUs",				"battlescape", _icons);
	add(_barEnergy,			"barEnergy",			"battlescape", _icons);
	add(_barHealth,			"barHealth",			"battlescape", _icons);
	add(_barMorale,			"barMorale",			"battlescape", _icons);
/*	add(_btnReserveNone,	"buttonReserveNone",	"battlescape", _icons);
	add(_btnReserveSnap,	"buttonReserveSnap",	"battlescape", _icons);
	add(_btnReserveAimed,	"buttonReserveAimed",	"battlescape", _icons);
	add(_btnReserveAuto,	"buttonReserveAuto",	"battlescape", _icons);
	add(_btnReserveKneel,	"buttonReserveKneel",	"battlescape", _icons); */
	add(_btnZeroTUs,		"buttonZeroTUs",		"battlescape", _icons);
	add(_btnLeftHandItem,	"buttonLeftHand",		"battlescape", _icons);
	add(_btnRightHandItem,	"buttonRightHand",		"battlescape", _icons);
	add(_numAmmoLeft,		"numAmmoLeft",			"battlescape", _icons);
	add(_numAmmoRight,		"numAmmoRight",			"battlescape", _icons);
//	add(_numFuseLeft,		"numAmmoLeft",			"battlescape", _icons);
//	add(_numFuseRight,		"numAmmoRight",			"battlescape", _icons);
	add(_visUnitTarget);

	_visUnitTarget->setVisible(false);
//	_iconsLayer->setVisible(false);

	for (size_t
			i = 0;
			i != INDICATORS;
			++i)
	{
		add(_btnVisibleUnit[i]);
		add(_numVisibleUnit[i]);
	}


	add(_btnLaunch);
	add(_btnPsi);

	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(0)->blit(_btnLaunch);
	_btnLaunch->onMouseClick((ActionHandler)& BattlescapeState::btnLaunchClick);
	_btnLaunch->setVisible(false);

	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blit(_btnPsi);
	_btnPsi->onMouseClick((ActionHandler)& BattlescapeState::btnPsiClick);
	_btnPsi->setVisible(false);

//	add(_txtTooltip, "textTooltip", "battlescape", _icons);
//	_txtTooltip->setHighContrast();

	add(_txtDebug,				"textName",			"battlescape");
	add(_warning,				"warning",			"battlescape", _icons);
	add(_txtOperationTitle,		"operationTitle",	"battlescape");
	add(_txtBaseLabel,			"infoText",			"battlescape");
	add(_txtRegion,				"infoText",			"battlescape");
	add(_txtControlDestroyed,	"infoText",			"battlescape");
	add(_txtMissionLabel,		"infoText",			"battlescape");

	_txtDebug->setHighContrast();
	_txtDebug->setAlign(ALIGN_RIGHT);

	_warning->setColor(static_cast<Uint8>(_rules->getInterface("battlescape")->getElement("warning")->color2));
	_warning->setTextColor(static_cast<Uint8>(_rules->getInterface("battlescape")->getElement("warning")->color));

	if (_battleSave->getOperation().empty() == false)
	{
		_txtOperationTitle->setText(_battleSave->getOperation().c_str());
		_txtOperationTitle->setHighContrast();
		_txtOperationTitle->setAlign(ALIGN_CENTER);
		_txtOperationTitle->setBig();
	}
	else
		_txtOperationTitle->setVisible(false);

	_txtControlDestroyed->setText(tr("STR_ALIEN_BASE_CONTROL_DESTROYED"));
	_txtControlDestroyed->setHighContrast();
	_txtControlDestroyed->setAlign(ALIGN_CENTER);
	_txtControlDestroyed->setVisible(false);

	const Target* target = NULL;

	std::wstring
		baseLabel,
		missionLabel;

	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end()
				&& baseLabel.empty() == true;
			++i)
	{
		if ((*i)->isInBattlescape() == true)
		{
			target = dynamic_cast<Target*>(*i);

			baseLabel = (*i)->getName(_game->getLanguage());
			missionLabel = tr("STR_BASE_DEFENSE");

			break;
		}

		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end()
					&& baseLabel.empty() == true;
				++j)
		{
			if ((*j)->isInBattlescape() == true)
			{
				target = dynamic_cast<Target*>(*j);

				baseLabel = (*i)->getName(_game->getLanguage());
			}
		}
	}
	_txtBaseLabel->setText(tr("STR_SQUAD_").arg(baseLabel.c_str())); // there'd better be a baseLabel ... or else. Pow! To the moon!!!
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setHighContrast();


	if (missionLabel.empty() == true)
	{
		std::wostringstream woststr;

		for (std::vector<Ufo*>::const_iterator
				i = _gameSave->getUfos()->begin();
				i != _gameSave->getUfos()->end()
					&& woststr.str().empty() == true;
				++i)
		{
			if ((*i)->isInBattlescape() == true)
			{
				target = dynamic_cast<Target*>(*i);

				if ((*i)->isCrashed() == true)
					woststr << tr("STR_UFO_CRASH_RECOVERY");
				else
					woststr << tr("STR_UFO_GROUND_ASSAULT");

				woststr << L"> " << (*i)->getName(_game->getLanguage());
			}
		}

		for (std::vector<MissionSite*>::const_iterator
				i = _gameSave->getMissionSites()->begin();
				i != _gameSave->getMissionSites()->end()
					&& woststr.str().empty() == true;
				++i)
		{
			if ((*i)->isInBattlescape() == true)
			{
				target = dynamic_cast<Target*>(*i);

				woststr << tr("STR_TERROR_MISSION") << L"> " << (*i)->getName(_game->getLanguage()); // <- not necessarily a Terror Mission ...
			}
		}

		for (std::vector<AlienBase*>::const_iterator
				i = _gameSave->getAlienBases()->begin();
				i != _gameSave->getAlienBases()->end()
					&& woststr.str().empty() == true;
				++i)
		{
			if ((*i)->isInBattlescape() == true)
			{
				target = dynamic_cast<Target*>(*i);

				woststr << tr("STR_ALIEN_BASE_ASSAULT") << L"> " << (*i)->getName(_game->getLanguage());
			}
		}

		missionLabel = woststr.str();
	}

	if (missionLabel.empty() == true)
		missionLabel = tr(_battleSave->getMissionType());

	_txtMissionLabel->setText(missionLabel.c_str()); // there'd better be a missionLabel ... or else. Pow! To the moon!!!
	_txtMissionLabel->setAlign(ALIGN_CENTER);
	_txtMissionLabel->setHighContrast();


	if (target != NULL)
	{
		std::wostringstream woststr;
		const double
			lon = target->getLongitude(),
			lat = target->getLatitude();

		for (std::vector<Region*>::const_iterator
				i = _game->getSavedGame()->getRegions()->begin();
				i != _game->getSavedGame()->getRegions()->end();
				++i)
		{
			if ((*i)->getRules()->insideRegion(
											lon,
											lat) == true)
			{
				woststr << tr((*i)->getRules()->getType());
				break;
			}
		}

		for (std::vector<Country*>::const_iterator
				i = _game->getSavedGame()->getCountries()->begin();
				i != _game->getSavedGame()->getCountries()->end();
				++i)
		{
			if ((*i)->getRules()->insideCountry(
											lon,
											lat) == true)
			{
				woststr << L"> " << tr((*i)->getRules()->getType());
				break;
			}
		}

		_txtRegion->setText(woststr.str()); // there'd better be a region ... or else. Pow! To the moon!!!
		_txtRegion->setAlign(ALIGN_RIGHT);
		_txtRegion->setHighContrast();
	}


//	add(_turnCounter);
//	_turnCounter->setColor(Palette::blockOffset(8));

	add(_lstTileInfo,	"textName",	"battlescape"); // blue
	add(_txtConsole1,	"textName",	"battlescape");
	add(_txtConsole2,	"textName",	"battlescape");
//	add(_txtConsole3,	"textName",	"battlescape");
//	add(_txtConsole4,	"textName",	"battlescape");

	_lstTileInfo->setColumns(2, 11, 7);
	_lstTileInfo->setHighContrast();

	_txtConsole1->setHighContrast();
	_txtConsole1->setVisible(_showConsole > 0);
	_txtConsole2->setHighContrast();
	_txtConsole2->setVisible(_showConsole > 1);
//	_txtConsole3->setHighContrast();
//	_txtConsole3->setVisible(_showConsole > 2);
//	_txtConsole4->setHighContrast();
//	_txtConsole4->setVisible(_showConsole > 3);

	add(_txtTerrain,		"infoText",			"battlescape"); // yellow
	add(_txtShade,			"infoText",			"battlescape");
	add(_txtTurn,			"infoText",			"battlescape");
	add(_txtOrder,			"operationTitle",	"battlescape"); // white
	add(_lstSoldierInfo,	"textName",			"battlescape"); // blue
//	add(_txtHasKill,		"infoText",			"battlescape");
	add(_alienMark);

	_txtTerrain->setHighContrast();
	_txtTerrain->setText(tr("STR_TEXTURE_").arg(tr(_battleSave->getBattleTerrain())));

	_txtShade->setHighContrast();
	_txtShade->setText(tr("STR_SHADE_").arg(_battleSave->getGlobalShade()));

	_txtTurn->setHighContrast();
	_txtTurn->setText(tr("STR_TURN").arg(_battleSave->getTurn()));

	_txtOrder->setHighContrast();

	_lstSoldierInfo->setHighContrast();
	_lstSoldierInfo->setColumns(2, 10, 15);

//	_txtHasKill->setHighContrast();
	Surface* const srfMark = _game->getResourcePack()->getSurface("ALIENINSIGNIA");
	srfMark->blit(_alienMark);
	_alienMark->setVisible(false);


//	_numLayers->setColor(Palette::blockOffset(5)+12);
//	_numLayers->setValue(1);

	_numDir->setColor(Palette::blockOffset(5)+12);
	_numDir->setValue(0);

	_numDirTur->setColor(Palette::blockOffset(5)+12);
	_numDirTur->setValue(0);

	_rank->setVisible(false);

	_kneel->drawRect(0,0,2,2, Palette::blockOffset(5)+12);
	_kneel->setVisible(false);

/*	Surface* srfOverload = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(97); // 274, brown dot 2px; 97, red sq 3px
	srfOverload->setX(-1);
	srfOverload->setY(-1);
	srfOverload->blit(_overWeight); */
	_overWeight->drawRect(0,0,2,2, Palette::blockOffset(2)+13); // dark.red
	_overWeight->setVisible(false);

	_btnWounds->setVisible(false);
	_btnWounds->onMousePress((ActionHandler)& BattlescapeState::btnWoundedPress);

	_numWounds->setColor(Palette::blockOffset(9)); // yellow
	_numWounds->setValue(0);
	_numWounds->setVisible(false);

	_numAmmoLeft->setValue(0);
	_numAmmoRight->setValue(0);

//	_numFuseLeft->setValue(0);
//	_numFuseRight->setValue(0);

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
					(ActionHandler)& BattlescapeState::btnUfoPaediaClick,
					SDL_BUTTON_LEFT);
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


/*	const SDLKey buttons[] =
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
	}; */

	const Uint8 color = static_cast<Uint8>(_rules->getInterface("battlescape")->getElement("visibleUnits")->color);
	for (size_t
			i = 0;
			i != INDICATORS;
			++i)
	{
		_btnVisibleUnit[i]->onMousePress((ActionHandler)& BattlescapeState::btnVisibleUnitPress);
//		_btnVisibleUnit[i]->onKeyboardPress(
//						(ActionHandler)& BattlescapeState::btnVisibleUnitPress,
//						buttons[i]);

//		std::ostringstream tooltip;
//		tooltip << "STR_CENTER_ON_ENEMY_" << (i + 1);
//		_btnVisibleUnit[i]->setTooltip(tooltip.str());
//		_btnVisibleUnit[i]->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//		_btnVisibleUnit[i]->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

		_numVisibleUnit[i]->setColor(color);
		_numVisibleUnit[i]->setValue(static_cast<unsigned>(i) + 1);
	}

	_txtName->setHighContrast();

	_numTULaunch->setColor(Palette::blockOffset(0)+7);
	_numTUAim->setColor(Palette::blockOffset(0)+7);
	_numTUAuto->setColor(Palette::blockOffset(0)+7);
	_numTUSnap->setColor(Palette::blockOffset(0)+7);

	_barTimeUnits->setScale();
	_barTimeUnits->setMax(100.);

	_barEnergy->setScale();
	_barEnergy->setMax(100.);

	_barHealth->setScale();
	_barHealth->setMax(100.);
//	_barHealth->setBorderColor(Palette::blockOffset(2)+7);

	_barMorale->setScale();
	_barMorale->setMax(100.);

/*	_btnReserveNone->setGroup(&_reserve);
	_btnReserveSnap->setGroup(&_reserve);
	_btnReserveAimed->setGroup(&_reserve);
	_btnReserveAuto->setGroup(&_reserve); */

	_animTimer = new Timer(DEFAULT_ANIM_SPEED);
	_animTimer->onTimer((StateHandler)& BattlescapeState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED + 32);
	_gameTimer->onTimer((StateHandler)& BattlescapeState::handleState);

	_battleGame = new BattlescapeGame(
									_battleSave,
									this);
	//Log(LOG_INFO) << "Create BattlescapeState EXIT";
}

/**
 * Deletes this BattlescapeState.
 */
BattlescapeState::~BattlescapeState()
{
	//Log(LOG_INFO) << "Delete BattlescapeState";
	delete _animTimer;
	delete _gameTimer;
	delete _battleGame;
}

/**
 * Initializes this BattlescapeState.
 */
void BattlescapeState::init()
{
	//Log(LOG_INFO) << "BattlescapeState::init()";
//	if (_battleSave->getAmbientSound() != -1)
//		_game->getResourcePack()->getSoundByDepth(
//												0,
//												_battleSave->getAmbientSound())
//											->loop();

	State::init();

	_animTimer->start();
	_gameTimer->start();

	_map->setFocus(true);
	_map->cacheUnits();
	_map->draw();
	_battleGame->init();

//	kL_TurnCount = _battleSave->getTurn();	// kL
//	_turnCounter->update();					// kL
//	_turnCounter->draw();					// kL

	updateSoldierInfo();

/*	switch (_battleSave->getBATReserved())
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
		_map->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());

		std::string
			music,
			terrain;
		_battleSave->calibrateMusic(
								music,
								terrain);
		_game->getResourcePack()->playMusic(
										music,
										terrain);

/*		_btnReserveNone->setGroup(&_reserve);
		_btnReserveSnap->setGroup(&_reserve);
		_btnReserveAimed->setGroup(&_reserve);
		_btnReserveAuto->setGroup(&_reserve); */
	}

	_numLayers->setValue(static_cast<unsigned int>(_map->getCamera()->getViewLevel() + 1));

	if (_iconsHidden == false
		&& _battleSave->getDestroyed() == true)
	{
		_txtControlDestroyed->setVisible();
	}
	else
		_txtControlDestroyed->setVisible(false);

//	_txtTooltip->setText(L"");
/*	if (_battleSave->getKneelReserved())
		_btnReserveKneel->invert(_btnReserveKneel->getColor()+3);

	_btnReserveKneel->toggle(_battleSave->getKneelReserved());
	_battleGame->setKneelReserved(_battleSave->getKneelReserved()); */
	//Log(LOG_INFO) << "BattlescapeState::init() EXIT";
}

/**
 * Runs the timers and handles popups.
 */
void BattlescapeState::think()
{
	//Log(LOG_INFO) << "BattlescapeState::think()";
	if (_gameTimer->isRunning() == true)
	{
		static bool popped; // inits to false.

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
				popped = false;
				_battleGame->handleNonTargetAction();
			}
		}
		else // Handle popups
		{
			popped = true;
			_game->pushState(*_popups.begin());
			_popups.erase(_popups.begin());
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
		if ((SDL_GetMouseState(0,0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeDragScroll);
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
//			stopScrolling(action); // newScroll

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
	else if (_showConsole > 0
		&& _mouseOverIcons == false
		&& allowButtons() == true)
	{
		_txtConsole1->setText(L"");
		_txtConsole2->setText(L"");
//		_txtConsole3->setText(L"");
//		_txtConsole4->setText(L"");

		bool showInfo = true;

		Position pos;
		_map->getSelectorPosition(&pos);

		Tile* const tile = _battleSave->getTile(pos);
		if (   tile != NULL
			&& tile->isDiscovered(2) == true
			&& tile->getInventory()->empty() == false)
		{
			showInfo = false;

			size_t row = 0;
			std::wostringstream
				woststr,	// test
				woststr1,	// Console #1
				woststr2;	// Console #2
			std::wstring
				wst,
				wst1,
				wst2,
				wst3;
			int qty = 1;

			for (size_t
					i = 0;
					i != tile->getInventory()->size() + 1;
					++i)
			{
				wst1 = L"> ";

				if (i < tile->getInventory()->size())
				{
					const BattleItem* const item = tile->getInventory()->at(i);
					const RuleItem* const itRule = item->getRules();

					if (item->getUnit() != NULL)
					{
						if (item->getUnit()->getType().compare(0,11, "STR_FLOATER") == 0) // TODO: require Floater autopsy research; also, InventoryState::setExtraInfo()
						{
							wst1 += tr("STR_FLOATER");
							wst1 += L" (status doubtful)";
						}
						else if (item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
						{
							wst1 += item->getUnit()->getName(_game->getLanguage());

							if (item->getUnit()->getGeoscapeSoldier() != NULL)
								wst1 += L" (" + Text::formatNumber(item->getUnit()->getHealth() - item->getUnit()->getStun() - 1) + L")";
						}
						else
						{
							wst1 += tr(itRule->getType());

							if (item->getUnit()->getGeoscapeSoldier() != NULL)
								wst1 += L" (" + item->getUnit()->getName(_game->getLanguage()) + L")";
						}
					}
					else if (_game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
					{
						wst1 += tr(itRule->getType());

						if (itRule->getBattleType() == BT_AMMO)
							wst1 += L" (" + Text::formatNumber(item->getAmmoQuantity()) + L")";
						else if (itRule->getBattleType() == BT_FIREARM
							&& item->getAmmoItem() != NULL
							&& item->getAmmoItem() != item)
						{
							wst = tr(item->getAmmoItem()->getRules()->getType());
							wst1 += L" | " + wst + L" (" + Text::formatNumber(item->getAmmoItem()->getAmmoQuantity()) + L")";
						}
						else if ((itRule->getBattleType() == BT_GRENADE
								|| itRule->getBattleType() == BT_PROXYGRENADE)
							&& item->getFuseTimer() > -1)
						{
							wst1 += L" (" + Text::formatNumber(item->getFuseTimer()) + L")";
						}
					}
					else
						wst1 += tr("STR_ALIEN_ARTIFACT");
				}

				if (i == 0)
				{
					wst3 =
					wst2 = wst1;
					continue;
				}

				if (wst1 == wst3)
				{
					++qty;
					continue;
				}
				else
					wst3 = wst1;

				if (qty > 1)
				{
					wst2 += L" * " + Text::formatNumber(qty);
					qty = 1;
				}

				wst2 += L"\n";
				woststr << wst2;
				wst3 =
				wst2 = wst1;


				if (row < 26) // Console #1
				{
					if (row == 24)
					{
						woststr << L"> more >>>";
						++row;
					}

					woststr1.str(L"");
					woststr1 << woststr.str();

					if (row == 25)
					{
						/* Log(LOG_INFO) << "row 25";
						std::string s (wst1.begin(), wst1.end());
						Log(LOG_INFO) << ". wst1 = " << s;
						std::string t (wst2.begin(), wst2.end());
						Log(LOG_INFO) << ". wst2 = " << t;
						std::string u (wst3.begin(), wst3.end());
						Log(LOG_INFO) << ". wst3 = " << u;
						std::wstring wstr1 (woststr1.str());
						std::string v (wstr1.begin(), wstr1.end());
						Log(LOG_INFO) << ". woststr1 = " << v;
						std::wstring wstr2 (woststr.str());
						std::string w (wstr2.begin(), wstr2.end());
						Log(LOG_INFO) << ". woststr = " << w; */

						if (wst1 == L"> ")
						{
							wst = woststr1.str();
							wst.erase(wst.length() - 8);
							woststr1.str(L"");
							woststr1 << wst;
						}

						if (_showConsole == 1)
							break;

						woststr.str(L"");
					}
				}

				if (row > 26) // Console #2
				{
					if (row == 50)
						woststr << L"> more >>>";

					woststr2.str(L"");
					woststr2 << woststr.str();

					if (row == 51)
					{
						if (wst1 == L"> ")
						{
							wst = woststr2.str();
							wst.erase(wst.length() - 8);
							woststr2.str(L"");
							woststr2 << wst;
						}

						break;
					}
				}

				++row;
			}

			_txtConsole1->setText(woststr1.str());
			_txtConsole2->setText(woststr2.str());
		}

		updateTileInfo(tile);

		_txtTerrain->setVisible(showInfo);
		_txtShade->setVisible(showInfo);
		_txtTurn->setVisible(showInfo);

		_txtOrder->setVisible(showInfo);
		_lstSoldierInfo->setVisible(showInfo);
		_alienMark->setVisible(showInfo && allowAlienMark());
//		_txtHasKill->setVisible(showInfo);
		_showSoldierData = showInfo;
	}
}

/**
 * Processes any presses on the map.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapPress(Action* action)
{
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons == false
		&& action->getDetails()->button.button == Options::battleDragScrollButton)
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

		_totalMouseMoveX =
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
			&& (SDL_GetMouseState(0,0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeDragScroll);
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
//			stopScrolling(action); // newScroll
		}
	}

	if (_isMouseScrolling == true) // DragScroll-Button release: release mouse-scroll-mode
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
//			stopScrolling(action); // newScroll
		}
		else
			return;

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (_mouseOverThreshold == false
			&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
//			_map->getCamera()->setMapOffset(_mapOffsetBeforeDragScroll); // oldScroll
//			stopScrolling(action); // newScroll
		}

		if (_isMouseScrolled == true)
			return;
	}

	// right-click aborts walking state
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		&& _battleGame->cancelCurrentAction() == true)
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

	// don't allow to click into void
	if (_battleSave->getTile(pos) != NULL)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
			&& playableUnitSelected() == true)
		{
			_battleGame->secondaryAction(pos);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			_battleGame->primaryAction(pos);

//		if (_battleSave->getDebugMode() == true)
//		{
		std::wostringstream woststr;
		if (_battleSave->getTile(pos)->getUnit() != NULL)
			woststr	<< L"unit "
					<< _battleSave->getTile(pos)->getUnit()->getId()
					<< L" ";

		woststr << L"pos " << pos;
		debug(woststr.str());
//		}
	}
}

/**
 * Handles mouse entering the map surface.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapIn(Action*)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(static_cast<Uint8>(Options::battleDragScrollButton), false);
}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * @param action - pointer to an Action
 */
/*void BattlescapeState::stopScrolling(Action* action)
{
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
		if (_battleGame->getCurrentAction()->actor == NULL
			&& (_save->getSide() == FACTION_PLAYER
				|| _save->getDebugMode() == true))
		{
			_map->setCursorType(CT_NORMAL);
		}
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
	_cursorPosition.z = 0;
} */

/**
 * Takes care of any events from the core game engine.
 * @param action - pointer to an Action
 */
inline void BattlescapeState::handle(Action* action)
{
	if (_firstInit == true) return;

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
			_map->setSelectorPosition(
								(_cursorPosition.x - _game->getScreen()->getCursorLeftBlackBand()) / action->getXScale(),
								(_cursorPosition.y - _game->getScreen()->getCursorTopBlackBand()) / action->getYScale());
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
			if (Options::debug == true)
			{
				if ((SDL_GetModState() & KMOD_CTRL) != 0)
				{
					if (action->getDetails()->key.keysym.sym == SDLK_d)		// "ctrl-d" - enable debug mode
					{
						_battleSave->setDebugMode();
//						_txtOperationTitle->setVisible(false);
						debug(L"Debug Mode");
					}
					else if (_battleSave->getDebugMode()
						&& action->getDetails()->key.keysym.sym == SDLK_v)	// "ctrl-v" - reset tile visibility
					{
						debug(L"Resetting tile visibility");
						_battleSave->resetTiles();
					}
					else if (_battleSave->getDebugMode()
						&& action->getDetails()->key.keysym.sym == SDLK_k)	// "ctrl-k" - kill all aliens
					{
						debug(L"Influenza bacterium dispersed");
						for (std::vector<BattleUnit*>::const_iterator
								i = _battleSave->getUnits()->begin();
								i !=_battleSave->getUnits()->end();
								++i)
						{
							if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
							{
								(*i)->instaKill();
								if ((*i)->getTile() != NULL)
									(*i)->getTile()->setUnit(NULL);
							}
						}
					}
				}
				else if (action->getDetails()->key.keysym.sym == SDLK_F10)	// f10 - voxel map dump
					saveVoxelMap();
				else if (action->getDetails()->key.keysym.sym == SDLK_F9	// f9 - ai dump
					&& Options::traceAI == true)
				{
					saveAIMap();
				}
			}
			else if (_gameSave->isIronman() == false)
			{
				// not works in debug mode to prevent conflict in hotkeys by default
				if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)		// f6 - quickSave
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_QUICK,
													_palette));
				else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)	// f5 - quickLoad
					_game->pushState(new LoadGameState(
													OPT_BATTLESCAPE,
													SAVE_QUICK,
													_palette));
			}

			if (action->getDetails()->key.keysym.sym == Options::keyBattleVoxelView)	// f11 - voxel view pic
				saveVoxelView();
		}
	}
}

/**
 * Moves the selected unit up.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitUpClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		Pathfinding* const pf = _battleSave->getPathfinding();
		pf->setPathingUnit(_battleSave->getSelectedUnit());
		const int valid = pf->validateUpDown(
										_battleSave->getSelectedUnit()->getPosition(),
										Pathfinding::DIR_UP);

		if (valid > 0) // gravLift or flying
		{
			_battleGame->cancelCurrentAction();
			_battleGame->moveUpDown(
								_battleSave->getSelectedUnit(),
								Pathfinding::DIR_UP);
		}
		else if (valid == -1) // no flight suit
			warning("STR_ACTION_NOT_ALLOWED_FLIGHT");
		else // valid==0 -> blocked by roof
			warning("STR_ACTION_NOT_ALLOWED_ROOF");
	}
}

/**
 * Moves the selected unit down.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitDownClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		Pathfinding* const pf = _battleSave->getPathfinding();
		pf->setPathingUnit(_battleSave->getSelectedUnit());
		const int valid = pf->validateUpDown(
										_battleSave->getSelectedUnit()->getPosition(),
										Pathfinding::DIR_DOWN);

		if (valid > 0) // gravLift or flying
		{
			_battleGame->cancelCurrentAction();
			_battleGame->moveUpDown(
								_battleSave->getSelectedUnit(),
								Pathfinding::DIR_DOWN);
		}
		else // blocked, floor
			warning("STR_ACTION_NOT_ALLOWED_FLOOR");
	}
}

/**
 * Shows the next map layer.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapUpClick(Action*)
{
/*	if (_battleSave->getSide() == FACTION_PLAYER
		|| _battleSave->getDebugMode() == true)
	{
		_map->getCamera()->up();
	} */
	if (allowButtons() == true)
		_map->getCamera()->up();
}

/**
 * Shows the previous map layer.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapDownClick(Action*)
{
/*	if (_battleSave->getSide() == FACTION_PLAYER
		|| _battleSave->getDebugMode() == true)
	{
		_map->getCamera()->down();
	} */
	if (allowButtons() == true)
		_map->getCamera()->down();
}

/**
 * Sets the level on the icons' Layers button.
 * @param level - Z level
 */
void BattlescapeState::setLayerValue(int level)
{
	if (level < 0)
		level = 0;
	_numLayers->setValue(static_cast<unsigned int>(level + 1));
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
										_battleSave));
}

/**
 * Toggles the current unit's kneel/standup status.
 * @param action, Pointer to an action.
 */
void BattlescapeState::btnKneelClick(Action*)
{
	if (allowButtons() == true)
	{
		BattleUnit* const unit = _battleSave->getSelectedUnit();
		if (unit != NULL)
		{
			//Log(LOG_INFO) << "BattlescapeState::btnKneelClick()";
			if (_battleGame->kneel(unit) == true)
			{
				updateSoldierInfo(false);

				_battleGame->getTileEngine()->calculateFOV(unit->getPosition());
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
				_battleGame->getTileEngine()->checkReactionFire(unit);

				Pathfinding* const pf = _battleGame->getPathfinding();
				if (pf->isPathPreviewed() == true)
				{
					pf->setPathingUnit(_battleGame->getCurrentAction()->actor);
					pf->calculate(
								_battleGame->getCurrentAction()->actor,
								_battleGame->getCurrentAction()->target);
					pf->removePreview();
					pf->previewPath();
				}
			}
		}
	}
}

/**
 * Goes to the inventory screen.
 * @note Additionally resets TUs for current side in debug mode.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnInventoryClick(Action*)
{
	if (_battleSave->getDebugMode() == true)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == _battleSave->getSide())
				(*i)->prepUnit(); // cheat for debugging.

			updateSoldierInfo();
		}
	}

	if (playableUnitSelected() == true)
	{
		const BattleUnit* const unit = _battleSave->getSelectedUnit();

		if (_battleSave->getDebugMode() == true
			|| (unit->getGeoscapeSoldier() != NULL
				|| (unit->getUnitRules()->isMechanical() == false
					&& unit->getRankString() != "STR_LIVE_TERRORIST")))
		{
			if (_battleGame->getCurrentAction()->type == BA_LAUNCH) // clean up the waypoints
			{
				_battleGame->getCurrentAction()->waypoints.clear();
				_map->getWaypoints()->clear();
				showLaunchButton(false);
			}

			_battleGame->getPathfinding()->removePreview();
			_battleGame->cancelCurrentAction(true);

			_game->pushState(new InventoryState(
											_battleSave->getDebugMode() == false,
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
	if (playableUnitSelected() == true)
	{
		_map->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects the next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextSoldierClick(Action*)
{
	if (allowButtons() == true)
		selectNextFactionUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextStopClick(Action*)
{
	if (allowButtons() == true)
		selectNextFactionUnit(true, true);
}

/**
 * Selects next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevSoldierClick(Action*)
{
	if (allowButtons() == true)
		selectPreviousFactionUnit(true);
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevStopClick(Action*)
{
	if (allowButtons() == true)
		selectPreviousFactionUnit(true, true);
}

/**
 * Selects the next soldier.
 * @param checkReselect		- don't select a unit that has been previously flagged
 * @param setDontReselect	- flag the current unit first
 * @param checkInventory	- don't select a unit that has no inventory
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

		BattleUnit* const unit = _battleSave->selectNextFactionUnit(
																checkReselect,
																setDontReselect,
																checkInventory);
		updateSoldierInfo();

		if (unit != NULL)
			_map->getCamera()->centerOnPosition(unit->getPosition());

		_battleGame->cancelCurrentAction();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}

/**
 * Selects the previous soldier.
 * @param checkReselect		- don't select a unit that has been previously flagged
 * @param setDontReselect	- flag the current unit first
 * @param checkInventory	- don't select a unit that has no inventory
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

		BattleUnit* const unit = _battleSave->selectPreviousFactionUnit(
																	checkReselect,
																	setDontReselect,
																	checkInventory);
		updateSoldierInfo();

		if (unit != NULL)
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
//	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());

	if (allowButtons() == true)
	{
		const bool showLayers = (_map->getCamera()->toggleShowAllLayers() == 2) ? true : false;

		if (showLayers == false)
			_iconsLayer->clear();
		else
		{
			Surface* const iconsLayer = _game->getResourcePack()->getSurface("ICONS_LAYER");
			iconsLayer->blit(_iconsLayer);
		}
	}
}

/**
 * Shows options.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnHelpClick(Action*)
{
	if (allowButtons(true) == true)
		_game->pushState(new PauseState(OPT_BATTLESCAPE));
}

/**
 * Requests the end of turn.
 * @note This will add a 0 to the end of the state queue so all ongoing actions
 * like explosions are finished first before really switching turn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnEndTurnClick(Action*)
{
	if (allowButtons() == true)
	{
//		_txtTooltip->setText(L"");
		_battleGame->requestEndTurn();
	}
}

/**
 * Aborts the mission.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnAbortClick(Action*)
{
	if (allowButtons() == true)
		_game->pushState(new AbortMissionState(
											_battleSave,
											this));
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

		if (Options::battleEdgeScroll == SCROLL_TRIGGER
			&& action->getDetails()->type == SDL_MOUSEBUTTONUP
			&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			const int
				posX = action->getXMouse(),
				posY = action->getYMouse();
			if ((posX < Camera::SCROLL_BORDER * action->getXScale()
					&& posX > 0)
				|| posX > (_map->getWidth() - Camera::SCROLL_BORDER) * action->getXScale()
				|| (posY < (Camera::SCROLL_BORDER * action->getYScale())
						&& posY > 0)
				|| posY > (_map->getHeight() - Camera::SCROLL_BORDER) * action->getYScale())
			{
				edge = true;	// To avoid handling this event as a click on the stats
			}					// button when the mouse is on the scroll-border
		}

		if (_battleGame->getCurrentAction()->type == BA_LAUNCH) // clean up the waypoints
		{
			_battleGame->getCurrentAction()->waypoints.clear();
			_map->getWaypoints()->clear();
			showLaunchButton(false);
		}

		_battleGame->cancelCurrentAction(true);

		if (edge == false)
			popup(new UnitInfoState(
								_battleSave->getSelectedUnit(),
								this));
	}
}

/**
 * Shows an action popup menu. Creates the action when clicked.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLeftHandLeftClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelCurrentAction();
		_battleSave->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");

		_map->cacheUnits();
		_map->draw();

		BattleItem* const leftHandItem = _battleSave->getSelectedUnit()->getItem("STR_LEFT_HAND");
		handClick(leftHandItem);
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

		_battleSave->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");
		updateSoldierInfo(false);

		_map->cacheUnits();
		_map->draw();
	}
}

/**
 * Shows an action popup menu. Creates the action when clicked.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnRightHandLeftClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelCurrentAction();
		_battleSave->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");

		_map->cacheUnits();
		_map->draw();

		BattleItem* const rightHandItem = _battleSave->getSelectedUnit()->getItem("STR_RIGHT_HAND");
		handClick(rightHandItem);
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

		_battleSave->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");
		updateSoldierInfo(false);

		_map->cacheUnits();
		_map->draw();
	}
}

/**
 * LMB centers on the unit corresponding to this button.
 * RMB cycles through spotters of the unit corresponding to this button.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnVisibleUnitPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		btnMapDownClick(NULL);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		btnMapUpClick(NULL);
	else
	{
		size_t i;
		for ( // find out which button was pressed
				i = 0;
				i != INDICATORS;
				++i)
		{
			if (_btnVisibleUnit[i] == action->getSender())
				break;
		}

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_map->getCamera()->centerOnPosition(_visibleUnit[i]->getPosition());

			_visUnitTarget->setVisible();
			_visUnitTargetFrame = 0;
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			BattleUnit* nextSpotter = NULL;
			size_t curIter = 0;

			for (std::vector<BattleUnit*>::const_iterator
				j = _battleSave->getUnits()->begin();
				j != _battleSave->getUnits()->end();
				++j)
			{
				++curIter;
				if (*j == _battleSave->getSelectedUnit())
					break;
			}

			for (std::vector<BattleUnit*>::const_iterator
				j = _battleSave->getUnits()->begin() + curIter;
				j != _battleSave->getUnits()->end();
				++j)
			{
				if ((*j)->getFaction() == FACTION_PLAYER
					&& (*j)->isOut() == false
					&& std::find(
								(*j)->getVisibleUnits()->begin(),
								(*j)->getVisibleUnits()->end(),
								_visibleUnit[i]) != (*j)->getVisibleUnits()->end())
				{
					nextSpotter = *j;
					break;
				}
			}

			if (nextSpotter == NULL)
			{
				for (std::vector<BattleUnit*>::const_iterator
					j = _battleSave->getUnits()->begin();
					j != _battleSave->getUnits()->end() - _battleSave->getUnits()->size() + curIter;
					++j)
				{
					if ((*j)->getFaction() == FACTION_PLAYER
						&& (*j)->isOut() == false
						&& std::find(
									(*j)->getVisibleUnits()->begin(),
									(*j)->getVisibleUnits()->end(),
									_visibleUnit[i]) != (*j)->getVisibleUnits()->end())
					{
						nextSpotter = *j;
						break;
					}
				}
			}

			if (nextSpotter != NULL)
			{
				if (nextSpotter != _battleSave->getSelectedUnit())
				{
					_battleSave->setSelectedUnit(nextSpotter);
					updateSoldierInfo();

					_battleGame->cancelCurrentAction();
					_battleGame->getCurrentAction()->actor = nextSpotter;
					_battleGame->setupCursor();
				}

				Camera* const camera = _map->getCamera();
				if (camera->isOnScreen(nextSpotter->getPosition()) == false
					|| camera->getViewLevel() != nextSpotter->getPosition().z)
				{
					camera->centerOnPosition(nextSpotter->getPosition());
				}
			}
		}
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Centers on the currently wounded soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnWoundedPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		btnMapDownClick(NULL);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		btnMapUpClick(NULL);
	else if ((action->getDetails()->button.button == SDL_BUTTON_LEFT
			|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		&& playableUnitSelected() == true)
	{
		_map->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());
	}

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
		Action a = Action(&ev, 0.,0., 0,0);
		action->getSender()->mousePress(&a, this);

		if		(_reserve == _btnReserveNone)	_battleGame->setReservedAction(BA_NONE);
		else if (_reserve == _btnReserveSnap)	_battleGame->setReservedAction(BA_SNAPSHOT);
		else if (_reserve == _btnReserveAimed)	_battleGame->setReservedAction(BA_AIMEDSHOT);
		else if (_reserve == _btnReserveAuto)	_battleGame->setReservedAction(BA_AUTOSHOT);

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

		Action a = Action(&ev, 0.,0., 0,0);
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
		&& _battleSave->getSelectedUnit()->checkAmmo() == true)
	{
//		_game->getResourcePack()->getSoundByDepth(
//												_battleSave->getDepth(),
		_game->getResourcePack()->getSound(
										"BATTLE.CAT",
										ResourcePack::ITEM_RELOAD)
									->play(
										-1,
										_map->getSoundAngle(_battleSave->getSelectedUnit()->getPosition()));

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

		Action a = Action(&ev, 0.,0.,0,0);
		action->getSender()->mousePress(&a, this);

		if (_battleSave->getSelectedUnit() != NULL)
		{
			_battleSave->getSelectedUnit()->setTimeUnits(0);
			updateSoldierInfo();
		}
	}
}

/**
 * Opens the UfoPaedia.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUfoPaediaClick(Action*)
{
	if (allowButtons() == true)
		Ufopaedia::open(
					_game,
					true);
}

/**
 * Toggles soldier's personal lighting (purely cosmetic).
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPersonalLightingClick(Action*)
{
	if (allowButtons() == true)
		_battleSave->getTileEngine()->togglePersonalLighting();
}

/**
 * Handler for toggling the console.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnConsoleToggle(Action*)
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

			_txtOrder->setVisible();
			_lstSoldierInfo->setVisible();
			_alienMark->setVisible(allowAlienMark());
//			_txtHasKill->setVisible();
			_showSoldierData = true;
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
			_txtOrder->setVisible();
			_lstSoldierInfo->setVisible();
			_alienMark->setVisible(allowAlienMark());
//			_txtHasKill->setVisible();
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
 * Determines whether a playable unit is selected.
 * @note Normally only player side units can be selected but in debug mode one
 * can play with aliens too :)
 * @note Is used to see if stats can be displayed and action buttons will work.
 * @return, true if a playable unit is selected
 */
bool BattlescapeState::playableUnitSelected()
{
	return _battleSave->getSelectedUnit() != NULL
		&& allowButtons() == true;
}

/**
 * Updates a unit's onScreen stats & info.
 * @param calcFoV - true to run calculateFOV() for the unit (default true)
 */
void BattlescapeState::updateSoldierInfo(bool calcFoV)
{
	for (size_t // remove target indicators
			i = 0;
			i != INDICATORS;
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
//	_numFuseRight		->setVisible(false);
//	_numFuseLeft		->setVisible(false);

	_isKneeled =
	_isOverweight = false;

	_kneel		->setVisible(false);
	_overWeight	->setVisible(false);
	_numDir		->setVisible(false);
	_numDirTur	->setVisible(false);

	_numTULaunch->setVisible(false);
	_numTUAim	->setVisible(false);
	_numTUAuto	->setVisible(false);
	_numTUSnap	->setVisible(false);

	_numWounds	->setVisible(false);
	_btnWounds	->setVisible(false);

	_txtOrder->setText(L"");


	if (playableUnitSelected() == false) // not a controlled unit; ie. aLien or civilian turn
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
	else // not aLien or civilian; ie. a controlled unit
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


	BattleUnit* const selUnit = _battleSave->getSelectedUnit(); // <- not sure if this is FACTION_PLAYER only.

	if (selUnit == NULL)
		return;


	if (calcFoV == true)
		_battleSave->getTileEngine()->calculateFOV(selUnit);

	size_t j = 0;
	for (std::vector<BattleUnit*>::const_iterator
		i = _battleSave->getUnits()->begin();
		i != _battleSave->getUnits()->end()
			&& j != INDICATORS;
		++i)
	{
		if ((*i)->isOut() == false
			&& (*i)->getUnitVisible() == true
			&& (*i)->getFaction() == FACTION_HOSTILE)
		{
			_btnVisibleUnit[j]->setVisible();
			_numVisibleUnit[j]->setVisible();

			_visibleUnit[j++] = *i;
		}
	}


	_txtName->setText(selUnit->getName(
									_game->getLanguage(),
									false));

	const Soldier* const soldier = selUnit->getGeoscapeSoldier();
	if (soldier != NULL)
	{
		SurfaceSet* const texture = _game->getResourcePack()->getSurfaceSet("SMOKE.PCK");
		texture->getFrame(20 + soldier->getRank())->blit(_rank);

		if (selUnit->isKneeled() == true)
		{
			_isKneeled = true;
			_kneel->setVisible(); // this doesn't blink, unlike _overWeight mark
		}

		_txtOrder->setText(tr("STR_ORDER")
							.arg(static_cast<int>(selUnit->getBattleOrder())));
	}

	const int strength = static_cast<int>(Round(
						 static_cast<double>(selUnit->getBaseStats()->strength) * (selUnit->getAccuracyModifier() / 2. + 0.5)));
	if (selUnit->getCarriedWeight() > strength)
	{
		_isOverweight = true;
//		_overWeight->setVisible(); // this needs to blink, unlike _kneel mark
	}

	_numDir->setValue(selUnit->getDirection());
	_numDir->setVisible();

	if (selUnit->getTurretType() != -1)
	{
		_numDirTur->setValue(selUnit->getTurretDirection());
		_numDirTur->setVisible();
	}

	const int wounds = selUnit->getFatalWounds();
	if (wounds > 0)
	{
		Surface* srfStatus = _game->getResourcePack()->getSurface("RANK_ROOKIE");
		if (srfStatus != NULL)
		{
			srfStatus->blit(_btnWounds); // red heart icon
			_btnWounds->setVisible();
		}

		_numWounds->setX(_btnWounds->getX() + 7);

		_numWounds->setValue(static_cast<unsigned>(wounds));
		_numWounds->setVisible();
	}


	double stat = static_cast<double>(selUnit->getBaseStats()->tu);
	const int tu = selUnit->getTimeUnits();
	_numTimeUnits->setValue(static_cast<unsigned>(tu));
	_barTimeUnits->setValue(std::ceil(
							static_cast<double>(tu) / stat * 100.));

	stat = static_cast<double>(selUnit->getBaseStats()->stamina);
	const int energy = selUnit->getEnergy();
	_numEnergy->setValue(static_cast<unsigned>(energy));
	_barEnergy->setValue(std::ceil(
							static_cast<double>(energy) / stat * 100.));

	stat = static_cast<double>(selUnit->getBaseStats()->health);
	const int health = selUnit->getHealth();
	_numHealth->setValue(static_cast<unsigned>(health));
	_barHealth->setValue(std::ceil(
							static_cast<double>(health) / stat * 100.));
	_barHealth->setValue2(std::ceil(
							static_cast<double>(selUnit->getStun()) / stat * 100.));

	const int morale = selUnit->getMorale();
	_numMorale->setValue(static_cast<unsigned>(morale));
	_barMorale->setValue(morale);


	const BattleItem
		* const rtItem = selUnit->getItem("STR_RIGHT_HAND"),
		* const ltItem = selUnit->getItem("STR_LEFT_HAND");

	const std::string activeHand = selUnit->getActiveHand();
	if (activeHand.empty() == false)
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
			tuLaunch = selUnit->getActionTUs(BA_LAUNCH, rtItem);
			tuAim = selUnit->getActionTUs(BA_AIMEDSHOT, rtItem);
			tuAuto = selUnit->getActionTUs(BA_AUTOSHOT, rtItem);
			tuSnap = selUnit->getActionTUs(BA_SNAPSHOT, rtItem);
			if (tuLaunch == 0
				&& tuAim == 0
				&& tuAuto == 0
				&& tuSnap == 0)
			{
				tuSnap = selUnit->getActionTUs(BA_HIT, rtItem);
			}
		}
		else if (activeHand == "STR_LEFT_HAND"
			&& (ltItem->getRules()->getBattleType() == BT_FIREARM
				|| ltItem->getRules()->getBattleType() == BT_MELEE))
		{
			tuLaunch = selUnit->getActionTUs(BA_LAUNCH, ltItem);
			tuAim = selUnit->getActionTUs(BA_AIMEDSHOT, ltItem);
			tuAuto = selUnit->getActionTUs(BA_AUTOSHOT, ltItem);
			tuSnap = selUnit->getActionTUs(BA_SNAPSHOT, ltItem);
			if (tuLaunch == 0
				&& tuAim == 0
				&& tuAuto == 0
				&& tuSnap == 0)
			{
				tuSnap = selUnit->getActionTUs(BA_HIT, ltItem);
			}
		}

		if (tuLaunch != 0)
		{
			_numTULaunch->setValue(tuLaunch);
			_numTULaunch->setVisible();
		}

		if (tuAim != 0)
		{
			_numTUAim->setValue(tuAim);
			_numTUAim->setVisible();
		}

		if (tuAuto != 0)
		{
			_numTUAuto->setValue(tuAuto);
			_numTUAuto->setVisible();
		}

		if (tuSnap != 0)
		{
			_numTUSnap->setValue(tuSnap);
			_numTUSnap->setVisible();
		}
	}

	if (rtItem != NULL)
	{
		rtItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_btnRightHandItem);
		_btnRightHandItem->setVisible();

		if (rtItem->getRules()->getBattleType() == BT_FIREARM
			&& (rtItem->usesAmmo() == true
				|| rtItem->getRules()->getClipSize() > 0))
		{
			_numAmmoRight->setVisible();
			if (rtItem->getAmmoItem() != NULL)
				_numAmmoRight->setValue(static_cast<unsigned>(rtItem->getAmmoItem()->getAmmoQuantity()));
			else
				_numAmmoRight->setValue(0);
		}
		else if (rtItem->getRules()->getBattleType() == BT_GRENADE
			&& rtItem->getFuseTimer() > 0)
		{
			_numAmmoRight->setVisible();
			_numAmmoRight->setValue(static_cast<unsigned>(rtItem->getFuseTimer()));
		}
	}

	if (ltItem != NULL)
	{
		ltItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_btnLeftHandItem);
		_btnLeftHandItem->setVisible();

		if (ltItem->getRules()->getBattleType() == BT_FIREARM
			&& (ltItem->usesAmmo() == true
				|| ltItem->getRules()->getClipSize() > 0))
		{
			_numAmmoLeft->setVisible();
			if (ltItem->getAmmoItem() != NULL)
				_numAmmoLeft->setValue(static_cast<unsigned>(ltItem->getAmmoItem()->getAmmoQuantity()));
			else
				_numAmmoLeft->setValue(0);
		}
		else if (ltItem->getRules()->getBattleType() == BT_GRENADE
			&& ltItem->getFuseTimer() > 0)
		{
			_numAmmoLeft->setVisible();
			_numAmmoLeft->setValue(static_cast<unsigned>(ltItem->getFuseTimer()));
		}
	}

	showPsiButton(
				selUnit->getOriginalFaction() == FACTION_HOSTILE
				&& selUnit->getBaseStats()->psiSkill > 0);
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
	_kneel->drawRect(0,0, 2,2, Palette::blockOffset(5)+12);
} */

/**
 * kL. Draws the fatal wounds indicator.
 */
/* void BattlescapeState::drawWoundIndicator() // kL
{
	_btnWounds->drawRect(0,0, 15,12, 15);		// black border
	_btnWounds->drawRect(1,1, 13,10, color);	// inner red square
} */

/**
 * Shifts the colors of the visible unit buttons' backgrounds.
 */
void BattlescapeState::blinkVisibleUnitButtons() // private.
{
	BattleUnit* const selUnit = _battleSave->getSelectedUnit();
	if (selUnit != NULL)
	{
		static int
			delta = 1,
			colorRed = 34,		// currently selected unit sees other unit
			colorBlue = 114,	// another unit can see other unit
			color_border = 15;	// dark.gray

		Uint8 color;
		bool isSpotted;

		for (size_t
				i = 0;
				i != INDICATORS;
				++i)
		{
			if (_btnVisibleUnit[i]->getVisible() == true)
			{
				isSpotted = false;

				for (std::vector<BattleUnit*>::const_iterator
					j = _battleSave->getUnits()->begin();
					j != _battleSave->getUnits()->end();
					++j)
				{
					if ((*j)->getFaction() == FACTION_PLAYER
						&& (*j)->isOut() == false)
					{
						if (std::find(
									(*j)->getVisibleUnits()->begin(),
									(*j)->getVisibleUnits()->end(),
									_visibleUnit[i]) != (*j)->getVisibleUnits()->end())
						{
							isSpotted = true;
							break;
						}
					}
				}

				if (isSpotted == true)
				{
					if (std::find(
								selUnit->getVisibleUnits()->begin(),
								selUnit->getVisibleUnits()->end(),
								_visibleUnit[i]) != selUnit->getVisibleUnits()->end())
					{
						color = static_cast<Uint8>(colorRed);
					}
					else
						color = static_cast<Uint8>(colorBlue);
				}
				else
					color = 51; // green // 114; // lt.blue <- hostile unit is visible but not currently viewed by friendly units; ergo do not cycle colors.

				_btnVisibleUnit[i]->drawRect(0,0, 15,13, static_cast<Uint8>(color_border));
				_btnVisibleUnit[i]->drawRect(1,1, 13,11, color);
			}
		}

		if (colorRed == 34)
			delta = 1;
		else if (colorRed == 45)
			delta = -1;

		colorRed += delta;
		colorBlue += delta;
		color_border -= delta;
	}
}

/**
 * Refreshes the visUnits indicators for UnitWalk/TurnBStates.
 */
void BattlescapeState::refreshVisUnits()
{
	if (playableUnitSelected() == true)
	{
		for (size_t // remove target indicators
				i = 0;
				i != INDICATORS;
				++i)
		{
			_btnVisibleUnit[i]->setVisible(false);
			_numVisibleUnit[i]->setVisible(false);

			_visibleUnit[i] = NULL;
		}

		size_t j = 0;
		for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end()
				&& j != INDICATORS;
			++i)
		{
			if ((*i)->isOut() == false
				&& (*i)->getUnitVisible() == true
				&& (*i)->getFaction() == FACTION_HOSTILE)
			{
				_btnVisibleUnit[j]->setVisible();
				_numVisibleUnit[j]->setVisible();

				_visibleUnit[j++] = *i;
			}
		}
	}
}

/**
 * Animates things on the map and in the HUD.
 */
void BattlescapeState::animate()
{
	_map->animateMap(_battleGame->isBusy() == false);	// this needs to happen regardless so that UFO
														// doors (&tc) do not stall walking units (&tc)
	if (_map->getMapHidden() == false)
	{
		blinkVisibleUnitButtons();
		drawFuse();
		flashMedic();
		drawVisUnitTarget();

		if (_isOverweight == true
			&& RNG::seedless(0,3) > 2)
		{
			_overWeight->setVisible(!_overWeight->getVisible());
		}
	}
}

/**
 * Pops up a context sensitive list of actions the player can choose from.
 * @note Some actions result in a change of gamestate.
 * @param item - pointer to the clicked BattleItem (righthand/lefthand)
 */
void BattlescapeState::handClick(BattleItem* const item) // private.
{
	if (_battleGame->isBusy() == false) // battlescape is in an idle state
	{
		BattleAction* const action = _battleGame->getCurrentAction();
		action->weapon = NULL; // safety.

		if (item != NULL) // make sure there is an item
			action->weapon = item;
		else if (action->actor->getUnitRules() != NULL // so player can hit w/ MC'd melee aLiens that are not 'livingWeapons'
			&& action->actor->getUnitRules()->getMeleeWeapon() == "STR_FIST")
		{
			// TODO: This can be generalized later; right now the only 'meleeWeapon' is "STR_FIST" - the Universal Fist!!
//			const RuleItem* const itRule = _rules->getItem(action->actor->getUnitRules()->getMeleeWeapon());
			action->weapon = _battleGame->getFist();
		}

		if (action->weapon != NULL)
			popup(new ActionMenuState(
									action,
									_icons->getX(),
									_icons->getY() + 16));
	}
}
/*	if (_battleGame->isBusy() == false	// battlescape is in an idle state
		&& item != NULL)				// make sure there is an item
	{
		if (_gameSave->isResearched(item->getRules()->getRequirements())
			|| _battleSave->getSelectedUnit()->getOriginalFaction() == FACTION_HOSTILE)
		{
			_battleGame->getCurrentAction()->weapon = item;
			popup(new ActionMenuState(
									_battleGame->getCurrentAction(),
									_icons->getX(),
									_icons->getY() + 16));
		}
		else warning("STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED");
	} */

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
 * Gets pointer to the SavedGame.
 * @return, pointer to SavedGame
 */
SavedGame* BattlescapeState::getSavedGame() const
{
	return _gameSave;
}

/**
 * Gets pointer to the SavedBattleGame.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* BattlescapeState::getSavedBattleGame() const
{
	return _battleSave;
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
//	if (_battleSave->getDebugMode() == true)
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
 * Adds a new popup window to the queue (this prevents popups from overlapping).
 * @param state - pointer to popup State
 */
void BattlescapeState::popup(State* state)
{
	_popups.push_back(state);
}

/**
 * Finishes up the current battle, shuts down the battlescape and presents the
 * debriefing screen for the mission.
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

//	if (_battleSave->getAmbientSound() != -1)
//		_game->getResourcePack()->getSoundByDepth(
//												0,
//												_battleSave->getAmbientSound())
//											->stopLoop();

	_game->getResourcePack()->fadeMusic(_game, 975);


	const std::string stType = _battleSave->getMissionType();

	std::string nextStage;
//	if (stType != "STR_UFO_GROUND_ASSAULT"
//		&& stType != "STR_UFO_CRASH_RECOVERY")
	if (_battleSave->getTacticalType() != TCT_UFOCRASHED
		&& _battleSave->getTacticalType() != TCT_UFOLANDED)
	{
		nextStage = _rules->getDeployment(stType)->getNextStage();
	}

	if (nextStage.empty() == false	// if there is a next mission stage, and
		&& inExitArea > 0)			// there are soldiers in Exit_Area OR all aLiens are dead, Load the Next Stage!!!
	{
/*		std::string nextStageRace = _rules->getDeployment(stType)->getNextStageRace();

		for (std::vector<TerrorSite*>::const_iterator
				ts = _gameSave->getTerrorSites()->begin();
				ts != _gameSave->getTerrorSites()->end()
					&& nextStageRace.empty() == true;
				++ts)
		{
			if ((*ts)->isInBattlescape() == true)
				nextStageRace = (*ts)->getAlienRace();
		}

		for (std::vector<AlienBase*>::const_iterator
				ab = _gameSave->getAlienBases()->begin();
				ab != _gameSave->getAlienBases()->end()
					&& nextStageRace.empty() == true;
				++ab)
		{
			if ((*ab)->isInBattlescape() == true)
				nextStageRace = (*ab)->getAlienRace();
		}

		if (nextStageRace.empty() == true)
			nextStageRace = "STR_MIXED";
		else if (_rules->getAlienRace(nextStageRace) == NULL)
		{
			throw Exception(nextStageRace + " race not found.");
		} */

		_popups.clear();
		_battleSave->setMissionType(nextStage);

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

		if (abort == true		// abort was done or no player is still alive
			|| inExitArea == 0)	// this concludes to defeat when in mars or mars landing mission
		{
			if (   _rules->getDeployment(stType) != NULL
				&& _rules->getDeployment(stType)->isNoRetreat() == true
				&& _gameSave->getMonthsPassed() != -1)
			{
				_game->pushState(new DefeatState());
			}
			else
				_game->pushState(new DebriefingState());
		}
		else					// no abort was done and at least a player is still alive
		{						// this concludes to victory when in mars mission
			if (   _rules->getDeployment(stType) != NULL
				&& _rules->getDeployment(stType)->isFinalMission() == true
				&& _gameSave->getMonthsPassed() != -1)
			{
				_game->pushState(new VictoryState());
			}
			else
				_game->pushState(new DebriefingState());
		}
	}
}

/**
 * Shows the launch button.
 * @param show - true to show launch button (default true)
 */
void BattlescapeState::showLaunchButton(bool show)
{
	_btnLaunch->setVisible(show);
}

/**
 * Shows the PSI button.
 * @param show - true to show PSI button (default true)
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
 * Returns a pointer to BattlescapeGame.
 * @return, pointer to BattlescapeGame
 */
BattlescapeGame* BattlescapeState::getBattleGame()
{
	return _battleGame;
}

/**
 * Handler for the mouse moving over the icons disabling the tile selection cube.
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

	_txtOrder->setVisible();
	_lstSoldierInfo->setVisible();
	_alienMark->setVisible(allowAlienMark());
//	_txtHasKill->setVisible();
	_showSoldierData = true;

	_lstTileInfo->setVisible(false);
}

/**
 * Handler for the mouse going out of the icons enabling the tile selection cube.
 * @param action - pointer to an Action
 */
void BattlescapeState::mouseOutIcons(Action*)
{
	_mouseOverIcons = false;

	_lstTileInfo->setVisible();
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
 * @note Buttons are disabled in the middle of a shot, during the alien turn,
 * and while a player's units are panicking. The save button is an exception to
 * still be able to save if something goes wrong during the alien turn and
 * submit the save file for dissection.
 * @param allowSaving - true if the help button was clicked
 * @return, true if the player can still press buttons
 */
bool BattlescapeState::allowButtons(bool allowSaving) const
{
	return (
			(allowSaving == true
					|| _battleSave->getSide() == FACTION_PLAYER
					|| _battleSave->getDebugMode() == true)
				&& (_battleGame->getPanicHandled() == true
					|| _firstInit == true)
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
	double pixelRatioY = 1.;

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
		if (   *i != _map
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
 * Returns the TurnCounter used by the Battlescape.
 * @return, pointer to TurnCounter
 */
/* TurnCounter* BattlescapeState::getTurnCounter() const
{
	return _turnCounter;
} */

/**
 * Updates the turn text.
 */
void BattlescapeState::updateTurn()
{
	_txtTurn->setText(tr("STR_TURN").arg(_battleSave->getTurn()));

/*	std::wostringstream woStr;
	woStr << L"turn ";
	woStr << _battleSave->getTurn();
	_txtTurn->setText(woStr.str()); */
}

/**
 * Toggles the icons' surfaces' visibility for Hidden Movement.
 * @param vis - true to show show icons and info
 */
void BattlescapeState::toggleIcons(bool vis)
{
	_iconsHidden = !vis;

	_icons->setVisible(vis);
	_iconsLayer->setVisible(vis);
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

	_txtOrder->setVisible(vis);
	_lstSoldierInfo->setVisible(vis);
	_alienMark->setVisible(vis && allowAlienMark());
//	_txtHasKill->setVisible(vis);
	_showSoldierData = vis;

//	_txtControlDestroyed->setVisible(vis);
	_txtMissionLabel->setVisible(vis);
	_lstTileInfo->setVisible(vis);

	// note: These two might not be necessary if selectedUnit is never aLien or Civie.
	_overWeight->setVisible(vis && _isOverweight);
	// no need for handling the kneel indicator i guess; do it anyway
	_kneel->setVisible(vis && _isKneeled);
}

/**
 * Animates primer warnings on all live grenades.
 */
void BattlescapeState::drawFuse()
{
	const BattleUnit* const selectedUnit = _battleSave->getSelectedUnit();
	if (selectedUnit == NULL)
		return;


	static const int pulse[PULSE_FRAMES] = { 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
											13,12,11,10, 9, 8, 7, 6, 5, 4, 3};

	if (_fuseFrame == PULSE_FRAMES)
		_fuseFrame = 0;

	static Surface* const srf = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9); // plus sign

	const BattleItem* item = selectedUnit->getItem("STR_LEFT_HAND");
	if (item != NULL
		&& ((item->getRules()->getBattleType() == BT_GRENADE
				|| item->getRules()->getBattleType() == BT_PROXYGRENADE)
			&& item->getFuseTimer() != -1))
	{
		_btnLeftHandItem->lock();
		srf->blitNShade(
					_btnLeftHandItem,
					_btnLeftHandItem->getX() + 27,
					_btnLeftHandItem->getY() - 1,
					pulse[_fuseFrame],
					false,
					3); // red
		_btnLeftHandItem->unlock();
	}

	item = selectedUnit->getItem("STR_RIGHT_HAND");
	if (item != NULL
		&& ((item->getRules()->getBattleType() == BT_GRENADE
				|| item->getRules()->getBattleType() == BT_PROXYGRENADE)
			&& item->getFuseTimer() != -1))
	{
		_btnRightHandItem->lock();
		srf->blitNShade(
					_btnRightHandItem,
					_btnRightHandItem->getX() + 27,
					_btnRightHandItem->getY() - 1,
					pulse[_fuseFrame],
					false,
					3); // red
		_btnRightHandItem->unlock();
	}

	++_fuseFrame;
}

/**
 * Gets the TimeUnits field from icons.
 * Note: these are for use in UnitWalkBState to update info when soldier walks.
 * @return, pointer to time units NumberText
 */
NumberText* BattlescapeState::getTimeUnitsField() const
{
	return _numTimeUnits;
}

/**
 * Gets the TimeUnits bar from icons.
 * @return, pointer to time units Bar
 */
Bar* BattlescapeState::getTimeUnitsBar() const
{
	return _barTimeUnits;
}

/**
 * Gets the Energy field from icons.
 * @return, pointer to stamina NumberText
 */
NumberText* BattlescapeState::getEnergyField() const
{
	return _numEnergy;
}

/**
 * Gets the Energy bar from icons.
 * @return, pointer to stamina Bar
 */
Bar* BattlescapeState::getEnergyBar() const
{
	return _barEnergy;
}

/**
 * Checks if it's okay to show a rookie's kill/stun alien icon.
 * @return, true if okay to show icon
 */
bool BattlescapeState::allowAlienMark() const
{
	return _battleSave->getSelectedUnit() != NULL
		&& _battleSave->getSelectedUnit()->getGeoscapeSoldier() != NULL
		&& _battleSave->getSelectedUnit()->hasFirstKill();
}

/**
 * Updates experience data for the currently selected soldier.
 */
void BattlescapeState::updateExperienceInfo()
{
	_lstSoldierInfo->clearList();
	_alienMark->setVisible(false);

	if (_showSoldierData == false)
		return;


	const BattleUnit* const unit = _battleSave->getSelectedUnit();
	if (unit == NULL
		|| unit->getGeoscapeSoldier() == NULL)
	{
		return;
	}

	if (unit->hasFirstKill() == true)
		_alienMark->setVisible();


	// keep this consistent ...
	std::vector<std::wstring> xpType;
	xpType.push_back(L"f "); // firing
	xpType.push_back(L"t "); // throwing
	xpType.push_back(L"m "); // melee
	xpType.push_back(L"r "); // reactions
	xpType.push_back(L"b "); // bravery
	xpType.push_back(L"a "); // psiSkill attack
	xpType.push_back(L"d "); // psiStrength defense

	// ... consistent with this
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

	for (size_t
			i = 0;
			i != sizeof(xp) / sizeof(xp[0]);
			++i)
	{
		_lstSoldierInfo->addRow(
							2,
							xpType.at(i).c_str(),
							Text::formatNumber(xp[i]).c_str());

		if (xp[i] > 10)
			_lstSoldierInfo->setCellColor(i, 1, Palette::blockOffset(5), true);		// lt.brown
		else if (xp[i] > 5)
			_lstSoldierInfo->setCellColor(i, 1, Palette::blockOffset(10), true);	// brown
		else if (xp[i] > 2)
			_lstSoldierInfo->setCellColor(i, 1, Palette::blockOffset(1), true);		// orange
		else if (xp[i] > 0)
			_lstSoldierInfo->setCellColor(i, 1, Palette::blockOffset(3), true);		// green
	}
}

/**
 * Updates tile info for the tile under mouseover.
 * @param tile - pointer to a Tile
 */
void BattlescapeState::updateTileInfo(const Tile* const tile)
{
	_lstTileInfo->clearList();

	if (tile == NULL
		|| tile->isDiscovered(2) == false)
	{
		return;
	}


	size_t rows = 3;
	int tuCost = 0;

	const BattleUnit* const unit = _battleSave->getSelectedUnit();
	if (unit != NULL
		&& unit->getFaction() == FACTION_PLAYER)
	{
		++rows;

		MovementType moveType = unit->getMoveTypeUnit();

		tuCost = tile->getTUCostTile(
								O_FLOOR,
								moveType)
			   + tile->getTUCostTile(
								O_OBJECT,
								moveType);

		if (   tile->getMapData(O_FLOOR) == NULL
			&& tile->getMapData(O_OBJECT) != NULL)
		{
			tuCost += 4;
		}
		else if (tuCost == 0)
		{
			if (   moveType == MT_FLY
				|| moveType == MT_FLOAT)
			{
				tuCost = 4;
			}
			else
				tuCost = 255;
		}
	}


	const int info[] =
	{
		static_cast<int>(tile->hasNoFloor(_battleSave->getTile(tile->getPosition() + Position(0,0,-1)))),
		tile->getSmoke(),
		tile->getFire(),
		tuCost
	};

	std::vector<std::wstring> infoType;
	infoType.push_back(L"F"); // Floor
	infoType.push_back(L"S"); // smoke
	infoType.push_back(L"I"); // fire
	infoType.push_back(L"M"); // tuCost


	Uint8 color = Palette::blockOffset(8); // blue, avoid VC++ linker warning.

	for (size_t
			i = 0;
			i != rows;
			++i)
	{
		if (i == 0) // Floor
		{
			std::wstring hasFloor;
			if (info[i] == 0)
			{
				hasFloor = L"F";
				color = Palette::blockOffset(3); // green, Floor
			}
			else
			{
				hasFloor = L"-";
				color = Palette::blockOffset(1); // orange, NO Floor
			}

			_lstTileInfo->addRow(
							2,
							hasFloor.c_str(),
							infoType.at(i).c_str());
		}
		else if (i < 3) // smoke & fire
		{
			if (i == 1)
				color = Palette::blockOffset(5); // brown, smoke
			else
				color = Palette::blockOffset(2); // red, fire

			std::wstring value;
			if (info[i] != 0)
				value = Text::formatNumber(info[i]).c_str();
			else
				value = L"";

			_lstTileInfo->addRow(
							2,
							value.c_str(),
							infoType.at(i).c_str());
		}
		else if (unit != NULL) // tuCost
		{
			color = Palette::blockOffset(8); // blue

			std::wstring cost;
			if (info[i] < 255)
				cost = Text::formatNumber(info[i]).c_str();
			else
				cost = L"-";

			_lstTileInfo->addRow(
							2,
							cost.c_str(),
							infoType.at(i).c_str());
		}

		_lstTileInfo->setCellColor(
								i,
								0,
								color,
								true);
	}
}

/**
 * Animates a red cross icon when an injured soldier is selected.
 */
void BattlescapeState::flashMedic()
{
	const BattleUnit* const selectedUnit = _battleSave->getSelectedUnit();
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

		_numWounds->setColor(Palette::blockOffset(9) + static_cast<Uint8>(phase)); // yellow shades


		phase += 2;
		if (phase == 16)
			phase = 0;
	}
}

/**
 * Animates targeting cursor over hostile unit when visUnit indicator is clicked.
 */
void BattlescapeState::drawVisUnitTarget()
{
	static const int cursorFrames[TARGET_FRAMES] = {0,1,2,3,4,0}; // note: does not show the last frame.

	if (_visUnitTarget->getVisible() == true)
	{
		Surface* const targetCursor = _game->getResourcePack()->getSurfaceSet("TARGET.PCK")->getFrame(cursorFrames[_visUnitTargetFrame]);
		targetCursor->blit(_visUnitTarget);

		++_visUnitTargetFrame;

		if (_visUnitTargetFrame == TARGET_FRAMES)
			_visUnitTarget->setVisible(false);
	}
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
//	Uint32 start = SDL_GetTicks(); // kL_note: Not used.

	const BattleUnit* const unit = _battleSave->getSelectedUnit();
	if (unit == NULL)
		return;

	int
		w = _battleSave->getMapSizeX(),
		h = _battleSave->getMapSizeY();

	SDL_Surface* const img = SDL_AllocSurface(
										0,
										w * 8,
										h * 8,
										24,
										0xff,
										0xff00,
										0xff0000,
										0);
	std::memset(
			img->pixels,
			0,
			static_cast<size_t>(img->pitch * static_cast<Uint16>(img->h)));

	Position pos (unit->getPosition()); // init.
	Position tilePos (pos); // init.

	SDL_Rect rect;
	rect.h =
	rect.w = 8;

/*	Tile* t; // kL_note: Not used ->
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
			t = _battleSave->getTile(tilePos);

			if (t == NULL)
				continue;
			if (t->isDiscovered(2) == false)
				continue;
		}
	}

	int expMax = 0;
	if (expMax < 100)
		expMax = 100; */

	const Tile* t;
	const BattleUnit* wat;
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
			t = _battleSave->getTile(tilePos);

			if (t == NULL)
				continue;
			if (t->isDiscovered(2) == false)
				continue;

			rect.x = static_cast<Sint16>(x) * static_cast<Sint16>(rect.w);
			rect.y = static_cast<Sint16>(y) * static_cast<Sint16>(rect.h);

			if (t->getTUCostTile(O_FLOOR, MT_FLY) != 255
				&& t->getTUCostTile(O_OBJECT, MT_FLY) != 255)
			{
				SDL_FillRect(
						img,
						&rect,
						SDL_MapRGB(
								img->format,
								255,
								0,
								0x20));
				characterRGBA(
						img,
						rect.x,
						rect.y,
						'*',
						0x7f,
						0x7f,
						0x7f,
						0x7f);
			}
			else
			{
				if (t->getUnit() == NULL)
					SDL_FillRect(
							img,
							&rect,
							SDL_MapRGB(
									img->format,
									0x50,
									0x50,
									0x50)); // gray for blocked tile
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

				t = _battleSave->getTile(pos);
				wat = t->getUnit();
				if (wat != NULL)
				{
					switch (wat->getFaction())
					{
						case FACTION_HOSTILE:
							// #4080C0 is Volutar Blue. CONGRATULATIONz!!!
							characterRGBA(
										img,
										rect.x,
										rect.y,
										(tilePos.z - z)? 'a': 'A',
										0x40,
										0x80,
										0xC0,
										0xff);
						break;
						case FACTION_PLAYER:
							characterRGBA(
										img,
										rect.x,
										rect.y,
										(tilePos.z - z)? 'x': 'X',
										255,
										255,
										127,
										0xff);
						break;
						case FACTION_NEUTRAL:
							characterRGBA(
										img,
										rect.x,
										rect.y,
										(tilePos.z - z)? 'c': 'C',
										255,
										127,
										127,
										0xff);
						break;
					}

					break;
				}

				--pos.z;
				if (z > 0
					&& t->hasNoFloor(_battleSave->getTile(pos)) == false)
				{
					break; // no seeing through floors
				}
			}

			if (t->getMapData(O_NORTHWALL)
				&& t->getMapData(O_NORTHWALL)->getTUCostObject(MT_FLY) == 255)
			{
				lineRGBA(
						img,
						rect.x,
						rect.y,
						rect.x + static_cast<Sint16>(rect.w),
						rect.y,
						0x50,
						0x50,
						0x50,
						255);
			}

			if (t->getMapData(O_WESTWALL)
				&& t->getMapData(O_WESTWALL)->getTUCostObject(MT_FLY) == 255)
			{
				lineRGBA(
						img,
						rect.x,
						rect.y,
						rect.x,
						rect.y + static_cast<Sint16>(rect.h),
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
			12,12,
			ss.str().c_str(),
			0,0,0,
			0x7f);

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getUserFolder() << "AIExposure" << std::setfill('0') << std::setw(3) << i << ".png";

		++i;
	}
	while (CrossPlatform::fileExists(ss.str()));


	unsigned error = lodepng::encode(
									ss.str(),
									static_cast<const unsigned char*>(img->pixels),
									img->w,
									img->h,
									LCT_RGB);
	if (error != 0)
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

	const BattleUnit* const bu = _battleSave->getSelectedUnit();
	if (bu == NULL) // no unit selected
		return;

	bool
		debug = _battleSave->getDebugMode(),
		black = false;
	int test;
	double
		ang_x,
		ang_y,
		dist = 0.,
		dir = static_cast<double>(bu->getDirection() + 4) / 4. * M_PI;

	std::vector<unsigned char> image;
	std::vector<Position> _trajectory;

	Position
		originVoxel = getBattleGame()->getTileEngine()->getSightOriginVoxel(bu),
		targetVoxel,
		hitPos;
	const Tile* tile = NULL;


	image.clear();

	for (int
			y = -256 + 32;
			y < 256 + 32;
			++y)
	{
		ang_y = (static_cast<double>(y) / 640. * M_PI) + (M_PI / 2.);

		for (int
				x = -256;
				x < 256;
				++x)
		{
			ang_x = ((static_cast<double>(x) / 1024.) * M_PI) + dir;

			targetVoxel.x = originVoxel.x + (static_cast<int>(-std::sin(ang_x) * 1024 * std::sin(ang_y)));
			targetVoxel.y = originVoxel.y + (static_cast<int>( std::cos(ang_x) * 1024 * std::sin(ang_y)));
			targetVoxel.z = originVoxel.z + (static_cast<int>( std::cos(ang_y) * 1024));

			_trajectory.clear();

			black = true;

			test = _battleSave->getTileEngine()->calculateLine(
															originVoxel,
															targetVoxel,
															false,
															&_trajectory,
															bu,
															true,
															debug == false) + 1;
			if (test != 0 && test != 6)
			{
				tile = _battleSave->getTile(Position(
												_trajectory.at(0).x / 16,
												_trajectory.at(0).y / 16,
												_trajectory.at(0).z / 24));
				if (debug == true
					|| (tile->isDiscovered(0) && test == 2)
					|| (tile->isDiscovered(1) && test == 3)
					|| (tile->isDiscovered(2) && (test == 1 || test == 4))
					|| test == 5)
				{
					if (test == 5)
					{
						if (tile->getUnit() != NULL)
						{
							if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
								test = 9;
							else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
								test = 8;
						}
						else
						{
							tile = _battleSave->getTile(Position(
															_trajectory.at(0).x / 16,
															_trajectory.at(0).y / 16,
															_trajectory.at(0).z / 24 - 1));
							if (tile != NULL
								&& tile->getUnit() != NULL)
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
					dist = std::sqrt(static_cast<double>(
								  (hitPos.x - originVoxel.x) * (hitPos.x - originVoxel.x)
								+ (hitPos.y - originVoxel.y) * (hitPos.y - originVoxel.y)
								+ (hitPos.z - originVoxel.z) * (hitPos.z - originVoxel.z)));

					black = false;
				}
			}

			if (black == true)
				dist = 0.;
			else
			{
				if (dist > 1000.) dist = 1000.;
				if (dist < 1.) dist = 1.;

				dist = (1000. - ((std::log(dist) * 140.)) / 700.);

				if (hitPos.x %16 == 15)
					dist *= 0.9;

				if (hitPos.y %16 == 15)
					dist *= 0.9;

				if (hitPos.z %24 == 23)
					dist *= 0.9;

				if (dist > 1.) dist = 1.;

				if (tile != NULL)
					dist *= (16. - static_cast<double>(tile->getShade())) / 16.;
			}

//			image.push_back((int)((float)(pal[test * 3 + 0]) * dist));
//			image.push_back((int)((float)(pal[test * 3 + 1]) * dist));
//			image.push_back((int)((float)(pal[test * 3 + 2]) * dist));
			image.push_back(static_cast<unsigned char>(static_cast<double>(pal[test * 3 + 0]) * dist));
			image.push_back(static_cast<unsigned char>(static_cast<double>(pal[test * 3 + 1]) * dist));
			image.push_back(static_cast<unsigned char>(static_cast<double>(pal[test * 3 + 2]) * dist));
		}
	}


	std::ostringstream osts;

	int i = 0;
	do
	{
		osts.str("");
		osts << Options::getUserFolder() << "fpslook" << std::setfill('0') << std::setw(3) << i << ".png";

		++i;
	}
	while (CrossPlatform::fileExists(osts.str()) == true
		&& i < 999);


	unsigned error = lodepng::encode(
								osts.str(),
								image,
								512,512,
								LCT_RGB);
	if (error != 0)
		Log(LOG_ERROR) << "bs::saveVoxelView() Saving to PNG failed: " << lodepng_error_text(error);
#ifdef _WIN32
	else
	{
		std::wstring wst = Language::cpToWstr("\"C:\\Program Files\\IrfanView\\i_view32.exe\" \"" + osts.str() + "\"");

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		if (CreateProcess(									//BOOL WINAPI CreateProcess
						NULL,								//  _In_opt_     LPCTSTR lpApplicationName,
						const_cast<LPWSTR>(wst.c_str()),	//  _Inout_opt_  LPTSTR lpCommandLine,
						NULL,								//  _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
						NULL,								//  _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
						FALSE,								//  _In_         BOOL bInheritHandles,
						0,									//  _In_         DWORD dwCreationFlags,
						NULL,								//  _In_opt_     LPVOID lpEnvironment,
						NULL,								//  _In_opt_     LPCTSTR lpCurrentDirectory,
						&si,								//  _In_         LPSTARTUPINFO lpStartupInfo,
						&pi) == false)						//  _Out_        LPPROCESS_INFORMATION lpProcessInformation
		{
			Log(LOG_ERROR) << "bs::saveVoxelView() CreateProcess() failed";
		}
		else
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
#endif

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

	for (int
			z = 0;
			z < _battleSave->getMapSizeZ() * 12;
			++z)
	{
		image.clear();

		for (int
				y = 0;
				y < _battleSave->getMapSizeY() * 16;
				++y)
		{
			for (int
					x = 0;
					x < _battleSave->getMapSizeX() * 16;
					++x)
			{
				int test = _battleSave->getTileEngine()->voxelCheck(
																Position(x,y,z * 2),
																0,0) + 1;
				float dist = 1.f;

				if (x %16 == 15)
					dist *= 0.9f;

				if (y %16 == 15)
					dist *= 0.9f;

				if (test == VOXEL_OUTOFBOUNDS)
				{
					tile = _battleSave->getTile(Position(
														x / 16,
														y / 16,
														z / 12));
					if (tile->getUnit() != NULL)
					{
						if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
							test = 9;
						else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
							test = 8;
					}
					else
					{
						tile = _battleSave->getTile(Position(
															x / 16,
															y / 16,
															z / 12 - 1));
						if (tile != NULL
							&& tile->getUnit() != NULL)
						{
							if (tile->getUnit()->getFaction() == FACTION_NEUTRAL)
								test = 9;
							else if (tile->getUnit()->getFaction() == FACTION_PLAYER)
								test = 8;
						}
					}
				}

//				image.push_back(static_cast<int>(static_cast<float>(pal[test * 3 + 0]) * dist));
//				image.push_back(static_cast<int>(static_cast<float>(pal[test * 3 + 1]) * dist));
//				image.push_back(static_cast<int>(static_cast<float>(pal[test * 3 + 2]) * dist));
				image.push_back(static_cast<unsigned char>(static_cast<double>(pal[test * 3 + 0]) * dist));
				image.push_back(static_cast<unsigned char>(static_cast<double>(pal[test * 3 + 1]) * dist));
				image.push_back(static_cast<unsigned char>(static_cast<double>(pal[test * 3 + 2]) * dist));
			}
		}

		ss.str("");
		ss << Options::getUserFolder() << "voxel" << std::setfill('0') << std::setw(2) << z << ".png";

		unsigned error = lodepng::encode(
									ss.str(),
									image,
									_battleSave->getMapSizeX() * 16,
									_battleSave->getMapSizeY() * 16,
									LCT_RGB);

		if (error != 0)
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}
}

}
