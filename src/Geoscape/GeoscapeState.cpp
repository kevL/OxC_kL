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

//#define _USE_MATH_DEFINES

#include "GeoscapeState.h"

//#include <assert.h>

//#include <algorithm>
//#include <cmath>
//#include <ctime>
//#include <functional>
//#include <iomanip>
//#include <sstream>

//#include "../fmath.h"

#include "AlienBaseState.h"
#include "AlienTerrorState.h"
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
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h" // in PCH.
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/ImageButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/ListSaveState.h"
#include "../Menu/LoadGameState.h"
#include "../Menu/PauseState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienRace.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/City.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleManufacture.h"
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
#include "../Savegame/Production.h"
#include "../Savegame/Region.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

size_t kL_currentBase = 0;

const double
	earthRadius					= 3440., //.0647948164,			// nautical miles.
	unitToRads					= (1. / 60.) * (M_PI / 180.),	// converts a minute of arc to rads
	greatCircleConversionFactor	= earthRadius * unitToRads;		// converts 'flat' distance to greatCircle distance.

Sound* GeoscapeState::soundPop = 0;


// myk002_begin: struct definitions used when enqueuing notification events
struct ProductionCompleteInfo
{
	bool showGotoBaseButton;
	std::wstring item;

	ProductProgress endType;

	/// cTor.
	ProductionCompleteInfo(
			const std::wstring& a_item,
			bool a_showGotoBaseButton,
			ProductProgress a_endType)
		:
			item(a_item),
			showGotoBaseButton(a_showGotoBaseButton),
			endType(a_endType)
	{
	}
};

struct NewPossibleResearchInfo
{
	bool showResearchButton;
	std::vector<RuleResearch*> newPossibleResearch;

	/// cTor.
	NewPossibleResearchInfo(
			const std::vector<RuleResearch*>& a_newPossibleResearch,
			bool a_showResearchButton)
		:
			newPossibleResearch(a_newPossibleResearch),
			showResearchButton(a_showResearchButton)
	{
	}
};

struct NewPossibleManufactureInfo
{
	bool showManufactureButton;
	std::vector<RuleManufacture*> newPossibleManufacture;

	/// cTor.
	NewPossibleManufactureInfo(
			const std::vector<RuleManufacture*>& a_newPossibleManufacture,
			bool a_showManufactureButton)
		:
			newPossibleManufacture(a_newPossibleManufacture),
			showManufactureButton(a_showManufactureButton)
	{
	}
}; // myk002_end.


/**
 * Initializes all the elements in the Geoscape screen.
 */
GeoscapeState::GeoscapeState()
	:
		_pause(false),
		_dfZoomInDone(false),
		_dfZoomOutDone(false),
		_minimizedDogfights(0),
		_dfLon(0.),
		_dfLat(0.),
		_day(-1),
		_month(-1),
		_year(-1),
		_dogfightEnded(false),
		_startMusic(false)
{
	_savedGame = _game->getSavedGame();

	const int
		screenWidth		= Options::baseXGeoscape,
		screenHeight	= Options::baseYGeoscape;

/*	_bg		= new Surface(
						320,
						200,
						screenWidth - 320,
						screenHeight / 2 - 100); */
/*	Surface* hd	= _game->getResourcePack()->getSurface("ALTGEOBORD.SCR");
	_bg			= new Surface(
						hd->getWidth(),
						hd->getHeight(),
						0,
						0); */
	_sideLine	= new Surface(
						64,
						screenHeight,
						screenWidth - 64,
						0);
/*	_sidebar	= new Surface(
						64,
						200,
						screenWidth - 64,
						screenHeight / 2 - 100); */

/*	_srfSpace	= new Surface( // kL
//							screenWidth - 64,
//							screenHeight,
							256,
							200,
							32,
							12); */
	_srfSpace	= new Surface(480, 270, 0, 0); // kL
	_globe		= new Globe(
						_game,					// GLOBE:
						(screenWidth - 64) / 2,	// center_x	= 160 pixels
						screenHeight / 2,		// center_y	= 120
						screenWidth - 64,		// x_width	= 320
						screenHeight,			// y_height	= 120
						0,						// start_x
						0);						// start_y
																	// BACKGROUND
//kL	_bg->setX((_globe->getWidth() - _bg->getWidth()) / 2);		// (160 - 768) / 2	= -304	= x
//kL	_bg->setY((_globe->getHeight() - _bg->getHeight()) / 2);	// (120 - 600) / 2	= -240	= y

/*kL
	_btnIntercept	= new TextButton(63, 11, screenWidth-63, screenHeight/2-100);
	_btnBases		= new TextButton(63, 11, screenWidth-63, screenHeight/2-88);
	_btnGraphs		= new TextButton(63, 11, screenWidth-63, screenHeight/2-76);
	_btnUfopaedia	= new TextButton(63, 11, screenWidth-63, screenHeight/2-64);
	_btnOptions		= new TextButton(63, 11, screenWidth-63, screenHeight/2-52);
	_btnFunding		= new TextButton(63, 11, screenWidth-63, screenHeight/2-40);

	_btn5Secs		= new TextButton(31, 13, screenWidth-63, screenHeight/2+12);
	_btn1Min		= new TextButton(31, 13, screenWidth-31, screenHeight/2+12);
	_btn5Mins		= new TextButton(31, 13, screenWidth-63, screenHeight/2+26);
	_btn30Mins		= new TextButton(31, 13, screenWidth-31, screenHeight/2+26);
	_btn1Hour		= new TextButton(31, 13, screenWidth-63, screenHeight/2+40);
	_btn1Day		= new TextButton(31, 13, screenWidth-31, screenHeight/2+40); */

	// kL_begin: revert to ImageButtons.
	_btnIntercept	= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 100);
	_btnBases		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 88);
	_btnGraphs		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 76);
	_btnUfopaedia	= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 64);
	_btnOptions		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 52);
	_btnFunding		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 40);
//	_btnOptions		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 100);
//	_btnUfopaedia	= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 88);
//	_btnFunding		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 76);
//	_btnGraphs		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 64);
//	_btnIntercept	= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 52);
//	_btnBases		= new ImageButton(63, 11, screenWidth - 63, screenHeight / 2 - 40); // change the GeoGraphic first ....

	_btn5Secs		= new ImageButton(31, 13, screenWidth - 63, screenHeight / 2 + 12);
	_btn1Min		= new ImageButton(31, 13, screenWidth - 31, screenHeight / 2 + 12);
	_btn5Mins		= new ImageButton(31, 13, screenWidth - 63, screenHeight / 2 + 26);
	_btn30Mins		= new ImageButton(31, 13, screenWidth - 31, screenHeight / 2 + 26);
	_btn1Hour		= new ImageButton(31, 13, screenWidth - 63, screenHeight / 2 + 40);
	_btn1Day		= new ImageButton(31, 13, screenWidth - 31, screenHeight / 2 + 40);
//	_btn5Secs		= new ImageButton(31, 13, screenWidth - 63, screenHeight / 2 + 12);
//	_btn1Min		= new ImageButton(31, 13, screenWidth - 63, screenHeight / 2 + 26);
//	_btn5Mins		= new ImageButton(31, 13, screenWidth - 63, screenHeight / 2 + 40);
//	_btn30Mins		= new ImageButton(31, 13, screenWidth - 31, screenHeight / 2 + 12);
//	_btn1Hour		= new ImageButton(31, 13, screenWidth - 31, screenHeight / 2 + 26);
//	_btn1Day		= new ImageButton(31, 13, screenWidth - 31, screenHeight / 2 + 40);

	// The old rotate buttons have now become the Detail & Radar toggles.
	_btnDetail		= new ImageButton(63, 46, screenWidth - 63, screenHeight / 2 + 54);
	// kL_end.

/*kL
	_btnRotateLeft	= new InteractiveSurface(12, 10, screenWidth-61, screenHeight/2+76);
	_btnRotateRight	= new InteractiveSurface(12, 10, screenWidth-37, screenHeight/2+76);
	_btnRotateUp	= new InteractiveSurface(13, 12, screenWidth-49, screenHeight/2+62);
	_btnRotateDown	= new InteractiveSurface(13, 12, screenWidth-49, screenHeight/2+87);
	_btnZoomIn		= new InteractiveSurface(23, 23, screenWidth-25, screenHeight/2+56);
	_btnZoomOut		= new InteractiveSurface(13, 17, screenWidth-20, screenHeight/2+82); */

/*kL
	int height = (screenHeight - Screen::ORIGINAL_HEIGHT) / 2 + 10;
	_sideTop	= new TextButton(
							63,
							height,
							screenWidth - 63,
							_sidebar->getY() - height - 1);
	_sideBottom	= new TextButton(
							63,
							height,
							screenWidth - 63,
							_sidebar->getY() + _sidebar->getHeight() + 1); */
/*	int height = ((screenHeight - Screen::ORIGINAL_HEIGHT) / 2) - 1;
	_btnTop		= new TextButton(
							63,
							height,
							screenWidth - 63,
							0);
	_btnBottom	= new TextButton(
							63,
							height,
							screenWidth - 63,
							screenHeight - height + 1); */ // kL

/*kL
	_txtHour		= new Text(20, 17, screenWidth - 61, screenHeight / 2 - 26);
	_txtHourSep		= new Text(4, 17, screenWidth - 41, screenHeight / 2 - 26);
	_txtMin			= new Text(20, 17, screenWidth - 37, screenHeight / 2 - 26);
	_txtMinSep		= new Text(4, 17, screenWidth - 17, screenHeight / 2 - 26);
	_txtSec			= new Text(11, 8, screenWidth - 13, screenHeight / 2 - 20); */

	_srfTime	= new Surface(63, 39, screenWidth - 63, screenHeight / 2 - 28);

/*	_txtHour	= new Text(19, 17, screenWidth - 54, screenHeight / 2 - 26);
	_txtHourSep	= new Text(5,  17, screenWidth - 35, screenHeight / 2 - 26);
	_txtMin		= new Text(19, 17, screenWidth - 30, screenHeight / 2 - 26); */
//kL	_txtHour	= new Text(19, 17, screenWidth - 61, screenHeight / 2 - 26);
//kL	_txtHourSep	= new Text(5,  17, screenWidth - 42, screenHeight / 2 - 26);
//kL	_txtMin		= new Text(19, 17, screenWidth - 37, screenHeight / 2 - 26);
//	_txtMinSep	= new Text(5,  17, screenWidth - 18, screenHeight / 2 - 26);
//	_txtSec		= new Text(10, 17, screenWidth - 13, screenHeight / 2 - 26);
	_txtHour	= new Text(19, 17, screenWidth - 54, screenHeight / 2 - 22);
	_txtHourSep	= new Text(5,  17, screenWidth - 35, screenHeight / 2 - 22);
	_txtMin		= new Text(19, 17, screenWidth - 30, screenHeight / 2 - 22);
	_txtSec		= new Text(6, 9, screenWidth - 8, screenHeight / 2 - 26);

/*	_txtWeekday	= new Text(59, 8, screenWidth - 61, screenHeight / 2 - 13);
	_txtDay		= new Text(29, 8, screenWidth - 61, screenHeight / 2 - 6);
	_txtMonth	= new Text(29, 8, screenWidth - 32, screenHeight / 2 - 6);
	_txtYear	= new Text(59, 8, screenWidth - 61, screenHeight / 2 + 1); */
//	_txtWeekday	= new Text(59, 8, screenWidth - 61, screenHeight / 2 - 13);
//	_txtDay		= new Text(12, 16, screenWidth - 61, screenHeight / 2 - 5);
//	_txtMonth	= new Text(21, 16, screenWidth - 49, screenHeight / 2 - 5);
//	_txtYear	= new Text(27, 16, screenWidth - 28, screenHeight / 2 - 5);
//	_txtDate	= new Text(60, 8, screenWidth - 62, screenHeight / 2 - 5);

	_srfDay1		= new Surface(3, 8, screenWidth - 45, screenHeight / 2 - 3);
	_srfDay2		= new Surface(3, 8, screenWidth - 41, screenHeight / 2 - 3);
	_srfMonth1		= new Surface(3, 8, screenWidth - 34, screenHeight / 2 - 3);
	_srfMonth2		= new Surface(3, 8, screenWidth - 30, screenHeight / 2 - 3);
	_srfYear1		= new Surface(3, 8, screenWidth - 23, screenHeight / 2 - 3);
	_srfYear2		= new Surface(3, 8, screenWidth - 19, screenHeight / 2 - 3);

	_txtFunds = new Text(63, 8, screenWidth - 64, screenHeight / 2 - 110); // kL
	if (Options::showFundsOnGeoscape == true)
	{
		_txtFunds = new Text(59, 8, screenWidth - 61, screenHeight / 2 - 27);
		_txtHour->		setY(_txtHour->		getY() + 6);
		_txtHourSep->	setY(_txtHourSep->	getY() + 6);
		_txtMin->		setY(_txtMin->		getY() + 6);
//kL	_txtMinSep->	setY(_txtMinSep->	getY() + 6);
//kL	_txtMinSep->	setX(_txtMinSep->	getX() - 10);
//kL	_txtSec->		setX(_txtSec->		getX() - 10);
	}

	_timeSpeed = _btn5Secs;

	_gameTimer		= new Timer(Options::geoClockSpeed);
	_dfZoomInTimer	= new Timer(Options::dogfightSpeed + 10);
	_dfZoomOutTimer	= new Timer(Options::dogfightSpeed + 10);
	_dfStartTimer	= new Timer(Options::dogfightSpeed + 10);

	_txtDebug		= new Text(320, 18, 0, 0);

	setPalette("PAL_GEOSCAPE");

	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);

	add(_srfSpace);	// kL
//	add(_bg);
	add(_sideLine);
//	add(_sidebar);
	add(_globe);

	add(_btnDetail);
//	add(_btnFake);

	add(_btnIntercept);
	add(_btnBases);
	add(_btnGraphs);
	add(_btnUfopaedia);
	add(_btnOptions);
	add(_btnFunding);

	add(_btn5Secs);
	add(_btn1Min);
	add(_btn5Mins);
	add(_btn30Mins);
	add(_btn1Hour);
	add(_btn1Day);

/*kL
	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut); */

//kL	add(_sideTop);
//kL	add(_sideBottom);

	add(_srfTime); // kL

//	if (Options::showFundsOnGeoscape)
	add(_txtFunds);

	add(_txtHour);
	add(_txtHourSep);
	add(_txtMin);
//	add(_txtMinSep);
	add(_txtSec);
//	add(_txtDate);
	add(_srfDay1);
	add(_srfDay2);
	add(_srfMonth1);
	add(_srfMonth2);
	add(_srfYear1);
	add(_srfYear2);
//	add(_txtWeekday);
//	add(_txtDay);
//	add(_txtMonth);
//	add(_txtYear);

	add(_txtDebug);

	_game->getResourcePack()->getSurface("Cygnus_BG")->blit(_srfSpace);			// kL
//	_game->getResourcePack()->getSurface("Antares_BG")->blit(_srfSpace);		// kL

//	_game->getResourcePack()->getSurface("LGEOBORD.SCR")->blit(_srfSpace);		// kL
//	_game->getResourcePack()->getSurface("ALTGEOBORD.SCR")->blit(_srfSpace);	// kL
//	_game->getResourcePack()->getSurface("GEOBORD.SCR")->blit(_bg);				// kL

/*	Surface* geobord = _game->getResourcePack()->getSurface("GEOBORD.SCR");
	geobord->setX(_sidebar->getX() - geobord->getWidth() + _sidebar->getWidth());
	geobord->setY(_sidebar->getY());
	_sidebar->copy(geobord);
	_game->getResourcePack()->getSurface("ALTGEOBORD.SCR")->blit(_bg); */

	_sideLine->drawRect(
					0,
					0,
					_sideLine->getWidth(),
					_sideLine->getHeight(),
					15);
/*kL
	_btnIntercept->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnIntercept->setColor(Palette::blockOffset(15)+6);
	_btnIntercept->setTextColor(Palette::blockOffset(15)+5);
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick((ActionHandler)& GeoscapeState::btnInterceptClick);
	_btnIntercept->onKeyboardPress((ActionHandler)& GeoscapeState::btnInterceptClick, Options::keyGeoIntercept);

	_btnBases->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnBases->setColor(Palette::blockOffset(15)+6);
	_btnBases->setTextColor(Palette::blockOffset(15)+5);
	_btnBases->setText(tr("STR_BASES"));
	_btnBases->onMouseClick((ActionHandler)& GeoscapeState::btnBasesClick);
	_btnBases->onKeyboardPress((ActionHandler)& GeoscapeState::btnBasesClick, Options::keyGeoBases);

	_btnGraphs->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnGraphs->setColor(Palette::blockOffset(15)+6);
	_btnGraphs->setTextColor(Palette::blockOffset(15)+5);
	_btnGraphs->setText(tr("STR_GRAPHS"));
	_btnGraphs->onMouseClick((ActionHandler)& GeoscapeState::btnGraphsClick);
	_btnGraphs->onKeyboardPress((ActionHandler)& GeoscapeState::btnGraphsClick, Options::keyGeoGraphs);

	_btnUfopaedia->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnUfopaedia->setColor(Palette::blockOffset(15)+6);
	_btnUfopaedia->setTextColor(Palette::blockOffset(15)+5);
	_btnUfopaedia->setText(tr("STR_UFOPAEDIA_UC"));
	_btnUfopaedia->onMouseClick((ActionHandler)& GeoscapeState::btnUfopaediaClick);
	_btnUfopaedia->onKeyboardPress((ActionHandler)& GeoscapeState::btnUfopaediaClick, Options::keyGeoUfopedia);

	_btnOptions->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnOptions->setColor(Palette::blockOffset(15)+6);
	_btnOptions->setTextColor(Palette::blockOffset(15)+5);
	_btnOptions->setText(tr("STR_OPTIONS_UC"));
	_btnOptions->onMouseClick((ActionHandler)& GeoscapeState::btnOptionsClick);
	_btnOptions->onKeyboardPress((ActionHandler)& GeoscapeState::btnOptionsClick, Options::keyGeoOptions);

	_btnFunding->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnFunding->setColor(Palette::blockOffset(15)+6);
	_btnFunding->setTextColor(Palette::blockOffset(15)+5);
	_btnFunding->setText(tr("STR_FUNDING_UC"));
	_btnFunding->onMouseClick((ActionHandler)& GeoscapeState::btnFundingClick);
	_btnFunding->onKeyboardPress((ActionHandler)& GeoscapeState::btnFundingClick, Options::keyGeoFunding);

	_btn5Secs->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn5Secs->setBig();
	_btn5Secs->setColor(Palette::blockOffset(15)+6);
	_btn5Secs->setTextColor(Palette::blockOffset(15)+5);
	_btn5Secs->setText(tr("STR_5_SECONDS"));
	_btn5Secs->setGroup(&_timeSpeed);
	_btn5Secs->onKeyboardPress((ActionHandler)& GeoscapeState::btnTimerClick, Options::keyGeoSpeed1);

	_btn1Min->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Min->setBig();
	_btn1Min->setColor(Palette::blockOffset(15)+6);
	_btn1Min->setTextColor(Palette::blockOffset(15)+5);
	_btn1Min->setText(tr("STR_1_MINUTE"));
	_btn1Min->setGroup(&_timeSpeed);
	_btn1Min->onKeyboardPress((ActionHandler)& GeoscapeState::btnTimerClick, Options::keyGeoSpeed2);

	_btn5Mins->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn5Mins->setBig();
	_btn5Mins->setColor(Palette::blockOffset(15)+6);
	_btn5Mins->setTextColor(Palette::blockOffset(15)+5);
	_btn5Mins->setText(tr("STR_5_MINUTES"));
	_btn5Mins->setGroup(&_timeSpeed);
	_btn5Mins->onKeyboardPress((ActionHandler)& GeoscapeState::btnTimerClick, Options::keyGeoSpeed3);

	_btn30Mins->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn30Mins->setBig();
	_btn30Mins->setColor(Palette::blockOffset(15)+6);
	_btn30Mins->setTextColor(Palette::blockOffset(15)+5);
	_btn30Mins->setText(tr("STR_30_MINUTES"));
	_btn30Mins->setGroup(&_timeSpeed);
	_btn30Mins->onKeyboardPress((ActionHandler)& GeoscapeState::btnTimerClick, Options::keyGeoSpeed4);

	_btn1Hour->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Hour->setBig();
	_btn1Hour->setColor(Palette::blockOffset(15)+6);
	_btn1Hour->setTextColor(Palette::blockOffset(15)+5);
	_btn1Hour->setText(tr("STR_1_HOUR"));
	_btn1Hour->setGroup(&_timeSpeed);
	_btn1Hour->onKeyboardPress((ActionHandler)& GeoscapeState::btnTimerClick, Options::keyGeoSpeed5);

	_btn1Day->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Day->setBig();
	_btn1Day->setColor(Palette::blockOffset(15)+6);
	_btn1Day->setTextColor(Palette::blockOffset(15)+5);
	_btn1Day->setText(tr("STR_1_DAY"));
	_btn1Day->setGroup(&_timeSpeed);
	_btn1Day->onKeyboardPress((ActionHandler)& GeoscapeState::btnTimerClick, Options::keyGeoSpeed6); */

	// kL_begin: GeoscapeState() revert to ImageButtons.
	Surface* const geobord = _game->getResourcePack()->getSurface("GEOBORD.SCR");
	geobord->setX(screenWidth - geobord->getWidth());
	geobord->setY((screenHeight - geobord->getHeight()) / 2);

	_btnIntercept->copy(geobord);
	_btnIntercept->setColor(Palette::blockOffset(15)+5);
	_btnIntercept->onMouseClick((ActionHandler)& GeoscapeState::btnInterceptClick);
	_btnIntercept->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnInterceptClick,
					Options::keyGeoIntercept);

	_btnBases->copy(geobord);
	_btnBases->setColor(Palette::blockOffset(15)+5);
	_btnBases->onMouseClick((ActionHandler)& GeoscapeState::btnBasesClick);
	_btnBases->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnBasesClick,
					Options::keyGeoBases);

	_btnGraphs->copy(geobord);
	_btnGraphs->setColor(Palette::blockOffset(15)+5);
	_btnGraphs->onMouseClick((ActionHandler)& GeoscapeState::btnGraphsClick);
	_btnGraphs->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnGraphsClick,
					Options::keyGeoGraphs);

	_btnUfopaedia->copy(geobord);
	_btnUfopaedia->setColor(Palette::blockOffset(15)+5);
	_btnUfopaedia->onMouseClick((ActionHandler)& GeoscapeState::btnUfopaediaClick);
	_btnUfopaedia->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnUfopaediaClick,
					Options::keyGeoUfopedia);

	_btnOptions->copy(geobord);
	_btnOptions->setColor(Palette::blockOffset(15)+5);
	_btnOptions->onMouseClick((ActionHandler)& GeoscapeState::btnOptionsClick);
	_btnOptions->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnOptionsClick,
					Options::keyGeoOptions);

	_btnFunding->copy(geobord);
	_btnFunding->setColor(Palette::blockOffset(15)+5);
	_btnFunding->onMouseClick((ActionHandler)& GeoscapeState::btnFundingClick);
	_btnFunding->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnFundingClick,
					Options::keyGeoFunding);


	_srfTime->copy(geobord);

	_btn5Secs->copy(geobord);
	_btn5Secs->setColor(Palette::blockOffset(15)+5);
	_btn5Secs->setGroup(&_timeSpeed);
	_btn5Secs->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnTimerClick,
					Options::keyGeoSpeed1);

	_btn1Min->copy(geobord);
	_btn1Min->setColor(Palette::blockOffset(15)+5);
	_btn1Min->setGroup(&_timeSpeed);
	_btn1Min->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnTimerClick,
					Options::keyGeoSpeed2);

	_btn5Mins->copy(geobord);
	_btn5Mins->setColor(Palette::blockOffset(15)+5);
	_btn5Mins->setGroup(&_timeSpeed);
	_btn5Mins->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnTimerClick,
					Options::keyGeoSpeed3);

	_btn30Mins->copy(geobord);
	_btn30Mins->setColor(Palette::blockOffset(15)+5);
	_btn30Mins->setGroup(&_timeSpeed);
	_btn30Mins->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnTimerClick,
					Options::keyGeoSpeed4);

	_btn1Hour->copy(geobord);
	_btn1Hour->setColor(Palette::blockOffset(15)+5);
	_btn1Hour->setGroup(&_timeSpeed);
	_btn1Hour->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnTimerClick,
					Options::keyGeoSpeed5);

	_btn1Day->copy(geobord);
	_btn1Day->setColor(Palette::blockOffset(15)+5);
	_btn1Day->setGroup(&_timeSpeed);
	_btn1Day->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnTimerClick,
					Options::keyGeoSpeed6);


	_btnDetail->copy(geobord);
	_btnDetail->setColor(Palette::blockOffset(15)+9);
	_btnDetail->onMousePress((ActionHandler)& GeoscapeState::btnDetailPress);
//	_btnDetail->onKeyboardPress((ActionHandler)& GeoscapeState::btnDetailPress, keyGeoToggleDetail);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateLeftPress,
					Options::keyGeoLeft);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateLeftRelease,
					Options::keyGeoLeft);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateRightPress,
					Options::keyGeoRight);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateRightRelease,
					Options::keyGeoRight);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateUpPress,
					Options::keyGeoUp);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateUpRelease,
					Options::keyGeoUp);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnRotateDownPress,
					Options::keyGeoDown);
	_btnDetail->onKeyboardRelease(
					(ActionHandler)& GeoscapeState::btnRotateDownRelease,
					Options::keyGeoDown);

	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnZoomInLeftClick,
					Options::keyGeoZoomIn);
	_btnDetail->onKeyboardPress(
					(ActionHandler)& GeoscapeState::btnZoomOutLeftClick,
					Options::keyGeoZoomOut);
	// kL_end.
/*kL
	_btnRotateLeft->onMousePress((ActionHandler)& GeoscapeState::btnRotateLeftPress);
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

//kL	_sideTop->setColor(Palette::blockOffset(15)+6);
//kL	_sideBottom->setColor(Palette::blockOffset(15)+6);

	_txtFunds->setColor(Palette::blockOffset(15)+4);	// kL
	_txtFunds->setAlign(ALIGN_CENTER);					// kL
	if (Options::showFundsOnGeoscape == true)
	{
		_txtFunds->setSmall();
		_txtFunds->setColor(Palette::blockOffset(15)+4);
		_txtFunds->setAlign(ALIGN_CENTER);

		_txtHour->setSmall();
		_txtHourSep->setSmall();
		_txtMin->setSmall();
//kL	_txtMinSep->setSmall();
	}
	else						// kL
	{
		_txtHour->setBig();		// kL
		_txtHourSep->setBig();	// kL
		_txtMin->setBig();		// kL
//kL	_txtMinSep->setBig();	// kL
	}

//kL	if (Options::showFundsOnGeoscape) _txtHour->setSmall(); else _txtHour->setBig();

	_txtHour->setColor(Palette::blockOffset(15)+2);
	_txtHour->setAlign(ALIGN_RIGHT);

//kL	if (Options::showFundsOnGeoscape) _txtHourSep->setSmall(); else _txtHourSep->setBig();
	_txtHourSep->setColor(Palette::blockOffset(15)+2);
	_txtHourSep->setText(L":");

//kL	if (Options::showFundsOnGeoscape) _txtMin->setSmall(); else _txtMin->setBig();
	_txtMin->setColor(Palette::blockOffset(15)+2);

//kL	if (Options::showFundsOnGeoscape) _txtMinSep->setSmall(); else _txtMinSep->setBig();
//	_txtMinSep->setColor(Palette::blockOffset(15)+2);
//	_txtMinSep->setText(L".");

//	_txtSec->setSmall();
//kL	_txtSec->setBig();
	_txtSec->setText(L".");
	_txtSec->setColor(Palette::blockOffset(15)+2);

//	_txtDate->setColor(Palette::blockOffset(15)+2);
//	_txtDate->setAlign(ALIGN_CENTER);

//	_txtWeekday->setSmall();
//	_txtWeekday->setColor(Palette::blockOffset(15)+2);
//	_txtWeekday->setAlign(ALIGN_CENTER);

//	_txtDay->setBig();
//	_txtDay->setColor(Palette::blockOffset(15)+2);
//	_txtDay->setAlign(ALIGN_CENTER);

//	_txtMonth->setBig();
//	_txtMonth->setColor(Palette::blockOffset(15)+2);
//	_txtMonth->setAlign(ALIGN_CENTER);

//	_txtYear->setBig();
//	_txtYear->setColor(Palette::blockOffset(15)+2);
//	_txtYear->setAlign(ALIGN_CENTER);

	_txtDebug->setColor(Palette::blockOffset(15)+4);

	_gameTimer->onTimer((StateHandler)& GeoscapeState::timeAdvance);
	_gameTimer->start();

	_dfZoomInTimer->onTimer((StateHandler)& GeoscapeState::dfZoomIn);
	_dfZoomOutTimer->onTimer((StateHandler)& GeoscapeState::dfZoomOut);
	_dfStartTimer->onTimer((StateHandler)& GeoscapeState::startDogfight);

	timeDisplay();
}

/**
 * Deletes timers.
 */
GeoscapeState::~GeoscapeState()
{
	delete _gameTimer;
	delete _dfZoomInTimer;
	delete _dfZoomOutTimer;
	delete _dfStartTimer;

	std::list<DogfightState*>::const_iterator i = _dogfights.begin();
	for (
			;
			i != _dogfights.end();
			)
	{
		delete *i;
		i = _dogfights.erase(i);
	}

	for (
			i = _dogfightsToStart.begin();
			i != _dogfightsToStart.end();
			)
	{
		delete *i;
		i = _dogfightsToStart.erase(i);
	}
}

/**
 * Handle blitting of Geoscape and Dogfights.
 */
void GeoscapeState::blit()
{
	State::blit();

/*	_game->getScreen()->getSurface()->drawLine(
											Options::baseXGeoscape - 64, // kL
											0,
											Options::baseXGeoscape - 64, // kL
											_game->getScreen()->getSurface()->getHeight(),
											15);
	_game->getScreen()->getSurface()->drawLine(
											Options::baseXGeoscape - 63, // kL
											Options::baseYGeoscape / 2 - 101, // kL
											_game->getScreen()->getSurface()->getWidth(),
											Options::baseYGeoscape / 2 - 101, // kL
											15);
	_game->getScreen()->getSurface()->drawLine(
											Options::baseXGeoscape - 63, // kL
											Options::baseYGeoscape / 2 + 100, // kL
											_game->getScreen()->getSurface()->getWidth(),
											Options::baseYGeoscape / 2 + 100, // kL
											15); */

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
	if (_dogfights.size() == _minimizedDogfights)
		State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		if (Options::debug == true)
		{
			if ((SDL_GetModState() & KMOD_CTRL) != 0)
			{
				if (action->getDetails()->key.keysym.sym == SDLK_d)	// "ctrl-d" - enable debug mode
																	// ctrl+c is also handled in Game::run() where the 'cycle' is determined.
				{
					_savedGame->setDebugMode();

					if (_savedGame->getDebugMode() == true)
					{
						_debug = "DEBUG MODE : ";

						if (_globe->getDebugType() == 0)
							_debug += "country : ";
						else if (_globe->getDebugType() == 1)
							_debug += "region : ";
						else if (_globe->getDebugType() == 2)
							_debug += "missionZones : ";
/*						if (_globe->getDebugType() == 0)
							_txtDebug->setText(L"DEBUG MODE : countries");
						else if (_globe->getDebugType() == 1)
							_txtDebug->setText(L"DEBUG MODE : regions");
						else if (_globe->getDebugType() == 2)
							_txtDebug->setText(L"DEBUG MODE : missionZones"); */
					}
					else
					{
						_txtDebug->setText(L"");
						_debug = "";
					}
				}
				else if (action->getDetails()->key.keysym.sym == SDLK_c	// "ctrl-c" - cycle areas
					&& _savedGame->getDebugMode() == true)				// ctrl+c is also handled in Game::run() where the 'cycle' is determined.
				{
					_txtDebug->setText(L"");
					_debug = "DEBUG MODE : "; // _savedGame->setDebugArgDone("");

					if (_globe->getDebugType() == 0)
						_debug += "country : ";
					else if (_globe->getDebugType() == 1)
						_debug += "region : ";
					else if (_globe->getDebugType() == 2)
						_debug += "missionZones : ";
				}
			}
		}
		else if (_savedGame->isIronman() == false) // quick save and quick load
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

		_minimizedDogfights = minimizedDogfightsCount();
	}
}

/**
 * Updates the timer display and resets the palette since it's bound to change on other screens.
 */
void GeoscapeState::init()
{
	State::init();

	timeDisplay();

	_globe->onMouseClick((ActionHandler)& GeoscapeState::globeClick);
	_globe->onMouseOver(0);
//	_globe->rotateStop();
	_globe->setFocus(true);
	_globe->draw();

	if (_savedGame->isIronman() == true
		&& _savedGame->getName().empty() == false)
	{
		popup(new ListSaveState(OPT_GEOSCAPE));
	}

	if (_startMusic == true
		&& _dogfights.empty() == true
		&& _dfStartTimer->isRunning() == false)
	{
		if (_dogfightEnded == true)
		{
			_dogfightEnded = false;
			_game->getResourcePack()->fadeMusic(_game, 425);
		}

		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}

	_globe->unsetNewBaseHover();
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

	if (_game->getSavedGame()->getDebugArgDone() == true // ie. do not write info until Globe actually sets it.
		&& _debug.compare(0, 5, "DEBUG") == 0)
	{
		const std::string debugStr = _debug + _game->getSavedGame()->getDebugArg();
		_txtDebug->setText(Language::cpToWstr(debugStr));
	}

	if (_savedGame->getMonthsPassed() == -1)
	{
		_savedGame->addMonth();

		determineAlienMissions(true);
		setupTerrorMission();

		_savedGame->setFunds(_savedGame->getFunds()
										- _savedGame->getBaseMaintenance()
										- _savedGame->getBases()->front()->getPersonnelMaintenance());
	}

	if (_popups.empty() == true
		&& _dogfights.empty() == true
		&& (_dfZoomInTimer->isRunning() == false
			|| _dfZoomInDone == true)
		&& (_dfZoomOutTimer->isRunning() == false
			|| _dfZoomOutDone == true))
	{
		_gameTimer->think(this, NULL); // Handle timers
	}
	else
	{
		if (_dogfights.empty() == false
			|| _minimizedDogfights != 0)
		{
			handleDogfights();
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
 * Updates the Geoscape clock.
 */
void GeoscapeState::timeDisplay()
{
	_txtFunds->setText(Text::formatFunding(_savedGame->getFunds()));

	std::wostringstream
		ss2,
		ss3;

	if (_timeSpeed != _btn5Secs)
		_txtSec->setVisible();
	else
		_txtSec->setVisible(_savedGame->getTime()->getSecond() %15 == 0);

	ss2 << std::setfill(L'0') << std::setw(2) << _savedGame->getTime()->getMinute();
	_txtMin->setText(ss2.str());

	ss3 << std::setfill(L'0') << std::setw(2) << _savedGame->getTime()->getHour();
	_txtHour->setText(ss3.str());

	int date = _savedGame->getTime()->getDay();
	if (_day != date)
	{
		_srfDay1->clear();
		_srfDay2->clear();

		_day = date;

		SurfaceSet* const digitSet = _game->getResourcePack()->getSurfaceSet("DIGITS");

		Surface* srfDate = digitSet->getFrame(date / 10);
		srfDate->blit(_srfDay1);

		srfDate = digitSet->getFrame(date %10);
		srfDate->blit(_srfDay2);

		_srfDay1->offset(Palette::blockOffset(15)+3);
		_srfDay2->offset(Palette::blockOffset(15)+3);


		date = _savedGame->getTime()->getMonth();
		if (_month != date)
		{
			_srfMonth1->clear();
			_srfMonth2->clear();

			_month = date;

			srfDate = digitSet->getFrame(date / 10);
			srfDate->blit(_srfMonth1);

			srfDate = digitSet->getFrame(date %10);
			srfDate->blit(_srfMonth2);

			_srfMonth1->offset(Palette::blockOffset(15)+3);
			_srfMonth2->offset(Palette::blockOffset(15)+3);


			date = _savedGame->getTime()->getYear() %100;
			if (_year != date)
			{
				_srfYear1->clear();
				_srfYear2->clear();

				_year = date;

				srfDate = digitSet->getFrame(date / 10);
				srfDate->blit(_srfYear1);

				srfDate = digitSet->getFrame(date %10);
				srfDate->blit(_srfYear2);

				_srfYear1->offset(Palette::blockOffset(15)+3);
				_srfYear2->offset(Palette::blockOffset(15)+3);
			}
		}
	}
}

/**
 * Advances the game timer according to the timer speed set, and calls the respective
 * triggers. The timer always advances in "5 secs" cycles, regardless of the speed,
 * otherwise it might skip important steps. Instead, it just keeps advancing the
 * timer until the next speed step (eg. the next day on 1 Day speed) or until an
 * event occurs, since updating the screen on each step would become cumbersomely slow.
 */
void GeoscapeState::timeAdvance()
{
	int timeSpan = 0;
	if		(_timeSpeed == _btn5Secs)	timeSpan = 1;
	else if (_timeSpeed == _btn1Min)	timeSpan = 12;
	else if (_timeSpeed == _btn5Mins)	timeSpan = 12 * 5;
	else if (_timeSpeed == _btn30Mins)	timeSpan = 12 * 5 * 6;
	else if (_timeSpeed == _btn1Hour)	timeSpan = 12 * 5 * 6 * 2;
	else if (_timeSpeed == _btn1Day)	timeSpan = 12 * 5 * 6 * 2 * 24;

	for (int
			i = 0;
			i < timeSpan
				&& _pause == false;
			++i)
	{
		const TimeTrigger trigger = _savedGame->getTime()->advance();
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

	_pause = (_dogfightsToStart.empty() == false);

	timeDisplay();
	_globe->draw();
}

/**
 * Takes care of any game logic that has to run every game second.
 */
void GeoscapeState::time5Seconds()
{
	//Log(LOG_INFO) << "GeoscapeState::time5Seconds()";
	// Game over if there are no more bases.
	if (_savedGame->getBases()->empty() == true)
	{
		popup(new DefeatState());
		return;
	}

	const Ufo* ufoExpired = NULL; // kL

	// Handle UFO logic
	for (std::vector<Ufo*>::const_iterator
			i = _savedGame->getUfos()->begin();
			i != _savedGame->getUfos()->end();
			++i)
	{
		switch ((*i)->getStatus())
		{
			case Ufo::FLYING:
				//Log(LOG_INFO) << "GeoscapeState::time5Seconds(), Ufo::FLYING";
				if (_dfZoomInTimer->isRunning() == false
					&& _dfZoomOutTimer->isRunning() == false)
				{
					(*i)->think();

					if ((*i)->reachedDestination() == true)
					{
						const size_t tsCount = _savedGame->getTerrorSites()->size();
						AlienMission* const mission = (*i)->getMission();
						const bool detected = (*i)->getDetected();
						mission->ufoReachedWaypoint(
												**i,
												*_game,
												*_globe);

						if (detected != (*i)->getDetected()
							&& (*i)->getFollowers()->empty() == false)
						{
							if (!
								((*i)->getTrajectory().getID() == "__RETALIATION_ASSAULT_RUN"
									&& (*i)->getStatus() ==  Ufo::LANDED))
							{
								timerReset();
								popup(new UfoLostState((*i)->getName(_game->getLanguage())));
							}
						}

						//Log(LOG_INFO) << ". tsCount size_t = " << (int)tsCount;
						if (tsCount < _savedGame->getTerrorSites()->size()) // kL_note <- how is this possible?
						{
							//Log(LOG_INFO) << ". create terrorSite";

							TerrorSite* const ts = _savedGame->getTerrorSites()->back();
							const City* const city = _game->getRuleset()->locateCity(
																				ts->getLongitude(),
																				ts->getLatitude());
							assert(city);
							// kL_note: need to delete the UFO here, before attempting to target w/ Craft.
							// see: Globe::getTargets() for the workaround...
							// or try something like this, latr:
							//
							//delete *i;
							//i = _savedGame->getUfos()->erase(i);
								// still need to handle minimized dogfights etc.

							popup(new AlienTerrorState(
													ts,
													city->getName(),
													this));
						}
						//Log(LOG_INFO) << ". create terrorSite DONE";

						// If UFO was destroyed, don't spawn missions
						if ((*i)->getStatus() == Ufo::DESTROYED)
							return;

						if (Base* const base = dynamic_cast<Base*>((*i)->getDestination()))
						{
							mission->setWaveCountdown(30 * (RNG::generate(0, 48) + 400));
							(*i)->setDestination(NULL);
							base->setupDefenses();

							timerReset();

							if (base->getDefenses()->empty() == false)
								popup(new BaseDefenseState(
														base,
														*i,
														this));
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

				if ((*i)->getSecondsRemaining() == 0)
				{
					AlienMission* const mission = (*i)->getMission();
					bool detected = (*i)->getDetected();
					mission->ufoLifting(
									**i,
									*_game,
									*_globe);

					if (detected != (*i)->getDetected()
						&& (*i)->getFollowers()->empty() == false)
					{
						timerReset();
						popup(new UfoLostState((*i)->getName(_game->getLanguage())));
					}
				}
			break;
			case Ufo::CRASHED:
				(*i)->think();

				if ((*i)->getSecondsRemaining() == 0)
				{
					ufoExpired = *i; // shot down while trying to outrun interceptor
					(*i)->setDetected(false);
					(*i)->setStatus(Ufo::DESTROYED);
				}
			break;
			case Ufo::DESTROYED:
				// Nothing to do
			break;
		}
	}

	// Handle craft logic
	for (std::vector<Base*>::const_iterator
			i = _savedGame->getBases()->begin();
			i != _savedGame->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				)
		{
			if ((*j)->isDestroyed() == true)
			{
				const double
					lon = (*j)->getLongitude(),
					lat = (*j)->getLatitude();

				for (std::vector<Country*>::const_iterator
						country = _savedGame->getCountries()->begin();
						country != _savedGame->getCountries()->end();
						++country)
				{
					if ((*country)->getRules()->insideCountry(
															lon,
															lat) == true)
					{
						(*country)->addActivityAlien((*j)->getRules()->getScore());
						(*country)->recentActivity();
						break;
					}
				}

				for (std::vector<Region*>::const_iterator
						region = _savedGame->getRegions()->begin();
						region != _savedGame->getRegions()->end();
						++region)
				{
					if ((*region)->getRules()->insideRegion(
														lon,
														lat) == true)
					{
						(*region)->addActivityAlien((*j)->getRules()->getScore());
						(*region)->recentActivity();
						break;
					}
				}

				// if a transport craft has been shot down, kill all the soldiers on board.
				if ((*j)->getRules()->getSoldiers() > 0)
				{
					for (std::vector<Soldier*>::const_iterator
							soldier = (*i)->getSoldiers()->begin();
							soldier != (*i)->getSoldiers()->end();
							)
					{
						if ((*soldier)->getCraft() == *j)
						{
							(*soldier)->die(_savedGame);

							delete *soldier;
							soldier = (*i)->getSoldiers()->erase(soldier);
						}
						else
							++soldier;
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
					if (ufo->getDetected() == false // lost radar contact
						&& ufo != ufoExpired) // ie. not just shot down while trying to outrun interceptor but it crashed into the sea instead Lol
					{
						if (ufo->getTrajectory().getID() == "__RETALIATION_ASSAULT_RUN" // note: this is where that targeting-terrorUfo/Site glitch should also be taken care of.
							&& (ufo->getStatus() == Ufo::LANDED
								|| ufo->getStatus() == Ufo::DESTROYED))
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

							timerReset();
							popup(new GeoscapeCraftState(
														*j,
														_globe,
														wp,
														this));
						}
					}
					else if (ufo->getStatus() == Ufo::DESTROYED
						|| (ufo->getStatus() == Ufo::CRASHED	// kL, http://openxcom.org/forum/index.php?topic=2406.0
							&& (*j)->getNumSoldiers() == 0))	// kL, Actually should set this on the UFO-crash event
//							&& (*j)->getNumVehicles() == 0))	// kL, so that crashed-ufos can still be targeted for Patrols
					{
						(*j)->returnToBase();
					}
				}
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
				const TerrorSite* const ts = dynamic_cast<TerrorSite*>((*j)->getDestination());
				const AlienBase* const ab = dynamic_cast<AlienBase*>((*j)->getDestination());

				if (ufo != NULL)
				{
					switch (ufo->getStatus())
					{
						case Ufo::FLYING:
							// Not more than 4 interceptions at a time. kL_note: I thought you could do 6 in orig.
							if (_dogfights.size() + _dogfightsToStart.size() > 3)
							{
								++j;
								continue;
							}

							if ((*j)->isInDogfight() == false
								&& AreSame((*j)->getDistance(ufo), 0.)) // craft ran into a UFO
							{
								_dogfightsToStart.push_back(new DogfightState(
																				_globe,
																				*j,
																				ufo,
																				this));

								if (_dfStartTimer->isRunning() == false)
								{
									_pause = true;
									timerReset();

									// store the current Globe co-ords. Globe will reset to these after dogfight ends.
									setDogfightCoords(*j);

									if (_dogfights.empty() == true)
										_game->getResourcePack()->fadeMusic(_game, 425);

									startDogfight();
									_dfStartTimer->start();
								}

								_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_INTERCEPT);
							}
						break;
						case Ufo::LANDED:
							// setSpeed 1/2 (need to speed up to full if UFO takes off)
						case Ufo::CRASHED:
							// setSpeed 1/2 (need to speed back up when setting a new destination)
						case Ufo::DESTROYED: // Just before expiration
							if ((*j)->getNumSoldiers() > 0)
//								|| (*j)->getNumVehicles() > 0)
							{
								if ((*j)->isInDogfight() == false)
								{
									int // look up polygon's texture
										texture,
										shade;
									_globe->getPolygonTextureAndShade(
																	ufo->getLongitude(),
																	ufo->getLatitude(),
																	&texture,
																	&shade);
									timerReset();
									popup(new ConfirmLandingState(
																*j,
																texture,
																shade));
								}
							}
							else if (ufo->getStatus() != Ufo::LANDED)
								(*j)->returnToBase();
						break;
					}
				}
				else if (wp != NULL)
				{
					timerReset();
					popup(new CraftPatrolState(
											*j,
											_globe,
											this));
					(*j)->setDestination(NULL);
				}
				else if (ts != NULL)
				{
					if ((*j)->getNumSoldiers() > 0)
					{
						int // look up polygon's texture
							texture,
							shade;
						_globe->getPolygonTextureAndShade(
														ts->getLongitude(),
														ts->getLatitude(),
														&texture,
														&shade);

						timerReset();
						popup(new ConfirmLandingState(
													*j,
													texture,
													shade));
					}
					else
						(*j)->setDestination(NULL);
//						(*j)->returnToBase();
				}
				else if (ab != NULL)
				{
					if (ab->isDiscovered() == true)
					{
						if ((*j)->getNumSoldiers() > 0)
						{
							int // look up polygon's texture
								texture,
								shade;
							_globe->getPolygonTextureAndShade(
															ab->getLongitude(),
															ab->getLatitude(),
															&texture,
															&shade);

							timerReset();
							popup(new ConfirmLandingState(
														*j,
														texture,
														shade));
						}
						else
							(*j)->setDestination(NULL);
//							(*j)->returnToBase();
					}
				}
			}

			++j;
		}
	}

	for (std::vector<Ufo*>::const_iterator // Clean up dead UFOs and end dogfights which were minimized.
			i = _savedGame->getUfos()->begin();
			i != _savedGame->getUfos()->end();
			)
	{
		if ((*i)->getStatus() == Ufo::DESTROYED)
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
					else
						++j;
				}
			}

			delete *i;
			i = _savedGame->getUfos()->erase(i);
		}
		else
			++i;
	}

	for (std::vector<Waypoint*>::const_iterator // Clean up unused waypoints
			i = _savedGame->getWaypoints()->begin();
			i != _savedGame->getWaypoints()->end();
			)
	{
		if ((*i)->getFollowers()->empty() == true)
		{
			delete *i;
			i = _savedGame->getWaypoints()->erase(i);
		}
		else
			++i;
	}


	// This is ONLY for allowing _dogfights to fill (or not)
	// before deciding whether to startMusic in init() --
	// and ONLY for Loading with a dogfight in progress:
	if (_startMusic == false
		&& _dogfights.empty() == true
		&& _dfStartTimer->isRunning() == false)
	{
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
	_startMusic = true;
}

/**
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
		bool operator()(const Ufo* ufo) const;
};

/**
 * Only UFOs within detection range of the base have a chance to detect it.
 * @param ufo - pointer to the UFO attempting detection
 * @return, true if base is detected by the ufo
 */
bool DetectXCOMBase::operator()(const Ufo* ufo) const
{
	//Log(LOG_INFO) << "DetectXCOMBase(), ufoID " << ufo->getId();
	bool ret = false;

	if (ufo->isCrashed() == true)
	{
		//Log(LOG_INFO) << ". . Crashed UFOs can't detect!";
		return false;
	}
	else if (ufo->getTrajectory().getID() == "__RETALIATION_ASSAULT_RUN")
	{
		//Log(LOG_INFO) << ". uFo's attacking a base don't bother with this!";
		return false;
	}
	else if (ufo->getMissionType() != "STR_ALIEN_RETALIATION"
		&& Options::aggressiveRetaliation == false)
	{
		//Log(LOG_INFO) << ". . Only uFo's on retaliation missions scan for bases unless 'aggressiveRetaliation' option is TRUE";
		return false;
	}
	else
	{
		const double
			ufoRange = static_cast<double>(ufo->getRules()->getSightRange()) * greatCircleConversionFactor,
			targetDist = _base.getDistance(ufo) * earthRadius;
		//Log(LOG_INFO) << ". . ufoRange = " << (int)ufoRange;
		//Log(LOG_INFO) << ". . targetDist = " << (int)targetDist;

		if (targetDist > ufoRange)
		{
			//Log(LOG_INFO) << ". . uFo's have a detection range of 600 nautical miles.";
			return false;
		}
		else
		{
			const double detFactor = targetDist * 12. / ufoRange; // should use log() ...
			int chance = static_cast<int>(
						 static_cast<double>(_base.getDetectionChance(_diff) + ufo->getDetectors()) / detFactor);
			if (ufo->getMissionType() == "STR_ALIEN_RETALIATION"
				&& Options::aggressiveRetaliation == true)
			{
				//Log(LOG_INFO) << ". . uFo's on retaliation missions will scan for base 'aggressively'";
				chance += 5;
			}

			//Log(LOG_INFO) << ". . . chance = " << chance;
			ret = RNG::percent(chance);
		}
	}

	//Log(LOG_INFO) << ". ret " << ret;
	return ret;
}


/**
 * Functor that marks an XCOM base for retaliation.
 * This is required because of the iterator type.
 */
struct SetRetaliationStatus
	:
		public std::unary_function<std::map<const Region*, Base*>::value_type, void>
{
	/// Mark as a valid retaliation target.
	void operator()(const argument_type& iter) const
	{
		iter.second->setIsRetaliationTarget();
	}
};


/**
 * Takes care of any game logic that has to run every game ten minutes.
 */
void GeoscapeState::time10Minutes()
{
	//Log(LOG_INFO) << "GeoscapeState::time10Minutes()";
	const int diff = static_cast<int>(_savedGame->getDifficulty());

	for (std::vector<Base*>::const_iterator
			b = _savedGame->getBases()->begin();
			b != _savedGame->getBases()->end();
			++b)
	{
		for (std::vector<Craft*>::const_iterator
				c = (*b)->getCrafts()->begin();
				c != (*b)->getCrafts()->end();
				++c)
		{
			if ((*c)->getStatus() == "STR_OUT")
			{
				(*c)->consumeFuel();

				if ((*c)->getLowFuel() == false
					&& (*c)->getFuel() <= (*c)->getFuelLimit())
				{
					(*c)->setLowFuel(true);
					(*c)->returnToBase();

					popup(new LowFuelState(
										*c,
										this));
				}

//				if ((*c)->getDestination() == NULL)
//				{
				//Log(LOG_INFO) << ". Patrol for alienBases";
				for (std::vector<AlienBase*>::const_iterator // patrol for aLien bases.
						ab = _savedGame->getAlienBases()->begin();
						ab != _savedGame->getAlienBases()->end();
						++ab)
				{
					if ((*ab)->isDiscovered() == true)
						continue;

					const double
						craftRadar = static_cast<double>((*c)->getRules()->getSightRange()) * greatCircleConversionFactor,
						targetDist = (*c)->getDistance(*ab) * earthRadius;
					//Log(LOG_INFO) << ". . craftRadar = " << (int)craftRadar;
					//Log(LOG_INFO) << ". . targetDist = " << (int)targetDist;

					if (targetDist < craftRadar)
					{
						const int chance = 100 - (diff * 10) - static_cast<int>(targetDist * 50. / craftRadar);
						//Log(LOG_INFO) << ". . . craft in Range, chance = " << chance;

						if (RNG::percent(chance) == true)
						{
							//Log(LOG_INFO) << ". . . . aLienBase discovered";
							(*ab)->setDiscovered(true);
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
				b = _savedGame->getBases()->begin();
				b != _savedGame->getBases()->end();
				++b)
		{
			if ((*b)->getIsRetaliationTarget() == true) // kL_begin:
			{
				//Log(LOG_INFO) << "base is already marked as RetaliationTarget";
				continue;
			} // kL_end.

			std::vector<Ufo*>::const_iterator u = std::find_if( // find a UFO that detected this base, if any.
															_savedGame->getUfos()->begin(),
															_savedGame->getUfos()->end(),
															DetectXCOMBase(**b, diff));

			if (u != _savedGame->getUfos()->end())
			{
				//Log(LOG_INFO) << ". xBase found, set RetaliationStatus";
				(*b)->setIsRetaliationTarget();
			}
		}
	}
	else
	{
		std::map<const Region*, Base*> discovered;
		for (std::vector<Base*>::const_iterator // remember only last base in each region.
				b = _savedGame->getBases()->begin();
				b != _savedGame->getBases()->end();
				++b)
		{
			std::vector<Ufo*>::const_iterator u = std::find_if( // find a UFO that detected this base, if any.
															_savedGame->getUfos()->begin(),
															_savedGame->getUfos()->end(),
															DetectXCOMBase(**b, diff));

			if (u != _savedGame->getUfos()->end())
				discovered[_savedGame->locateRegion(**b)] = *b;
		}

		std::for_each( // mark the bases as discovered.
					discovered.begin(),
					discovered.end(),
					SetRetaliationStatus());
	}


	for (std::vector<Ufo*>::const_iterator // handle UFO detection
			u = _savedGame->getUfos()->begin();
			u != _savedGame->getUfos()->end();
			++u)
	{
		if ((*u)->getStatus() == Ufo::FLYING)
		{
			std::vector<Base*> hyperBases; // = std::vector<Base*>();

			if ((*u)->getDetected() == false)
			{
				const bool hyperDet_pre = (*u)->getHyperDetected();
				bool
					hyperDet = false,
					contact = false;

				for (std::vector<Base*>::const_iterator
						b = _savedGame->getBases()->begin();
						b != _savedGame->getBases()->end();
						++b)
				{
					switch ((*b)->detect(*u))
					{
						case 3:
							contact = true;
						case 1:
							hyperDet = true;

							if (hyperDet_pre == false)
								hyperBases.push_back(*b);
						break;

						case 2:
							contact = true;
						break;
					}

					for (std::vector<Craft*>::const_iterator
							c = (*b)->getCrafts()->begin();
							c != (*b)->getCrafts()->end()
								&& contact == false;
							++c)
					{
						if ((*c)->getStatus() == "STR_OUT"
							&& (*c)->detect(*u) == true)
						{
							contact = true;
						}
					}
				}

				(*u)->setDetected(contact);
				(*u)->setHyperDetected(hyperDet);

				if (contact == true
					|| (hyperDet == true
						&& hyperDet_pre == false))
				{
					popup(new UfoDetectedState(
											*u,
											this,
											true,
											hyperDet,
											contact,
											&hyperBases));
				}
			}
			else // ufo is already detected
			{
				const bool hyperDet_pre = (*u)->getHyperDetected();
				bool
					hyperDet = false,
					contact = false;

				for (std::vector<Base*>::const_iterator
						b = _savedGame->getBases()->begin();
						b != _savedGame->getBases()->end();
						++b)
				{
					switch ((*b)->detect(*u)) // base attempts redetection; this lets a UFO blip off radar scope
					{
						case 3:
							contact = true;
						case 1:
							hyperDet = true;

							if (hyperDet_pre == false)
								hyperBases.push_back(*b);
						break;

						case 2:
							contact = true;
						break;
					}

					for (std::vector<Craft*>::const_iterator
							c = (*b)->getCrafts()->begin();
							c != (*b)->getCrafts()->end()
								&& contact == false;
							++c)
					{
						if ((*c)->getStatus() == "STR_OUT"
							&& (*c)->detect(*u) == true)
						{
							contact = true;
						}
					}
				}

				(*u)->setDetected(contact);
				(*u)->setHyperDetected(hyperDet);

				if (hyperDet == true
					&& hyperDet_pre == false)
				{
					popup(new UfoDetectedState(
											*u,
											this,
											false,
											hyperDet,
											contact,
											&hyperBases));
				}

				if (contact == false
					&& (*u)->getFollowers()->empty() == false)
				{
					timerReset();
					popup(new UfoLostState((*u)->getName(_game->getLanguage())));
				}
			}
		}
	}
}

/**
 * @brief Call AlienMission::think() with proper parameters.
 * This function object calls AlienMission::think() with the proper parameters.
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
		{
			/* Empty by design. */
		}

		/// Call AlienMission::think() with stored parameters.
		void operator()(AlienMission* am) const
		{
			am->think(
					_game,
					_globe);
		}
};

/**
 * @brief Process a TerrorSite.
 * This function object will count down towards expiring a TerrorSite
 * and handle expired TerrorSites.
 * @param ts - pointer to a TerrorSite
 * @return, true if terror is finished ( w/out xCom mission success )
 */
bool GeoscapeState::processTerrorSite(TerrorSite* ts) const
{
	bool expired = true;

	const int diff = static_cast<int>(_savedGame->getDifficulty());
	int aLienPts = (_game->getRuleset()->getAlienMission("STR_ALIEN_TERROR")->getPoints() * 50) + (diff * 250);

	if (ts->getSecondsRemaining() >= 30 * 60)
	{
		ts->setSecondsRemaining(ts->getSecondsRemaining() - 30 * 60);

		aLienPts = 10 + (diff * 10);
		expired = false;
	}

	Region* const region = _savedGame->locateRegion(*ts);
	if (region != NULL)
	{
		region->addActivityAlien(aLienPts);
		region->recentActivity();
	}

	for (std::vector<Country*>::const_iterator
			i = _savedGame->getCountries()->begin();
			i != _savedGame->getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(
										ts->getLongitude(),
										ts->getLatitude()))
		{
			(*i)->addActivityAlien(aLienPts);
			(*i)->recentActivity();
			break;
		}
	}

	if (expired == true)
	{
		delete ts;
		return true;
	}

	return false;
}

/**
 * @brief Advance time for crashed UFOs.
 * This function object will decrease the expiration timer for crashed UFOs.
 */
struct expireCrashedUfo: public std::unary_function<Ufo*, void>
{
	/// Decrease UFO expiration timer.
	void operator()(Ufo* ufo) const
	{
		if (ufo->getStatus() == Ufo::CRASHED)
		{
			const int sec = ufo->getSecondsRemaining();
			if (sec >= 30 * 60)
			{
				ufo->setSecondsRemaining(sec - 30 * 60);
				return;
			}

			ufo->setStatus(Ufo::DESTROYED); // mark expired UFOs for removal.
		}
	}
};

/**
 * Takes care of any game logic that has to run every game half hour.
 */
void GeoscapeState::time30Minutes()
{
	//Log(LOG_INFO) << "GeoscapeState::time30Minutes()";
	std::for_each( // decrease mission countdowns
			_savedGame->getAlienMissions().begin(),
			_savedGame->getAlienMissions().end(),
			callThink(
					*_game,
					*_globe));
	//Log(LOG_INFO) << ". . decreased mission countdowns";

	for (std::vector<AlienMission*>::const_iterator // remove finished missions
			am = _savedGame->getAlienMissions().begin();
			am != _savedGame->getAlienMissions().end();
			)
	{
		if ((*am)->isOver() == true)
		{
			//Log(LOG_INFO) << ". aLienMission isOver() : " << (*am)->getType();
			delete *am;
			am = _savedGame->getAlienMissions().erase(am);
		}
		else
			++am;
	}
	//Log(LOG_INFO) << ". . removed finished missions";

	std::for_each( // handle crashed UFOs expiration
			_savedGame->getUfos()->begin(),
			_savedGame->getUfos()->end(),
			expireCrashedUfo());
	//Log(LOG_INFO) << ". . handled crashed UFO expirations";

	for (std::vector<Base*>::const_iterator // handle craft maintenance & refueling.
			i = _savedGame->getBases()->begin();
			i != _savedGame->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getStatus() == "STR_OUT")
				continue;

			if ((*j)->getStatus() == "STR_REPAIRS")
				(*j)->repair();
			else if ((*j)->getStatus() == "STR_REARMING")
			{
				const std::string rearmClip = (*j)->rearm(_game->getRuleset());

				if (rearmClip.empty() == false
					&& (*j)->getWarned() == false)
				{
					(*j)->setWarned();

					const std::wstring msg = tr("STR_NOT_ENOUGH_ITEM_TO_REARM_CRAFT_AT_BASE")
											.arg(tr(rearmClip))
											.arg((*j)->getName(_game->getLanguage()))
											.arg((*i)->getName());
					popup(new CraftErrorState(
											this,
											msg));
				}
			}
			else if ((*j)->getStatus() == "STR_REFUELLING")
			{
				const std::string refuelItem = (*j)->getRules()->getRefuelItem();

				if (refuelItem.empty() == true)
					(*j)->refuel();
				else
				{
					if ((*i)->getItems()->getItem(refuelItem) > 0)
					{
						(*j)->refuel();
						(*i)->getItems()->removeItem(refuelItem);
					}
					else if ((*j)->getWarned() == false)
					{
						(*j)->setWarned();

						const std::wstring msg = tr("STR_NOT_ENOUGH_ITEM_TO_REFUEL_CRAFT_AT_BASE")
												.arg(tr(refuelItem))
												.arg((*j)->getName(_game->getLanguage()))
												.arg((*i)->getName());
						popup(new CraftErrorState(
												this,
												msg));
					}
				}
			}
		}
	}
	//Log(LOG_INFO) << ". . handled craft maintenance & alien base detection";

	const int pts = _savedGame->getMonthsPassed() / 3 + static_cast<int>(_savedGame->getDifficulty());
	int aLienPts = 0;
//	pts = pts * _game->getRuleset()->getAlienMission("STR_ALIEN_*")->getPoints() / 100; // kL_stuff

	for (std::vector<Ufo*>::const_iterator // handle UFO detection and give aliens points
			ufo = _savedGame->getUfos()->begin();
			ufo != _savedGame->getUfos()->end();
			++ufo)
	{
		//Log(LOG_INFO) << ". . . . for " << *ufo;
		aLienPts = pts;

		switch ((*ufo)->getStatus())
		{
			case Ufo::LANDED:
				aLienPts += 5;
				//Log(LOG_INFO) << ". . . . . . ufo has Landed";
			break;
			case Ufo::CRASHED:
				aLienPts += 3;
				//Log(LOG_INFO) << ". . . . . . ufo is Crashed";
			break;
			case Ufo::FLYING:
				aLienPts += 1;
				//Log(LOG_INFO) << ". . . . . . ufo is Flying";
			break;
			case Ufo::DESTROYED:
				//Log(LOG_INFO) << ". . . . . . ufo is Destroyed";
			break;

			default:
			break;
		}

		aLienPts = std::max(
						1,
						aLienPts / 2);
//		if (aLienPts)
//		{
		const double
			lon = (*ufo)->getLongitude(),
			lat = (*ufo)->getLatitude();

		// Get area
		for (std::vector<Region*>::const_iterator
				k = _savedGame->getRegions()->begin();
				k != _savedGame->getRegions()->end();
				++k)
		{
			if ((*k)->getRules()->insideRegion(
											lon,
											lat) == true)
			{
				(*k)->addActivityAlien(aLienPts); // one point per UFO in-Region per half hour
				(*k)->recentActivity();

				break;
			}
		}
		//Log(LOG_INFO) << ". . . . . . get Area done";

		// Get country
		for (std::vector<Country*>::const_iterator
				k = _savedGame->getCountries()->begin();
				k != _savedGame->getCountries()->end();
				++k)
		{
			if ((*k)->getRules()->insideCountry(
											lon,
											lat) == true)
			{
				(*k)->addActivityAlien(aLienPts); // two points per UFO in-Country per half hour <- no, KEEP IT CONSISTENT, pending investigation.
				(*k)->recentActivity();

				break;
			}
		}
		//Log(LOG_INFO) << ". . . . . . get Country done";
//		}
	}
	//Log(LOG_INFO) << ". . handled alien points";

	// Processes TerrorSites
	for (std::vector<TerrorSite*>::const_iterator
			ts = _savedGame->getTerrorSites()->begin();
			ts != _savedGame->getTerrorSites()->end();
			)
	{
		if (processTerrorSite(*ts))
			ts = _savedGame->getTerrorSites()->erase(ts);
		else
			++ts;
	}
	//Log(LOG_INFO) << ". . processed terror sites";
}

/**
 * Takes care of any game logic that has to run every game hour.
 */
void GeoscapeState::time1Hour()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Hour()";
	for (std::vector<Region*>::const_iterator
			region = _savedGame->getRegions()->begin();
			region != _savedGame->getRegions()->end();
			++region)
	{
		(*region)->recentActivity(false);
		(*region)->recentActivityXCOM(false);
	}

	for (std::vector<Country*>::const_iterator
			country = _savedGame->getCountries()->begin();
			country != _savedGame->getCountries()->end();
			++country)
	{
		(*country)->recentActivity(false);
		(*country)->recentActivityXCOM(false);
	}


	//Log(LOG_INFO) << ". arrivals";
	bool arrivals = false;

	for (std::vector<Base*>::const_iterator // handle transfers
			base = _savedGame->getBases()->begin();
			base != _savedGame->getBases()->end();
			++base)
	{
		for (std::vector<Transfer*>::const_iterator
				transfer = (*base)->getTransfers()->begin();
				transfer != (*base)->getTransfers()->end();
				++transfer)
		{
			(*transfer)->advance(*base);

			if ((*transfer)->getHours() == 0
				&& arrivals == false)
			{
				arrivals = true;
			}
		}
	}
	//Log(LOG_INFO) << ". arrivals DONE";

	if (arrivals == true)
		popup(new ItemsArrivingState(this));


	std::vector<ProductionCompleteInfo> events; // myk002

	for (std::vector<Base*>::const_iterator // handle Production
			base = _savedGame->getBases()->begin();
			base != _savedGame->getBases()->end();
			++base)
	{
		std::map<Production*, ProductProgress> toRemove;

		for (std::vector<Production*>::const_iterator
				product = (*base)->getProductions().begin();
				product != (*base)->getProductions().end();
				++product)
		{
			toRemove[*product] = (*product)->step(
												*base,
												_savedGame,
												_game->getRuleset());
		}

		for (std::map<Production*, ProductProgress>::const_iterator
				product = toRemove.begin();
				product != toRemove.end();
				++product)
		{
			if (product->second > PROGRESS_NOT_COMPLETE)
			{
				(*base)->removeProduction(product->first);

				if (!events.empty()) // myk002_begin: only show the action button for the last completion notification
					events.back().showGotoBaseButton = false;

				events.push_back(ProductionCompleteInfo(
													tr(product->first->getRules()->getName()),
													true,
													product->second)); // myk002_end.
/*myk002
				popup(new ProductionCompleteState(
												*base,
												tr(product->first->getRules()->getName()),
												this,
												product->second)); */
			}
		}

		if (Options::storageLimitsEnforced == true
			&& (*base)->storesOverfull() == true)
		{
			timerReset();
			popup(new ErrorMessageState(
									tr("STR_STORAGE_EXCEEDED").arg((*base)->getName()).c_str(),
									_palette,
//kL								Palette::blockOffset(15)+1,
									Palette::blockOffset(6)+4, // kL
//kL								"BACK13.SCR",
									"BACK12.SCR", // kL
									6));
			popup(new SellState(*base));
		}

		for (std::vector<ProductionCompleteInfo>::const_iterator // myk002_begin:
				event = events.begin();
				event != events.end();
//kL			++event
				)
		{
			popup(new ProductionCompleteState(
											*base,
											event->item,
											this,
											event->showGotoBaseButton,
											event->endType)); // myk002_end.

			event = events.erase(event); // kL
		}
	}
	//Log(LOG_INFO) << "GeoscapeState::time1Hour() EXIT";
}


/**
 * This class will attempt to generate a supply mission for a base.
 * Each alien base has a 4% chance to generate a supply mission.
 */
class GenerateSupplyMission
	:
		public std::unary_function<const AlienBase*, void>
{

private:
	const Ruleset& _ruleset;
	SavedGame& _save;

	public:
		/// Store rules and game data references for later use.
		GenerateSupplyMission(
				const Ruleset& ruleset,
				SavedGame& save)
			:
				_ruleset(ruleset),
				_save(save)
		{
			/* Empty by design */
		}

		/// Check and spawn mission.
		void operator()(const AlienBase* base) const;
};

/**
 * Check and create supply mission for the given base.
 * There is a 4% chance of the mission spawning.
 * @param base A pointer to the alien base.
 */
void GenerateSupplyMission::operator()(const AlienBase* base) const
{
	if (RNG::percent(4) == true)
	{
		// Spawn supply mission for this base.
		const RuleAlienMission& rule = *_ruleset.getAlienMission("STR_ALIEN_SUPPLY");
		AlienMission* const mission = new AlienMission(
													rule,
													_save);
		mission->setRegion(
					_save.locateRegion(*base)->getRules()->getType(),
					_ruleset);
		mission->setId(_save.getId("ALIEN_MISSIONS"));
		mission->setRace(base->getAlienRace());
		mission->setAlienBase(base);
		mission->start();

		_save.getAlienMissions().push_back(mission);
	}
}


/**
 * Takes care of any game logic that has to run every game day.
 */
void GeoscapeState::time1Day()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Day()";
	for (std::vector<Base*>::const_iterator
			b = _savedGame->getBases()->begin();
			b != _savedGame->getBases()->end();
			++b)
	{
		// myk002_begin: create list of pending events for this base so we can
		// show a slightly different dialog layout for the last event of each type
		std::vector<ProductionCompleteInfo> productionCompleteEvents; // myk002_end.

		for (std::vector<BaseFacility*>::const_iterator // handle facility construction
				f = (*b)->getFacilities()->begin();
				f != (*b)->getFacilities()->end();
				++f)
		{
			if ((*f)->getBuildTime() > 0)
			{
				(*f)->build();

				if ((*f)->getBuildTime() == 0)
				{
					if (productionCompleteEvents.empty() == false) // myk002_begin: only show the action button for the last completion notification
						productionCompleteEvents.back().showGotoBaseButton = false;

					productionCompleteEvents.push_back(
													ProductionCompleteInfo(tr((*f)->getRules()->getType()),
													true,
													PROGRESS_CONSTRUCTION)); // myk002_end.
/*myk002
					popup(new ProductionCompleteState(
													*b,
													tr((*f)->getRules()->getType()),
													this,
													PROGRESS_CONSTRUCTION)); */
				}
			}
		}


		std::vector<ResearchProject*> finished; // handle science projects

		for (std::vector<ResearchProject*>::const_iterator
				rp = (*b)->getResearch().begin();
				rp != (*b)->getResearch().end();
				++rp)
		{
			if ((*rp)->step())
				finished.push_back(*rp);
		}

		if (finished.empty() == false)
			timerReset();

		std::vector<State*> researchCompleteEvents;								// myk002
		std::vector<NewPossibleResearchInfo> newPossibleResearchEvents;			// myk002
		std::vector<NewPossibleManufactureInfo> newPossibleManufactureEvents;	// myk002

		for (std::vector<ResearchProject*>::const_iterator
				rp = finished.begin();
				rp != finished.end();
				++rp)
		{
			(*b)->removeResearch(
							*rp,
							_game->getRuleset()->getUnit((*rp)->getRules()->getName()) != NULL); // kL, interrogation of aLien Unit complete.

			RuleResearch* bonus = NULL;
			const RuleResearch* research = (*rp)->getRules();
			//if (research) Log(LOG_INFO) << ". research Valid";
			//else Log(LOG_INFO) << ". research NOT valid"; // end_TEST

			if (Options::spendResearchedItems // if "researched" the live alien, his body sent to the stores.
				&& research->needItem()
				&& _game->getRuleset()->getUnit(research->getName()))
			{
				(*b)->getItems()->addItem(
									_game->getRuleset()->getArmor(
															_game->getRuleset()->getUnit(
																					research->getName())->getArmor())
									->getCorpseGeoscape());
				// ;) -> kL_note: heh i noticed that.
			}

			if ((*rp)->getRules()->getGetOneFree().empty() == false)
			{
				std::vector<std::string> possibilities;

				for (std::vector<std::string>::const_iterator
						gof = (*rp)->getRules()->getGetOneFree().begin();
						gof != (*rp)->getRules()->getGetOneFree().end();
						++gof)
				{
					bool oneFree = true;

					for (std::vector<const RuleResearch*>::const_iterator
							discovered = _savedGame->getDiscoveredResearch().begin();
							discovered != _savedGame->getDiscoveredResearch().end();
							++discovered)
					{
						if (*gof == (*discovered)->getName())
							oneFree = false;
					}

					if (oneFree)
						possibilities.push_back(*gof);
				}

				if (possibilities.empty() == false)
				{
					size_t randFree = static_cast<size_t>(RNG::generate(
																	0,
																	static_cast<int>(possibilities.size() - 1)));
					std::string free = possibilities.at(randFree);
					bonus = _game->getRuleset()->getResearch(free);

					_savedGame->addFinishedResearch(
												bonus,
												_game->getRuleset());

					if (bonus->getLookup().empty() == false)
						_savedGame->addFinishedResearch(
													_game->getRuleset()->getResearch(bonus->getLookup()),
													_game->getRuleset());
				}
			}

			const RuleResearch* newResearch = research;
			//if (newResearch) Log(LOG_INFO) << ". newResearch Valid";
			//else Log(LOG_INFO) << ". newResearch NOT valid"; // end_TEST

			std::string name = research->getLookup();
			if (name.empty() == true)
				name = research->getName();
			//Log(LOG_INFO) << ". Research = " << name;

			if (_savedGame->isResearched(name) == true)
			{
				//Log(LOG_INFO) << ". newResearch NULL, already been researched";
				newResearch = NULL;
			}

			_savedGame->addFinishedResearch( // this adds the actual research project to _discovered vector.
										research,
										_game->getRuleset());

			if (research->getLookup().empty() == false)
				_savedGame->addFinishedResearch(
											_game->getRuleset()->getResearch(research->getLookup()),
											_game->getRuleset());

			researchCompleteEvents.push_back(new ResearchCompleteState( // myk002
																	newResearch,
																	bonus));
/*myk002
			popup(new ResearchCompleteState(
										newResearch,
										bonus)); */

			std::vector<RuleResearch*> newPossibleResearch;
			_savedGame->getDependableResearch(
										newPossibleResearch,
										(*rp)->getRules(),
										_game->getRuleset(),
										*b);

			std::vector<RuleManufacture*> newPossibleManufacture;
			_savedGame->getDependableManufacture(
											newPossibleManufacture,
											(*rp)->getRules(),
											_game->getRuleset(),
											*b);

			if (newResearch != NULL) // check for possible researching weapon before clip
			{
				RuleItem* item = _game->getRuleset()->getItem(newResearch->getName());
				if (item
					&& item->getBattleType() == BT_FIREARM
					&& item->getCompatibleAmmo()->empty() == false)
				{
					RuleManufacture* manufRule = _game->getRuleset()->getManufacture(item->getType());
					if (manufRule
						&& manufRule->getRequirements().empty() == false)
					{
						const std::vector<std::string>& req = manufRule->getRequirements();
						RuleItem* ammo = _game->getRuleset()->getItem(item->getCompatibleAmmo()->front());
						if (ammo
							&& std::find(
										req.begin(),
										req.end(),
										ammo->getType()) != req.end()
							&& _savedGame->isResearched(manufRule->getRequirements()) == false)
						{
							researchCompleteEvents.push_back(new ResearchRequiredState(item)); // myk002
/*myk002
							popup(new ResearchRequiredState(item)); */
						}
					}
				}
			}

			if (newPossibleResearch.empty() == false) // myk002_begin: only show the "allocate research" button for the last notification
			{
				if (newPossibleResearchEvents.empty() == false)
					newPossibleResearchEvents.back().showResearchButton = false;

				newPossibleResearchEvents.push_back(NewPossibleResearchInfo(
																		newPossibleResearch,
																		true));
			} // myk002_end.
/*myk002
			popup(new NewPossibleResearchState(
											*b,
											newPossibleResearch)); */

			if (newPossibleManufacture.empty() == false)
			{
				if (newPossibleManufactureEvents.empty() == false) // myk002_begin: only show the "allocate production" button for the last notification
					newPossibleManufactureEvents.back().showManufactureButton = false;

				newPossibleManufactureEvents.push_back(NewPossibleManufactureInfo(
																				newPossibleManufacture,
																				true)); // myk002_end.
/*myk002
				popup(new NewPossibleManufactureState(
													*b,
													newPossibleManufacture)); */
			}

			for (std::vector<Base*>::const_iterator // now iterate through all the bases and remove this project from their labs
					b2 = _savedGame->getBases()->begin();
					b2 != _savedGame->getBases()->end();
					++b2)
			{
				for (std::vector<ResearchProject*>::const_iterator
						rp2 = (*b2)->getResearch().begin();
						rp2 != (*b2)->getResearch().end();
						++rp2)
				{
					if ((*rp)->getRules()->getName() == (*rp2)->getRules()->getName()
						&& _game->getRuleset()->getUnit((*rp2)->getRules()->getName()) == NULL)
					{
						(*b2)->removeResearch(
											*rp2,
											false);

						break;
					}
				}
			}

			delete *rp; // DONE Research.
		}

		//Log(LOG_INFO) << ". Soldiers";
		for (std::vector<Soldier*>::const_iterator // handle soldiers in sickbay
				soldier = (*b)->getSoldiers()->begin();
				soldier != (*b)->getSoldiers()->end();
				)
		{
			//Log(LOG_INFO) << ". Soldier = " << (*soldier)->getId() << " woundPct = " << (*soldier)->getWoundPercent();
			if ((*soldier)->getWoundPercent() > 10
				&& RNG::percent((*soldier)->getWoundPercent() / 5) == true)	// more than 10% wounded, %chance to die today
			{
				//Log(LOG_INFO) << ". . he's dead, Jim!!" << (*soldier)->getId();
				timerReset();

				popup(new SoldierDiedState(
										(*soldier)->getName(),
										(*b)->getName()));

				(*soldier)->die(_savedGame); // holy * This copies the Diary-object
				// so to delete Soldier-instance I need to use a CopyConstructor
				// on either or both of SoldierDiary and SoldierCommendations.
				// Oh, and maybe an operator= assignment overload also.
				// Learning C++ is like standing around while 20 people constantly
				// throw cow's dung at you. (But don't mention "const" or they'll throw
				// twice as fast.) i miss you, Alan Turing ....

				delete *soldier;
				soldier = (*b)->getSoldiers()->erase(soldier);

				// note: Could return any armor the soldier was wearing to Stores.
			}
			else
			{
				//Log(LOG_INFO) << ". . heal up.";
				if ((*soldier)->getWoundRecovery() > 0)
					(*soldier)->heal();

				++soldier;
			}
			//Log(LOG_INFO) << ". Soldier DONE";
		}
		//Log(LOG_INFO) << ". iterate Soldiers DONE";

		if ((*b)->getAvailablePsiLabs() > 0 // handle psionic training
			&& Options::anytimePsiTraining == true)
		{
			for (std::vector<Soldier*>::const_iterator
					soldier = (*b)->getSoldiers()->begin();
					soldier != (*b)->getSoldiers()->end();
					++soldier)
			{
				if ((*soldier)->trainPsiDay() == true)
					(*soldier)->autoStat();

//				(*soldier)->calcStatString(
//								_game->getRuleset()->getStatStrings(),
//								(Options::psiStrengthEval
//									&& _savedGame->isResearched(_game->getRuleset()->getPsiRequirements())));
			}
		}

		// myk002_begin: if research has been completed but no new research
		// events are triggered, show an empty NewPossibleResearchState so
		// players have a chance to allocate the now-free scientists
		if (researchCompleteEvents.empty() == false
			&& newPossibleResearchEvents.empty() == true)
		{
			newPossibleResearchEvents.push_back(NewPossibleResearchInfo(
																	std::vector<RuleResearch*>(),
																	true));
		}

		// show events
		for (std::vector<ProductionCompleteInfo>::const_iterator
				pcEvent = productionCompleteEvents.begin();
				pcEvent != productionCompleteEvents.end();
//kL			++pcEvent
				)
		{
			popup(new ProductionCompleteState(
											*b,
											pcEvent->item,
											this,
											pcEvent->showGotoBaseButton,
											pcEvent->endType));

			pcEvent = productionCompleteEvents.erase(pcEvent); // kL
		}

		for (std::vector<State*>::const_iterator
				rcEvent = researchCompleteEvents.begin();
				rcEvent != researchCompleteEvents.end();
//kL			++rcEvent
				)
		{
			popup(*rcEvent);

			rcEvent = researchCompleteEvents.erase(rcEvent); // kL
		}

		for (std::vector<NewPossibleResearchInfo>::const_iterator
				resEvent = newPossibleResearchEvents.begin();
				resEvent != newPossibleResearchEvents.end();
//kL			++resEvent
				)
		{
			popup(new NewPossibleResearchState(
											*b,
											resEvent->newPossibleResearch,
											resEvent->showResearchButton));

			resEvent = newPossibleResearchEvents.erase(resEvent); // kL
		}

		for (std::vector<NewPossibleManufactureInfo>::const_iterator
				manEvent = newPossibleManufactureEvents.begin();
				manEvent != newPossibleManufactureEvents.end();
//kL			++manEvent
				)
		{
			popup(new NewPossibleManufactureState(
												*b,
												manEvent->newPossibleManufacture,
												manEvent->showManufactureButton));

			manEvent = newPossibleManufactureEvents.erase(manEvent); // kL
		} // myk002_end.
	}


	const int
		diff = static_cast<int>(_savedGame->getDifficulty()) + 1,
		aLienPts = _game->getRuleset()->getAlienMission("STR_ALIEN_BASE")->getPoints() * diff / 100;

	if (aLienPts > 0) // handle regional and country points for alien bases
	{
		for (std::vector<AlienBase*>::const_iterator
				ab = _savedGame->getAlienBases()->begin();
				ab != _savedGame->getAlienBases()->end();
				++ab)
		{
			const double
				lon = (*ab)->getLongitude(),
				lat = (*ab)->getLatitude();

			for (std::vector<Region*>::const_iterator
					r = _savedGame->getRegions()->begin();
					r != _savedGame->getRegions()->end();
					++r)
			{
				if ((*r)->getRules()->insideRegion(
												lon,
												lat) == true)
				{
					(*r)->addActivityAlien(aLienPts);
					(*r)->recentActivity();

					break;
				}
			}

			for (std::vector<Country*>::const_iterator
					c = _savedGame->getCountries()->begin();
					c != _savedGame->getCountries()->end();
					++c)
			{
				if ((*c)->getRules()->insideCountry(
												lon,
												lat) == true)
				{
					(*c)->addActivityAlien(aLienPts);
					(*c)->recentActivity();

					break;
				}
			}
		}
	}


	std::for_each( // handle supply of alien bases.
			_savedGame->getAlienBases()->begin(),
			_savedGame->getAlienBases()->end(),
			GenerateSupplyMission(
								*_game->getRuleset(),
								*_savedGame));

	// Autosave 3 times a month. kL_note: every day.
//	int day = _savedGame->getTime()->getDay();
//	if (day == 10 || day == 20)
//	{
	if (_savedGame->isIronman() == true)
		popup(new SaveGameState(
							OPT_GEOSCAPE,
							SAVE_IRONMAN,
							_palette));
	else if (Options::autosave == true)
		popup(new SaveGameState(
							OPT_GEOSCAPE,
							SAVE_AUTO_GEOSCAPE,
							_palette));
//	}
	//Log(LOG_INFO) << "GeoscapeState::time1Day() EXIT";
}

/**
 * Takes care of any game logic that has to run every game month.
 */
void GeoscapeState::time1Month()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Month()";
	timerReset();
	_savedGame->addMonth();

	determineAlienMissions(); // determine alien mission for this month.

	const int monthsPassed = _savedGame->getMonthsPassed();
//kL	if (monthsPassed > 5)
	if (RNG::percent(monthsPassed * 2)) // kL
		determineAlienMissions(); // kL_note: determine another one, I guess.

	setupTerrorMission();

	// kL_note: Used for determining % retaliation & % agents discovering aLienBases.
	const int diff = static_cast<int>(_savedGame->getDifficulty()); // kL

	bool newRetaliation = false;
//kL	if (monthsPassed > 13 - static_cast<int>(_savedGame->getDifficulty())
	if (RNG::percent(monthsPassed * diff + 3) // kL, Beginner == %0
		|| _savedGame->isResearched("STR_THE_MARTIAN_SOLUTION") == true)
	{
		newRetaliation = true;
	}

	bool psi = false;

	for (std::vector<Base*>::const_iterator // handle Psi-Training and initiate a new retaliation mission, if applicable
			i = _savedGame->getBases()->begin();
			i != _savedGame->getBases()->end();
			++i)
	{
		if (newRetaliation == true)
		{
			for (std::vector<Region*>::const_iterator
					j = _savedGame->getRegions()->begin();
					j != _savedGame->getRegions()->end();
					++j)
			{
				if ((*j)->getRules()->insideRegion(
												(*i)->getLongitude(),
												(*i)->getLatitude()) == true)
				{
					if (_savedGame->getAlienMission(
												(*j)->getRules()->getType(),
												"STR_ALIEN_RETALIATION") == NULL)
					{
						const RuleAlienMission& rule = *_game->getRuleset()->getAlienMission("STR_ALIEN_RETALIATION");
						AlienMission* mission = new AlienMission(
																rule,
																*_savedGame);
						mission->setId(_savedGame->getId("ALIEN_MISSIONS"));
						mission->setRegion(
										(*j)->getRules()->getType(),
										*_game->getRuleset());

/*						int race = RNG::generate(
											0,
											static_cast<int>(
													_game->getRuleset()->getAlienRacesList().size())
												- 2); // -2 to avoid "MIXED" race
						mission->setRace(_game->getRuleset()->getAlienRacesList().at(race)); */
						// get races for retaliation missions
						std::vector<std::string> races = _game->getRuleset()->getAlienRacesList();
						for (std::vector<std::string>::const_iterator
								k = races.begin();
								k != races.end();
								)
						{
							if (_game->getRuleset()->getAlienRace(*k)->canRetaliate() == true)
								++k;
							else
								k = races.erase(k);
						}

						const size_t race = static_cast<size_t>(RNG::generate(
																			0,
																			static_cast<int>(races.size()) - 1));
						mission->setRace(races[race]);
						mission->start(150);
						_savedGame->getAlienMissions().push_back(mission);

						newRetaliation = false;
					}

					break;
				}
			}
		}

		if (Options::anytimePsiTraining == false
			&& (*i)->getAvailablePsiLabs() > 0)
		{
			psi = true;

			for (std::vector<Soldier*>::const_iterator
					j = (*i)->getSoldiers()->begin();
					j != (*i)->getSoldiers()->end();
					++j)
			{
				if ((*j)->isInPsiTraining() == true)
				{
					(*j)->trainPsi();
//					(*j)->calcStatString(
//									_game->getRuleset()->getStatStrings(),
//									(Options::psiStrengthEval
//									&& _savedGame->isResearched(_game->getRuleset()->getPsiRequirements())));
				}
			}
		}
	}

	_savedGame->monthlyFunding(); // handle Funding
	popup(new MonthlyReportState(
								psi,
								_globe));


	// kL_note: Might want to change this to time1day() ...
	const int rdiff = 20 - (diff * 5); // kL, Superhuman == 0%

//	if (RNG::percent(20)
	if (RNG::percent(rdiff + 50) == true // kL
		&& _savedGame->getAlienBases()->empty() == false)
	{
		for (std::vector<AlienBase*>::const_iterator // handle Xcom Operatives discovering bases
				i = _savedGame->getAlienBases()->begin();
				i != _savedGame->getAlienBases()->end();
				++i)
		{
			if ((*i)->isDiscovered() == false
				&& RNG::percent(rdiff + 5) == true) // kL
			{
				(*i)->setDiscovered(true);
				popup(new AlienBaseState(
									*i,
									this));
//kL			break;
			}
		}
	}
}

/**
 * Slows down the timer back to minimum speed, for when important events occur.
 */
void GeoscapeState::timerReset()
{
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
 * Adds a new popup window to the queue - this prevents popups from overlapping -
 * and pauses the game timer respectively.
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
		mouseX = static_cast<int>(std::floor(action->getAbsoluteXMouse())),
		mouseY = static_cast<int>(std::floor(action->getAbsoluteYMouse()));

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		const std::vector<Target*> targets = _globe->getTargets(
															mouseX,
															mouseY,
															false);
		if (targets.empty() == false)
			_game->pushState(new MultipleTargetsState(
													targets,
													NULL,
													this));
	}

	if (_savedGame->getDebugMode() == true)
	{
		_savedGame->setDebugArg("COORD");	// tells think() to stop writing area-info and display lon/lat instead.
		_debug = "";						// ditto

		double
			lonRad,
			latRad;
		_globe->cartToPolar(
						mouseX,
						mouseY,
						&lonRad,
						&latRad);

		const double
			lonDeg = lonRad / M_PI * 180.,
			latDeg = latRad / M_PI * 180.;

		std::wostringstream ss;
		ss << std::fixed << std::setprecision(3)
			<< L"RAD Lon " << lonRad << L"  Lat " << latRad
			<< std::endl
			<< L"DEG Lon " << lonDeg << L"  Lat " << latDeg;

		_txtDebug->setText(ss.str());
	}
}

/**
 * Opens the Intercept window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnInterceptClick(Action*)
{
	_game->pushState(new InterceptState(
									_globe,
									NULL,
//									NULL,
									this));
}

/**
 * Goes to the Basescape screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnBasesClick(Action*)
{
	soundPop->play(Mix_GroupAvailable(0));
	timerReset();

	if (_savedGame->getBases()->empty() == false)
	{
		if (kL_currentBase == 0
			|| kL_currentBase >= _savedGame->getBases()->size())
		{
			_game->pushState(new BasescapeState(
											_savedGame->getBases()->front(),
											_globe));
		}
		else
			_game->pushState(new BasescapeState(
											_savedGame->getBases()->at(kL_currentBase),
											_globe));
	}
	else
		_game->pushState(new BasescapeState(
										NULL,
										_globe));
}

/**
 * Goes to the Graphs screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnGraphsClick(Action*)
{
	soundPop->play(Mix_GroupAvailable(0));

	timerReset();
	_game->pushState(new GraphsState(_savedGame->getCurrentGraph()));
}

/**
 * Goes to the Ufopaedia window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnUfopaediaClick(Action*)
{
	timerReset();
	Ufopaedia::open(_game);
}

/**
 * Opens the Options window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnOptionsClick(Action*)
{
	timerReset();
	_game->pushState(new PauseState(OPT_GEOSCAPE));
}

/**
 * Goes to the Funding screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnFundingClick(Action*)
{
	timerReset();
	_game->pushState(new FundingState());
}

/**
 * Handler for clicking the Detail area.
 * @param action, Pointer to an action.
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
 * Stops rotating the globe to the left.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftRelease(Action*)
{
	_globe->rotateStopLon();
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
 * Stops rotating the globe to the right.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightRelease(Action*)
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
 * Stops rotating the globe upwards.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateUpRelease(Action*)
{
	_globe->rotateStopLat();
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
 * Stops rotating the globe downwards.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateDownRelease(Action*)
{
	_globe->rotateStopLat();
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
 * Zooms the globe maximum.
 * @param action - pointer to an Action
 */
/* void GeoscapeState::btnZoomInRightClick(Action*)
{
	_globe->zoomMax();
} */

/**
 * Zooms out of the globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnZoomOutLeftClick(Action*)
{
	_globe->zoomOut();
}

/**
 * Zooms the globe minimum.
 * @param action - pointer to an Action
 */
/* void GeoscapeState::btnZoomOutRightClick(Action*)
{
	_globe->zoomMin();
} */

/**
 * Gets the Timer for dogfight zoom-ins.
 * @return, pointer to the zoom-in Timer
 */
Timer* GeoscapeState::getDogfightZoomInTimer() const
{
	return _dfZoomInTimer;
}

/**
 * Gets the Timer for dogfight zoom-outs.
 * @return, pointer to the zoom-out Timer
 */
Timer* GeoscapeState::getDogfightZoomOutTimer() const
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

		_globe->center(
					_dfLon,
					_dfLat);

		init();
	}
}

/**
 * Sets current dogfight's current Globe coordinates.
 * @param craft - pointer to relevant interception Craft
 */
void GeoscapeState::setDogfightCoords(Craft* craft)
{
	_dfLon = _savedGame->getGlobeLongitude();
	_dfLat = _savedGame->getGlobeLatitude();

	_globe->center(
				craft->getLongitude(),
				craft->getLatitude());
}

/**
 * Dogfight logic.
 */
void GeoscapeState::handleDogfights()
{
	// if all dogfights are minimized rotate the globe, etc.
	if (_dogfights.size() == _minimizedDogfights)
	{
		_pause = false;
		_gameTimer->think(this, NULL);
	}

	_minimizedDogfights = 0; // handle dogfights logic.

	std::list<DogfightState*>::const_iterator i = _dogfights.begin();
	while (i != _dogfights.end())
	{
		if ((*i)->isMinimized() == true)
			++_minimizedDogfights;
//		else
//			_globe->rotateStop();

		(*i)->think();

		if ((*i)->dogfightEnded() == true)
		{
			if ((*i)->isMinimized() == true)
				--_minimizedDogfights;

			delete *i;
			i = _dogfights.erase(i);
		}
		else
			++i;
	}

	if (_dogfights.empty() == true)
	{
		_dogfightEnded = true;
		_globe->setChasingUfo(false);
		_dfZoomOutTimer->start();
	}
}

/**
 * Gets the number of minimized dogfights.
 * @return, number of minimized dogfights
 */
int GeoscapeState::minimizedDogfightsCount() const
{
	int ret = 0;

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
 * Starts a new dogfight.
 */
void GeoscapeState::startDogfight()
{
	if (_globe->getPreDogfightZoom() == _globe->getZoomLevels())
		_globe->setPreDogfightZoom();

	if (_globe->getZoom() < _globe->getZoomLevels() - 1)
	{
		if (_dfZoomInTimer->isRunning() == false)
		{
//			_globe->rotateStop();
			_dfZoomInTimer->start();
		}
	}
	else
	{
		_dfStartTimer->stop();
		_dfZoomInTimer->stop();

		timerReset();

		while (_dogfightsToStart.empty() == false)
		{
			_dogfights.push_back(_dogfightsToStart.back());
			_dogfightsToStart.pop_back();

			_dogfights.back()->setInterceptSlot(getNextDogfightSlot());
			_dogfights.back()->setInterceptQty(_dogfights.size() + _dogfightsToStart.size());
		}

		// Set correct number of interceptions for every [all] dogfight[s].
		for (std::list<DogfightState*>::const_iterator
				i = _dogfights.begin();
				i != _dogfights.end();
				++i)
		{
			(*i)->setInterceptQty(_dogfights.size());
		}
	}
}

/**
 * Gets the first free dogfight slot available.
 * @return, the next slot open
 */
int GeoscapeState::getNextDogfightSlot() const
{
	int slot = 1;

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
 * @param ufo	- pointer to Ufo attacking base
 */
void GeoscapeState::handleBaseDefense(
		Base* base,
		Ufo* ufo)
{
	ufo->setStatus(Ufo::DESTROYED);

	if (base->getAvailableSoldiers(true) > 0)
//		|| !base->getVehicles()->empty())
	{
		SavedBattleGame* const battle = new SavedBattleGame();
		_savedGame->setBattleGame(battle);
		battle->setMissionType("STR_BASE_DEFENSE");

		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.setBase(base);
		bgen.setAlienRace(ufo->getAlienRace());
		bgen.run();

		_pause = true;

		popup(new BriefingState(
							NULL,
							base));
	}
	else
		popup(new BaseDestroyedState(
									base,
									_globe));
}

/**
 * Determine the alien missions to start this month.
 * In the vanilla game each month a terror mission
 * and one other are started in random regions.
 */
void GeoscapeState::determineAlienMissions(bool atGameStart)
{
	if (atGameStart == false)
	{
		//
		// One randomly selected mission.
		//
		AlienStrategy& strategy = _savedGame->getAlienStrategy();
		const std::string& targetRegion = strategy.chooseRandomRegion(_game->getRuleset());
		const std::string& targetMission = strategy.chooseRandomMission(targetRegion);

		// Choose race for this mission.
		const RuleAlienMission& missionRules = *_game->getRuleset()->getAlienMission(targetMission);
		const std::string& missionRace = missionRules.generateRace(_savedGame->getMonthsPassed());

		AlienMission* const otherMission = new AlienMission(
														missionRules,
														*_savedGame);
		otherMission->setId(_savedGame->getId("ALIEN_MISSIONS"));
		otherMission->setRegion(targetRegion, *_game->getRuleset());
		otherMission->setRace(missionRace);
		otherMission->start();

		_savedGame->getAlienMissions().push_back(otherMission);

		// Make sure this combination never comes up again.
		strategy.removeMission(targetRegion, targetMission);
	}
	else
	{
		//
		// Sectoid Research at base's region. haha
		//
		AlienStrategy& strategy = _savedGame->getAlienStrategy();
		std::string targetRegion = _savedGame->locateRegion(*_savedGame->getBases()->front())->getRules()->getType();

		// Choose race for this mission.
		std::string research = _game->getRuleset()->getAlienMissionList().front();
		const RuleAlienMission& missionRules = *_game->getRuleset()->getAlienMission(research);

		AlienMission* const otherMission = new AlienMission(
														missionRules,
														*_savedGame);
		otherMission->setId(_savedGame->getId("ALIEN_MISSIONS"));
		otherMission->setRegion(targetRegion, *_game->getRuleset());
		std::string sectoid = missionRules.getTopRace(_savedGame->getMonthsPassed());
		otherMission->setRace(sectoid);
		otherMission->start(150);

		_savedGame->getAlienMissions().push_back(otherMission);

		// Make sure this combination never comes up again.
		strategy.removeMission(targetRegion, research);
	}
}

/**
 * Sets up a Terror mission.
 */
void GeoscapeState::setupTerrorMission()
{
	// Determine a random region with at least one city and no currently running terror mission.
	RuleRegion* region = NULL;
	int counter = 0;
	const std::vector<std::string> regions = _game->getRuleset()->getRegionsList();

	do // we try 40 times to pick a valid zone for a terror mission
	{
		if (counter == 40)
			return;

		region = _game->getRuleset()->getRegion(regions[RNG::generate(
																	0,
																	static_cast<int>(regions.size()) - 1)]);

		++counter;
	}
	while (region->getCities()->empty() == true
		|| _savedGame->getAlienMission(
									region->getType(),
									"STR_ALIEN_TERROR") != NULL);

	// Choose race for terror mission.
	const RuleAlienMission& terrorRules = *_game->getRuleset()->getAlienMission("STR_ALIEN_TERROR");
	const std::string& terrorRace = terrorRules.generateRace(_savedGame->getMonthsPassed());

	AlienMission* const terrorMission = new AlienMission(
													terrorRules,
													*_savedGame);
	terrorMission->setId(_savedGame->getId("ALIEN_MISSIONS"));
	terrorMission->setRegion(
						region->getType(),
						*_game->getRuleset());
	terrorMission->setRace(terrorRace);
	terrorMission->start(150);

	_savedGame->getAlienMissions().push_back(terrorMission);
}

/**
 * Handler for clicking on a timer button.
 * @param action - pointer to the mouse Action
 */
void GeoscapeState::btnTimerClick(Action* action)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEBUTTONDOWN;
	ev.button.button = SDL_BUTTON_LEFT;
	Action a = Action(&ev, 0., 0., 0, 0);
	action->getSender()->mousePress(&a, this);
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
	if (_savedGame->getSavedBattle() != NULL)
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

//kL	_bg->setX((_globe->getWidth() - _bg->getWidth()) / 2);
//kL	_bg->setY((_globe->getHeight() - _bg->getHeight()) / 2);

/*
// kL_begin:
	int height = ((Options::baseYResolution - Screen::ORIGINAL_HEIGHT) / 2) - 1;
	_sideTop->setHeight(height);
	_sideTop->setY(0);
	_sideBottom->setHeight(height);
	_sideBottom->setY(Options::baseYResolution - height + 1);
// kL_end.
*/
/*kL
	int height = (Options::baseYResolution - Screen::ORIGINAL_HEIGHT) / 2 + 10;
	_sideTop->setHeight(height);
	_sideTop->setY(_sidebar->getY() - height - 1);
	_sideBottom->setHeight(height);
	_sideBottom->setY(_sidebar->getY() + _sidebar->getHeight() + 1);

	_sideLine->setHeight(Options::baseYResolution);
	_sideLine->setY(0);
	_sideLine->drawRect(0, 0, _sideLine->getWidth(), _sideLine->getHeight(), 15); */
}

}
