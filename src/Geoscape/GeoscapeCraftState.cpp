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

#include "GeoscapeCraftState.h"

//#include <sstream>

#include "GeoscapeState.h"
#include "Globe.h"
#include "SelectDestinationState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the GeoscapeCraft window.
 * @param craft		- pointer to a Craft for display
 * @param geo		- pointer to the Geoscape
 * @param waypoint	- pointer to the last UFO position if redirecting the craft (default NULL)
 * @param doublePop	- true if two windows need to pop on exit (default false)
 * @param transpose	- true to start the state transposed (default false)
 */
GeoscapeCraftState::GeoscapeCraftState(
		Craft* craft,
		GeoscapeState* geo,
		Waypoint* waypoint,
		bool doublePop,
		bool transpose)
	:
		_craft(craft),
		_geo(geo),
		_waypoint(waypoint),
		_doublePop(doublePop),
		_delayPop(true)
{
	_screen = false;

	_window			= new Window(this, 224, 176, 16, 8, POPUP_BOTH);

	_txtTitle		= new Text(192, 17, 32, 15);

	_sprite			= new Surface(
								32,38,
								_window->getX() + _window->getWidth() - 16,
								_window->getY() - 11);

	_txtKills		= new Text(60, 9, _sprite->getX() - 61, 15);

	_txtStatus		= new Text(192, 17,  32, 31);
	_txtBase		= new Text(192,  9,  32, 43);
	_txtRedirect	= new Text(120, 17, 120, 46);
	_txtSpeed		= new Text(192,  9,  32, 55);
	_txtMaxSpeed	= new Text(120,  9,  32, 64);
	_txtSoldier		= new Text( 80,  9, 160, 64);
	_txtAltitude	= new Text(120,  9,  32, 73);
	_txtHWP			= new Text( 80,  9, 160, 73);
	_txtFuel		= new Text(120,  9,  32, 82);
	_txtDamage		= new Text( 80,  9, 160, 82);

	_txtW1Name		= new Text(120,  9,  32,  91);
	_txtW1Ammo		= new Text( 80,  9, 160,  91);
	_txtW2Name		= new Text(120,  9,  32, 100);
	_txtW2Ammo		= new Text( 80,  9, 160, 100);

	_btnTarget		= new TextButton( 90, 16,  32, 114);
	_btnPatrol		= new TextButton( 90, 16, 134, 114);
	_btnCenter		= new TextButton(192, 16,  32, 136);
	_btnBase		= new TextButton( 90, 16,  32, 158);
	_btnCancel		= new TextButton( 90, 16, 134, 158);

	_srfTarget		= new Surface(29, 29, 114, 86);

	setInterface("geoCraftScreens");

	add(_window,		"window",	"geoCraftScreens");
	add(_txtTitle,		"text1",	"geoCraftScreens");
	add(_txtKills,		"text1",	"geoCraftScreens");
	add(_sprite);
	add(_txtStatus,		"text1",	"geoCraftScreens");
	add(_txtBase,		"text3",	"geoCraftScreens");
	add(_txtRedirect,	"text3",	"geoCraftScreens");
	add(_txtSpeed,		"text3",	"geoCraftScreens");
	add(_txtMaxSpeed,	"text3",	"geoCraftScreens");
	add(_txtSoldier,	"text3",	"geoCraftScreens");
	add(_txtAltitude,	"text3",	"geoCraftScreens");
	add(_txtHWP,		"text3",	"geoCraftScreens");
	add(_txtFuel,		"text3",	"geoCraftScreens");
	add(_txtDamage,		"text3",	"geoCraftScreens");
	add(_txtW1Name,		"text3",	"geoCraftScreens");
	add(_txtW1Ammo,		"text3",	"geoCraftScreens");
	add(_txtW2Name,		"text3",	"geoCraftScreens");
	add(_txtW2Ammo,		"text3",	"geoCraftScreens");

	add(_btnTarget,		"button",	"geoCraftScreens");
	add(_btnPatrol,		"button",	"geoCraftScreens");
	add(_btnCenter,		"button",	"geoCraftScreens");
	add(_btnBase,		"button",	"geoCraftScreens");
	add(_btnCancel,		"button",	"geoCraftScreens");

	add(_srfTarget);

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCenterClick);

	_btnBase->setText(tr("STR_RETURN_TO_BASE"));
	_btnBase->onMouseClick((ActionHandler)& GeoscapeCraftState::btnBaseClick);

	_btnTarget->setText(tr("STR_SELECT_NEW_TARGET"));
	_btnTarget->onMouseClick((ActionHandler)& GeoscapeCraftState::btnTargetClick);

	_btnPatrol->setText(tr("STR_PATROL"));
	_btnPatrol->onMouseClick((ActionHandler)& GeoscapeCraftState::btnPatrolClick);

	_btnCancel->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnCancelClick,
					Options::keyCancel);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnCancelClick,
					Options::keyOk);

	_txtTitle->setText(_craft->getName(_game->getLanguage()));
	_txtTitle->setBig();

	if (_craft->getRules()->getWeapons() != 0)
	{
		_txtKills->setText(tr("STR_KILLS_LC_").arg(_craft->getKills()));
		_txtKills->setAlign(ALIGN_RIGHT);
	}


	const std::string stat = _craft->getCraftStatus();
	std::wstring status;
	const bool
		lowFuel = _craft->getLowFuel(),
		missionComplete = _craft->getTacticalReturn();
	int speed = _craft->getSpeed();

	// note: Could add "DAMAGED - Return to Base" around here.
	if (stat != "STR_OUT")
		status = tr("STR_BASED");
	else if (lowFuel)
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
	else if (missionComplete)
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
	else if (_craft->getDestination() == dynamic_cast<Target*>(_craft->getBase()))
		status = tr("STR_RETURNING_TO_BASE");
	else if (_craft->getDestination() == NULL)
		status = tr("STR_PATROLLING");
	else
	{
		const Ufo* const ufo = dynamic_cast<Ufo*>(_craft->getDestination());
		if (ufo != NULL)
		{
			if (_craft->isInDogfight() == true)
			{
				speed = ufo->getSpeed();	// THIS DOES NOT CHANGE THE SPEED of the xCom CRAFT
											// for Fuel usage. (ie. it should)
				status = tr("STR_TAILING_UFO").arg(ufo->getId());
			}
			else if (ufo->getUfoStatus() == Ufo::FLYING)
				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
			else
				status = tr("STR_DESTINATION_UC_")
							.arg(ufo->getName(_game->getLanguage()));
		}
		else
			status = tr("STR_DESTINATION_UC_")
						.arg(_craft->getDestination()->getName(_game->getLanguage()));
	}
	_txtStatus->setText(tr("STR_STATUS_").arg(status));

	_txtBase->setText(tr("STR_BASE_UC").arg(_craft->getBase()->getName()));


	std::wostringstream
		woststr1,
		woststr2,
		woststr3,
		woststr4,
		woststr5;

	woststr1 << tr("STR_SPEED_").arg(Text::formatNumber(speed));
	_txtSpeed->setText(woststr1.str());

	_txtMaxSpeed->setText(tr("STR_MAXIMUM_SPEED_UC")
							.arg(Text::formatNumber(_craft->getRules()->getMaxSpeed())));

	std::string alt;
	if (stat == "STR_READY"
		|| stat == "STR_REPAIRS"
		|| stat == "STR_REFUELLING"
		|| stat == "STR_REARMING")
	{
		alt = "STR_GROUND";
	}
	else
		alt = _craft->getAltitude();

	_txtAltitude->setText(tr("STR_ALTITUDE_").arg(tr(alt)));

	_txtFuel->setText(tr("STR_FUEL").arg(Text::formatPct(_craft->getFuelPct())));
	_txtDamage->setText(tr("STR_HULL_").arg(Text::formatPct(100 - _craft->getCraftDamagePct())));


	if (_craft->getRules()->getSoldiers() > 0)
	{
		woststr2 << tr("STR_SOLDIERS") << L" " << L'\x01' << _craft->getNumSoldiers();
		woststr2 << L" (" << _craft->getRules()->getSoldiers() << L")";
		_txtSoldier->setText(woststr2.str());
	}
	else
		_txtSoldier->setVisible(false);

	if (_craft->getRules()->getVehicles() > 0)
	{
		woststr3 << tr("STR_HWPS") << L" " << L'\x01' << _craft->getNumVehicles();
		woststr3 << L" (" << _craft->getRules()->getVehicles() << L")";
		_txtHWP->setText(woststr3.str());
	}
	else
		_txtHWP->setVisible(false);


	if (_craft->getRules()->getWeapons() > 0
		&& _craft->getWeapons()->at(0) != NULL)
	{
		const CraftWeapon* const w1 = _craft->getWeapons()->at(0);
		_txtW1Name->setText(tr("STR_WEAPON_ONE").arg(tr(w1->getRules()->getType())));

		woststr4 << tr("STR_ROUNDS_").arg(w1->getAmmo());
		woststr4 << L" (" << w1->getRules()->getAmmoMax() << L")";
		_txtW1Ammo->setText(woststr4.str());
	}
	else
	{
		_txtW1Name->setVisible(false);
		_txtW1Ammo->setVisible(false);
	}

	if (_craft->getRules()->getWeapons() > 1
		&& _craft->getWeapons()->at(1) != NULL)
	{
		const CraftWeapon* const w2 = _craft->getWeapons()->at(1);
		_txtW2Name->setText(tr("STR_WEAPON_TWO").arg(tr(w2->getRules()->getType())));

		woststr5 << tr("STR_ROUNDS_").arg(w2->getAmmo());
		woststr5 << L" (" << w2->getRules()->getAmmoMax() << L")";
		_txtW2Ammo->setText(woststr5.str());
	}
	else
	{
		_txtW2Name->setVisible(false);
		_txtW2Ammo->setVisible(false);
	}


	if (_waypoint == NULL)
	{
		_txtRedirect->setVisible(false);
		_btnCancel->setText(tr("STR_CANCEL_UC"));
	}
	else
	{
		_txtRedirect->setText(tr("STR_REDIRECT_CRAFT"));
		_txtRedirect->setAlign(ALIGN_CENTER);
		_txtRedirect->setBig();
		_btnCancel->setText(tr("STR_GO_TO_LAST_KNOWN_UFO_POSITION"));
	}

	// set Base button visibility FALSE for already-Based crafts.
	// note these could be set up there where status was set.....
	if (stat != "STR_OUT"
		|| lowFuel == true
		|| missionComplete == true
		|| _craft->getTakeoff() == false)
	{
		_btnBase->setVisible(false);
		_btnPatrol->setVisible(false);

		if (stat == "STR_REPAIRS"
			|| stat == "STR_REFUELLING"
			|| stat == "STR_REARMING"
			|| lowFuel == true
			|| missionComplete == true
			|| _craft->getTakeoff() == false)
		{
			_btnTarget->setVisible(false);
		}
	}
	else
	{
		if (_craft->getDestination() == dynamic_cast<Target*>(_craft->getBase()))
			_btnBase->setVisible(false);
		else if (_craft->getDestination() == NULL
			&& waypoint == NULL)
		{
			_btnPatrol->setVisible(false);
		}
	}


	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("INTICON.PCK");
	const int craftSprite = _craft->getRules()->getSprite();
	srt->getFrame(craftSprite + 11)->blit(_sprite);

	if (transpose == true)
	{
		_delayPop = false;
		transposeWindow();
	}
}

/**
 * dTor.
 */
GeoscapeCraftState::~GeoscapeCraftState()
{}

/**
 * Centers the Craft on the globe.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnCenterClick(Action*)
{
	_geo->getGlobe()->center(
						_craft->getLongitude(),
						_craft->getLatitude());

	if (_doublePop == true)
	{
		_game->popState();
		_game->popState();

		_game->pushState(new GeoscapeCraftState(
											_craft,
											_geo,
											NULL,
											false,
											true));
		delete _waypoint;
		return;
	}

	if (_delayPop == true)
	{
		_delayPop = false;
		transposeWindow();
		return;
	}

	_geo->setPaused();
	_geo->resetTimer();
	_game->popState();

	delete _waypoint;
}

/**
 * Returns the Craft back to its base.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnBaseClick(Action*)
{
	if (_doublePop == true)
		_game->popState();

	_game->popState();
	_craft->returnToBase();

	delete _waypoint;
}

/**
 * Changes the Craft's target.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnTargetClick(Action*)
{
	if (_doublePop == true)
		_game->popState();

	_game->popState();
	_game->pushState(new SelectDestinationState(
											_craft,
											_geo->getGlobe()));
	delete _waypoint;
}

/**
 * Sets the Craft to patrol the current location.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnPatrolClick(Action*)
{
	if (_doublePop == true)
		_game->popState();

	_game->popState();
	_craft->setDestination(NULL);

	delete _waypoint;
}

/**
 * Closes the window.
 * @note The button doubles as the redirect Craft btn.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnCancelClick(Action*)
{
	if (_waypoint != NULL) // Go to the last known UFO position
	{
		_waypoint->setId(_game->getSavedGame()->getCanonicalId("STR_WAYPOINT"));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
		_craft->setDestination(_waypoint);
	}

	_game->popState(); // and Cancel.
}

/**
 * Hides various screen-elements to reveal the globe & Craft.
 */
void GeoscapeCraftState::transposeWindow() // private.
{
	_window->setVisible(false);

	_txtRedirect->setVisible(false);
	_txtSpeed->setVisible(false);
	_txtMaxSpeed->setVisible(false);
	_txtSoldier->setVisible(false);
	_txtAltitude->setVisible(false);
	_txtHWP->setVisible(false);
	_txtFuel->setVisible(false);
	_txtDamage->setVisible(false);
	_txtW1Name->setVisible(false);
	_txtW1Ammo->setVisible(false);
	_txtW2Name->setVisible(false);
	_txtW2Ammo->setVisible(false);

	int dy = 26;
	_btnTarget->setY(_btnTarget->getY() + dy);
	_btnPatrol->setY(_btnPatrol->getY() + dy);
	_btnCenter->setY(_btnCenter->getY() + dy);
	_btnBase->setY(_btnBase->getY() + dy);
	_btnCancel->setY(_btnCancel->getY() + dy);

	if (_geo->getPaused() == false)
		_btnCenter->setText(tr("STR_PAUSE"));
	else
	{
		_btnCenter->setVisible(false);

		dy = _btnCenter->getY();
		_btnTarget->setY(dy);
		_btnPatrol->setY(dy);
	}

	_game->getResourcePack()->getSurface("TARGET_UFO")->blit(_srfTarget);
}

}
