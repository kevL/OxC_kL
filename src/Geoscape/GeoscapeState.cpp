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

#include "GeoscapeState.h"

//#include <algorithm>
//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <functional>
//#include <iomanip>
//#include <sstream>
//#include "../fmath.h"

#include "AlienBaseState.h"
#include "BaseDefenseState.h"
#include "BaseDestroyedState.h"
#include "ConfirmLandingState.h"
#include "CraftErrorState.h"
#include "CraftPatrolState.h"
#include "DefeatState.h"
#include "DogfightState.h"
#include "FundingState.h"
#include "GeoscapeCraftState.h"
#include "Globe.h"
#include "GraphsState.h"
#include "InterceptState.h"
#include "ItemsArrivingState.h"
#include "LowFuelState.h"
#include "MissionDetectedState.h"
#include "MonthlyReportState.h"
#include "MultipleTargetsState.h"
#include "NewPossibleManufactureState.h"
#include "NewPossibleResearchState.h"
#include "ProductionCompleteState.h"
#include "ResearchCompleteState.h"
#include "ResearchRequiredState.h"
#include "SoldierDiedState.h"
#include "UfoDetectedState.h"
#include "UfoLostState.h"

#include "../Basescape/BasescapeState.h"
#include "../Basescape/SellState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Action.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/ListSaveState.h"
#include "../Menu/LoadGameState.h"
#include "../Menu/PauseState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleMissionScript.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/AlienStrategy.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Production.h"
#include "../Savegame/Region.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

size_t kL_curBase = 0;
bool
	kL_geoMusicPlaying		= false,
	kL_geoMusicReturnState	= false;

const double
	earthRadius					= 3440., //.0647948164,			// nautical miles.
	unitToRads					= (1. / 60.) * (M_PI / 180.),	// converts a minute of arc to rads
	greatCircleConversionFactor	= earthRadius * unitToRads;		// converts 'flat' distance to greatCircle distance.


// UFO blobs graphics ...
const int GeoscapeState::_ufoBlobs[8][13][13] =
{
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0 - STR_VERY_SMALL
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 3, 5, 3, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 1 - STR_SMALL
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 4, 5, 4, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 2 - STR_MEDIUM_UC
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 5, 5, 5, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 3 - STR_LARGE
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // 4 - STR_VERY_LARGE
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}, // 5 - STR_HUGE
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}
	},
	{
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}, // 6 - STR_VERY_HUGE :p
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}
	},
	{
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}, // 7 - STR_ENORMOUS
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}
	}
};


// struct definitions used when enqueuing notification events
struct ProductionCompleteInfo
{
	bool gotoBaseBtn;
	std::wstring item;

	Base* base;

	ProductionProgress endType;

	/// cTor.
	ProductionCompleteInfo(
			Base* a_base,
			const std::wstring& a_item,
			bool a_gotoBaseBtn,
			ProductionProgress a_endType)
		:
			base(a_base),
			item(a_item),
			gotoBaseBtn(a_gotoBaseBtn),
			endType(a_endType)
	{}
};

struct NewPossibleResearchInfo
{
	bool showResearchButton;
	std::vector<const RuleResearch*> newPossibleResearch;

	Base* base;

	/// cTor.
	NewPossibleResearchInfo(
			Base* a_base,
			const std::vector<const RuleResearch*>& a_newPossibleResearch,
			bool a_showResearchButton)
		:
			base(a_base),
			newPossibleResearch(a_newPossibleResearch),
			showResearchButton(a_showResearchButton)
	{}
};

struct NewPossibleManufactureInfo
{
	bool showManufactureButton;
	std::vector<const RuleManufacture*> newPossibleManufacture;

	Base* base;

	/// cTor.
	NewPossibleManufactureInfo(
			Base* a_base,
			const std::vector<const RuleManufacture*>& a_newPossibleManufacture,
			bool a_showManufactureButton)
		:
			base(a_base),
			newPossibleManufacture(a_newPossibleManufacture),
			showManufactureButton(a_showManufactureButton)
	{}
};


/**
 * Initializes all the elements in the Geoscape screen.
 */
GeoscapeState::GeoscapeState()
	:
		_gameSave(_game->getSavedGame()),
		_rules(_game->getRuleset()),
		_pause(false),
		_pauseHard(false),
		_dfZoomInDone(false),
		_dfZoomOutDone(false),
		_dfZoomOut(true),
		_dfCenterCurrentCoords(false),
		_dfCCC_lon(0.),
		_dfCCC_lat(0.),
		_dfMinimized(0),
		_day(-1),
		_month(-1),
		_year(-1),
		_windowPops(0),
		_delayMusicDfCheck(0),
		_timeCache(0)
{
	const int
		screenWidth		= Options::baseXGeoscape,
		screenHeight	= Options::baseYGeoscape,
		halfHeight		= screenHeight / 2;

	_srfSpace	= new Surface(
						screenWidth,
						screenHeight);
	_globe		= new Globe(
						_game,
						(screenWidth - 64) / 2,
						halfHeight,
						screenWidth - 64,
						screenHeight);
	_sideBlack	= new Surface(
						64,
						screenHeight,
						screenWidth - 64,
						0);

	// revert to ImageButtons. Stock build uses TextButton's
	_btnIntercept	= new ImageButton(63, 11, screenWidth - 63, halfHeight - 100);
	_btnBases		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 88);
	_btnGraphs		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 76);
	_btnUfopaedia	= new ImageButton(63, 11, screenWidth - 63, halfHeight - 64);
	_btnOptions		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 52);
	_btnFunding		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 40);
//	_btnOptions		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 100); // change the GeoGraphic first .... ->
//	_btnUfopaedia	= new ImageButton(63, 11, screenWidth - 63, halfHeight - 88);
//	_btnFunding		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 76);
//	_btnGraphs		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 64);
//	_btnIntercept	= new ImageButton(63, 11, screenWidth - 63, halfHeight - 52);
//	_btnBases		= new ImageButton(63, 11, screenWidth - 63, halfHeight - 40);

	_btn5Secs		= new ImageButton(31, 13, screenWidth - 63, halfHeight + 12);
	_btn1Min		= new ImageButton(31, 13, screenWidth - 31, halfHeight + 12);
	_btn5Mins		= new ImageButton(31, 13, screenWidth - 63, halfHeight + 26);
	_btn30Mins		= new ImageButton(31, 13, screenWidth - 31, halfHeight + 26);
	_btn1Hour		= new ImageButton(31, 13, screenWidth - 63, halfHeight + 40);
	_btn1Day		= new ImageButton(31, 13, screenWidth - 31, halfHeight + 40);
//	_btn5Secs		= new ImageButton(31, 13, screenWidth - 63, halfHeight + 12); // change the GeoGraphic first .... ->
//	_btn1Min		= new ImageButton(31, 13, screenWidth - 63, halfHeight + 26);
//	_btn5Mins		= new ImageButton(31, 13, screenWidth - 63, halfHeight + 40);
//	_btn30Mins		= new ImageButton(31, 13, screenWidth - 31, halfHeight + 12);
//	_btn1Hour		= new ImageButton(31, 13, screenWidth - 31, halfHeight + 26);
//	_btn1Day		= new ImageButton(31, 13, screenWidth - 31, halfHeight + 40);

	// The old Globe rotate buttons have become the Detail & Radar toggles.
	_btnDetail		= new ImageButton(63, 46, screenWidth - 63, halfHeight + 54);
/*	_btnRotateLeft	= new InteractiveSurface(12, 10, screenWidth-61, screenHeight/2+76);
	_btnRotateRight	= new InteractiveSurface(12, 10, screenWidth-37, screenHeight/2+76);
	_btnRotateUp	= new InteractiveSurface(13, 12, screenWidth-49, screenHeight/2+62);
	_btnRotateDown	= new InteractiveSurface(13, 12, screenWidth-49, screenHeight/2+87);
	_btnZoomIn		= new InteractiveSurface(23, 23, screenWidth-25, screenHeight/2+56);
	_btnZoomOut		= new InteractiveSurface(13, 17, screenWidth-20, screenHeight/2+82); */

	const int height = (screenHeight - Screen::ORIGINAL_HEIGHT) / 2 - 12;
	_sideTop	= new TextButton(
							64,
							height,
							screenWidth - 64,
							(halfHeight) - (Screen::ORIGINAL_HEIGHT / 2) - (height + 12));
	_sideBottom	= new TextButton(
							64,
							height,
							screenWidth - 64,
							(halfHeight) + (Screen::ORIGINAL_HEIGHT / 2) + 12);

	_ufoDetected = new Text(17, 17, _sideBottom->getX() + 6, _sideBottom->getY() + 4);

	std::fill_n(
			_hostileUfos,
			UFO_HOTBLOBS,
			static_cast<Ufo*>(NULL));

	int
		x,y,
		offset_x,
		offset_y;
	for (size_t
			i = 0;
			i != UFO_HOTBLOBS;
			++i)
	{
		offset_x = ((i % 4) * 13); // 4 UFOs per row on top sidebar
		offset_y = ((i / 4) * 13); // 4 rows going up
		x = _sideTop->getX() + offset_x + 3;
		y = _sideTop->getY() + height - offset_y - 16;

		_isfUfoBlobs[i] = new InteractiveSurface(13, 13, x, y);
		_numUfoBlobs[i] = new NumberText(11, 5, x + 2, y + 8);
	}

	_isfTime	= new InteractiveSurface(63, 39, screenWidth - 63, halfHeight - 28);

	_txtHour	= new Text(19, 17, screenWidth - 54, halfHeight - 22);
	_txtColon	= new Text( 5, 17, screenWidth - 35, halfHeight - 22);
	_txtMin		= new Text(19, 17, screenWidth - 30, halfHeight - 22);
	_txtSec		= new Text( 6,  9, screenWidth -  8, halfHeight - 26);

	_txtDay		= new Text(11, 9, screenWidth - 57, halfHeight - 4);
	_txtMonth	= new Text(17, 9, screenWidth - 45, halfHeight - 4);
	_txtYear	= new Text(21, 9, screenWidth - 27, halfHeight - 4);

	_txtFunds	= new Text(63, 8, screenWidth - 64, halfHeight - 110);
	_txtScore	= new Text(63, 8, screenWidth - 64, halfHeight + 102);

	_timeComp = _btn5Secs;

	_geoTimer		= new Timer(static_cast<Uint32>(FAST_GEO_INTERVAL)); // Volutar_smoothGlobe shadows.
	_dfZoomInTimer	= new Timer(static_cast<Uint32>(Options::geoClockSpeed));
	_dfZoomOutTimer	= new Timer(static_cast<Uint32>(Options::geoClockSpeed));
	_dfStartTimer	= new Timer(static_cast<Uint32>(Options::dogfightSpeed));
	_dfTimer		= new Timer(static_cast<Uint32>(Options::dogfightSpeed));

	_txtDebug = new Text(320, 27);


	setInterface("geoscape");

	add(_srfSpace);
	add(_sideBlack);
	add(_globe);

	add(_btnDetail);

	add(_btnIntercept,	"button", "geoscape");
	add(_btnBases,		"button", "geoscape");
	add(_btnGraphs,		"button", "geoscape");
	add(_btnUfopaedia,	"button", "geoscape");
	add(_btnOptions,	"button", "geoscape");
	add(_btnFunding,	"button", "geoscape");

	add(_btn5Secs,		"button", "geoscape");
	add(_btn1Min,		"button", "geoscape");
	add(_btn5Mins,		"button", "geoscape");
	add(_btn30Mins,		"button", "geoscape");
	add(_btn1Hour,		"button", "geoscape");
	add(_btn1Day,		"button", "geoscape");

/*	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut); */

	add(_sideTop,		"button", "geoscape");
	add(_sideBottom,	"button", "geoscape");
	add(_ufoDetected);

	for (size_t
			i = 0;
			i != UFO_HOTBLOBS;
			++i)
	{
		add(_isfUfoBlobs[i]);
		_isfUfoBlobs[i]->setVisible(false);
//		_isfUfoBlobs[i]->onMousePress(
//						(ActionHandler)& GeoscapeState::btnUfoBlobPress,
//						SDL_BUTTON_LEFT);
		_isfUfoBlobs[i]->onMousePress((ActionHandler)& GeoscapeState::btnUfoBlobPress);

		add(_numUfoBlobs[i]);
		_numUfoBlobs[i]->setVisible(false);
		_numUfoBlobs[i]->setColor(11); // dk.yellow

//		_isfUfoBlobs[i]->onKeyboardPress((ActionHandler)& GeoscapeState::btnUfoBlobPress, buttons[i]);
	}

	add(_isfTime);
	add(_txtDebug,	"text", "geoscape");
	add(_txtFunds,	"text", "geoscape");
	add(_txtScore,	"text", "geoscape");
	add(_txtHour);
	add(_txtColon);
	add(_txtMin);
	add(_txtSec);
	add(_txtDay);
	add(_txtMonth);
	add(_txtYear);

	_game->getResourcePack()->getSurface("Cygnus_BG")->blit(_srfSpace);

	_sideBlack->drawRect(
					0,0,
					static_cast<Sint16>(_sideBlack->getWidth()),
					static_cast<Sint16>(_sideBlack->getHeight()),
					15); // black

/*	_btnIntercept->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
//	_btnIntercept->setColor(Palette::blockOffset(15)+6);
//	_btnIntercept->setTextColor(Palette::blockOffset(15)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& GeoscapeState::btnInterceptClick);
	_btnIntercept->onKeyboardPress((ActionHandler)& GeoscapeState::btnInterceptClick, Options::keyGeoIntercept);
	_btnIntercept->setGeoscapeButton(true);

	_btnBases->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
//	_btnBases->setColor(Palette::blockOffset(15)+6);
//	_btnBases->setTextColor(Palette::blockOffset(15)+5);
	_btnBases->setText(tr("STR_BASES"));
	_btnBases->onMouseClick((ActionHandler)& GeoscapeState::btnBasesClick);
	_btnBases->onKeyboardPress((ActionHandler)& GeoscapeState::btnBasesClick, Options::keyGeoBases);
	_btnBases->setGeoscapeButton(true);

	_btnGraphs->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
//	_btnGraphs->setColor(Palette::blockOffset(15)+6);
//	_btnGraphs->setTextColor(Palette::blockOffset(15)+5);
	_btnGraphs->setText(tr("STR_GRAPHS"));
	_btnGraphs->onMouseClick((ActionHandler)& GeoscapeState::btnGraphsClick);
	_btnGraphs->onKeyboardPress((ActionHandler)& GeoscapeState::btnGraphsClick, Options::keyGeoGraphs);
	_btnGraphs->setGeoscapeButton(true);

	_btnUfopaedia->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
//	_btnUfopaedia->setColor(Palette::blockOffset(15)+6);
//	_btnUfopaedia->setTextColor(Palette::blockOffset(15)+5);
	_btnUfopaedia->setText(tr("STR_UFOPAEDIA_UC"));
	_btnUfopaedia->onMouseClick((ActionHandler)& GeoscapeState::btnUfopaediaClick);
	_btnUfopaedia->onKeyboardPress((ActionHandler)& GeoscapeState::btnUfopaediaClick, Options::keyGeoUfopedia);
	_btnUfopaedia->setGeoscapeButton(true);

	_btnOptions->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
//	_btnOptions->setColor(Palette::blockOffset(15)+6);
//	_btnOptions->setTextColor(Palette::blockOffset(15)+5);
	_btnOptions->setText(tr("STR_OPTIONS_UC"));
	_btnOptions->onMouseClick((ActionHandler)& GeoscapeState::btnOptionsClick);
	_btnOptions->onKeyboardPress((ActionHandler)& GeoscapeState::btnOptionsClick, Options::keyGeoOptions);
	_btnOptions->setGeoscapeButton(true);

	_btnFunding->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
//	_btnFunding->setColor(Palette::blockOffset(15)+6);
//	_btnFunding->setTextColor(Palette::blockOffset(15)+5);
	_btnFunding->setText(tr("STR_FUNDING_UC"));
	_btnFunding->onMouseClick((ActionHandler)& GeoscapeState::btnFundingClick);
	_btnFunding->onKeyboardPress((ActionHandler)& GeoscapeState::btnFundingClick, Options::keyGeoFunding);
	_btnFunding->setGeoscapeButton(true);

	_btn5Secs->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn5Secs->setBig();
//	_btn5Secs->setColor(Palette::blockOffset(15)+6);
//	_btn5Secs->setTextColor(Palette::blockOffset(15)+5);
	_btn5Secs->setText(tr("STR_5_SECONDS"));
	_btn5Secs->setGroup(&_timeComp);
	_btn5Secs->onKeyboardPress((ActionHandler)& GeoscapeState::keyTimeCompressionPress, Options::keyGeoSpeed1);
	_btn5Secs->setGeoscapeButton(true);

	_btn1Min->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Min->setBig();
//	_btn1Min->setColor(Palette::blockOffset(15)+6);
//	_btn1Min->setTextColor(Palette::blockOffset(15)+5);
	_btn1Min->setText(tr("STR_1_MINUTE"));
	_btn1Min->setGroup(&_timeComp);
	_btn1Min->onKeyboardPress((ActionHandler)& GeoscapeState::keyTimeCompressionPress, Options::keyGeoSpeed2);
	_btn1Min->setGeoscapeButton(true);

	_btn5Mins->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn5Mins->setBig();
//	_btn5Mins->setColor(Palette::blockOffset(15)+6);
//	_btn5Mins->setTextColor(Palette::blockOffset(15)+5);
	_btn5Mins->setText(tr("STR_5_MINUTES"));
	_btn5Mins->setGroup(&_timeComp);
	_btn5Mins->onKeyboardPress((ActionHandler)& GeoscapeState::keyTimeCompressionPress, Options::keyGeoSpeed3);
	_btn5Mins->setGeoscapeButton(true);

	_btn30Mins->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn30Mins->setBig();
//	_btn30Mins->setColor(Palette::blockOffset(15)+6);
//	_btn30Mins->setTextColor(Palette::blockOffset(15)+5);
	_btn30Mins->setText(tr("STR_30_MINUTES"));
	_btn30Mins->setGroup(&_timeComp);
	_btn30Mins->onKeyboardPress((ActionHandler)& GeoscapeState::keyTimeCompressionPress, Options::keyGeoSpeed4);
	_btn30Mins->setGeoscapeButton(true);

	_btn1Hour->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Hour->setBig();
//	_btn1Hour->setColor(Palette::blockOffset(15)+6);
//	_btn1Hour->setTextColor(Palette::blockOffset(15)+5);
	_btn1Hour->setText(tr("STR_1_HOUR"));
	_btn1Hour->setGroup(&_timeComp);
	_btn1Hour->onKeyboardPress((ActionHandler)& GeoscapeState::keyTimeCompressionPress, Options::keyGeoSpeed5);
	_btn1Hour->setGeoscapeButton(true);

	_btn1Day->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Day->setBig();
//	_btn1Day->setColor(Palette::blockOffset(15)+6);
//	_btn1Day->setTextColor(Palette::blockOffset(15)+5);
	_btn1Day->setText(tr("STR_1_DAY"));
	_btn1Day->setGroup(&_timeComp);
	_btn1Day->onKeyboardPress((ActionHandler)& GeoscapeState::keyTimeCompressionPress, Options::keyGeoSpeed6);
	_btn1Day->setGeoscapeButton(true); */

	// revert to ImageButtons.
	Surface* const geobord = _game->getResourcePack()->getSurface("GEOBORD.SCR");
	geobord->setX(screenWidth - geobord->getWidth());
	geobord->setY((screenHeight - geobord->getHeight()) / 2);

	_btnIntercept->copy(geobord);
	_btnIntercept->onMouseClick((ActionHandler)& GeoscapeState::btnInterceptClick);
	_btnIntercept->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnInterceptClick,
					Options::keyGeoIntercept);

	_btnBases->copy(geobord);
	_btnBases->onMouseClick((ActionHandler)& GeoscapeState::btnBasesClick);
	_btnBases->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnBasesClick,
					Options::keyGeoBases);

	_btnGraphs->copy(geobord);
	_btnGraphs->onMouseClick((ActionHandler)& GeoscapeState::btnGraphsClick);
	_btnGraphs->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnGraphsClick,
					Options::keyGeoGraphs);

	_btnUfopaedia->copy(geobord);
	_btnUfopaedia->onMouseClick((ActionHandler)& GeoscapeState::btnUfopaediaClick);
	_btnUfopaedia->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnUfopaediaClick,
					Options::keyGeoUfopedia);

	_btnOptions->copy(geobord);
	_btnOptions->onMouseClick((ActionHandler)& GeoscapeState::btnOptionsClick);	// why the f are these buttons calling resetTimeCacheClick() AND EVEN GETTING
	_btnOptions->onKeyboardPress(												// THE SENDER RIGHT when a true click on a time-compression btn does not.
					(ActionHandler)& GeoscapeState::btnOptionsClick,			// Note that happens only if the 'resetTimeCache' is on a Press, not a Click! holy diana
					Options::keyGeoOptions); // Escape key.
//	_btnOptions->onKeyboardPress( // note: These interfere w/ opening minimized Dogfights.
//					(ActionHandler)& GeoscapeState::btnOptionsClick,
//					Options::keyOk);
//	_btnOptions->onKeyboardPress(
//					(ActionHandler)& GeoscapeState::btnOptionsClick,
//					Options::keyOkKeypad);

	_btnFunding->copy(geobord);
	_btnFunding->onMouseClick((ActionHandler)& GeoscapeState::btnFundingClick);
	_btnFunding->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnFundingClick,
					Options::keyGeoFunding);


	_isfTime->copy(geobord);
	_isfTime->onMouseClick(
					(ActionHandler)& GeoscapeState::btnPauseClick,
					SDL_BUTTON_LEFT);
	_isfTime->onMouseClick(
					(ActionHandler)& GeoscapeState::btnPauseClick,
					SDL_BUTTON_RIGHT);
	_isfTime->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnPauseClick,
					SDLK_SPACE);

	_btn5Secs->copy(geobord);
	_btn5Secs->setGroup(&_timeComp);
	_btn5Secs->onKeyboardPress(
					(ActionHandler)& GeoscapeState::keyTimeCompressionPress,
					Options::keyGeoSpeed1);
	_btn5Secs->onMouseClick((ActionHandler)& GeoscapeState::resetTimeCacheClick);

	_btn1Min->copy(geobord);
	_btn1Min->setGroup(&_timeComp);
	_btn1Min->onKeyboardPress(
					(ActionHandler)& GeoscapeState::keyTimeCompressionPress,
					Options::keyGeoSpeed2);
	_btn1Min->onMouseClick((ActionHandler)& GeoscapeState::resetTimeCacheClick);

	_btn5Mins->copy(geobord);
	_btn5Mins->setGroup(&_timeComp);
	_btn5Mins->onKeyboardPress(
					(ActionHandler)& GeoscapeState::keyTimeCompressionPress,
					Options::keyGeoSpeed3);
	_btn5Mins->onMouseClick((ActionHandler)& GeoscapeState::resetTimeCacheClick);

	_btn30Mins->copy(geobord);
	_btn30Mins->setGroup(&_timeComp);
	_btn30Mins->onKeyboardPress(
					(ActionHandler)& GeoscapeState::keyTimeCompressionPress,
					Options::keyGeoSpeed4);
	_btn30Mins->onMouseClick((ActionHandler)& GeoscapeState::resetTimeCacheClick);

	_btn1Hour->copy(geobord);
	_btn1Hour->setGroup(&_timeComp);
	_btn1Hour->onKeyboardPress(
					(ActionHandler)& GeoscapeState::keyTimeCompressionPress,
					Options::keyGeoSpeed5);
	_btn1Hour->onMouseClick((ActionHandler)& GeoscapeState::resetTimeCacheClick);

	_btn1Day->copy(geobord);
	_btn1Day->setGroup(&_timeComp);
	_btn1Day->onKeyboardPress(
					(ActionHandler)& GeoscapeState::keyTimeCompressionPress,
					Options::keyGeoSpeed6);
	_btn1Day->onMouseClick((ActionHandler)& GeoscapeState::resetTimeCacheClick);


	_btnDetail->copy(geobord);
	_btnDetail->setColor(PURPLE_D);
	_btnDetail->onMousePress((ActionHandler)& GeoscapeState::btnDetailPress);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateLeftPress,
					Options::keyGeoLeft);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateLeftPress,
					SDLK_KP4);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLonStop,
					Options::keyGeoLeft);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLonStop,
					SDLK_KP4);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateRightPress,
					Options::keyGeoRight);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateRightPress,
					SDLK_KP6);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLonStop,
					Options::keyGeoRight);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLonStop,
					SDLK_KP6);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateUpPress,
					Options::keyGeoUp);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateUpPress,
					SDLK_KP8);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLatStop,
					Options::keyGeoUp);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLatStop,
					SDLK_KP8);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateDownPress,
					Options::keyGeoDown);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateDownPress,
					SDLK_KP2);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLatStop,
					Options::keyGeoDown);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLatStop,
					SDLK_KP2);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateLeftUpPress,
					SDLK_KP7);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateStop,
					SDLK_KP7);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateLeftDownPress,
					SDLK_KP1);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateStop,
					SDLK_KP1);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateRightUpPress,
					SDLK_KP9);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateStop,
					SDLK_KP9);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateRightDownPress,
					SDLK_KP3);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateStop,
					SDLK_KP3);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnZoomInLeftClick,
					Options::keyGeoZoomIn);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnZoomInLeftClick,
					SDLK_KP_PLUS);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnZoomOutLeftClick,
					Options::keyGeoZoomOut);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnZoomOutLeftClick,
					SDLK_KP_MINUS);

	_ufoDetected->setColor(SLATE);
	_ufoDetected->setBig();
	_ufoDetected->setVisible(false);

/*	_btnRotateLeft->onMousePress((ActionHandler)& GeoscapeState::btnRotateLeftPress);
	_btnRotateLeft->onMouseRelease((ActionHandler)& GeoscapeState::btnRotateLeftRelease);
	_btnRotateLeft->onKeyboardPress((ActionHandler)&GeoscapeState::btnRotateLeftPress, Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease((ActionHandler)&GeoscapeState::btnRotateLeftRelease, Options::keyGeoLeft);

	_btnRotateRight->onMousePress((ActionHandler)& GeoscapeState::btnRotateRightPress);
	_btnRotateRight->onMouseRelease((ActionHandler)& GeoscapeState::btnRotateRightRelease);
	_btnRotateRight->onKeyboardPress((ActionHandler)&GeoscapeState::btnRotateRightPress, Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease((ActionHandler)&GeoscapeState::btnRotateRightRelease, Options::keyGeoRight);

	_btnRotateUp->onMousePress((ActionHandler)& GeoscapeState::btnRotateUpPress);
	_btnRotateUp->onMouseRelease((ActionHandler)& GeoscapeState::btnRotateUpRelease);
	_btnRotateUp->onKeyboardPress((ActionHandler)&GeoscapeState::btnRotateUpPress, Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease((ActionHandler)&GeoscapeState::btnRotateUpRelease, Options::keyGeoUp);

	_btnRotateDown->onMousePress((ActionHandler)& GeoscapeState::btnRotateDownPress);
	_btnRotateDown->onMouseRelease((ActionHandler)& GeoscapeState::btnRotateDownRelease);
	_btnRotateDown->onKeyboardPress((ActionHandler)&GeoscapeState::btnRotateDownPress, Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease((ActionHandler)&GeoscapeState::btnRotateDownRelease, Options::keyGeoDown);

	_btnZoomIn->onMouseClick((ActionHandler)& GeoscapeState::btnZoomInLeftClick, SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick((ActionHandler)& GeoscapeState::btnZoomInRightClick, SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress((ActionHandler)&GeoscapeState::btnZoomInLeftClick, Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick((ActionHandler)& GeoscapeState::btnZoomOutLeftClick, SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick((ActionHandler)& GeoscapeState::btnZoomOutRightClick, SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress((ActionHandler)&GeoscapeState::btnZoomOutLeftClick, Options::keyGeoZoomOut); */

	if (Options::showFundsOnGeoscape == true)
	{
		_txtFunds->setAlign(ALIGN_CENTER);
		_txtScore->setAlign(ALIGN_CENTER);
	}
	else
	{
		_txtFunds->setVisible(false);
		_txtScore->setVisible(false);
	}

	_txtHour->setColor(GREEN_SEA);
	_txtHour->setAlign(ALIGN_RIGHT);
	_txtHour->setBig();

	_txtColon->setColor(GREEN_SEA);
	_txtColon->setText(L":");
	_txtColon->setBig();

	_txtMin->setColor(GREEN_SEA);
	_txtMin->setBig();

	_txtSec->setColor(GREEN_SEA);
	_txtSec->setText(L".");

	_txtDay->setColor(GREEN_SEA);
	_txtDay->setAlign(ALIGN_RIGHT);

	_txtMonth->setColor(GREEN_SEA);
	_txtMonth->setAlign(ALIGN_CENTER);

	_txtYear->setColor(GREEN_SEA);

	_geoTimer->onTimer((StateHandler)& GeoscapeState::timeAdvance);
	_geoTimer->start();

	_dfZoomInTimer->onTimer((StateHandler)& GeoscapeState::dfZoomIn);
	_dfZoomOutTimer->onTimer((StateHandler)& GeoscapeState::dfZoomOut);
	_dfStartTimer->onTimer((StateHandler)& GeoscapeState::startDogfight);
	_dfTimer->onTimer((StateHandler)& GeoscapeState::thinkDogfights);

//	updateTimeDisplay();

	kL_geoMusicPlaying =
	kL_geoMusicReturnState = false;
}

/**
 * Deletes timers and clears dogfights.
 */
GeoscapeState::~GeoscapeState()
{
	delete _geoTimer;
	delete _dfZoomInTimer;
	delete _dfZoomOutTimer;
	delete _dfStartTimer;
	delete _dfTimer;

	std::list<DogfightState*>::const_iterator pDf = _dogfights.begin();
	for (
			;
			pDf != _dogfights.end();
			)
	{
        (*pDf)->clearCraft();	// kL_note->
        (*pDf)->clearUfo();		// I don't have a clue why this started to CTD.

		delete *pDf;
		pDf = _dogfights.erase(pDf);
	}

	for (
			pDf = _dogfightsToStart.begin();
			pDf != _dogfightsToStart.end();
			)
	{
        (*pDf)->clearCraft();	// kL_note->
        (*pDf)->clearUfo();		// Not sure if these need to be here but see above^.

		delete *pDf;
		pDf = _dogfightsToStart.erase(pDf);
	}
}

/**
 * Handle blitting of Geoscape and Dogfights.
 */
void GeoscapeState::blit()
{
	State::blit();

	for (std::list<DogfightState*>::const_iterator
			i = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		(*i)->blit();
	}
}

/**
 * Handle key shortcuts.
 * @param action - pointer to an Action
 */
void GeoscapeState::handle(Action* action)
{
	if (_dogfights.size() == _dfMinimized)
		State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		if (Options::debug == true)
		{
			if ((SDL_GetModState() & KMOD_CTRL) != 0)
			{
				if (action->getDetails()->key.keysym.sym == SDLK_d)	// "ctrl-d" - enable debug mode
				{
					_gameSave->toggleDebugMode();
					if (_gameSave->getDebugMode() == true)
					{
						_debug = "DEBUG MODE : ";

						if (_globe->getDebugType() == 0)
							_debug += "country : ";
						else if (_globe->getDebugType() == 1)
							_debug += "region : ";
						else if (_globe->getDebugType() == 2)
							_debug += "missionZones : ";
					}
					else
					{
						_txtDebug->setText(L"");
						_debug = "";
					}
				}
				else if (action->getDetails()->key.keysym.sym == SDLK_c	// "ctrl-c" - cycle areas
					&& _gameSave->getDebugMode() == true)				// ctrl+c is also handled in Game::run() where the 'cycle' is determined.
				{
					_txtDebug->setText(L"");
					_debug = "DEBUG MODE : ";

					if (_globe->getDebugType() == 0)
						_debug += "country : ";
					else if (_globe->getDebugType() == 1)
						_debug += "region : ";
					else if (_globe->getDebugType() == 2)
						_debug += "missionZones : ";
				}
			}
		}
		else if (_gameSave->isIronman() == false) // quick save and quick load
		{
			if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)
				popup(new SaveGameState(
									OPT_GEOSCAPE,
									SAVE_QUICK,
									_palette));
			else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)
				popup(new LoadGameState(
									OPT_GEOSCAPE,
									SAVE_QUICK,
									_palette));
		}
	}

	if (_dogfights.empty() == false)
	{
		for (std::list<DogfightState*>::const_iterator
				i = _dogfights.begin();
				i != _dogfights.end();
				++i)
		{
			(*i)->handle(action);
		}

		_dfMinimized = getMinimizedDfCount();
	}
}

/**
 * Updates the timer display and resets the palette and checks for/ plays
 * geoscape music since these are bound to change on other screens.
 * @note Also sets up funds and missions at game start.
 */
void GeoscapeState::init()
{
	State::init();

	_timeCache = 0;
	updateTimeDisplay();

	_globe->onMouseClick((ActionHandler)& GeoscapeState::globeClick);
	_globe->onMouseOver(0);
//	_globe->rotateStop();
	_globe->setFocus(true);
	_globe->draw();

	if (_gameSave->isIronman() == true
		&& _gameSave->getName().empty() == false)
	{
		popup(new ListSaveState(OPT_GEOSCAPE));
	}

	if (kL_geoMusicPlaying == true)
	{
		std::string trackType;
		if (_dogfights.empty() == true
			&& _dfStartTimer->isRunning() == false)
		{
			trackType = OpenXcom::res_MUSIC_GEO_GLOBE;
		}
		else
			trackType = OpenXcom::res_MUSIC_GEO_INTERCEPT;

		if (_game->getResourcePack()->isMusicPlaying(trackType) == false)
		{
			_game->getResourcePack()->fadeMusic(_game, 425);
			_game->getResourcePack()->playMusic(trackType);
		}
	}

	_globe->unsetNewBaseHover();

	if (_gameSave->getMonthsPassed() == -1							// run once
		&& _gameSave->getBases()->empty() == false					// as long as there's a base
		&& _gameSave->getBases()->front()->getBasePlaced() == true)	// THIS prevents missions running prior to the first base being placed.
	{
		_gameSave->addMonth();
		determineAlienMissions();
		_gameSave->setFunds(_gameSave->getFunds() - static_cast<int64_t>(_gameSave->getBases()->front()->getMonthlyMaintenace()));
	}
}

/**
 * Runs the game timer and handles popups.
 */
void GeoscapeState::think()
{
	State::think();

	_dfZoomInTimer->think(this, NULL);
	_dfZoomOutTimer->think(this, NULL);
	_dfStartTimer->think(this, NULL);


	if (Options::debug == true
		&& _game->getSavedGame()->getDebugArgDone() == true // ie. do not write info until Globe actually sets it.
		&& _debug.compare(0,5, "DEBUG") == 0)
	{
		const std::string stDebug = _debug + _game->getSavedGame()->getDebugArg();
		_txtDebug->setText(Language::cpToWstr(stDebug));
	}

	if (_popups.empty() == true
		&& _dogfights.empty() == true
		&& (_dfZoomInTimer->isRunning() == false
			|| _dfZoomInDone == true)
		&& (_dfZoomOutTimer->isRunning() == false
			|| _dfZoomOutDone == true))
	{
		_geoTimer->think(this, NULL); // Handle timers
	}
	else
	{
		if (_dogfights.empty() == false
			|| _dfMinimized != 0)
		{
			if (_dogfights.size() == _dfMinimized) // if all dogfights are minimized rotate the globe, etc.
			{
				_pause = false;
				_geoTimer->think(this, NULL);
			}
			_dfTimer->think(this, NULL);
		}

		if (_popups.empty() == false) // Handle popups
		{
//			_globe->rotateStop();
			_game->pushState(_popups.front());
			_popups.erase(_popups.begin());
		}
	}
}

/**
 * Draws the UFO indicators for known UFOs.
 */
void GeoscapeState::drawUfoBlobs()
{
	for (size_t
			i = 0;
			i != UFO_HOTBLOBS;
			++i)
	{
		_isfUfoBlobs[i]->setVisible(false);
		_numUfoBlobs[i]->setVisible(false);
		_hostileUfos[i] = NULL;
	}

	size_t
		j = 0,
		ufoSize;
	Uint8
		color,
		colorBasic;

	for (std::vector<Ufo*>::const_iterator
			i = _gameSave->getUfos()->begin();
			i != _gameSave->getUfos()->end()
				&& j != UFO_HOTBLOBS;
			++i)
	{
		if ((*i)->getDetected() == true)
		{
			_isfUfoBlobs[j]->setVisible();
			_numUfoBlobs[j]->setVisible();
			_numUfoBlobs[j]->setValue(static_cast<unsigned>((*i)->getId()) % 1000); // truncate to 3 least significant digits

			_hostileUfos[j] = *i;

			ufoSize = (*i)->getRadius();

			if ((*i)->getTimerTicked() == true)
				colorBasic = RED; // TODO: blink
			else
			{
				switch ((*i)->getUfoStatus())
				{
					case Ufo::CRASHED:
						colorBasic = BROWN;
					break;
					case Ufo::LANDED:
						colorBasic = GREEN;
					break;
					default: // Ufo::FLYING
						colorBasic = SLATE_D;
				}
			}

			for (int
					y = 0;
					y != 13;
					++y)
			{
				for (int
						x = 0;
						x != 13;
						++x)
				{
					color = static_cast<Uint8>(_ufoBlobs[ufoSize]
														[static_cast<size_t>(y)]
														[static_cast<size_t>(x)]);
					if (color != 0)
						_isfUfoBlobs[j]->setPixelColor(
													x,y,
													colorBasic - color);
				}
			}

			++j;
		}
	}
}

/**
 * Updates the Geoscape clock.
 * @note Also updates the player's current score.
 */
void GeoscapeState::updateTimeDisplay()
{
	_txtFunds->setText(Text::formatFunding(_gameSave->getFunds()));

	if (_gameSave->getMonthsPassed() != -1) // update Player's current score
	{
		const size_t id = _gameSave->getFundsList().size() - 1; // use fundsList to determine which entries in other vectors to use for the current month.

		int score = _gameSave->getResearchScores().at(id);
		for (std::vector<Region*>::const_iterator
				i = _gameSave->getRegions()->begin();
				i != _gameSave->getRegions()->end();
				++i)
		{
			score += (*i)->getActivityXCom().at(id) - (*i)->getActivityAlien().at(id);
		}
		_txtScore->setText(Text::formatNumber(score));
	}
	else
		_txtScore->setText(Text::intWide(0));


	if (_timeComp != _btn5Secs)
		_txtSec->setVisible();
	else
	{
		const int sec = _gameSave->getTime()->getSecond();
		_txtSec->setVisible(sec % 15 > 9);
//		_txtSec->setVisible(_gameSave->getTime()->getSecond() % 15 == 0);
	}

	std::wostringstream
		woststr1,
		woststr2;

	woststr1 << std::setfill(L'0') << std::setw(2) << _gameSave->getTime()->getMinute();
	_txtMin->setText(woststr1.str());

	woststr2 << std::setfill(L'0') << std::setw(2) << _gameSave->getTime()->getHour();
	_txtHour->setText(woststr2.str());

	int date = _gameSave->getTime()->getDay();
	if (_day != date)
	{
		_day = date;
		_txtDay->setText(Text::intWide(_day));

		date = _gameSave->getTime()->getMonth();
		if (_month != date)
		{
			_month = date;
			_txtMonth->setText(convertDateToMonth(_month));

			date = _gameSave->getTime()->getYear();
			if (_year != date)
			{
				_year = date;
				_txtYear->setText(Text::intWide(_year));
			}
		}
	}
}

/**
 * Converts the date to a month string.
 * @param date - the date
 */
std::wstring GeoscapeState::convertDateToMonth(int date)
{
	switch (date)
	{
		case  1: return L"jan";
		case  2: return L"feb";
		case  3: return L"mar";
		case  4: return L"apr";
		case  5: return L"may";
		case  6: return L"jun";
		case  7: return L"jul";
		case  8: return L"aug";
		case  9: return L"sep";
		case 10: return L"oct";
		case 11: return L"nov";
		case 12: return L"dec";
	}

	return L"error";
}

/**
 * Advances the game timer according to the set timer-speed and calls
 * respective triggers.
 * @note The game always advances in 5sec cycles regardless of the speed
 * otherwise this will skip important steps. Instead it just keeps advancing the
 * timer until the next compression step - eg. the next day on 1 Day speed - or
 * until an event occurs.
 */
void GeoscapeState::timeAdvance()
{
	if (_pauseHard == false)
	{
		int
			timeLapse,
			timeLap_t;

		if		(_timeComp == _btn5Secs)	timeLapse = 1;
		else if (_timeComp == _btn1Min)		timeLapse = 12;
		else if (_timeComp == _btn5Mins)	timeLapse = 12 * 5;
		else if (_timeComp == _btn30Mins)	timeLapse = 12 * 5 * 6;
		else if (_timeComp == _btn1Hour)	timeLapse = 12 * 5 * 6 * 2;
		else if (_timeComp == _btn1Day)		timeLapse = 12 * 5 * 6 * 2 * 24;
		else
			timeLapse = 0; // what'ver.

		timeLapse *= 5; // true one-second intervals. based on Volutar's smoothGlobe.

		timeLap_t = ((_timeCache + timeLapse) * 4) / Options::geoClockSpeed;
		_timeCache = ((_timeCache + timeLapse) * 4) % Options::geoClockSpeed;

		if (timeLap_t != 0)
		{
			bool update = false;
			for (int
					i = 0;
					i != timeLap_t && _pause == false;
					++i)
			{
				const TimeTrigger trigger = _gameSave->getTime()->advance();
				if (trigger != TIME_1SEC)
				{
					update = true;
					switch (trigger)
					{
						case TIME_1MONTH:	time1Month();
						case TIME_1DAY:		time1Day();
						case TIME_1HOUR:	time1Hour();
						case TIME_30MIN:	time30Minutes();
						case TIME_10MIN:	time10Minutes();
						case TIME_5SEC:		time5Seconds();
					}
				}
			}

			if (update == true) updateTimeDisplay();

			_pause = (_dogfightsToStart.empty() == false);
			_globe->draw();
		}
	}
}

/**
 * Takes care of any game logic that has to run every 5 game seconds.
 */
void GeoscapeState::time5Seconds()
{
	//Log(LOG_INFO) << "GeoscapeState::time5Seconds()";
	if (_gameSave->getBases()->empty() == true) // Game Over if there are no more bases.
	{
		popup(new DefeatState());
		return;
	}

	const Ufo* ufoExpired = NULL; // kL, see below_

	for (std::vector<Ufo*>::const_iterator // Handle UFO logic
			i = _gameSave->getUfos()->begin();
			i != _gameSave->getUfos()->end();
			++i)
	{
		switch ((*i)->getUfoStatus())
		{
			case Ufo::FLYING:
				if (_dfZoomInTimer->isRunning() == false
					&& _dfZoomOutTimer->isRunning() == false)
				{
					(*i)->think();

					if ((*i)->reachedDestination() == true)
					{
						const size_t qtySites = _gameSave->getMissionSites()->size();
						const bool detected = (*i)->getDetected();

						AlienMission* const mission = (*i)->getAlienMission();
						mission->ufoReachedWaypoint(**i, *_rules, *_globe); // recomputes 'qtySites' & 'detected'; also sets ufo Status

						if ((*i)->getAltitude() == "STR_GROUND")
						{
							_dfZoomOut = false;
							for (std::list<DogfightState*>::const_iterator
									j = _dogfights.begin();
									j != _dogfights.end();
									++j)
							{
								if ((*j)->getUfo() != *i) // huh, sometimes I wonder what the hell i code.
								{
									_dfZoomOut = true;
									break;
								}
							}
						}


						if (detected != (*i)->getDetected()
							&& (*i)->getFollowers()->empty() == false)
						{
							if (!
								((*i)->getTrajectory().getId() == UfoTrajectory::RETALIATION_ASSAULT_RUN
									&& (*i)->getUfoStatus() == Ufo::LANDED))
							{
								resetTimer();
								popup(new UfoLostState((*i)->getName(_game->getLanguage())));
							}
						}

						if (qtySites < _gameSave->getMissionSites()->size()) // new MissionSite appeared when UFO reached waypoint, above^
						{
							MissionSite* const site = _gameSave->getMissionSites()->back();
							site->setDetected();

							popup(new MissionDetectedState(site, this));
						}

						if ((*i)->getUfoStatus() == Ufo::DESTROYED) // if UFO was destroyed don't spawn missions
							return;

						if (Base* const base = dynamic_cast<Base*>((*i)->getDestination()))
						{
							mission->setWaveCountdown(30 * (RNG::generate(0,48) + 400));
							(*i)->setDestination(NULL);
							base->setupDefenses();

							resetTimer();

							if (base->getDefenses()->empty() == false)
								popup(new BaseDefenseState(base, *i, this));
							else
							{
								handleBaseDefense(base, *i);
								return;
							}
						}
					}
				}
			break;

			case Ufo::LANDED:
				(*i)->think();

				if ((*i)->getSecondsLeft() == 0)
				{
					AlienMission* const mission = (*i)->getAlienMission();
					const bool detected = (*i)->getDetected();
					mission->ufoLifting(**i, *_rules, *_globe);

					if (detected != (*i)->getDetected()
						&& (*i)->getFollowers()->empty() == false)
					{
						resetTimer();
						popup(new UfoLostState((*i)->getName(_game->getLanguage())));
					}
				}
			break;

			case Ufo::CRASHED:
				(*i)->think();

				if ((*i)->getSecondsLeft() == 0)
				{
					ufoExpired = *i; // shot down while trying to outrun interceptor
					(*i)->setDetected(false);
					(*i)->setUfoStatus(Ufo::DESTROYED);
				}
		}
	}

	// Handle craft logic
	bool initDfMusic = false;

	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				)
		{
			if ((*j)->isDestroyed() == true)
			{
				_gameSave->scorePoints(
								(*j)->getLongitude(),
								(*j)->getLatitude(),
								(*j)->getRules()->getScore(),
								true);

				if ((*j)->getRules()->getSoldiers() != 0) // if a transport craft has been shot down all soldiers aboard are dead.
				{
					for (std::vector<Soldier*>::const_iterator
							k = (*i)->getSoldiers()->begin();
							k != (*i)->getSoldiers()->end();
							)
					{
						if ((*k)->getCraft() == *j)
						{
							(*k)->die(_gameSave);
							delete *k;
							k = (*i)->getSoldiers()->erase(k);
						}
						else ++k;
					}
				}

				delete *j;
				j = (*i)->getCrafts()->erase(j);
				continue;
			}

			if ((*j)->getDestination() != NULL)
			{
				const Ufo* const ufo = dynamic_cast<Ufo*>((*j)->getDestination());
				if (ufo != NULL)
				{
					if (ufo->getUfoStatus() != Ufo::FLYING)
						(*j)->setInDogfight(false);

					if (ufo->getDetected() == false	// lost radar contact
						&& ufo != ufoExpired)		// <- ie. not recently shot down while trying to outrun interceptor but it crashed into the sea instead Lol
					{
						if (ufo->getTrajectory().getId() == UfoTrajectory::RETALIATION_ASSAULT_RUN
							&& (ufo->getUfoStatus() == Ufo::LANDED // base defense
								|| ufo->getUfoStatus() == Ufo::DESTROYED))
						{
							(*j)->returnToBase();
						}
						else
						{
							(*j)->setDestination(NULL);

							Waypoint* const wp = new Waypoint();
							wp->setLongitude(ufo->getLongitude());
							wp->setLatitude(ufo->getLatitude());
							wp->setId(ufo->getId());

							resetTimer();
							popup(new GeoscapeCraftState(*j, this, wp));
						}
					}
					else if (ufo->getUfoStatus() == Ufo::DESTROYED
						|| (ufo->getUfoStatus() == Ufo::CRASHED	// http://openxcom.org/forum/index.php?topic=2406.0
							&& (*j)->getNumSoldiers() == 0))	// Actually should set this on the UFO-crash event
//							&& (*j)->getNumVehicles() == 0))	// so that crashed-ufos can still be targeted for Patrols.
					{
						(*j)->returnToBase();
					}
				}
				else
					(*j)->setInDogfight(false); // safety.
			}

			if (_dfZoomInTimer->isRunning() == false
				&& _dfZoomOutTimer->isRunning() == false)
			{
				(*j)->think();
			}

			if ((*j)->reachedDestination() == true)
			{
				Ufo* const ufo = dynamic_cast<Ufo*>((*j)->getDestination());
				const Waypoint* const wp = dynamic_cast<Waypoint*>((*j)->getDestination());
				const MissionSite* const site = dynamic_cast<MissionSite*>((*j)->getDestination());
//				const AlienBase* const aBase = dynamic_cast<AlienBase*>((*j)->getDestination());

				if (ufo != NULL)
				{
					switch (ufo->getUfoStatus())
					{
						case Ufo::FLYING:
							if (_dogfights.size() + _dogfightsToStart.size() < 4) // Not more than 4 interceptions at a time. _note: I thought orig could do up to 6.
							{
								if ((*j)->isInDogfight() == false
									&& AreSame((*j)->getDistance(ufo), 0.)) // craft ran into a UFO
								{
									_dogfightsToStart.push_back(new DogfightState(_globe, *j, ufo, this));
									if (_dfStartTimer->isRunning() == false)
									{
										_pause = true;
										resetTimer();
										storePreDfCoords();	// store current Globe coords & zoom;
										_globe->center(		// Globe will reset to these after dogfight ends
													(*j)->getLongitude(),
													(*j)->getLatitude());

										if (_dogfights.empty() == true // first dogfight, start music
											&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_GEO_INTERCEPT) == false) // unless reloading to another dogfight ...
										{
											_game->getResourcePack()->fadeMusic(_game, 425);
										}

										startDogfight();
										_dfStartTimer->start();
									}

									initDfMusic = true;
									_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_INTERCEPT);
								}
							}
						break;

						case Ufo::LANDED:		// setSpeed 1/2 (need to speed up to full if UFO takes off)
						case Ufo::CRASHED:		// setSpeed 1/2 (need to speed back up when setting a new destination)
						case Ufo::DESTROYED:	// just before expiration
							if ((*j)->getNumSoldiers() > 0)
							{
								if ((*j)->isInDogfight() == false)
								{
									resetTimer();
									int // look up polygon's texId + shade
										texId,
										shade;
									_globe->getPolygonTextureAndShade(
																	ufo->getLongitude(),
																	ufo->getLatitude(),
																	&texId, &shade);
									popup(new ConfirmLandingState(
															*j, // countryside Texture; choice of Terrain made in ConfirmLandingState
															_rules->getGlobe()->getTextureRule(texId),
															shade));
								}
							}
							else if (ufo->getUfoStatus() != Ufo::LANDED)
							{
								popup(new CraftPatrolState(*j, this));
								(*j)->setDestination(NULL);
							}
					}
				}
				else // wp, site, OR aBase
				{
					if (wp == NULL && (*j)->getNumSoldiers() != 0)
					{
						resetTimer();
						if (site != NULL)
						{
							const int texId = site->getSiteTextureId();
							int shade;
							_globe->getPolygonShade(
												site->getLongitude(),
												site->getLatitude(),
												&shade);
							popup(new ConfirmLandingState( // preset missionSite Texture; choice of Terrain made via texture-deployment, in ConfirmLandingState
													*j,
													_rules->getGlobe()->getTextureRule(texId),
													shade));
						}
						else // aLien Base.
							popup(new ConfirmLandingState(*j)); // choice of Terrain made in BattlescapeGenerator.
					}
					else // do Patrol.
					{
						popup(new CraftPatrolState(*j, this));
						(*j)->setDestination(NULL);
					}
				}
			}

			++j;
		}
	}

	for (std::vector<Ufo*>::const_iterator // Clean up dead UFOs and end dogfights which were minimized.
			i = _gameSave->getUfos()->begin();
			i != _gameSave->getUfos()->end();
			)
	{
		if ((*i)->getUfoStatus() == Ufo::DESTROYED)
		{
			if ((*i)->getFollowers()->empty() == false)
			{
				for (std::list<DogfightState*>::const_iterator // Remove all dogfights with this UFO.
						j = _dogfights.begin();
						j != _dogfights.end();
						)
				{
					if ((*j)->getUfo() == *i)
					{
						delete *j;
						j = _dogfights.erase(j);
					}
					else ++j;
				}

				resetInterceptPorts();
			}

			delete *i;
			i = _gameSave->getUfos()->erase(i);
		}
		else ++i;
	}

	for (std::vector<Waypoint*>::const_iterator // Clean up unused waypoints
			i = _gameSave->getWaypoints()->begin();
			i != _gameSave->getWaypoints()->end();
			)
	{
		if ((*i)->getFollowers()->empty() == true)
		{
			delete *i;
			i = _gameSave->getWaypoints()->erase(i);
		}
		else ++i;
	}

	drawUfoBlobs();


	// This is ONLY for allowing _dogfights to fill (or not) before deciding whether
	// to startMusic in init() -- and ONLY for Loading with a dogfight in progress:
	// But now it's also used for resuming Geoscape music on returning from another state ....
	if (kL_geoMusicPlaying == false)
	{
		if (++_delayMusicDfCheck > 3	// this is because initDfMusic takes uh 2+ passes before the _dogfight vector fills.
			|| initDfMusic == true		// and if it doesn't fill by that time I want some music playing.
			|| kL_geoMusicReturnState == true)
		{
			kL_geoMusicPlaying = true;	// if there's a dogfight then dogfight music
										// will play when a SavedGame is loaded
			if (_dogfights.empty() == true
				&& _dfStartTimer->isRunning() == false)
			{
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
			}
		}
	}
}

/**
 * *** FUNCTOR ***
 * Functor that attempts to detect an XCOM base.
 */
class DetectXCOMBase
	:
		public std::unary_function<Ufo*, bool>
{
private:
	const int _diff;
	const Base& _base;

	public:
		/// Create a detector for the given base.
		DetectXCOMBase(
				const Base& base,
				int diff)
			:
				_base(base),
				_diff(diff)
		{}

		/// Attempt detection
		bool operator() (const Ufo* ufo) const;
};

/**
 * Only UFOs within detection range of the base have a chance to detect it.
 * @param ufo - pointer to the UFO attempting detection
 * @return, true if base detected
 */
bool DetectXCOMBase::operator() (const Ufo* ufo) const
{
	//Log(LOG_INFO) << "DetectXCOMBase(), ufoID " << ufo->getId();
	if (ufo->isCrashed() == true)
	{
		//Log(LOG_INFO) << ". . Crashed UFOs can't detect!";
		return false;
	}

	if (ufo->getTrajectoryPoint() < 2)
	{
		//Log(LOG_INFO) << ". . UFO just entered atmosphere - can't detect!";
		return false;
	}

	if (ufo->getTrajectory().getZone(ufo->getTrajectoryPoint()) == 5)
	{
		//Log(LOG_INFO) << ". . UFO about to leave atmosphere - can't detect!";
		return false;
	}

	if (ufo->getTrajectory().getId() == UfoTrajectory::RETALIATION_ASSAULT_RUN)
	{
		//Log(LOG_INFO) << ". uFo's attacking a base don't bother with this!";
		return false;
	}

	if (ufo->getAlienMission()->getRules().getObjective() != alm_RETAL
		&& Options::aggressiveRetaliation == false)
	{
		//Log(LOG_INFO) << ". . Only uFo's on retaliation missions scan for bases unless 'aggressiveRetaliation' option is TRUE";
		return false;
	}


	const double
		range = static_cast<double>(ufo->getRules()->getReconRange()) * greatCircleConversionFactor,
		dist = _base.getDistance(ufo) * earthRadius;
	//Log(LOG_INFO) << ". . range = " << (int)range;
	//Log(LOG_INFO) << ". . dist = " << (int)dist;

	if (dist > range)
	{
		//Log(LOG_INFO) << ". . uFo's have a detection range of " << (int)range << " nautical miles.";
		return false;
	}


	const double inverseFactor = dist * 12. / range; // should use log() ...
	int pct = static_cast<int>(Round(
			  static_cast<double>(_base.getDetectionChance(_diff) + ufo->getDetectors()) / inverseFactor));
	if (ufo->getAlienMission()->getRules().getObjective() == alm_RETAL
		&& Options::aggressiveRetaliation == true)
	{
		//Log(LOG_INFO) << ". . uFo's on retaliation missions scan for bases 'aggressively'";
		pct += 3 + _diff;
	}
	//Log(LOG_INFO) << ". . . pct = " << pct;

	return RNG::percent(pct);
}


/**
 * *** FUNCTOR ***
 * Functor that marks an XCOM base for retaliation.
 * @note This is required because of the iterator type.
 * @note Only used if Aggressive Retaliation option is false.
 */
struct SetRetaliationStatus
	:
		public std::unary_function<std::map<const Region*, Base*>::value_type, void>
{
	/// Mark as a valid retaliation target.
	void operator() (const argument_type& i) const
	{
		i.second->setBaseExposed();
	}
};


/**
 * Takes care of any game logic that runs every ten game minutes.
 */
void GeoscapeState::time10Minutes()
{
	//Log(LOG_INFO) << "GeoscapeState::time10Minutes()";
	const int diff = static_cast<int>(_gameSave->getDifficulty());

	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getCraftStatus() == "STR_OUT"
				&& (*j)->getTakeoff() == true)
			{
				(*j)->consumeFuel();

				if ((*j)->getLowFuel() == false
					&& (*j)->getFuel() <= (*j)->getFuelLimit())
				{
					(*j)->setLowFuel();
					(*j)->returnToBase();

					popup(new LowFuelState(*j, this));
				}

//				if ((*j)->getDestination() == NULL) // Remove that; patrolling for aBases/10min was getting too bothersome.
//				{
				//Log(LOG_INFO) << ". Patrol for alienBases";
				for (std::vector<AlienBase*>::const_iterator // patrol for aLien bases.
						k = _gameSave->getAlienBases()->begin();
						k != _gameSave->getAlienBases()->end();
						++k)
				{
					if ((*k)->isDiscovered() == false)
					{
						const double
							craftRadar = static_cast<double>((*j)->getRules()->getReconRange()) * greatCircleConversionFactor,
							targetDist = (*j)->getDistance(*k) * earthRadius;
						//Log(LOG_INFO) << ". . craftRadar = " << (int)craftRadar;
						//Log(LOG_INFO) << ". . targetDist = " << (int)targetDist;

						if (targetDist < craftRadar)
						{
							const int chance = 100 - (diff * 10) - static_cast<int>(targetDist * 50. / craftRadar);
							//Log(LOG_INFO) << ". . . craft in Range, chance = " << chance;

							if (RNG::percent(chance) == true)
							{
								//Log(LOG_INFO) << ". . . . aLienBase discovered";
								(*k)->setDiscovered(true);
							}
						}
					}
				}
//				}
			}
		}
	}

	if (Options::aggressiveRetaliation == true)
	{
		for (std::vector<Base*>::const_iterator // detect as many bases as possible.
				i = _gameSave->getBases()->begin();
				i != _gameSave->getBases()->end();
				++i)
		{
			if ((*i)->getBaseExposed() == false)
			{
				std::vector<Ufo*>::const_iterator ufo = std::find_if( // find a UFO that detected this base, if any.
																_gameSave->getUfos()->begin(),
																_gameSave->getUfos()->end(),
																DetectXCOMBase(**i, diff));

				if (ufo != _gameSave->getUfos()->end())
					(*i)->setBaseExposed();
			}
		}
	}
	else
	{
		std::map<const Region*, Base*> discovered;
		for (std::vector<Base*>::const_iterator // remember only last base in each region.
				i = _gameSave->getBases()->begin();
				i != _gameSave->getBases()->end();
				++i)
		{
			std::vector<Ufo*>::const_iterator ufo = std::find_if( // find a UFO that detected this base, if any.
															_gameSave->getUfos()->begin(),
															_gameSave->getUfos()->end(),
															DetectXCOMBase(**i, diff));

			if (ufo != _gameSave->getUfos()->end())
				discovered[_gameSave->locateRegion(**i)] = *i;
		}

		std::for_each( // mark the bases as discovered.
					discovered.begin(),
					discovered.end(),
					SetRetaliationStatus());
	}


	_windowPops = 0;

	for (std::vector<Ufo*>::const_iterator // handle UFO detection
			i = _gameSave->getUfos()->begin();
			i != _gameSave->getUfos()->end();
			++i)
	{
		if ((*i)->getUfoStatus() == Ufo::FLYING
			|| (*i)->getUfoStatus() == Ufo::LANDED)
		{
			std::vector<Base*> hyperBases; // = std::vector<Base*>();

			if ((*i)->getDetected() == false)
			{
				const bool hyperDet_pre = (*i)->getHyperDetected();
				bool
					hyperDet = false,
					contact = false;

				for (std::vector<Base*>::const_iterator
						j = _gameSave->getBases()->begin();
						j != _gameSave->getBases()->end();
						++j)
				{
					switch ((*j)->detect(*i))
					{
						case 3:
							contact = true;
						case 1:
							hyperDet = true;

							if (hyperDet_pre == false)
								hyperBases.push_back(*j);
						break;

						case 2:
							contact = true;
					}

					for (std::vector<Craft*>::const_iterator
							k = (*j)->getCrafts()->begin();
							k != (*j)->getCrafts()->end()
								&& contact == false;
							++k)
					{
						if ((*k)->getCraftStatus() == "STR_OUT"
							&& (*k)->getTakeoff() == true
							&& (*k)->detect(*i) == true)
						{
							contact = true;
						}
					}
				}

				(*i)->setDetected(contact);
				(*i)->setHyperDetected(hyperDet);

				if (contact == true
					|| (hyperDet == true
						&& hyperDet_pre == false))
				{
					++_windowPops;
					popup(new UfoDetectedState(
											*i,
											this,
											true,
											hyperDet,
											contact,
											&hyperBases));
				}
			}
			else // ufo is already detected
			{
				const bool hyperDet_pre = (*i)->getHyperDetected();
				bool
					hyperDet = false,
					contact = false;

				for (std::vector<Base*>::const_iterator
						j = _gameSave->getBases()->begin();
						j != _gameSave->getBases()->end();
						++j)
				{
					switch ((*j)->detect(*i)) // base attempts redetection; this lets a UFO blip off radar scope
					{
						case 3:
							contact = true;
						case 1:
							hyperDet = true;

							if (hyperDet_pre == false)
								hyperBases.push_back(*j);
						break;

						case 2:
							contact = true;
					}

					for (std::vector<Craft*>::const_iterator
							k = (*j)->getCrafts()->begin();
							k != (*j)->getCrafts()->end()
								&& contact == false;
							++k)
					{
						if ((*k)->getCraftStatus() == "STR_OUT"
							&& (*k)->getTakeoff() == true
							&& (*k)->detect(*i) == true)
						{
							contact = true;
						}
					}
				}

				(*i)->setDetected(contact);
				(*i)->setHyperDetected(hyperDet);

				if (hyperDet == true
					&& hyperDet_pre == false)
				{
					++_windowPops;
					popup(new UfoDetectedState(
											*i,
											this,
											false,
											hyperDet,
											contact,
											&hyperBases));
				}

				if (contact == false
					&& (*i)->getFollowers()->empty() == false)
				{
					resetTimer();
					popup(new UfoLostState((*i)->getName(_game->getLanguage())));
				}
			}
		}
	}


	if (_windowPops > 0)
	{
		_ufoDetected->setText(Text::intWide(static_cast<int>(_windowPops)));
		_ufoDetected->setVisible();
	}
}

/**
 * *** FUNCTOR ***
 * @brief Call AlienMission::think() with proper parameters.
 * @note This function object calls AlienMission::think() with the proper parameters.
 */
class callThink
	:
		public std::unary_function<AlienMission*, void>
{
private:
	Game& _game;
	const Globe& _globe;

	public:
		/// Store the parameters.
		/**
		 * @param game	- reference to the Game engine
		 * @param globe	- reference to the Globe object
		 */
		callThink(
				Game& game,
				const Globe& globe)
			:
				_game(game),
				_globe(globe)
		{}

		/// Call AlienMission::think() with stored parameters.
		void operator() (AlienMission* mission) const
		{
			mission->think(_game, _globe);
		}
};

/**
 * *** FUNCTOR ***
 * @brief Process a MissionSite.
 * This function object will count down towards expiring a MissionSite and
 * handle expired MissionSites.
 * @param site - pointer to a MissionSite
 * @return, true if mission is finished (w/out xCom mission success)
 */
bool GeoscapeState::processMissionSite(MissionSite* const site) const
{
	bool expired;

	const int
		diff = static_cast<int>(_gameSave->getDifficulty()),
		month = _gameSave->getMonthsPassed();
	int
		aLienPts,
		basicPts;

	if (site->getSecondsLeft() > 1799)
	{
		expired = false;
		site->setSecondsLeft(site->getSecondsLeft() - 1800);

		basicPts = site->getDeployment()->getPointsPer30(); // AlienDeployments pts have priority over RuleAlienMission pts
		if (basicPts == 0)
			basicPts = site->getRules()->getPoints() / 10;

		aLienPts = basicPts + (diff * 10) + month;
	}
	else
	{
		expired = true;

		basicPts = site->getDeployment()->getDespawnPenalty(); // AlienDeployments pts have priority over RuleAlienMission pts
		if (basicPts == 0)
			basicPts = site->getRules()->getPoints() * 5;

		aLienPts = basicPts + (diff * (235 + month));
	}

	if (aLienPts != 0)
		_gameSave->scorePoints(
							site->getLongitude(),
							site->getLatitude(),
							aLienPts,
							true);

	if (expired == true)
	{
		delete site;
		return true;
	}

	return false;
}

/**
 * *** FUNCTOR ***
 * @brief Advance time for crashed UFOs.
 * @note This function object will decrease the expiration timer for crashed UFOs.
 */
struct expireCrashedUfo: public std::unary_function<Ufo*, void>
{
	/// Decrease UFO expiration timer.
	void operator() (Ufo* ufo) const
	{
		if (ufo->getUfoStatus() == Ufo::CRASHED)
		{
			const int sec = ufo->getSecondsLeft();
			if (sec >= 30 * 60)
			{
				ufo->setSecondsLeft(sec - 30 * 60);
				return;
			}

			ufo->setUfoStatus(Ufo::DESTROYED); // mark expired UFOs for removal.
		}
	}
};

/**
 * Takes care of any game logic that has to run every game half hour.
 */
void GeoscapeState::time30Minutes()
{
	std::for_each( // decrease mission countdowns
			_gameSave->getAlienMissions().begin(),
			_gameSave->getAlienMissions().end(),
			callThink(
					*_game,
					*_globe));

	for (std::vector<AlienMission*>::const_iterator // remove finished missions
			i = _gameSave->getAlienMissions().begin();
			i != _gameSave->getAlienMissions().end();
			)
	{
		if ((*i)->isOver() == true)
		{
			delete *i;
			i = _gameSave->getAlienMissions().erase(i);
		}
		else
			++i;
	}

	std::for_each( // handle crashed UFOs expiration
			_gameSave->getUfos()->begin(),
			_gameSave->getUfos()->end(),
			expireCrashedUfo());


	for (std::vector<Base*>::const_iterator // handle craft maintenance & refueling.
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getCraftStatus() != "STR_OUT")
			{
				if ((*j)->getCraftStatus() == "STR_REPAIRS")
					(*j)->repair();
				else if ((*j)->getCraftStatus() == "STR_REARMING")
				{
					const std::string rearmClip = (*j)->rearm(_rules);

					if (rearmClip.empty() == false
						&& (*j)->getWarned() == false)
					{
						(*j)->setWarned();
						const std::wstring msg = tr("STR_NOT_ENOUGH_ITEM_TO_REARM_CRAFT_AT_BASE")
												.arg(tr(rearmClip))
												.arg((*j)->getName(_game->getLanguage()))
												.arg((*i)->getName());
						popup(new CraftErrorState(this, msg));
					}
				}
				else if ((*j)->getCraftStatus() == "STR_REFUELLING")
				{
					const std::string refuelItem = (*j)->getRules()->getRefuelItem();

					if (refuelItem.empty() == true)
						(*j)->refuel();
					else
					{
						if ((*j)->getRules()->getRefuelRate() <= (*i)->getStorageItems()->getItemQty(refuelItem))
						{
							(*j)->refuel();
							(*i)->getStorageItems()->removeItem(
															refuelItem,
															(*j)->getRules()->getRefuelRate());
						}
						else if ((*j)->getWarned() == false)
						{
							(*j)->setWarned();
							(*j)->setWarning(CW_CANTREFUEL);
							const std::wstring msg = tr("STR_NOT_ENOUGH_ITEM_TO_REFUEL_CRAFT_AT_BASE")
													.arg(tr(refuelItem))
													.arg((*j)->getName(_game->getLanguage()))
													.arg((*i)->getName());
							popup(new CraftErrorState(this, msg));
						}
					}
				}
			}
		}
	}


	const int pt = ((_gameSave->getMonthsPassed() + 2) / 4)
				 + static_cast<int>(_gameSave->getDifficulty());	// basic Victory Points
	int aLienPts;													// Beginner @ 1st/2nd month ought add 0pts. here

	// TODO: pt = pt * _rules->getAlienMission("STR_ALIEN_*")->getPoints() / 100;

	for (std::vector<Ufo*>::const_iterator // handle UFO detection and give aLiens points
			i = _gameSave->getUfos()->begin();
			i != _gameSave->getUfos()->end();
			++i)
	{
		aLienPts = (*i)->getVictoryPoints() + pt; // reset for each UFO
		if (aLienPts > 0)
		{
			const double
				lon = (*i)->getLongitude(),
				lat = (*i)->getLatitude();

			for (std::vector<Region*>::const_iterator
					j = _gameSave->getRegions()->begin();
					j != _gameSave->getRegions()->end();
					++j)
			{
				if ((*j)->getRules()->insideRegion(
												lon,
												lat) == true)
				{
					(*j)->addActivityAlien(aLienPts); // points per UFO in-Region per half hour
					(*j)->recentActivityAlien();

					break;
				}
			}

			for (std::vector<Country*>::const_iterator
					j = _gameSave->getCountries()->begin();
					j != _gameSave->getCountries()->end();
					++j)
			{
				if ((*j)->getRules()->insideCountry(
												lon,
												lat) == true)
				{
					(*j)->addActivityAlien(aLienPts); // points per UFO in-Country per half hour
					(*j)->recentActivityAlien();

					break;
				}
			}
		}
	}


	for (std::vector<MissionSite*>::const_iterator
			i = _gameSave->getMissionSites()->begin();
			i != _gameSave->getMissionSites()->end();
			)
	{
		if (processMissionSite(*i))
			i = _gameSave->getMissionSites()->erase(i);
		else
			++i;
	}
}

/**
 * Takes care of any game logic that has to run every game hour.
 */
void GeoscapeState::time1Hour()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Hour()";
	for (std::vector<Region*>::const_iterator
			i = _gameSave->getRegions()->begin();
			i != _gameSave->getRegions()->end();
			++i)
	{
		(*i)->recentActivityAlien(false);
		(*i)->recentActivityXCom(false);
	}

	for (std::vector<Country*>::const_iterator
			i = _gameSave->getCountries()->begin();
			i != _gameSave->getCountries()->end();
			++i)
	{
		(*i)->recentActivityAlien(false);
		(*i)->recentActivityXCom(false);
	}


	//Log(LOG_INFO) << ". arrivals";
	bool arrivals = false;

	for (std::vector<Base*>::const_iterator // handle transfers
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		for (std::vector<Transfer*>::const_iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			(*j)->advance(*i);

			if ((*j)->getHours() == 0)
				arrivals = true;
		}
	}
	//Log(LOG_INFO) << ". arrivals DONE";


	std::vector<ProductionCompleteInfo> events;
	// Note that if transfers arrive at the same time Production(s) complete
	// the gotoBase button handling below is obviated by RMB on transfers ....
	// But that's been amended by showing Transfers after ProdCompleted screens;
	// now I'm not sure how this will all interact with time1Day()'s facility
	// construction and research completed screens.

	for (std::vector<Base*>::const_iterator // handle Production
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		std::map<Production*, ProductionProgress> progress;

		for (std::vector<Production*>::const_iterator
				j = (*i)->getProductions().begin();
				j != (*i)->getProductions().end();
				++j)
		{
			progress[*j] = (*j)->step(*i, _gameSave, _rules);
		}

		for (std::map<Production*, ProductionProgress>::const_iterator
				j = progress.begin();
				j != progress.end();
				++j)
		{
			if (j->second > PROGRESS_NOT_COMPLETE)
			{
				(*i)->removeProduction(j->first);

				if (events.empty() == false) // set the previous event to NOT show btn.
					events.back().gotoBaseBtn = false;

				events.push_back(ProductionCompleteInfo(
													*i,
													tr(j->first->getRules()->getType()),
													(arrivals == false),
													j->second));
			}
		}

		if (Options::storageLimitsEnforced == true
			&& (*i)->storesOverfull() == true)
		{
			resetTimer();
			popup(new ErrorMessageState(
								tr("STR_STORAGE_EXCEEDED").arg((*i)->getName()).c_str(),
								_palette,
								_rules->getInterface("geoscape")->getElement("errorMessage")->color,
								"BACK12.SCR", // "BACK13.SCR"
								_rules->getInterface("geoscape")->getElement("errorPalette")->color));
//			popup(new SellState(*i));
		}
	}

	for (std::vector<ProductionCompleteInfo>::const_iterator
			j = events.begin();
			j != events.end();
			)
	{
		popup(new ProductionCompleteState(
										j->base,
										j->item,
										this,
										j->gotoBaseBtn,
										j->endType));
		j = events.erase(j);
	}

	if (arrivals == true)
		popup(new ItemsArrivingState(this));


	// TFTD stuff: 'detected' see MissionSite class
	for (std::vector<MissionSite*>::const_iterator
			i = _gameSave->getMissionSites()->begin();
			i != _gameSave->getMissionSites()->end();
			++i)
	{
		if ((*i)->getDetected() == false)
		{
			(*i)->setDetected();
			popup(new MissionDetectedState(*i, this));
			break;
		}
	}
	//Log(LOG_INFO) << "GeoscapeState::time1Hour() EXIT";
}


/**
 * *** FUNCTOR ***
 * This class will attempt to generate a supply mission for a base.
 * @note Each alien base has a 4% chance to generate a supply mission.
 */
class GenerateSupplyMission
	:
		public std::unary_function<const AlienBase*, void>
{

private:
	const Ruleset& _rules;
	SavedGame& _gameSave;

	public:
		/// Store rules and game data references for later use.
		GenerateSupplyMission(
				const Ruleset& rules,
				SavedGame& gameSave)
			:
				_rules(rules),
				_gameSave(gameSave)
		{}

		/// Check and spawn mission.
		void operator() (const AlienBase* const base) const;
};

/**
 * Check and create supply mission for the given base.
 * @note There is a 4% chance of the mission spawning.
 * @param base A pointer to the alien base.
 */
void GenerateSupplyMission::operator() (const AlienBase* const base) const
{
	if (RNG::percent(4) == true)
	{
		// Spawn supply mission for this base.
		const RuleAlienMission& missionRule = *_rules.getAlienMission("STR_ALIEN_SUPPLY");
		AlienMission* const mission = new AlienMission(missionRule, _gameSave);
		mission->setRegion(
					_gameSave.locateRegion(*base)->getRules()->getType(),
					_rules);
		mission->setId(_gameSave.getCanonicalId("ALIEN_MISSIONS"));
		mission->setRace(base->getAlienRace());
		mission->setAlienBase(base);
		mission->start();

		_gameSave.getAlienMissions().push_back(mission);
	}
}


/**
 * Takes care of any game logic that has to run every game day.
 */
void GeoscapeState::time1Day()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Day()";
	// Create a vector of pending events for this base so a slightly
	// different dialog layout can be shown for the last event of each type.
	std::vector<ProductionCompleteInfo> prodEvents;
	std::vector<State*> resEvents;
	std::vector<NewPossibleResearchInfo> newResEvents;
	std::vector<NewPossibleManufactureInfo> newProdEvents;

	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		bool dead;
		for (std::vector<Soldier*>::const_iterator // handle soldiers in sickbay
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				)
		{
			dead = false;

			if ((*j)->getRecovery() != 0)
			{
				int chanceDeath = (*j)->getRecoveryPct();
				if (chanceDeath > 10)
				{
					//Log(LOG_INFO) << "\n";
					//Log(LOG_INFO) << ". Soldier = " << (*j)->getId() << " woundsPCT = " << chanceDeath;
					const int lastMissionId ((*j)->getDiary()->getMissionIdList().back());
					const std::vector<MissionStatistics*>* const missionStats (_gameSave->getMissionStatistics());
					const int daysTotal (missionStats->at(lastMissionId)->injuryList[(*j)->getId()]);

					//Log(LOG_INFO) << ". . daysTotal = " << daysTotal;
					if (daysTotal > 0) // safety.
					{
						const float healthFactor (static_cast<float>(daysTotal) / static_cast<float>((*j)->getCurrentStats()->health));
						//Log(LOG_INFO) << ". . healthFactor = " << healthFactor;
						chanceDeath = static_cast<int>(std::ceil(
									  static_cast<float>(chanceDeath) * healthFactor));

						const int roll (RNG::generate(1,1000));
						//Log(LOG_INFO) << ". . chance to Die = " << chanceDeath << " roll = " << roll;
						if (roll <= chanceDeath)
						{
							//Log(LOG_INFO) << "he's dead, Jim!!";
							resetTimer();
							if ((*j)->getArmor()->isBasic() == false) // return soldier's armor to Stores
								(*i)->getStorageItems()->addItem((*j)->getArmor()->getStoreItem());

							popup(new SoldierDiedState(
													(*j)->getName(),
													(*i)->getName()));

							(*j)->die(_gameSave); // holy * This copies the Diary-object
							// so to delete Soldier-instance I need to use a CopyConstructor
							// on either or both of SoldierDiary and SoldierAward.
							// Oh, and maybe an operator= assignment overload also.
							// Learning C++ is like standing around while 20 people constantly
							// throw cow's dung at you. (But don't mention "const" or they'll throw
							// twice as fast.) i miss you, Alan Turing ....

							delete *j;
							j = (*i)->getSoldiers()->erase(j);

							dead = true;
						}
					}
				}

				if (dead == false)
					(*j)->heal();
			}

			if (dead == false)
				++j;
		}

		if ((*i)->getAvailablePsiLabs() != 0) // handle psi-training and AutoStat.
		{
			bool sortSoldiers = false;

			for (std::vector<Soldier*>::const_iterator
					j = (*i)->getSoldiers()->begin();
					j != (*i)->getSoldiers()->end();
					++j)
			{
				if ((*j)->trainPsiDay() == true)
				{
					(*j)->autoStat();
					sortSoldiers = true;
				}
//				(*j)->calcStatString(_rules->getStatStrings(), Options::psiStrengthEval && _gameSave->isResearched(_rules->getPsiRequirements()));
			}

			if (sortSoldiers == true)
				(*i)->sortSoldiers();
		}


		for (std::vector<BaseFacility*>::const_iterator // handle facility construction
				j = (*i)->getFacilities()->begin();
				j != (*i)->getFacilities()->end();
				++j)
		{
			if ((*j)->buildFinished() == false
				&& (*j)->buildFacility() == true) // completed.
			{
				if (prodEvents.empty() == false) // set the previous event to NOT show btn.
					prodEvents.back().gotoBaseBtn = false;

				prodEvents.push_back(ProductionCompleteInfo(
														*i,
														tr((*j)->getRules()->getType()),
														true,
														PROGRESS_CONSTRUCTION));
			}
		}

		std::vector<ResearchProject*> projectsComplete; // handle science projects - Research.
		for (std::vector<ResearchProject*>::const_iterator
				j = (*i)->getResearch().begin();
				j != (*i)->getResearch().end();
				++j)
		{
			if ((*j)->stepProject() == true)
				projectsComplete.push_back(*j);
		}

		if (projectsComplete.empty() == false)
			resetTimer();


		for (std::vector<ResearchProject*>::const_iterator
				j = projectsComplete.begin();
				j != projectsComplete.end();
				++j)
		{
			const RuleResearch* const resRule ((*j)->getRules());
			const std::string& resType (resRule->getType());

			const bool liveAlien (_rules->getUnit(resType) != NULL);
			(*i)->removeResearch(*j, liveAlien == true); // interrogation of aLien Unit complete.

			if (liveAlien == true // if the live alien is "researched" its corpse is sent to stores.
				&& resRule->needsItem() == true
				&& Options::spendResearchedItems == true)
			{
				(*i)->getStorageItems()->addItem(_rules->getArmor(_rules->getUnit(resType)->getArmor())->getCorpseGeoscape());
				// ;) <- kL_note: heh i noticed that.
			}

			bool
				gofCrack,
				unlocksCrack;

			if (liveAlien == true)
				getAlienCracks(
							resType,
							gofCrack,
							unlocksCrack);
			else
			{
				gofCrack = true;
				unlocksCrack = true;
			}

			const RuleResearch* gofRule (NULL);
			if (gofCrack == true
				&& resRule->getGetOneFree().empty() == false)
			{
				std::vector<std::string> gofChoices;
				for (std::vector<std::string>::const_iterator
						k = resRule->getGetOneFree().begin();
						k != resRule->getGetOneFree().end();
						++k)
				{
					if (std::find(
							_gameSave->getDiscoveredResearch().begin(),
							_gameSave->getDiscoveredResearch().end(),
							_rules->getResearch(*k)) == _gameSave->getDiscoveredResearch().end())
					{
						gofChoices.push_back(*k);
					}
				}

				if (gofChoices.empty() == false)
				{
					gofRule = _rules->getResearch(gofChoices.at(RNG::pick(gofChoices.size())));
					_gameSave->addFinishedResearch(gofRule);

					if (gofRule->getLookup().empty() == false)
						_gameSave->addFinishedResearch(_rules->getResearch(gofRule->getLookup()));
				}
			}


			std::string lookUp = resRule->getLookup(); // UfoPaedia article.
			if (lookUp.empty() == true)
				lookUp = resType;

			const RuleResearch* resRule0;
			if (_gameSave->isResearched(lookUp) == false)
				resRule0 = resRule;
			else
				resRule0 = NULL;


			_gameSave->addFinishedResearch(resRule); // this adds the research project to _discovered vector.

			if (resRule->getLookup().empty() == false)
				_gameSave->addFinishedResearch(_rules->getResearch(resRule->getLookup()));

			resEvents.push_back(new ResearchCompleteState(resRule0, gofRule));


			std::vector<const RuleResearch*> newResearchPossible;
			_gameSave->getDependentResearch(
										newResearchPossible,
										resRule,
										*i);

			for (std::vector<const RuleResearch*>::const_iterator	// -> moved here from NewPossibleResearchState cTor.
					k = newResearchPossible.begin();
					k != newResearchPossible.end();
					)
			{
				if ((*k)->getCost() == 0							// no fake projects pls.
					|| _gameSave->wasResearchPopped(*k) == true		// do not show twice.
					|| _rules->getUnit((*k)->getType()) != NULL)	// and no aLiens ->
				{
					k = newResearchPossible.erase(k);
				}
				else ++k;
			}

			std::vector<const RuleManufacture*> newManufacturePossible;
			_gameSave->getDependentManufacture(
											newManufacturePossible,
											resRule);

			if (resRule0 != NULL) // check for need to research the clip before the weapon itself is allowed to be manufactured.
			{
				const RuleItem* const itRule (_rules->getItem(resRule0->getType()));
				if (itRule != NULL
					&& itRule->getBattleType() == BT_FIREARM
					&& itRule->getCompatibleAmmo()->empty() == false)
				{
					const RuleManufacture* const manufRule (_rules->getManufacture(itRule->getType()));
					if (manufRule != NULL
						&& manufRule->getRequirements().empty() == false)
					{
						const std::vector<std::string>& prereqs (manufRule->getRequirements());
						const RuleItem* const aRule (_rules->getItem(itRule->getCompatibleAmmo()->front()));
						if (aRule != NULL
							&& std::find(
									prereqs.begin(),
									prereqs.end(),
									aRule->getType()) != prereqs.end()
							&& _gameSave->isResearched(manufRule->getRequirements()) == false)
						{
							resEvents.push_back(new ResearchRequiredState(itRule));
						}
					}
				}
			}

			if (newResearchPossible.empty() == false)
			{
				if (newResEvents.empty() == false) // only show the "allocate research" button for the last notification
					newResEvents.back().showResearchButton = false;

				newResEvents.push_back(NewPossibleResearchInfo(
															*i,
															newResearchPossible,
															true));
			}

			if (newManufacturePossible.empty() == false)
			{
				if (newProdEvents.empty() == false) // only show the "allocate production" button for the last notification
					newProdEvents.back().showManufactureButton = false;

				newProdEvents.push_back(NewPossibleManufactureInfo(
															*i,
															newManufacturePossible,
															true));
			}

			for (std::vector<Base*>::const_iterator // iterate through all the bases and remove this completed project from their labs
					k = _gameSave->getBases()->begin();
					k != _gameSave->getBases()->end();
					++k)
			{
				for (std::vector<ResearchProject*>::const_iterator
						l = (*k)->getResearch().begin();
						l != (*k)->getResearch().end();
						++l)
				{
					if (resType == (*l)->getRules()->getType()
						&& _rules->getUnit((*l)->getRules()->getType()) == NULL)
					{
						(*k)->removeResearch(*l, false);
						break;
					}
				}
			}

			delete *j; // DONE Research.
		}
	}

	// if research has been completed but no new research events are triggered show an empty
	// NewPossibleResearchState so players have a chance to allocate the now-free scientists.
	// kL_note: already taken care of. Just reset timer to 5sec and let ResearchCompleteState poke the player.
/*	if (resEvents.empty() == false && newResEvents.empty() == true)
		newResEvents.push_back(NewPossibleResearchInfo(std::vector<const RuleResearch*>(), true)); */

	// show events
	for (std::vector<ProductionCompleteInfo>::const_iterator
			j = prodEvents.begin();
			j != prodEvents.end();
			)
	{
		popup(new ProductionCompleteState(
									j->base,
									j->item,
									this,
									j->gotoBaseBtn,
									j->endType));
		j = prodEvents.erase(j);
	}

	for (std::vector<State*>::const_iterator
			j = resEvents.begin();
			j != resEvents.end();
			)
	{
		popup(*j);
		j = resEvents.erase(j);
	}

	for (std::vector<NewPossibleResearchInfo>::const_iterator
			j = newResEvents.begin();
			j != newResEvents.end();
			)
	{
		popup(new NewPossibleResearchState(
									j->base,
									j->newPossibleResearch,
									j->showResearchButton));
		j = newResEvents.erase(j);
	}

	for (std::vector<NewPossibleManufactureInfo>::const_iterator
			j = newProdEvents.begin();
			j != newProdEvents.end();
			)
	{
		popup(new NewPossibleManufactureState(
										j->base,
										j->newPossibleManufacture,
										j->showManufactureButton));
		j = newProdEvents.erase(j);
	}



	const RuleAlienMission* const missionRule (_rules->getRandomMission( // handle regional and country points for alien bases
																	alm_BASE,
																	_game->getSavedGame()->getMonthsPassed()));
	const int aLienPts ((missionRule->getPoints() * (static_cast<int>(_gameSave->getDifficulty()) + 1)) / 100);
	if (aLienPts != 0)
	{
		for (std::vector<AlienBase*>::const_iterator
				i = _gameSave->getAlienBases()->begin();
				i != _gameSave->getAlienBases()->end();
				++i)
		{
			_gameSave->scorePoints(
								(*i)->getLongitude(),
								(*i)->getLatitude(),
								aLienPts,
								true);
		}
	}


	std::for_each( // handle supply of alien bases.
			_gameSave->getAlienBases()->begin(),
			_gameSave->getAlienBases()->end(),
			GenerateSupplyMission(*_rules, *_gameSave));

	// Autosave 3 times a month. kL_note: every day.
//	int day = _gameSave->getTime()->getDay();
//	if (day == 10 || day == 20)
//	{
	if (_gameSave->isIronman() == true)
		popup(new SaveGameState(OPT_GEOSCAPE, SAVE_IRONMAN, _palette));
	else if (Options::autosave == true)
		popup(new SaveGameState(OPT_GEOSCAPE, SAVE_AUTO_GEOSCAPE, _palette));
//	}
	//Log(LOG_INFO) << "GeoscapeState::time1Day() EXIT";
}

/**
 * Assigns whether an aLien cracked under pressure.
 * @param alienType		-
 * @param gofCrack		-
 * @param unlockCrack	-
 */
void GeoscapeState::getAlienCracks( // private.
			const std::string& alienType,
			bool& gof,
			bool& unlocks) const
{
	int
		gofPct (100),
		unlocksPct (100); // defaults.

	if (alienType.find("_TERRORIST") != std::string::npos)
	{
		gofPct = 10;
		unlocksPct = 50;
	}
	else if (alienType.find("_FLOATER") != std::string::npos)
	{
		gofPct = 80;
		unlocksPct = 30;
	}
	else if (alienType.find("_SECTOID") != std::string::npos)
	{
		gofPct = 70;
		unlocksPct = 40;
	}
	else if (alienType.find("_SNAKEMAN") != std::string::npos)
	{
		gofPct = 60;
		unlocksPct = 50;
	}
	else if (alienType.find("_MUTON") != std::string::npos)
	{
		gofPct = 50;
		unlocksPct = 60;
	}
	else if (alienType.find("_ETHEREAL") != std::string::npos)
	{
		gofPct = 40;
		unlocksPct = 70;
	}
	else if (alienType.find("_WASPITE") != std::string::npos)
	{
		gofPct = 30;
		unlocksPct = 80;
	}

	gof = RNG::percent(gofPct);
	unlocks = RNG::percent(unlocksPct);
}

/**
 * Takes care of any game logic that has to run every game month.
 */
void GeoscapeState::time1Month()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Month()";
	resetTimer();
	_gameSave->addMonth();

	determineAlienMissions(); // determine alien mission for this month.

/*	const int monthsPassed = _gameSave->getMonthsPassed();
//	if (monthsPassed > 5)
	if (RNG::percent(monthsPassed * 2) == true)
		determineAlienMissions(); // kL_note: determine another one, I guess.

	setupLandMission(); // always add a Mission, eg. TerrorMission, to the regular mission(s) <-

	const int diff = static_cast<int>(_gameSave->getDifficulty());

	bool newRetaliation = false;
//	if (monthsPassed > 13 - static_cast<int>(_gameSave->getDifficulty())
	if (RNG::percent((monthsPassed * diff) + 3) == true
		|| _gameSave->isResearched("STR_THE_MARTIAN_SOLUTION") == true)
	{
		newRetaliation = true;
	} */

/*	for (std::vector<Base*>::const_iterator // initiate a new retaliation mission if applicable
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end();
			++i)
	{
		if (newRetaliation == true)
		{
			for (std::vector<Region*>::const_iterator
					j = _gameSave->getRegions()->begin();
					j != _gameSave->getRegions()->end();
					++j)
			{
				if ((*j)->getRules()->insideRegion(
												(*i)->getLongitude(),
												(*i)->getLatitude()) == true)
				{
					if (_gameSave->findAlienMission(
												(*j)->getRules()->getType(),
												alm_RETAL) == false)
					{
						const RuleAlienMission& missionRule = *_rules->getRandomMission(
																					alm_RETAL,
																					_game->getSavedGame()->getMonthsPassed());
						AlienMission* const mission = new AlienMission(
																	missionRule,
																	*_gameSave);
						mission->setId(_gameSave->getId("ALIEN_MISSIONS"));
						mission->setRegion(
										(*j)->getRules()->getType(),
										*_rules);

//						int race = RNG::generate(
//											0,
//											static_cast<int>(
//													_rules->getAlienRacesList().size())
//												- 2); // -2 to avoid "MIXED" race
//						mission->setRace(_rules->getAlienRacesList().at(race));
						// get races for retaliation missions
						std::vector<std::string> raceList = _rules->getAlienRacesList();
						for (std::vector<std::string>::const_iterator
								k = raceList.begin();
								k != raceList.end();
								)
						{
							if (_rules->getAlienRace(*k)->canRetaliate() == true)
								++k;
							else
								k = raceList.erase(k);
						}

						const size_t race = static_cast<size_t>(RNG::generate(
																		0,
																		static_cast<int>(raceList.size()) - 1));
						mission->setRace(raceList[race]);
						mission->start(150);
						_gameSave->getAlienMissions().push_back(mission);

						newRetaliation = false;
					}

					break;
				}
			}
		}
	} */

	_gameSave->monthlyFunding(); // handle Funding
	_game->getResourcePack()->fadeMusic(_game, 1232);

	popup(new MonthlyReportState());


	// handle Xcom Operatives discovering bases
	// NOTE: might want to change this to time1day() ...
	if (_gameSave->getAlienBases()->empty() == false)
	{
		const int pct (20 - (static_cast<int>(_gameSave->getDifficulty()) * 5));
		if (RNG::percent(pct + 50) == true)
		{
			for (std::vector<AlienBase*>::const_iterator
					i = _gameSave->getAlienBases()->begin();
					i != _gameSave->getAlienBases()->end();
					++i)
			{
				if ((*i)->isDiscovered() == false
					&& RNG::percent(pct + 5) == true)
				{
					(*i)->setDiscovered(true);
					popup(new AlienBaseState(*i, this)); // NOTE: multiple popups may glitch.
				}
			}
		}
	}
}

/**
 * Slows down the timer back to minimum speed for when important events occur.
 */
void GeoscapeState::resetTimer()
{
	_globe->rotateStop();

	SDL_Event ev;
	ev.button.button = SDL_BUTTON_LEFT;
	Action act(
			&ev,
			_game->getScreen()->getXScale(),
			_game->getScreen()->getYScale(),
			_game->getScreen()->getCursorTopBlackBand(),
			_game->getScreen()->getCursorLeftBlackBand());

	_btn5Secs->mousePress(&act, this);
}

/**
 * Gets if time compression is set to 5 second intervals.
 * @return, true if time compression is set to 5 seconds
 */
bool GeoscapeState::is5Sec() const
{
	return (_timeComp == _btn5Secs);
}

/**
 * Adds a new popup window to the queue and pauses the game timer respectively.
 * @note Doing it this way this prevents popups from overlapping.
 * @param state - pointer to popup state
 */
void GeoscapeState::popup(State* state)
{
	_pause = true;
	_popups.push_back(state);
}

/**
 * Returns a pointer to the Geoscape globe for access by other substates.
 * @return, pointer to Globe
 */
Globe* GeoscapeState::getGlobe() const
{
	return _globe;
}

/**
 * Processes any left-clicks on globe markers or right-clicks to scroll the globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::globeClick(Action* action)
{
	const int
		mX (static_cast<int>(std::floor(action->getAbsoluteXMouse()))),
		mY (static_cast<int>(std::floor(action->getAbsoluteYMouse())));

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		const std::vector<Target*> targets (_globe->getTargets(mX,mY, false));
		if (targets.empty() == false)
			_game->pushState(new MultipleTargetsState(targets, NULL, this));
	}

	if (_gameSave->getDebugMode() == true)
	{
		_gameSave->setDebugArg("COORD");	// tells think() to stop writing area-info and display lon/lat instead.
		_debug = "";						// ditto

		double
			lonRad,
			latRad;
		_globe->cartToPolar(
						static_cast<Sint16>(mX),
						static_cast<Sint16>(mY),
						&lonRad, &latRad);

		const double
			lonDeg (lonRad / M_PI * 180.),
			latDeg (latRad / M_PI * 180.);
		int
			texture,
			shade;
		_globe->getPolygonTextureAndShade(lonDeg, latDeg, &texture, &shade);

		std::wostringstream woststr;
		woststr << std::fixed << std::setprecision(3)
			<< L"RAD Lon " << lonRad << L"  Lat " << latRad
			<< std::endl
			<< L"DEG Lon " << lonDeg << L"  Lat " << latDeg
			<< std::endl
			<< L"texture " << texture << L" shade " << shade;

		_txtDebug->setText(woststr.str());
	}
}

/**
 * Opens the Intercept window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnInterceptClick(Action*)
{
	_game->pushState(new InterceptState(NULL, this));
}

/**
 * Goes to the Basescape screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnBasesClick(Action*)
{
	kL_soundPop->play(Mix_GroupAvailable(0));
	resetTimer();

	if (_gameSave->getBases()->empty() == false)
	{
		if (kL_curBase == 0
			|| kL_curBase >= _gameSave->getBases()->size())
		{
			_game->pushState(new BasescapeState(
											_gameSave->getBases()->front(),
											_globe));
		}
		else
			_game->pushState(new BasescapeState(
											_gameSave->getBases()->at(kL_curBase),
											_globe));
	}
	else
		_game->pushState(new BasescapeState(NULL, _globe));
}

/**
 * Goes to the Graphs screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnGraphsClick(Action*)
{
	kL_soundPop->play(Mix_GroupAvailable(0));

	resetTimer();
	_game->pushState(new GraphsState(_gameSave->getCurrentGraph()));
}

/**
 * Goes to the Ufopaedia window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnUfopaediaClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 276);

	resetTimer();
	Ufopaedia::open(_game);

	_game->getResourcePack()->playMusic(
									OpenXcom::res_MUSIC_UFOPAEDIA,
									"", 1);
}

/**
 * Opens the Options window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnOptionsClick(Action*)
{
//	_dfStartTimer,
//	_dfTimer;
	if (_dfZoomInTimer->isRunning() == false
		&& _dfZoomOutTimer->isRunning() == false)
	{
		resetTimer();
		_game->pushState(new PauseState(OPT_GEOSCAPE));
	}
}

/**
 * Goes to the Funding screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnFundingClick(Action*)
{
	resetTimer();
	_game->pushState(new FundingState());
}

/**
 * Handler for clicking the Detail area.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnDetailPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_globe->toggleDetail();
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		_globe->toggleRadarLines();
}

/**
 * Starts rotating the globe to the left.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftPress(Action*)
{
	_globe->rotateLeft();
}

/**
 * Starts rotating the globe to the right.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightPress(Action*)
{
	_globe->rotateRight();
}

/**
 * Stops rotating the globe horizontally.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLonStop(Action*)
{
	_globe->rotateStopLon();
}

/**
 * Starts rotating the globe upwards.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateUpPress(Action*)
{
	_globe->rotateUp();
}

/**
 * Starts rotating the globe downwards.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateDownPress(Action*)
{
	_globe->rotateDown();
}

/**
 * Stops rotating the globe vertically.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLatStop(Action*)
{
	_globe->rotateStopLat();
}

/**
 * Starts rotating the globe to the left up.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftUpPress(Action*)
{
	_globe->rotateLeft();
	_globe->rotateUp();
}

/**
 * Starts rotating the globe to the left down.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftDownPress(Action*)
{
	_globe->rotateLeft();
	_globe->rotateDown();
}

/**
 * Starts rotating the globe to the right up.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightUpPress(Action*)
{
	_globe->rotateRight();
	_globe->rotateUp();
}

/**
 * Starts rotating the globe to the right down.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightDownPress(Action*)
{
	_globe->rotateRight();
	_globe->rotateDown();
}

/**
 * Stops rotating the globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateStop(Action*)
{
	_globe->rotateStop();
}

/**
 * Zooms into the globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnZoomInLeftClick(Action*)
{
	_globe->zoomIn();
}

/**
 * Zooms out of the globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnZoomOutLeftClick(Action*)
{
	_globe->zoomOut();
}

/*
 * Zooms the globe maximum.
 * @param action - pointer to an Action
 *
void GeoscapeState::btnZoomInRightClick(Action*)
{
	_globe->zoomMax();
} */

/*
 * Zooms the globe minimum.
 * @param action - pointer to an Action
 *
void GeoscapeState::btnZoomOutRightClick(Action*)
{
	_globe->zoomMin();
} */

/**
 * Gets the Timer for dogfight zoom-ins.
 * @return, pointer to the zoom-in Timer
 */
Timer* GeoscapeState::getDfZoomInTimer() const
{
	return _dfZoomInTimer;
}

/**
 * Gets the Timer for dogfight zoom-outs.
 * @return, pointer to the zoom-out Timer
 */
Timer* GeoscapeState::getDfZoomOutTimer() const
{
	return _dfZoomOutTimer;
}

/**
 * Zoom in effect for dogfights.
 */
void GeoscapeState::dfZoomIn()
{
	if (_globe->zoomDogfightIn() == true)
	{
		_dfZoomInDone = true;
		_dfZoomInTimer->stop();
	}
}

/**
 * Zoom out effect for dogfights.
 */
void GeoscapeState::dfZoomOut()
{
	if (_globe->zoomDogfightOut() == true)
	{
		_dfZoomOutDone = true;
		_dfZoomOutTimer->stop();

		if (_dfCenterCurrentCoords == true)
		{
			_dfCenterCurrentCoords = false;
			_globe->center(
						_dfCCC_lon,
						_dfCCC_lat);
		}
		else
			_globe->center(
						_gameSave->getDfLongitude(),
						_gameSave->getDfLatitude());

		init();
	}
}

/**
 * Stores current Globe coordinates and zoom before a dogfight.
 */
void GeoscapeState::storePreDfCoords()
{
	_gameSave->setDfLongitude(_gameSave->getGlobeLongitude());
	_gameSave->setDfLatitude(_gameSave->getGlobeLatitude());

	_gameSave->setDfZoom(_globe->getZoom());
}

/**
 * Sets the zoom-out timer to ignore stored pre-Dogfight coordinates and use
 * current coordinates of the Dogfight instead.
 * @note Used only if UFO is breaking off from its last dogfight.
 */
void GeoscapeState::setDfCCC(
		double lon,
		double lat)
{
	_dfCenterCurrentCoords = true;
	_dfCCC_lon = lon;
	_dfCCC_lat = lat;
}

/**
 * Gets whether the zoom-out timer should ignore stored pre-Dogfight coordinates
 * and use current coordinates of the Dogfight instead.
 * @return, true if UFO is set to break off from its dogfight
 */
bool GeoscapeState::getDfCCC() const
{
	return _dfCenterCurrentCoords;
}

/**
 * Gets the number of minimized dogfights.
 * @return, number of minimized dogfights
 */
size_t GeoscapeState::getMinimizedDfCount() const
{
	size_t ret (0);
	for (std::list<DogfightState*>::const_iterator
			i = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		if ((*i)->isMinimized() == true)
			++ret;
	}

	return ret;
}

/**
 * Dogfight logic.
 */
void GeoscapeState::thinkDogfights()
{
	std::list<DogfightState*>::const_iterator pDf (_dogfights.begin());
	for (
			;
			pDf != _dogfights.end();
			++pDf)
	{
		(*pDf)->getUfo()->setTimerTicked(false);
	}


	_dfMinimized = 0;
	bool resetPorts = false;

	pDf = _dogfights.begin();
	while (pDf != _dogfights.end())
	{
		if ((*pDf)->isMinimized() == true)
			++_dfMinimized;
//		else _globe->rotateStop();

		(*pDf)->think();

		if ((*pDf)->dogfightEnded() == true)
		{
			if ((*pDf)->isMinimized() == true)
				--_dfMinimized;

			//std::string st1 = (*pDf)->getCraft()->getRules()->getType();
			//std::ostringstream oststr;
			//oststr << st1 << "-" << ((*pDf)->getCraft()->getId());
			//Log(LOG_INFO) << "geo thinkDf DELETE " << oststr.str().c_str();

			delete *pDf;
			pDf = _dogfights.erase(pDf);
			resetPorts = true;
		}
		else
			++pDf;
	}

	if (_dogfights.empty() == true)
	{
		_dfTimer->stop();

		if (_dfZoomOut == true) // or if UFO just landed, reset dfCoords to current globe position: DO NOT ZOOM OUT
			_dfZoomOutTimer->start();
		else // STOP INTERCEPTION MUSIC. Start Geo music ...
		{
			if (_game->getResourcePack()->isMusicPlaying(res_MUSIC_GEO_TERROR_SPLASH) == false) // unless a Mission/TerrorSite just popped
			{
				_game->getResourcePack()->fadeMusic(_game, 425);
				_game->getResourcePack()->playMusic(res_MUSIC_GEO_GLOBE);
			}
		}
	}
	else if (resetPorts == true)
		resetInterceptPorts();

	_dfZoomOut = true;
}

/**
 * Starts a new dogfight.
 */
void GeoscapeState::startDogfight() // private.
{
	if (_globe->getZoom() < _globe->getZoomLevels() - 1)
	{
		if (_dfZoomInTimer->isRunning() == false)
		{
			_dfZoomInTimer->start();
//			_globe->rotateStop();
		}
	}
	else
	{
		_dfStartTimer->stop();
		_dfZoomInTimer->stop();
		_dfTimer->start();

		resetTimer();

		while (_dogfightsToStart.empty() == false)
		{
			_dogfights.push_back(_dogfightsToStart.back());
			_dogfightsToStart.pop_back();

			_dogfights.back()->setInterceptSlot(getOpenDfSlot());
		}

		resetInterceptPorts(); // set window positions for all dogfights
	}
}

/**
 * Updates total current interceptions quantity in all Dogfights and repositions
 * their view windows accordingly.
 */
void GeoscapeState::resetInterceptPorts()
{
	const size_t dfQty = _dogfights.size();
	for (std::list<DogfightState*>::const_iterator
			i = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		(*i)->setTotalIntercepts(dfQty);
	}

	const size_t dfOpenTotal (dfQty - getMinimizedDfCount());
	size_t dfOpen (0);
	for (std::list<DogfightState*>::const_iterator
			i = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		if ((*i)->isMinimized() == false)
			++dfOpen;

		(*i)->resetInterceptPort(dfOpen, dfOpenTotal); // set window position for dogfight
	}
}

/**
 * Gets the first free dogfight slot available.
 * @return, the next slot open
 */
size_t GeoscapeState::getOpenDfSlot() const
{
	size_t slot (1);
	for (std::list<DogfightState*>::const_iterator
			i = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		if ((*i)->getInterceptSlot() == slot)
			++slot;
	}
	return slot;
}

/**
 * Gets the dogfights.
 * @return, reference to a list of pointers to DogfightStates
 */
std::list<DogfightState*>& GeoscapeState::getDogfights()
{
	return _dogfights;
}

/**
 * Handle base defense.
 * @param base	- pointer to Base to defend
 * @param ufo	- pointer to the attacking Ufo
 */
void GeoscapeState::handleBaseDefense(
		Base* const base,
		Ufo* const ufo)
{
	ufo->setUfoStatus(Ufo::DESTROYED);

	if (base->getAvailableSoldiers(true) != 0)
	{
		SavedBattleGame* const battle = new SavedBattleGame(&_rules->getOperations());
		_gameSave->setBattleSave(battle);
		battle->setTacticalType("STR_BASE_DEFENSE");

		BattlescapeGenerator bGen = BattlescapeGenerator(_game);
		bGen.setBase(base);
		bGen.setAlienRace(ufo->getAlienRace());
		bGen.run();

		_pause = true;
		popup(new BriefingState(NULL, base));
	}
	else
		popup(new BaseDestroyedState(base, _globe));
}

/**
 * Determine the alien missions to start this month.
 */
void GeoscapeState::determineAlienMissions() // private.
{
	AlienStrategy& strategy (_gameSave->getAlienStrategy());
	const int month (_game->getSavedGame()->getMonthsPassed());
	std::vector<RuleMissionScript*> availableMissions;
	std::map<int, bool> conditions;

	RuleMissionScript* directive;

	for (std::vector<std::string>::const_iterator
			i = _rules->getMissionScriptList()->begin();
			i != _rules->getMissionScriptList()->end();
			++i)
	{
		directive = _rules->getMissionScript(*i);

		if (directive->getFirstMonth() <= month
			&& (directive->getLastMonth() >= month
				|| directive->getLastMonth() == -1)
			&& (directive->getMaxRuns() == -1
				|| directive->getMaxRuns() > strategy.getMissionsRun(directive->getVarName()))
			&& directive->getMinDifficulty() <= _gameSave->getDifficulty())
		{
			bool go (true);
			for (std::map<std::string, bool>::const_iterator
					j = directive->getResearchTriggers().begin();
					j != directive->getResearchTriggers().end()
						&& go == true;
					++j)
			{
				go = (_gameSave->isResearched(j->first) == j->second);
			}

			if (go == true)
				availableMissions.push_back(directive);
		}
	}


	for (std::vector<RuleMissionScript*>::const_iterator
			i = availableMissions.begin();
			i != availableMissions.end();
			++i)
	{
		directive = *i;
		bool
			process (true),
			success (false);

		for (std::vector<int>::const_iterator
				j = directive->getConditionals().begin();
				j != directive->getConditionals().end()
					&& process == true;
				++j)
		{
			std::map<int, bool>::const_iterator found (conditions.find(std::abs(*j)));
			process = (found == conditions.end()
				   || (found->second == true && *j > 0)
				   || (found->second == false && *j < 0));
		}

		if (directive->getLabel() > 0
			&& conditions.find(directive->getLabel()) != conditions.end())
		{
			std::stringstream ststr;
			ststr << "Mission generator encountered an error: multiple commands: ["
				  << directive->getType() << "] and ";
			for (std::vector<RuleMissionScript*>::const_iterator
					j = availableMissions.begin();
					j != availableMissions.end();
					++j)
			{
				if (*j != *i
					&& directive->getLabel() == (*j)->getLabel())
				{
					ststr << "["
						  << (*j)->getType()
						  << "]";
				}
			}
			ststr << " are sharing the same label: ["
				  << directive->getLabel()
				  << "]";
			throw Exception(ststr.str());
		}

		if (process == true
			&& RNG::percent(directive->getExecutionOdds()) == true)
		{
			success = processCommand(directive);
		}

		if (directive->getLabel() > 0)
		{
			if (conditions.find(directive->getLabel()) != conditions.end())
			{
				throw Exception("Error in mission scripts: " + directive->getType()
								+ ". Two or more commands share the same label.");
			}
			conditions[directive->getLabel()] = success;
		}
	}
}

/**
 * Proccesses a directive to start up a mission if possible.
 * @param directive - the directive from which to read information
 * @return, true if the command successfully produced a new mission
 */
bool GeoscapeState::processCommand(RuleMissionScript* const directive) // private.
{
	AlienStrategy& strategy (_gameSave->getAlienStrategy());
	const int month (_game->getSavedGame()->getMonthsPassed());
	const RuleAlienMission* missionRule;
	std::string
		targetRegion,
		missionType,
		raceType;
	size_t targetZone (std::numeric_limits<size_t>::max()); // darn vc++ linker warning ...

	if (directive->getSiteType() == true)
	{
		missionType = directive->genMissionDatum(month, GT_MISSION);
		const std::vector<std::string> missions (directive->getMissionTypes(month));
		size_t
			missionsTotal (missions.size()),
			testMission (0);

		for (
				;
				testMission != missionsTotal;
				++testMission)
		{
			if (missions[testMission] == missionType)
				break;
		}

		std::vector<std::pair<std::string, size_t> > validAreas;

		for (size_t
				i = 0;
				i != missionsTotal;
				++i)
		{
			missionRule = _rules->getAlienMission(missionType);
			targetZone = missionRule->getSpawnZone();

			std::vector<std::string> regions;
			if (directive->hasRegionWeights() == true)
				regions = directive->getRegions(month);
			else
				regions = _rules->getRegionsList();

			for (std::vector<std::string>::const_iterator
					j = regions.begin();
					j != regions.end();
					)
			{
				bool processThisRegion = true;
				for (std::vector<AlienMission*>::const_iterator
						k = _gameSave->getAlienMissions().begin();
						k != _gameSave->getAlienMissions().end();
						++k)
				{
					if ((*k)->getRules().getType() == missionRule->getType()
						&& (*k)->getRegion() == *j)
					{
						processThisRegion = false;
						break;
					}
				}

				if (processThisRegion == true)
				{
					const RuleRegion* const regionRule (_rules->getRegion(*j));
					if (regionRule->getMissionZones().size() > targetZone)
					{
						size_t testZone (0);
						const std::vector<MissionArea> areas (regionRule->getMissionZones()[targetZone].areas);
						for (std::vector<MissionArea>::const_iterator
								k = areas.begin();
								k != areas.end();
								++k)
						{
							if ((*k).isPoint() == true
								&& strategy.validateMissionLocation(
																directive->getVarName(),
																regionRule->getType(),
																testZone) == true)
							{
								validAreas.push_back(std::make_pair(
																regionRule->getType(),
																testZone));
							}
							++testZone;
						}
					}
					++j;
				}
				else
					j = regions.erase(j);
			}

			if (validAreas.empty() == true)
			{
				if (missionsTotal > 1 && ++testMission == missionsTotal)
					testMission = 0;

				missionType = missions[testMission];
			}
			else
				break;
		}

		if (validAreas.empty() == true)
			return false;


		targetZone = std::numeric_limits<size_t>::max();
		while (targetZone == std::numeric_limits<size_t>::max())
		{
			if (directive->hasRegionWeights() == true)
				targetRegion = directive->genMissionDatum(month, GT_REGION);
			else
				targetRegion = _rules->getRegionsList().at(RNG::pick(_rules->getRegionsList().size()));

			int
				low (-1),
				high (-1), // avoid vc++ linker warning.
				testArea (0);

			for (std::vector<std::pair<std::string, size_t> >::const_iterator
					i = validAreas.begin();
					i != validAreas.end();
					++i)
			{
				if ((*i).first == targetRegion)
				{
					if (low == -1) low = testArea;
					high = testArea;
				}
				else if (low > -1)
					break;

				++testArea;
			}

			if (low != -1)
				targetZone = validAreas[static_cast<size_t>(RNG::generate(low, high))].second;
		}

		strategy.addMissionLocation(
								directive->getVarName(),
								targetRegion,
								targetZone,
								directive->getRepeatAvoidance());
	}
	else if (RNG::percent(directive->getTargetBaseOdds()) == true)
	{
		std::vector<std::string> regionsMaster;
		for (std::vector<Base*>::const_iterator
				i = _gameSave->getBases()->begin();
				i != _gameSave->getBases()->end();
				++i)
		{
			regionsMaster.push_back(_gameSave->locateRegion(**i)->getRules()->getType());
		}

		std::vector<std::string> types (directive->getMissionTypes(month));
		if (types.empty() == true)
		{
			for (std::vector<std::string>::const_iterator
					i = regionsMaster.begin();
					i != regionsMaster.end();
					)
			{
				if (strategy.validateMissionRegion(*i) == true)
					++i;
				else
					i = regionsMaster.erase(i);
			}

			if (regionsMaster.empty() == true)
				return false;


			targetRegion = regionsMaster[RNG::pick(regionsMaster.size())];
		}
		else
		{
			std::vector<std::string> regions;
			size_t
				typesTotal (types.size()),
				id (RNG::pick(typesTotal));

			for (size_t
					i = 0;
					i != typesTotal;
					++i)
			{
				regions = regionsMaster;

				for (std::vector<AlienMission*>::const_iterator
						j = _gameSave->getAlienMissions().begin();
						j != _gameSave->getAlienMissions().end();
						++j)
				{
					if (types[id] == (*j)->getRules().getType())
					{
						for (std::vector<std::string>::const_iterator
								k = regions.begin();
								k != regions.end();
								)
						{
							if (*k != (*j)->getRegion())
								++k;
							else
								k = regions.erase(k);
						}
					}
				}

				if (regions.empty() == false)
				{
					missionType = types[id];
					targetRegion = regions[RNG::pick(regions.size())];
					break;
				}

				if (typesTotal > 1 && ++id == typesTotal)
					id = 0;
			}
		}
	}
	else if (directive->hasRegionWeights() == false)
		targetRegion = strategy.chooseRandomRegion(_rules);
	else
		targetRegion = directive->genMissionDatum(month, GT_REGION);

	if (targetRegion.empty() == true)
		return false;


	if (_rules->getRegion(targetRegion) == NULL)
	{
		throw Exception("Error proccessing mission script named: "
						+ directive->getType()
						+ ", region named: " + targetRegion
						+ " is not defined");
	}

	if (missionType.empty() == true)
	{
		if (directive->hasMissionWeights() == false)
			missionType = strategy.chooseRandomMission(targetRegion);
		else
			missionType = directive->genMissionDatum(month, GT_MISSION);
	}

	if (missionType.empty() == true)
		return false;


	missionRule = _rules->getAlienMission(missionType);

	if (missionRule == NULL)
	{
		throw Exception("Error proccessing mission script named: "
						+ directive->getType()
						+ ", mission type: " + missionType +
						" is not defined");
	}

	if (directive->hasRaceWeights() == false)
		raceType = missionRule->generateRace(month);
	else
		raceType = directive->genMissionDatum(month, GT_RACE);

	if (_rules->getAlienRace(raceType) == NULL)
	{
		throw Exception("Error proccessing mission script named: "
						+ directive->getType()
						+ ", race: " + raceType
						+ " is not defined");
	}


	AlienMission* const mission = new AlienMission(
												*missionRule,
												*_gameSave);
	mission->setRace(raceType);
	mission->setId(_gameSave->getCanonicalId("ALIEN_MISSIONS"));
	mission->setRegion(targetRegion, *_rules);
	mission->setMissionSiteZone(targetZone);
	strategy.addMissionRun(directive->getVarName());
	mission->start(directive->getDelay());

	_gameSave->getAlienMissions().push_back(mission);

	if (directive->usesTable() == true)
		strategy.removeMission(
							targetRegion,
							missionType);

	return true;
}

/*
 * Determine the alien missions to start each month.
 * @note In the vanilla game a terror mission plus one other are started in
 * random regions. The very first mission is Sectoid Research in the region of
 * player's first Base.
 * @param atGameStart - true if called at start (default false)
 *
void GeoscapeState::determineAlienMissions(bool atGameStart) // private.
{
	if (atGameStart == false)
	{
		//
		// One randomly selected mission.
		//
		AlienStrategy& strategy = _gameSave->getAlienStrategy();
		const std::string& region = strategy.chooseRandomRegion(_rules);
		const std::string& mission = strategy.chooseRandomMission(region);

		// Choose race for this mission.
		const RuleAlienMission& missionRule = *_rules->getAlienMission(mission);
		const std::string& race = missionRule.generateRace(_gameSave->getMonthsPassed());

		AlienMission* const alienMission = new AlienMission(
														missionRule,
														*_gameSave);
		alienMission->setId(_gameSave->getId("ALIEN_MISSIONS"));
		alienMission->setRegion(
							region,
							*_rules);
		alienMission->setRace(race);
		alienMission->start();

		_gameSave->getAlienMissions().push_back(alienMission);

		// Make sure this combination never comes up again.
		strategy.removeMission(
							region,
							mission);
	}
	else
	{
		//
		// Sectoid Research at base's region. haha
		//
		AlienStrategy& strategy = _gameSave->getAlienStrategy();
		const std::string region = _gameSave->locateRegion(*_gameSave->getBases()->front())->getRules()->getType();

		// Choose race for this mission.
		const std::string mission = _rules->getAlienMissionList().front();
		const RuleAlienMission& missionRule = *_rules->getAlienMission(mission);

		AlienMission* const alienMission = new AlienMission(
														missionRule,
														*_gameSave);
		alienMission->setId(_gameSave->getId("ALIEN_MISSIONS"));
		alienMission->setRegion(
							region,
							*_rules);
		const std::string sectoid = missionRule.getTopRace(_gameSave->getMonthsPassed());
		alienMission->setRace(sectoid);
		alienMission->start(150);

		_gameSave->getAlienMissions().push_back(alienMission);

		// Make sure this combination never comes up again.
		strategy.removeMission(
							region,
							mission);
	}
} */

/*
 * Sets up a land mission. Eg TERROR!!!
 *
void GeoscapeState::setupLandMission() // private.
{
	const RuleAlienMission& missionRule = *_rules->getRandomMission(
																alm_SITE,
																_game->getSavedGame()->getMonthsPassed());

	// Determine a random region with a valid mission zone and no mission already running.
	const RuleRegion* regRule = NULL; // avoid VC++ linker warning.
	bool picked = false;
	const std::vector<std::string> regionsList = _rules->getRegionsList();

	// Try 40 times to pick a valid zone for a land mission.
	for (int
			i = 0;
			i != 40
				&& picked == false;
			++i)
	{
		regRule = _rules->getRegion(regionsList[static_cast<size_t>(RNG::generate(
																			0,
																			static_cast<int>(regionsList.size()) - 1))]);
		if (regRule->getMissionZones().size() > missionRule.getSpawnZone()
			&& _game->getSavedGame()->findAlienMission(
													regRule->getType(),
													alm_SITE) == NULL)
		{
			const MissionZone& zone = regRule->getMissionZones().at(missionRule.getSpawnZone());
			for (std::vector<MissionArea>::const_iterator
					j = zone.areas.begin();
					j != zone.areas.end()
						&& picked == false;
					++j)
			{
				if (j->isPoint() == true)
					picked = true;
			}
		}
	}

	// Choose race for land mission.
	if (picked == true) // safety.
	{
		AlienMission* const mission = new AlienMission(
													missionRule,
													*_gameSave);
		mission->setId(_game->getSavedGame()->getId("ALIEN_MISSIONS"));
		mission->setRegion(
						regRule->getType(),
						*_rules);
		const std::string& race = missionRule.generateRace(static_cast<size_t>(_game->getSavedGame()->getMonthsPassed()));
		mission->setRace(race);
		mission->start(150);

		_game->getSavedGame()->getAlienMissions().push_back(mission);
	}
} */

/**
 * Handler for hot-keying a time-compression key.
 * @note The 0xC_kL build does not accept mouse-clicks here; now that I look at
 * it neither does the vanilla code.
 * @param action - pointer to the hot-key Action
 */
void GeoscapeState::keyTimeCompressionPress(Action* action) // private.
{
	//Log(LOG_INFO) << "reset time cache KEY";
	if (action->getSender() != _timeComp)
	{
		//Log(LOG_INFO) << ". accepted";
		_timeCache = 0;

		SDL_Event ev; // so let's fake a mouse-click
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;

		Action act = Action(&ev, 0.,0., 0,0);
		action->getSender()->mousePress(&act, this);
	}
}

/**
 * Handler for clicking a time-compression button.
 * @param action - pointer to an Action
 */
void GeoscapeState::resetTimeCacheClick(Action* /* action */) // private.
{
	//Log(LOG_INFO) << "reset time cache BTN";
//	if (action->getSender() != _timeComp) //action->getDetails()->button.button == SDL_BUTTON_LEFT
//	{
		//Log(LOG_INFO) << ". accepted";
	_timeCache = 0;
//	}
}

/**
 * Pauses and unpauses the Geoscape.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnPauseClick(Action*) // private.
{
	_pauseHard = !_pauseHard;
	_globe->toggleBlink();
}

/**
 * LMB Centers on the UFO corresponding to this button.
 * RMB Opens info.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnUfoBlobPress(Action* action) // private.
{
	for (size_t // find out which button was pressed
			i = 0;
			i != UFO_HOTBLOBS;
			++i)
	{
		if (_isfUfoBlobs[i] == action->getSender())
		{
			if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
				_globe->center(
							_hostileUfos[i]->getLongitude(),
							_hostileUfos[i]->getLatitude());
			else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
				_game->pushState(new UfoDetectedState(
												_hostileUfos[i],
												this,
												false,
												_hostileUfos[i]->getHyperDetected()));
			break;
		}
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Updates the scale.
 * @param dX - reference to delta of X
 * @param dY - reference to delta of Y
 */
void GeoscapeState::resize(
		int& dX,
		int& dY)
{
	if (_gameSave->getBattleSave() != NULL)
		return;

	dX = Options::baseXResolution;
	dY = Options::baseYResolution;

	int divisor = 1;
	double pixelRatioY = 1.;

	if (Options::nonSquarePixelRatio)
		pixelRatioY = 1.2;

	switch (Options::geoscapeScale)
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
			dX =
			dY = 0;
		return;
	}

	Options::baseXResolution = std::max(
									Screen::ORIGINAL_WIDTH,
									Options::displayWidth / divisor);
	Options::baseYResolution = std::max(
									Screen::ORIGINAL_HEIGHT,
									static_cast<int>(static_cast<double>(Options::displayHeight)
									/ pixelRatioY / static_cast<double>(divisor)));

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;

	_globe->resize();

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (*i != _globe)
		{
			(*i)->setX((*i)->getX() + dX);
			(*i)->setY((*i)->getY() + dY / 2);
		}
	}
}

/**
 * Examines the quantity of remaining UFO-detected popups.
 * @note Reduces the number by one and decides whether to display the value.
 */
void GeoscapeState::assessUfoPopups()
{
	if (--_windowPops == 0)
		_ufoDetected->setVisible(false);
	else
		_ufoDetected->setText(Text::intWide(static_cast<int>(_windowPops)));
}

/**
 * Sets pause.
 */
void GeoscapeState::setPaused()
{
	_pauseHard = true;
	_globe->toggleBlink();
}

/**
 * Gets pause.
 * @return, true if state is paused
 */
bool GeoscapeState::getPaused() const
{
	return _pauseHard;
}

}
