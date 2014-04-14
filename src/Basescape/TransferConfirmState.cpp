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

#include "TransferConfirmState.h"

#include "TransferItemsState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm Transfer window.
 * @param game Pointer to the core game.
 * @param base Pointer to the destination base.
 * @param state Pointer to the Transfer state.
 */
TransferConfirmState::TransferConfirmState(
		Game* game,
		Base* base,
		TransferItemsState* state)
	:
		State(game),
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

	setPalette("PAL_BASESCAPE", 6);

	add(_window);
	add(_txtTitle);
	add(_txtCost);
	add(_txtTotal);
	add(_btnCancel);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnCancel->setColor(Palette::blockOffset(15)+6);
	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& TransferConfirmState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferConfirmState::btnCancelClick,
					Options::keyCancel);

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& TransferConfirmState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransferConfirmState::btnOkClick,
					Options::keyOk);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFER_ITEMS_TO").arg(_base->getName()));

	_txtCost->setColor(Palette::blockOffset(13)+10);
	_txtCost->setBig();
	_txtCost->setText(tr("STR_COST"));

	_txtTotal->setColor(Palette::blockOffset(15)+1);
	_txtTotal->setBig();
	_txtTotal->setText(Text::formatFunding(_state->getTotal()));
}

/**
 *
 */
TransferConfirmState::~TransferConfirmState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TransferConfirmState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Completes the transfer.
 * @param action Pointer to an action.
 */
void TransferConfirmState::btnOkClick(Action*)
{
	_state->completeTransfer();
	_state->reinit(); // kL

	_game->popState(); // pop Confirmation (this)
//kL	_game->popState(); // pop main Transfer
//kL	_game->popState(); // pop choose Destination
}

}
