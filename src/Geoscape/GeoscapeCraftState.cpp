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

#include "GeoscapeCraftState.h"

#include <sstream>

#include "GeoscapeState.h"
#include "Globe.h"
#include "SelectDestinationState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
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
 * Initializes all the elements in the Geoscape Craft window.
 * @param game, Pointer to the core game.
 * @param craft, Pointer to the craft to display.
 * @param globe, Pointer to the Geoscape globe.
 * @param waypoint, Pointer to the last UFO position (if redirecting the craft).
 */
GeoscapeCraftState::GeoscapeCraftState(
		Craft* craft,
		Globe* globe,
		Waypoint* waypoint,
		GeoscapeState* geo,
		bool doublePop) // kL_add.
	:
		_craft(craft),
		_globe(globe),
		_waypoint(waypoint),
		_geo(geo),
		_doublePop(doublePop) // kL_add.
{
	_screen = false;

	_window			= new Window(this, 224, 174, 16, 8, POPUP_BOTH);
	_txtTitle		= new Text(192, 17, 32, 16);

//	_sprite			= new Surface(32, 38, 220, -11); // kL
	_sprite			= new Surface(32, 38, 224, -1); // kL

	_txtStatus		= new Text(192, 17, 32, 31);

	_txtBase		= new Text(192, 9, 32, 43);

	_txtRedirect	= new Text(120, 17, 120, 46);

	_txtSpeed		= new Text(192, 9, 32, 55);

	_txtMaxSpeed	= new Text(120, 9, 32, 64);
	_txtSoldier		= new Text(80, 9, 160, 64);

	_txtAltitude	= new Text(120, 9, 32, 73);
	_txtHWP			= new Text(80, 9, 160, 73);

	_txtFuel		= new Text(120, 9, 32, 82);
	_txtDamage		= new Text(80, 9, 160, 82);

	_txtW1Name		= new Text(120, 9, 32, 91);
	_txtW1Ammo		= new Text(80, 9, 160, 91);

	_txtW2Name		= new Text(120, 9, 32, 100);
	_txtW2Ammo		= new Text(80, 9, 160, 100);

	_btnTarget		= new TextButton(90, 16, 32, 113);
	_btnPatrol		= new TextButton(90, 16, 134, 113);
	_btnCenter		= new TextButton(192, 16, 32, 135);
	_btnBase		= new TextButton(90, 16, 32, 157);
	_btnCancel		= new TextButton(90, 16, 134, 157);

	setPalette("PAL_GEOSCAPE", 4);

	add(_window);
	add(_txtTitle);
	add(_sprite); // kL
	add(_txtStatus);
	add(_txtBase);
	add(_txtRedirect);
	add(_txtSpeed);
	add(_txtMaxSpeed);
	add(_txtSoldier);
	add(_txtAltitude);
	add(_txtHWP);
	add(_txtFuel);
	add(_txtDamage);
	add(_txtW1Name);
	add(_txtW1Ammo);
	add(_txtW2Name);
	add(_txtW2Ammo);

	add(_btnTarget);
	add(_btnBase);
	add(_btnCenter);
	add(_btnPatrol);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnCenter->setColor(Palette::blockOffset(8)+5);
	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCenterClick);

	_btnBase->setColor(Palette::blockOffset(8)+5);
	_btnBase->setText(tr("STR_RETURN_TO_BASE"));
	_btnBase->onMouseClick((ActionHandler)& GeoscapeCraftState::btnBaseClick);

	_btnTarget->setColor(Palette::blockOffset(8)+5);
	_btnTarget->setText(tr("STR_SELECT_NEW_TARGET"));
	_btnTarget->onMouseClick((ActionHandler)& GeoscapeCraftState::btnTargetClick);

	_btnPatrol->setColor(Palette::blockOffset(8)+5);
	_btnPatrol->setText(tr("STR_PATROL"));
	_btnPatrol->onMouseClick((ActionHandler)& GeoscapeCraftState::btnPatrolClick);

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setText(_craft->getName(_game->getLanguage()));

	_txtStatus->setColor(Palette::blockOffset(15)-1);
	_txtStatus->setSecondaryColor(Palette::blockOffset(8)+10);
//	_txtStatus->setWordWrap(true);

	std::string stat = _craft->getStatus();
	std::wstring status;
	bool
		lowFuel = _craft->getLowFuel(),
		missionComplete = _craft->getMissionComplete();
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
		Ufo* ufo = dynamic_cast<Ufo*>(_craft->getDestination());
		if (ufo != NULL)
		{
			if (_craft->isInDogfight())
			{
				speed = ufo->getSpeed();	// THIS DOES NOT CHANGE THE SPEED of the xCom CRAFT
											// for Fuel usage. ( ie. it should )
				status = tr("STR_TAILING_UFO");
			}
			else if (ufo->getStatus() == Ufo::FLYING)
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

	_txtBase->setColor(Palette::blockOffset(15)-1);
	_txtBase->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtBase->setText(tr("STR_BASE_UC").arg(_craft->getBase()->getName()));

	std::wostringstream
		ss1,
		ss2,
		ss3;

	_txtSpeed->setColor(Palette::blockOffset(15)-1);
	_txtSpeed->setSecondaryColor(Palette::blockOffset(8)+5);
	ss1 << tr("STR_SPEED_").arg(Text::formatNumber(speed, L"", false));
	_txtSpeed->setText(ss1.str());

	_txtMaxSpeed->setColor(Palette::blockOffset(15)-1);
	_txtMaxSpeed->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtMaxSpeed->setText(tr("STR_MAXIMUM_SPEED_UC")
							.arg(Text::formatNumber(_craft->getRules()->getMaxSpeed())));

	_txtSoldier->setColor(Palette::blockOffset(15)-1);
	_txtSoldier->setSecondaryColor(Palette::blockOffset(8)+5);
	ss2 << tr("STR_SOLDIERS") << " " << L'\x01' << _craft->getNumSoldiers();
	_txtSoldier->setText(ss2.str());

	_txtAltitude->setColor(Palette::blockOffset(15)-1);
	_txtAltitude->setSecondaryColor(Palette::blockOffset(8)+5);
	std::string alt = _craft->getAltitude();
	if (alt == "STR_GROUND"
		|| stat == "STR_READY"
		|| stat == "STR_REPAIRS"
		|| stat == "STR_REFUELLING"
		|| stat == "STR_REARMING")
	{
		alt = "STR_GROUNDED";
	}
	_txtAltitude->setText(tr("STR_ALTITUDE_").arg(tr(alt)));

	_txtHWP->setColor(Palette::blockOffset(15)-1);
	_txtHWP->setSecondaryColor(Palette::blockOffset(8)+5);
	ss3 << tr("STR_HWPS") << " " << L'\x01' << _craft->getNumVehicles();
	_txtHWP->setText(ss3.str());

	_txtFuel->setColor(Palette::blockOffset(15)-1);
	_txtFuel->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtFuel->setText(tr("STR_FUEL").arg(Text::formatPercentage(_craft->getFuelPercentage())));

	_txtDamage->setColor(Palette::blockOffset(15)-1);
	_txtDamage->setSecondaryColor(Palette::blockOffset(8)+5);
	_txtDamage->setText(tr("STR_HULL_").arg(Text::formatPercentage(100 - _craft->getDamagePercent())));


	_txtW1Name->setColor(Palette::blockOffset(15)-1);
	_txtW1Name->setSecondaryColor(Palette::blockOffset(8)+5);

	_txtW1Ammo->setColor(Palette::blockOffset(15)-1);
	_txtW1Ammo->setSecondaryColor(Palette::blockOffset(8)+5);

	if (_craft->getRules()->getWeapons() > 0
		&& _craft->getWeapons()->at(0) != 0)
	{
		CraftWeapon* w1 = _craft->getWeapons()->at(0);
		_txtW1Name->setText(tr("STR_WEAPON_ONE").arg(tr(w1->getRules()->getType())));
		_txtW1Ammo->setText(tr("STR_ROUNDS_").arg(w1->getAmmo()));
	}
	else
	{
//kL	_txtW1Name->setText(tr("STR_WEAPON_ONE").arg(tr("STR_NONE_UC")));
		_txtW1Name->setVisible(false); // kL
		_txtW1Ammo->setVisible(false);
	}

	_txtW2Name->setColor(Palette::blockOffset(15)-1);
	_txtW2Name->setSecondaryColor(Palette::blockOffset(8)+5);

	_txtW2Ammo->setColor(Palette::blockOffset(15)-1);
	_txtW2Ammo->setSecondaryColor(Palette::blockOffset(8)+5);

	if (_craft->getRules()->getWeapons() > 1
		&& _craft->getWeapons()->at(1) != 0)
	{
		CraftWeapon* w2 = _craft->getWeapons()->at(1);
		_txtW2Name->setText(tr("STR_WEAPON_TWO").arg(tr(w2->getRules()->getType())));
		_txtW2Ammo->setText(tr("STR_ROUNDS_").arg(w2->getAmmo()));
	}
	else
	{
//kL	_txtW2Name->setText(tr("STR_WEAPON_TWO").arg(tr("STR_NONE_UC")));
		_txtW2Name->setVisible(false); // kL
		_txtW2Ammo->setVisible(false);
	}


	_txtRedirect->setColor(Palette::blockOffset(15)-1);
	_txtRedirect->setBig();
	_txtRedirect->setAlign(ALIGN_CENTER);
	_txtRedirect->setText(tr("STR_REDIRECT_CRAFT"));

	if (_waypoint == NULL)
		_txtRedirect->setVisible(false);
	else
		_btnCancel->setText(tr("STR_GO_TO_LAST_KNOWN_UFO_POSITION"));

	// kL_begin: set Base button visibility FALSE for already-Based crafts.
	// note these could be set up there where status was set.....
	if (stat != "STR_OUT"
		|| lowFuel
		|| missionComplete)
	{
		_btnBase->setVisible(false);
		_btnPatrol->setVisible(false);

		if (stat == "STR_REPAIRS"
			|| stat == "STR_REFUELLING"
			|| stat == "STR_REARMING"
			|| lowFuel
			|| missionComplete)
		{
			_btnTarget->setVisible(false);
		}
	}
	else
	{
		if (_craft->getDestination() == dynamic_cast<Target*>(_craft->getBase()))
			_btnBase->setVisible(false);

		if (_craft->getDestination() == NULL)
			_btnPatrol->setVisible(false);
	}

	if (_craft->getRules()->getSoldiers() == 0)
		_txtSoldier->setVisible(false);
	if (_craft->getRules()->getVehicles() == 0)
		_txtHWP->setVisible(false);
	// kL_end.

	// kL_begin ( was in init() ):
//	SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
//	texture->getFrame(_craft->getRules()->getSprite() + 33)->setX(0);
//	texture->getFrame(_craft->getRules()->getSprite() + 33)->setY(0);
//	texture->getFrame(_craft->getRules()->getSprite() + 33)->blit(_sprite);
	int craftSprite = _craft->getRules()->getSprite();
	SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("INTICON.PCK");
//	texture->getFrame(craftSprite + 11)->setX(0);
//	texture->getFrame(craftSprite + 11)->setY(0);
	texture->getFrame(craftSprite + 11)->blit(_sprite);
	// kL_end.
}

/**
 *
 */
GeoscapeCraftState::~GeoscapeCraftState()
{
}

/**
 * Centers the craft on the globe.
 * @param action - pointer to an action
 */
void GeoscapeCraftState::btnCenterClick(Action*)
{
	if (_doublePop)			// kL
		_game->popState();	// kL

	_game->popState();
	_globe->center(
				_craft->getLongitude(),
				_craft->getLatitude());
}

/**
 * Returns the craft back to its base.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnBaseClick(Action*)
{
	if (_doublePop)			// kL
		_game->popState();	// kL

	_game->popState();
	_craft->returnToBase();

	delete _waypoint;
}

/**
 * Changes the craft's target.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnTargetClick(Action*)
{
	if (_doublePop)			// kL
		_game->popState();	// kL

	_game->popState();
	_game->pushState(new SelectDestinationState(
											_craft,
											_globe,
											_geo));

	delete _waypoint;
}

/**
 * Sets the craft to patrol the current location.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnPatrolClick(Action*)
{
	if (_doublePop)			// kL
		_game->popState();	// kL

	_game->popState();
	_craft->setDestination(NULL);

	delete _waypoint;
}

/**
 * Closes the window.
 * @param action, Pointer to an action.
 */
void GeoscapeCraftState::btnCancelClick(Action*) // kL_note: "scout"/go to last known UFO position
{
	if (_waypoint != NULL) // Go to the last known UFO position
	{
		_waypoint->setId(_game->getSavedGame()->getId("STR_WAYPOINT"));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
		_craft->setDestination(_waypoint);
	}

	_game->popState(); // and Cancel.
}

}
