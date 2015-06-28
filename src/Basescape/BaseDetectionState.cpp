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

#include "BaseDetectionState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the BaseDetection window.
 * @param base - pointer to the Base to get info from
 */
BaseDetectionState::BaseDetectionState(Base* base)
	:
		_base(base)
{
	_screen = false;

	_window			= new Window(this, 200, 100, 60, 50, POPUP_BOTH);
	_txtTitle		= new Text(200, 17, 60, 60);

	_txtFacilities		= new Text(60, 9, 76, 83);
	_txtFacilitiesVal	= new Text(15, 9, 136, 83);
	_txtShields			= new Text(60, 9, 76, 93);
	_txtShieldsVal		= new Text(15, 9, 136, 93);
//	_txtDifficulty		= new Text(60, 9, 76, 103);
//	_txtDifficultyVal	= new Text(15, 9, 136, 103);
	_txtSpotted			= new Text(200, 9, 60, 109);

	_txtExposure	= new Text(102, 9, 152, 79);
	_txtExposureVal	= new Text(102, 16, 152, 90);
//	_txtTimePeriod	= new Text(102, 9, 152, 107);

	_btnOk			= new TextButton(168, 16, 76, 125);

	setPalette("PAL_BASESCAPE", 6);

	add(_window);
	add(_txtTitle);
	add(_txtFacilities);
	add(_txtShields);
//	add(_txtDifficulty);
	add(_txtFacilitiesVal);
	add(_txtShieldsVal);
//	add(_txtDifficultyVal);
	add(_txtSpotted);
	add(_txtExposure);
	add(_txtExposureVal);
//	add(_txtTimePeriod);
	add(_btnOk);

	centerAllSurfaces();

	Uint8 color1 = Palette::blockOffset(15)+6; // purple

	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));
	_window->setColor(color1);

	_txtTitle->setText(tr("STR_BASEDETECTION"));
	_txtTitle->setColor(color1);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setColor(color1);
	_btnOk->onMouseClick((ActionHandler)& BaseDetectionState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDetectionState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDetectionState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDetectionState::btnOkClick,
					SDLK_KP_ENTER);

	_txtFacilities->setText(tr("STR_FACILITIES"));
	_txtFacilities->setColor(color1);

	_txtShields->setText(tr("STR_MINDSHIELDS"));
	_txtShields->setColor(color1);

//	_txtDifficulty->setColor(color1);
//	_txtDifficulty->setText(tr("STR_DIFFICULTY"));

	if (_base->getBaseExposed() == true)
	{
		_blinkTimer = new Timer(325);
		_blinkTimer->onTimer((StateHandler)& BaseDetectionState::blink);
		_blinkTimer->start();

		_txtSpotted->setText(tr("STR_SPOTTED"));
		_txtSpotted->setColor(Palette::blockOffset(2)); // red
		_txtSpotted->setHighContrast();
		_txtSpotted->setAlign(ALIGN_CENTER);
	}
	_txtSpotted->setVisible(false); // wait for blink.

	_txtExposure->setText(tr("STR_EXPOSURE"));
	_txtExposure->setColor(color1);
	_txtExposure->setAlign(ALIGN_CENTER);

//	_txtTimePeriod->setColor(color1);
//	_txtTimePeriod->setAlign(ALIGN_CENTER);
//	_txtTimePeriod->setText(tr("STR_PER10MIN"));

	// TODO: Add gravShield info. And baseDefense power.
	std::wostringstream
		woststr1,
		woststr2,
//		woststr3,
		woststr4;

	int
		facQty,
		shields,
		det = _base->getDetectionChance(
									static_cast<int>(_game->getSavedGame()->getDifficulty()),
									&facQty,
									&shields);

	_txtFacilitiesVal->setColor(color1);
	woststr1 << facQty;
	_txtFacilitiesVal->setText(woststr1.str());

	if (_game->getSavedGame()->isResearched("STR_MIND_SHIELD") == true)
	{
		_txtShieldsVal->setColor(color1);
		if (shields == 0)
			woststr2 << L"-";
		else
			woststr2 << shields;
		_txtShieldsVal->setText(woststr2.str());
	}
	else
	{
		_txtShields->setVisible(false);
		_txtShieldsVal->setVisible(false);
	}

//	_txtDifficultyVal->setColor(color1);
//	woststr3 << diff;
//	_txtDifficultyVal->setText(woststr3.str());

	_txtExposureVal->setColor(Palette::blockOffset(9));
	_txtExposureVal->setHighContrast();
	_txtExposureVal->setBig();
	_txtExposureVal->setAlign(ALIGN_CENTER);
	woststr4 << det;
	_txtExposureVal->setText(woststr4.str());
}

/**
 * dTor.
 */
BaseDetectionState::~BaseDetectionState()
{
	if (_base->getBaseExposed() == true)
		delete _blinkTimer;
}

/**
 * Runs the blink timer.
 */
void BaseDetectionState::think()
{
	if (_window->isPopupDone() == false)
		_window->think();
	else if (_base->getBaseExposed() == true)
		_blinkTimer->think(this, NULL);
}

/**
 * Blinks the message text.
 */
void BaseDetectionState::blink()
{
	_txtSpotted->setVisible(!_txtSpotted->getVisible());
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BaseDetectionState::btnOkClick(Action*)
{
	_game->popState();
}

}
