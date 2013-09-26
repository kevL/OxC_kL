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

#include <sstream>
#include "GeoscapeCraftState.h"
#include "SelectDestinationState.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
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
 * Initializes all the elements in the Geoscape Craft window.
 * @param game, Pointer to the core game.
 * @param craft, Pointer to the craft to display.
 * @param globe, Pointer to the Geoscape globe.
 * @param waypoint, Pointer to the last UFO position (if redirecting the craft).
 */
GeoscapeCraftState::GeoscapeCraftState(Game* game, Craft* craft, Globe* globe, Waypoint* waypoint)
	:
	State(game),
	_craft(craft),
	_globe(globe),
	_waypoint(waypoint)
{
	_screen = false;

	// Create objects
	_window			= new Window(this, 240, 184, 8, 8, POPUP_BOTH);

	_btnCenter		= new TextButton(192, 12, 32, 112);		// kL
	_btnBase		= new TextButton(192, 12, 32, 127);
	_btnTarget		= new TextButton(192, 12, 32, 142);
	_btnPatrol		= new TextButton(192, 12, 32, 157);
	_btnCancel		= new TextButton(192, 12, 32, 172);

	_txtTitle		= new Text(200, 16, 32, 20);
	_txtStatus		= new Text(210, 16, 32, 36);

	// kL. move these up 1px
	_txtBase		= new Text(200, 9, 32, 51);

	_txtSpeed		= new Text(200, 9, 32, 59);

	_txtMaxSpeed	= new Text(200, 9, 32, 67);
	_txtSoldier		= new Text(200, 9, 161, 67);		// kL

	_txtAltitude	= new Text(200, 9, 32, 75);
	_txtHWP			= new Text(200, 9, 161, 75);		// kL

	_txtFuel		= new Text(120, 9, 32, 83);
	_txtDamage		= new Text(75, 9, 161, 83);			// kL: moved left 3px

	_txtW1Name		= new Text(120, 9, 32, 91);
	_txtW1Ammo		= new Text(60, 9, 161, 91);			// kL: moved left 3px

	_txtW2Name		= new Text(120, 9, 32, 99);
	_txtW2Ammo		= new Text(60, 9, 161, 99);			// kL: moved left 3px
	// kL. end up.

//kL	_txtRedirect	= new Text(230, 16, 13, 108);	// kL_note: move up horizontal w/ Base
	_txtRedirect	= new Text(120, 16, 120, 50);		// kL: move up horizontal w/ Base

	// Set palette
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(4)), Palette::backPos, 16);

	add(_window);
	add(_btnCenter);
	add(_btnBase);
	add(_btnTarget);
	add(_btnPatrol);
	add(_btnCancel);
	add(_txtTitle);
	add(_txtStatus);
	add(_txtBase);
	add(_txtSpeed);
	add(_txtMaxSpeed);
	add(_txtSoldier);	// kL
	add(_txtAltitude);
	add(_txtHWP);		// kL
	add(_txtFuel);
	add(_txtDamage);
	add(_txtW1Name);
	add(_txtW1Ammo);
	add(_txtW2Name);
	add(_txtW2Ammo);
	add(_txtRedirect);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnCenter->setColor(Palette::blockOffset(8)+5);
	_btnCenter->setText(_game->getLanguage()->getString("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCenterClick);

	_btnBase->setColor(Palette::blockOffset(8)+5);
	_btnBase->setText(_game->getLanguage()->getString("STR_RETURN_TO_BASE"));
	_btnBase->onMouseClick((ActionHandler)& GeoscapeCraftState::btnBaseClick);

	_btnTarget->setColor(Palette::blockOffset(8)+5);
	_btnTarget->setText(_game->getLanguage()->getString("STR_SELECT_NEW_TARGET"));
	_btnTarget->onMouseClick((ActionHandler)& GeoscapeCraftState::btnTargetClick);

	_btnPatrol->setColor(Palette::blockOffset(8)+5);
	_btnPatrol->setText(_game->getLanguage()->getString("STR_PATROL"));
	_btnPatrol->onMouseClick((ActionHandler)& GeoscapeCraftState::btnPatrolClick);

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(_game->getLanguage()->getString("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)& GeoscapeCraftState::btnCancelClick, (SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setText(_craft->getName(_game->getLanguage()));

	_txtStatus->setColor(Palette::blockOffset(15)-1);
	_txtStatus->setSecondaryColor(Palette::blockOffset(8)+10);
	_txtStatus->setWordWrap(true);

	std::wstringstream ss;
	ss << _game->getLanguage()->getString("STR_STATUS_") << L'\x01';
	if (_waypoint != 0)
	{
		ss << _game->getLanguage()->getString("STR_INTERCEPTING_UFO") << _waypoint->getId();
	}
	else if (_craft->getLowFuel())
	{
		ss << _game->getLanguage()->getString("STR_LOW_FUEL_RETURNING_TO_BASE");
	}
	// kL_begin: craft string, Based
//	else if (_craft->getDestination() == 0
	else if (_craft->getStatus() == "STR_READY"
				|| _craft->getStatus() == "STR_REPAIRS"
				|| _craft->getStatus() == "STR_REFUELLING"
				|| _craft->getStatus() == "STR_REARMING")
	{
		ss << _game->getLanguage()->getString("STR_BASED");
	}
	// Could add "Damaged - returning to base" around here.
	// kL_end.
	else if (_craft->getDestination() == 0)
	{
		ss << _game->getLanguage()->getString("STR_PATROLLING");
	}
	else if (_craft->getDestination() == (Target* )_craft->getBase())
	{
		ss << _game->getLanguage()->getString("STR_RETURNING_TO_BASE");
	}
	else
	{
		Ufo* u = dynamic_cast<Ufo* >(_craft->getDestination());
		if (u != 0)
		{
			if (u->getStatus() == Ufo::FLYING)
			{
				ss << _game->getLanguage()->getString("STR_INTERCEPTING_UFO") << u->getId();
			}
			else
			{
				ss << _game->getLanguage()->getString("STR_DESTINATION_UC_") << u->getName(_game->getLanguage());
			}
		}
		else
		{
			ss << _game->getLanguage()->getString("STR_DESTINATION_UC_") << _craft->getDestination()->getName(_game->getLanguage());
		}
	}

	_txtStatus->setText(ss.str());

	_txtBase->setColor(Palette::blockOffset(15)-1);
	_txtBase->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss2;
	ss2 << _game->getLanguage()->getString("STR_BASE_UC_") << L'\x01' << _craft->getBase()->getName();
	_txtBase->setText(ss2.str());

	_txtSpeed->setColor(Palette::blockOffset(15)-1);
	_txtSpeed->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss3;
	if (_craft->isInDogfight())		// kL
	{
		ss3 << _game->getLanguage()->getString("STR_SPEED_") << L'\x01' << "UFO";
	}
	// kL_note: If in dogfight or more accurately in chase_mode, insert UFO speed.
	else
	{
		ss3 << _game->getLanguage()->getString("STR_SPEED_") << L'\x01' << _craft->getSpeed();
	}
	_txtSpeed->setText(ss3.str());

	_txtMaxSpeed->setColor(Palette::blockOffset(15)-1);
	_txtMaxSpeed->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss4;
	ss4 << _game->getLanguage()->getString("STR_MAXIMUM_SPEED_UC") << L'\x01' << _craft->getRules()->getMaxSpeed();
	_txtMaxSpeed->setText(ss4.str());

	// kL_begin: GeoscapeCraftState, add #Soldier on transports.
	_txtSoldier->setColor(Palette::blockOffset(15)-1);
	_txtSoldier->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss12;
	ss12 << _game->getLanguage()->getString("STR_SOLDIERS") << " " << L'\x01' << _craft->getNumSoldiers();
	_txtSoldier->setText(ss12.str());
	// kL_end.

	_txtAltitude->setColor(Palette::blockOffset(15)-1);
	_txtAltitude->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss5;
	// kL_begin: GeoscapeCraftState, add #HWP on transports. ->Moved to Craft.cpp
	std::string altitude;
	if (_craft->getAltitude() == "STR_GROUND"
		|| _craft->getStatus() == "STR_READY"
		|| _craft->getStatus() == "STR_REPAIRS"
		|| _craft->getStatus() == "STR_REFUELLING"
		|| _craft->getStatus() == "STR_REARMING")
	{
		altitude = "STR_GROUNDED";
	}
	else altitude = _craft->getAltitude();
	// kL_end.

//kL	std::string altitude = _craft->getAltitude() == "STR_GROUND" ? "STR_GROUNDED" : _craft->getAltitude();
//	std::string altitude = _craft->getAltitude();																		// kL
	ss5 << _game->getLanguage()->getString("STR_ALTITUDE_") << L'\x01' << _game->getLanguage()->getString(altitude);	// kL
	_txtAltitude->setText(ss5.str());

	// kL_begin: GeoscapeCraftState, add #HWP on transports.
	_txtHWP->setColor(Palette::blockOffset(15)-1);
	_txtHWP->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss11;
	ss11 << _game->getLanguage()->getString("STR_HWPS") << " " << L'\x01' << _craft->getNumVehicles();
	_txtHWP->setText(ss11.str());
	// kL_end.

	_txtFuel->setColor(Palette::blockOffset(15)-1);
	_txtFuel->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss6;
//kL	ss6 << _game->getLanguage()->getString("STR_FUEL") << L'\x01' << _craft->getFuelPercentage() << "%";
	ss6 << _game->getLanguage()->getString("STR_FUEL") << L'\x01' << _craft->getFuelPercentage();		// kL
	_txtFuel->setText(ss6.str());

	_txtDamage->setColor(Palette::blockOffset(15)-1);
	_txtDamage->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss62;
//kL	ss62 << _game->getLanguage()->getString("STR_DAMAGE_UC_") << L'\x01' << _craft->getDamagePercentage() << "%";
	ss62 << _game->getLanguage()->getString("STR_DAMAGE_UC_") << L'\x01' << _craft->getDamagePercentage();		// kL
	_txtDamage->setText(ss62.str());

	_txtW1Name->setColor(Palette::blockOffset(15)-1);
	_txtW1Name->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss7;
	ss7 << _game->getLanguage()->getString("STR_WEAPON_ONE") << L'\x01';

	_txtW1Ammo->setColor(Palette::blockOffset(15)-1);
	_txtW1Ammo->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss8;
	ss8 << _game->getLanguage()->getString("STR_ROUNDS_") << L'\x01';

	if (_craft->getRules()->getWeapons() > 0
		&& _craft->getWeapons()->at(0) != 0)
	{
		CraftWeapon* w1 = _craft->getWeapons()->at(0);

		ss7 << _game->getLanguage()->getString(w1->getRules()->getType());
		_txtW1Name->setText(ss7.str());

		ss8 << w1->getAmmo();
		_txtW1Ammo->setText(ss8.str());
	}
	else
	{
		ss7 << _game->getLanguage()->getString("STR_NONE_UC");
		_txtW1Name->setText(ss7.str());
		_txtW1Ammo->setVisible(false);
	}

	_txtW2Name->setColor(Palette::blockOffset(15)-1);
	_txtW2Name->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss9;
	ss9 << _game->getLanguage()->getString("STR_WEAPON_TWO") << L'\x01';

	_txtW2Ammo->setColor(Palette::blockOffset(15)-1);
	_txtW2Ammo->setSecondaryColor(Palette::blockOffset(8)+5);
	std::wstringstream ss10;
	ss10 << _game->getLanguage()->getString("STR_ROUNDS_") << L'\x01';

	if (_craft->getRules()->getWeapons() > 1
		&& _craft->getWeapons()->at(1) != 0)
	{
		CraftWeapon* w2 = _craft->getWeapons()->at(1);

		ss9 << _game->getLanguage()->getString(w2->getRules()->getType());
		_txtW2Name->setText(ss9.str());

		ss10 << w2->getAmmo();
		_txtW2Ammo->setText(ss10.str());
	}
	else
	{
		ss9 << _game->getLanguage()->getString("STR_NONE_UC");
		_txtW2Name->setText(ss9.str());
		_txtW2Ammo->setVisible(false);
	}

	_txtRedirect->setColor(Palette::blockOffset(15)-1);
	_txtRedirect->setBig();
	_txtRedirect->setAlign(ALIGN_CENTER);
	_txtRedirect->setText(_game->getLanguage()->getString("STR_REDIRECT_CRAFT"));

	if (_waypoint == 0)
	{
		_txtRedirect->setVisible(false);
	}
	else
	{
		_btnCancel->setText(_game->getLanguage()->getString("STR_GO_TO_LAST_KNOWN_UFO_POSITION"));
	}

/*kL	if (_craft->getLowFuel())
	{
		_btnBase->setVisible(false);
		_btnTarget->setVisible(false);
		_btnPatrol->setVisible(false);
	} */

	// kL_begin: set Base button visibility FALSE for already-Based crafts.
	if (_craft->getStatus() == "STR_READY")
	{
		_btnBase->setVisible(false);
		_btnPatrol->setVisible(false);
	}

	if (_craft->getStatus() == "STR_REPAIRS"
		|| _craft->getStatus() == "STR_REFUELLING"
		|| _craft->getStatus() == "STR_REARMING"
		|| _craft->getLowFuel())
	{
		_btnBase->setVisible(false);
		_btnTarget->setVisible(false);
		_btnPatrol->setVisible(false);
	}

	if (_craft->getRules()->getSoldiers() == 0)
	{
		_txtSoldier->setVisible(false);
	}

	if (_craft->getRules()->getVehicles() == 0)
	{
		_txtHWP->setVisible(false);
	}
	// kL_end.
}

/**
 *
 */
GeoscapeCraftState::~GeoscapeCraftState()
{
}

/**
 * Resets the palette.
 */
void GeoscapeCraftState::init()
{
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(4)), Palette::backPos, 16);
}

/**
 * Centers the craft on the globe.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnCenterClick(Action* )
{
	_game->popState();

	_globe->center(_craft->getLongitude(), _craft->getLatitude());
}

/**
 * Returns the craft back to its base.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnBaseClick(Action* )
{
	_game->popState();
	_craft->returnToBase();

	delete _waypoint;
}

/**
 * Changes the craft's target.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnTargetClick(Action* )
{
	_game->popState();
	_game->pushState(new SelectDestinationState(_game, _craft, _globe));

	delete _waypoint;
}

/**
 * Sets the craft to patrol the current location.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnPatrolClick(Action* )
{
	_game->popState();
	_craft->setDestination(0);

	delete _waypoint;
}

/**
 * Closes the window.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnCancelClick(Action* ) // kL_note: "scout"/go to last known UFO position
{
	if (_waypoint != 0) // Go to the last known UFO position
	{
		_waypoint->setId(_game->getSavedGame()->getId("STR_WAYPOINT"));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
		_craft->setDestination(_waypoint);
	}

	_game->popState(); // Cancel
}

}
