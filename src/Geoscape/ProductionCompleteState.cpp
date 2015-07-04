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

#include "ProductionCompleteState.h"

//#include <assert.h>

#include "GeoscapeState.h"

#include "../Basescape/BasescapeState.h"
#include "../Basescape/ManufactureState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a Production Complete window.
 * @param base			- pointer to Base the production belongs to
 * @param item			- reference the item that finished producing
 * @param state			- pointer to GeoscapeState
 * @param gotoBaseBtn	-
 * @param endType		- what ended the production (default PROGRESS_COMPLETE) (see Production.h)
 */
ProductionCompleteState::ProductionCompleteState(
		Base* base,
		const std::wstring& item,
		GeoscapeState* state,
		bool gotoBaseBtn,
		ProductProgress endType)
	:
		_base(base),
		_state(state),
		_endType(endType)
{
	_screen = false;

	_window			= new Window(this, 256, 160, 32, 20, POPUP_BOTH);

	_txtMessage		= new Text(246, 110, 37, 35);

	_btnGotoBase	= new TextButton(72, 16,  48, 154);
	_btnOk5Secs		= new TextButton(72, 16, 124, 154);
	_btnOk			= new TextButton(72, 16, 200, 154);

	setInterface("geoManufacture");

	add(_window,		"window",	"geoManufacture");
	add(_txtMessage,	"text1",	"geoManufacture");
	add(_btnGotoBase,	"button",	"geoManufacture");
	add(_btnOk5Secs,	"button",	"geoManufacture");
	add(_btnOk,			"button",	"geoManufacture");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_btnOk->setText(tr(gotoBaseBtn ? "STR_OK" : "STR_MORE"));
	_btnOk->onMouseClick((ActionHandler)& ProductionCompleteState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ProductionCompleteState::btnOkClick,
					Options::keyCancel);

	_btnOk5Secs->setText(tr("STR_OK_5_SECONDS"));
	_btnOk5Secs->onMouseClick((ActionHandler)& ProductionCompleteState::btnOk5SecsClick);
	_btnOk5Secs->onKeyboardPress(
					(ActionHandler)& ProductionCompleteState::btnOk5SecsClick,
					Options::keyGeoSpeed1);

	_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	_btnGotoBase->setVisible(gotoBaseBtn);
	_btnGotoBase->onMouseClick((ActionHandler)& ProductionCompleteState::btnGotoBaseClick);
	_btnGotoBase->onKeyboardPress(
					(ActionHandler)& ProductionCompleteState::btnGotoBaseClick,
					Options::keyOk);

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();

	std::wstring wst;
	switch (_endType)
	{
		case PROGRESS_CONSTRUCTION:
			wst = tr("STR_CONSTRUCTION_OF_FACILITY_AT_BASE_IS_COMPLETE")
					.arg(item).arg(base->getName());
		break;
		case PROGRESS_COMPLETE:
			wst = tr("STR_PRODUCTION_OF_ITEM_AT_BASE_IS_COMPLETE")
					.arg(item).arg(base->getName());
		break;
		case PROGRESS_NOT_ENOUGH_MONEY:
			wst = tr("STR_NOT_ENOUGH_MONEY_TO_PRODUCE_ITEM_AT_BASE")
					.arg(item).arg(base->getName());
		break;
		case PROGRESS_NOT_ENOUGH_MATERIALS:
			wst = tr("STR_NOT_ENOUGH_SPECIAL_MATERIALS_TO_PRODUCE_ITEM_AT_BASE")
					.arg(item).arg(base->getName());
		break;

		default:
			assert(false);
	}

	_txtMessage->setText(wst);
}

/**
 * dTor.
 */
ProductionCompleteState::~ProductionCompleteState()
{}

/**
 * Initializes the state.
 */
void ProductionCompleteState::init()
{
	State::init();
	_btnOk5Secs->setVisible(_state->is5Sec() == false);
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void ProductionCompleteState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Reduces the speed to 5 Secs and returns to the previous screen.
 * @param action - pointer to an Action
 */
void ProductionCompleteState::btnOk5SecsClick(Action*)
{
	_state->resetTimer();
	_game->popState();
}

/**
 * Goes to the base for the respective production.
 * @param action - pointer to an Action
 */
void ProductionCompleteState::btnGotoBaseClick(Action*)
{
	_state->resetTimer();
	_game->popState();

	_game->pushState(new BasescapeState(
									_base,
									_state->getGlobe()));
}

}
