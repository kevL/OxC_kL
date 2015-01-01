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

#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
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
 * @param base - pointer to the base to get info from
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
	_txtDifficulty		= new Text(60, 9, 76, 103);
	_txtDifficultyVal	= new Text(15, 9, 136, 103);

	_txtExposure	= new Text(102, 9, 152, 79);
	_txtExposureVal	= new Text(102, 16, 152, 90);
	_txtTimePeriod	= new Text(102, 9, 152, 107);

	_btnOk			= new TextButton(168, 16, 76, 125);

	setPalette("PAL_BASESCAPE", 6);

	add(_window);
	add(_txtTitle);
	add(_txtFacilities);
	add(_txtShields);
	add(_txtDifficulty);
	add(_txtFacilitiesVal);
	add(_txtShieldsVal);
	add(_txtDifficultyVal);
	add(_txtExposure);
	add(_txtExposureVal);
	add(_txtTimePeriod);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)+6);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_txtTitle->setColor(Palette::blockOffset(15)+6);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_BASEDETECTION"));

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseDetectionState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDetectionState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDetectionState::btnOkClick,
					Options::keyCancel);

	_txtFacilities->setColor(Palette::blockOffset(15)+6);
	_txtFacilities->setText(tr("STR_FACILITIES"));

	_txtShields->setColor(Palette::blockOffset(15)+6);
	_txtShields->setText(tr("STR_MINDSHIELDS"));

	_txtDifficulty->setColor(Palette::blockOffset(15)+6);
	_txtDifficulty->setText(tr("STR_DIFFICULTY"));

	_txtExposure->setColor(Palette::blockOffset(15)+6);
	_txtExposure->setAlign(ALIGN_CENTER);
	_txtExposure->setText(tr("STR_EXPOSURE"));

	_txtTimePeriod->setColor(Palette::blockOffset(15)+6);
	_txtTimePeriod->setAlign(ALIGN_CENTER);
	_txtTimePeriod->setText(tr("STR_PER10MIN"));

	int
		facQty = 0,
		shields = 0,
		diff = 0;

	std::vector<BaseFacility*>* facs = _base->getFacilities();
	for (std::vector<BaseFacility*>::const_iterator
			i = facs->begin();
			i != facs->end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			facQty++;

			if ((*i)->getRules()->isMindShield())
				shields++;
		}
	}

	facQty	= facQty / 6 + 9;
	shields	= shields * 2 + 1;
	diff	= static_cast<int>(_game->getSavedGame()->getDifficulty());

	int detchance = facQty / shields + diff;

	std::wostringstream
		val1,
		val2,
		val3,
		val4;

	_txtFacilitiesVal->setColor(Palette::blockOffset(15)+6);
	val1 << facQty;
	_txtFacilitiesVal->setText(val1.str());

	_txtShieldsVal->setColor(Palette::blockOffset(15)+6);
	val2 << shields;
	_txtShieldsVal->setText(val2.str());

	_txtDifficultyVal->setColor(Palette::blockOffset(15)+6);
	val3 << diff;
	_txtDifficultyVal->setText(val3.str());

	_txtExposureVal->setColor(Palette::blockOffset(9));
	_txtExposureVal->setHighContrast();
	_txtExposureVal->setBig();
	_txtExposureVal->setAlign(ALIGN_CENTER);
	val4 << detchance;
	_txtExposureVal->setText(val4.str());
}

/**
 * dTor.
 */
BaseDetectionState::~BaseDetectionState()
{
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
