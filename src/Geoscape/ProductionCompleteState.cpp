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

#include "ProductionCompleteState.h"

#include <assert.h>

#include "GeoscapeState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Basescape/BasescapeState.h"
#include "../Basescape/ManufactureState.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in a Production Complete window.
 * @param game Pointer to the core game.
 * @param base Pointer to base the production belongs to.
 * @param item Item that finished producing.
 * @param state Pointer to the Geoscape state.
 * @param endType What ended the production.
 */
ProductionCompleteState::ProductionCompleteState(Game* game, Base* base, const std::wstring& item, GeoscapeState* state, ProdProgress endType)
	:
		State(game),
		_base(base),
		_state(state),
		_endType(endType)
{
	_screen = false;

	_window			= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage		= new Text(246, 110, 37, 35);

	_btnOk			= new TextButton(72, 16, 48, 154);
	_btnOk5Secs		= new TextButton(72, 16, 124, 154);
	_btnGotoBase	= new TextButton(72, 16, 200, 154);


	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)), Palette::backPos, 16);

	add(_window);
	add(_txtMessage);
	add(_btnOk);
	add(_btnOk5Secs);
	add(_btnGotoBase);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ProductionCompleteState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)& ProductionCompleteState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_btnOk5Secs->setColor(Palette::blockOffset(8)+5);
	_btnOk5Secs->setText(tr("STR_OK_5_SECS"));
	_btnOk5Secs->onMouseClick((ActionHandler)& ProductionCompleteState::btnOk5SecsClick);
	_btnOk5Secs->onKeyboardPress((ActionHandler)& ProductionCompleteState::btnOk5SecsClick, (SDLKey)Options::getInt("keyGeoSpeed1"));

	_btnGotoBase->setColor(Palette::blockOffset(8)+5);
	if (_endType != PROGRESS_CONSTRUCTION)
	{
		_btnGotoBase->setText(tr("STR_ALLOCATE_MANUFACTURE"));
	}
	else
	{
		_btnGotoBase->setText(tr("STR_GO_TO_BASE"));
	}
	_btnGotoBase->onMouseClick((ActionHandler)& ProductionCompleteState::btnGotoBaseClick);
	_btnGotoBase->onKeyboardPress((ActionHandler)& ProductionCompleteState::btnGotoBaseClick, (SDLKey)Options::getInt("keyOk"));

	_txtMessage->setColor(Palette::blockOffset(15)-1);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap(true);

	std::wstring s;
	switch (_endType)
	{
		case PROGRESS_CONSTRUCTION:
			s = tr("STR_CONSTRUCTION_OF_FACILITY_AT_BASE_IS_COMPLETE").arg(item).arg(base->getName());
		break;
		case PROGRESS_COMPLETE:
			s = tr("STR_PRODUCTION_OF_ITEM_AT_BASE_IS_COMPLETE").arg(item).arg(base->getName());
		break;
		case PROGRESS_NOT_ENOUGH_MONEY:
			s = tr("STR_NOT_ENOUGH_MONEY_TO_PRODUCE_ITEM_AT_BASE").arg(item).arg(base->getName());
		break;
		case PROGRESS_NOT_ENOUGH_MATERIALS:
			s = tr("STR_NOT_ENOUGH_SPECIAL_MATERIALS_TO_PRODUCE_ITEM_AT_BASE").arg(item).arg(base->getName());
		break;

		default:
			assert(false);
	}

	_txtMessage->setText(s);
}

/**
 *
 */
ProductionCompleteState::~ProductionCompleteState()
{
}

/**
 * Resets the palette.
 */
void ProductionCompleteState::init()
{
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)), Palette::backPos, 16);
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Reduces the speed to 5 Secs and returns to the previous screen.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::btnOk5SecsClick(Action*)
{
	_state->timerReset();

	_game->popState();
}

/**
 * Goes to the base for the respective production.
 * @param action Pointer to an action.
 */
void ProductionCompleteState::btnGotoBaseClick(Action*)
{
	_state->timerReset();

	_game->popState();

	if (_endType != PROGRESS_CONSTRUCTION)
	{
		_game->pushState(new ManufactureState(_game, _base));
	}
	else
	{
		_game->pushState(new BasescapeState(_game, _base, _state->getGlobe()));
	}
}

}
