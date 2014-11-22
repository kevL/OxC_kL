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

#include "SelectDestinationState.h"

//#include <cmath>

#include "ConfirmCydoniaState.h"
#include "CraftErrorState.h"
#include "GeoscapeState.h"
#include "Globe.h"
#include "MultipleTargetsState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Select Destination window.
 * @param craft	- pointer to the Craft
 * @param globe	- pointer to the Globe
 * @param geo	- pointer to the GeoscapeState
 */
SelectDestinationState::SelectDestinationState(
		Craft* craft,
		Globe* globe, // kL_note: Globe is redudant here, use geo->getGlobe()
		GeoscapeState* geo) // kL_add
	:
		_craft(craft),
		_globe(globe),
		_geo(geo) // kL_add
{
	_screen = false;

	const int dx = _game->getScreen()->getDX();
//kL	dy = _game->getScreen()->getDY();

/*	_btnRotateLeft	= new InteractiveSurface(12, 10, 259 + dx * 2, 176 + dy);
	_btnRotateRight	= new InteractiveSurface(12, 10, 283 + dx * 2, 176 + dy);
	_btnRotateUp	= new InteractiveSurface(13, 12, 271 + dx * 2, 162 + dy);
	_btnRotateDown	= new InteractiveSurface(13, 12, 271 + dx * 2, 187 + dy);
	_btnZoomIn		= new InteractiveSurface(23, 23, 295 + dx * 2, 156 + dy);
	_btnZoomOut		= new InteractiveSurface(13, 17, 300 + dx * 2, 182 + dy); */

	_window = new Window(this, 256, 30, 0, 0);
	_window->setX(dx);
	_window->setDY(0);

//	_txtTitle	= new Text(100, 9, 16 + dx, 10);

	_btnCancel	= new TextButton(60, 14, 120 + dx, 8);
	_btnCydonia	= new TextButton(60, 14, 180 + dx, 8);

	_txtError	= new Text(100, 9, 12 + dx, 11);

	setPalette("PAL_GEOSCAPE", 0);

/*	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut); */

	add(_window);
	add(_btnCancel);
	add(_btnCydonia);
//	add(_txtTitle);
	add(_txtError);

	_txtError->setColor(Palette::blockOffset(5)); // (8)+5);
	_txtError->setText(tr("STR_OUTSIDE_CRAFT_RANGE"));
	_txtError->setVisible(false);

	_globe->onMouseClick((ActionHandler)& SelectDestinationState::globeClick);

/*	_btnRotateLeft->onMousePress((ActionHandler)& SelectDestinationState::btnRotateLeftPress);
	_btnRotateLeft->onMouseRelease((ActionHandler)& SelectDestinationState::btnRotateLeftRelease);
	_btnRotateLeft->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateLeftPress, Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateLeftRelease, Options::keyGeoLeft);

	_btnRotateRight->onMousePress((ActionHandler)& SelectDestinationState::btnRotateRightPress);
	_btnRotateRight->onMouseRelease((ActionHandler)& SelectDestinationState::btnRotateRightRelease);
	_btnRotateRight->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateRightPress, Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateRightRelease, Options::keyGeoRight);

	_btnRotateUp->onMousePress((ActionHandler)& SelectDestinationState::btnRotateUpPress);
	_btnRotateUp->onMouseRelease((ActionHandler)& SelectDestinationState::btnRotateUpRelease);
	_btnRotateUp->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateUpPress, Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateUpRelease, Options::keyGeoUp);

	_btnRotateDown->onMousePress((ActionHandler)& SelectDestinationState::btnRotateDownPress);
	_btnRotateDown->onMouseRelease((ActionHandler)& SelectDestinationState::btnRotateDownRelease);
	_btnRotateDown->onKeyboardPress((ActionHandler)&SelectDestinationState::btnRotateDownPress, Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease((ActionHandler)&SelectDestinationState::btnRotateDownRelease, Options::keyGeoDown);

	_btnZoomIn->onMouseClick((ActionHandler)& SelectDestinationState::btnZoomInLeftClick, SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick((ActionHandler)& SelectDestinationState::btnZoomInRightClick, SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress((ActionHandler)&SelectDestinationState::btnZoomInLeftClick, Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick((ActionHandler)& SelectDestinationState::btnZoomOutLeftClick, SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick((ActionHandler)& SelectDestinationState::btnZoomOutRightClick, SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress((ActionHandler)&SelectDestinationState::btnZoomOutLeftClick, Options::keyGeoZoomOut); */

	// dirty hacks to get the rotate buttons to work in "classic" style
/*	_btnRotateLeft->setListButton();
	_btnRotateRight->setListButton();
	_btnRotateUp->setListButton();
	_btnRotateDown->setListButton(); */

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnCancel->setColor(Palette::blockOffset(8)+5);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& SelectDestinationState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& SelectDestinationState::btnCancelClick,
					Options::keyCancel);

//kL	_txtTitle->setColor(Palette::blockOffset(15)-1);
//kL	_txtTitle->setText(tr("STR_SELECT_DESTINATION"));
//kL	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
//kL	_txtTitle->setWordWrap();


	bool goCydonia = true; // if all Soldiers have Power or Flight suits .......

	if (_craft->getRules()->getSpacecraft() == true
		&& _craft->getNumSoldiers() > 0
		&& _game->getSavedGame()->isResearched("STR_CYDONIA_OR_BUST") == true)
	{
		for (std::vector<Soldier*>::const_iterator
			i = _craft->getBase()->getSoldiers()->begin();
			i != _craft->getBase()->getSoldiers()->end()
				&& goCydonia == true;
			++i)
		{
			if ((*i)->getCraft() == _craft
				&& (*i)->getArmor()->isSpacesuit() == false)
			{
				goCydonia = false;
			}
		}
	}
	else
		goCydonia = false;

	if (goCydonia == true)
	{
		_btnCydonia->setColor(Palette::blockOffset(8)+5);
		_btnCydonia->setText(tr("STR_CYDONIA"));
		_btnCydonia->onMouseClick((ActionHandler)& SelectDestinationState::btnCydoniaClick);
	}
	else
		_btnCydonia->setVisible(false);
}

/**
 * dTor.
 */
SelectDestinationState::~SelectDestinationState()
{
//	if (_error != NULL)
//		delete _error;
}

/**
 * Stop the globe movement.
 */
void SelectDestinationState::init()
{
	State::init();
//	_globe->rotateStop();
}

/**
 * Runs the globe rotation timer.
 */
/* void SelectDestinationState::think()
{
//	State::think();
//	_globe->think();
} */

/**
 * Handles the globe.
 * @param action - pointer to an Action
 */
void SelectDestinationState::handle(Action* action)
{
	State::handle(action);
	_globe->handle(action, this);
}

/**
 * Processes any left-clicks for picking a target, or right-clicks to scroll the globe.
 * @param action - pointer to an Action
 */
void SelectDestinationState::globeClick(Action* action)
{
	const int
		mouseX = static_cast<int>(floor(action->getAbsoluteXMouse())),
		mouseY = static_cast<int>(floor(action->getAbsoluteYMouse()));
	double
		lon,
		lat;

	_globe->cartToPolar(
					static_cast<Sint16>(mouseX),
					static_cast<Sint16>(mouseY),
					&lon,
					&lat);

	if (mouseY < 30) // Ignore window clicks
		return;

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT) // Clicking on a valid target
	{
		int range = 0;
//		_error = NULL;

		const RuleCraft* const craftRule = _craft->getRules();
		if (craftRule->getRefuelItem() == "")
			range = _craft->getFuel() * 100 / 6; // <- gasoline-powered (speed factored into consumption)
		else
			range = craftRule->getMaxSpeed() * _craft->getFuel() / 6; // six doses per hour on Geoscape.
		//Log(LOG_INFO) << ". range = " << range;

		Waypoint* const wp = new Waypoint();
		wp->setLongitude(lon);
		wp->setLatitude(lat);

		//Log(LOG_INFO) << ". dist = " << (int)(_craft->getDistance(wp) * 3440.0) << " + " << (int)(_craft->getBase()->getDistance(wp) * 3440.0);

		if (static_cast<double>(range) < (_craft->getDistance(wp) + _craft->getBase()->getDistance(wp)) * earthRadius)
		{
			//Log(LOG_INFO) << ". . outside Range";
			_txtError->setVisible();

			delete wp;

//			std::wstring msg = tr("STR_OUTSIDE_CRAFT_RANGE");
//			_geo->popup(new CraftErrorState(
//			_error = new CraftErrorState(
//										_geo,
//										msg);
		}
		else
		{
			//Log(LOG_INFO) << ". . inside Range";
			_txtError->setVisible(false);

			std::vector<Target*> targets = _globe->getTargets(
															mouseX,
															mouseY,
															true);
			if (targets.empty() == false)
				delete wp;
			else
				targets.push_back(wp);

			_game->pushState(new MultipleTargetsState(
													targets,
													_craft,
													NULL));
		}
	}
}

/**
 * Starts rotating the globe to the left.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateLeftPress(Action*)
{
	_globe->rotateLeft();
} */

/**
 * Stops rotating the globe to the left.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateLeftRelease(Action*)
{
	_globe->rotateStopLon();
} */

/**
 * Starts rotating the globe to the right.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateRightPress(Action*)
{
	_globe->rotateRight();
} */

/**
 * Stops rotating the globe to the right.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateRightRelease(Action*)
{
	_globe->rotateStopLon();
} */

/**
 * Starts rotating the globe upwards.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateUpPress(Action*)
{
	_globe->rotateUp();
} */

/**
 * Stops rotating the globe upwards.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateUpRelease(Action*)
{
	_globe->rotateStopLat();
} */

/**
 * Starts rotating the globe downwards.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateDownPress(Action*)
{
	_globe->rotateDown();
} */

/**
 * Stops rotating the globe downwards.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnRotateDownRelease(Action*)
{
	_globe->rotateStopLat();
} */

/**
 * Zooms into the globe.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnZoomInLeftClick(Action*)
{
	_globe->zoomIn();
} */

/**
 * Zooms the globe maximum.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnZoomInRightClick(Action*)
{
	_globe->zoomMax();
} */

/**
 * Zooms out of the globe.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnZoomOutLeftClick(Action*)
{
	_globe->zoomOut();
} */

/**
 * Zooms the globe minimum.
 * @param action - pointer to an Action
 */
/* void SelectDestinationState::btnZoomOutRightClick(Action*)
{
	_globe->zoomMin();
} */

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SelectDestinationState::btnCancelClick(Action*)
{
//	if (_error == NULL)
	_game->popState();
}

/**
 *
 */
void SelectDestinationState::btnCydoniaClick(Action*)
{
//	if (_error == NULL)
//	{
	if (_craft->getNumSoldiers() > 0)
//kL	|| _craft->getNumVehicles() > 0)
	{
		_game->pushState(new ConfirmCydoniaState(_craft));
	}
//	}
}

/**
 * Updates the scale.
 * @param dX - delta of X
 * @param dY - delta of Y
 */
void SelectDestinationState::resize(
		int& dX,
		int& dY)
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + dX / 2);

		if (*i != _window
			&& *i != _btnCancel
//kL		&& *i != _txtTitle
			&& *i != _btnCydonia)
		{
			(*i)->setY((*i)->getY() + dY / 2);
		}
	}
}

}
