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

#include "TransferConfirmState.h"

//#include <sstream>

#include "TransferItemsState.h"

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
 * Initializes all the elements in the Confirm Transfer window.
 * @param base	- pointer to the destination base
 * @param state	- pointer to the Transfer state
 */
TransferConfirmState::TransferConfirmState(
		Base* base,
		TransferItemsState* state)
	:
		_base(base),
		_state(state)
{
	_screen = false;

	_window		= new Window(this, 320, 80, 0, 60);
	_txtTitle	= new Text(310, 17, 5, 72);

	_txtCost	= new Text(60, 17, 110, 93);
	_txtTotal	= new Text(100, 17, 170, 93);

	_btnCancel	= new TextButton(134, 16, 16, 115);
	_btnOk		= new TextButton(134, 16, 170, 115);

	setInterface("transferConfirm");

	add(_window,	"window",	"transferConfirm");
	add(_txtTitle,	"text",		"transferConfirm");
	add(_txtCost,	"text",		"transferConfirm");
	add(_txtTotal,	"text",		"transferConfirm");
	add(_btnCancel,	"button",	"transferConfirm");
	add(_btnOk,		"button",	"transferConfirm");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& TransferConfirmState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferConfirmState::btnCancelClick,
					Options::keyCancel);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& TransferConfirmState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransferConfirmState::btnOkClick,
					Options::keyOk);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER_ITEMS_TO").arg(_base->getName()));

	_txtCost->setBig();
	_txtCost->setText(tr("STR_COST"));

	_txtTotal->setBig();
	std::wostringstream ss;
	ss << L'\x01' << Text::formatFunding(_state->getTotalCost());
	_txtTotal->setText(ss.str().c_str());
}

/**
 * dTor.
 */
TransferConfirmState::~TransferConfirmState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void TransferConfirmState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Completes the transfer.
 * @param action - pointer to an Action
 */
void TransferConfirmState::btnOkClick(Action*)
{
	_state->completeTransfer();

	_game->popState(); // pop Confirmation (this)
//	_game->popState(); // pop main Transfer
//	_game->popState(); // pop choose Destination
}

}
