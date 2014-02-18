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

#include "ActionMenuItem.h"

#include "../Engine/Game.h"
#include "../Engine/Palette.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Sets up an Action menu item.
 * @param id, The unique identifier of the menu item.
 * @param game, Pointer to the game.
 * @param x, Position on the x-axis.
 * @param y, Position on the y-asis.
 */
ActionMenuItem::ActionMenuItem(
		int id,
		Game* game,
		int x,
		int y)
	:
		InteractiveSurface(
			270,
			40,
			x + 25,
			y - (id * 40)),
		_highlighted(false),
		_action(BA_NONE),
		_tu(0)
{
	Font
		* big = game->getResourcePack()->getFont("FONT_BIG"),
		* small = game->getResourcePack()->getFont("FONT_SMALL");
	Language* lang = game->getLanguage();

	_frame		= new Frame(
						getWidth(),
						getHeight(),
						0,
						0);
	_txtDesc	= new Text(160, 20, 10, 13);
	_txtAcc		= new Text(63, 20, 151, 13);
	_txtTU		= new Text(50, 20, 214, 13);

	_frame->setHighContrast(true);
	_frame->setColor(Palette::blockOffset(0)+7);
	_frame->setBackground(Palette::blockOffset(0)+14);
	_frame->setThickness(9);

	_txtDesc->initText(big, small, lang);
	_txtDesc->setBig();
	_txtDesc->setHighContrast(true);
	_txtDesc->setColor(Palette::blockOffset(0)-1);
	_txtDesc->setVisible(true);

	_txtAcc->initText(big, small, lang);
	_txtAcc->setBig();
	_txtAcc->setHighContrast(true);
	_txtAcc->setColor(Palette::blockOffset(0)-1);

	_txtTU->initText(big, small, lang);
	_txtTU->setBig();
	_txtTU->setHighContrast(true);
	_txtTU->setColor(Palette::blockOffset(0)-1);
}

/**
 * Deletes the ActionMenuItem.
 */
ActionMenuItem::~ActionMenuItem()
{
	delete _frame;
	delete _txtDesc;
	delete _txtAcc;
	delete _txtTU;
}

/**
 * Links with an action and fills in the text fields.
 * @param action, The battlescape action.
 * @param description, The actions description.
 * @param accuracy, The actions accuracy, including the Acc> prefix.
 * @param timeunits, The timeunits string, including the TUs> prefix.
 * @param tu, The timeunits value.
 */
void ActionMenuItem::setAction(
		BattleActionType action,
		std::wstring description,
		std::wstring accuracy,
		std::wstring timeunits,
		int tu)
{
	_action = action;
	_txtDesc->setText(description);
	_txtAcc->setText(accuracy);
	_txtTU->setText(timeunits);
	_tu = tu;

	_redraw = true;
}

/**
 * Gets the action that was linked to this menu item.
 * @return Action that was linked to this menu item.
 */
BattleActionType ActionMenuItem::getAction() const
{
	return _action;
}

/**
 * Gets the action tus that were linked to this menu item.
 * @return The timeunits that were linked to this menu item.
 */
int ActionMenuItem::getTUs() const
{
	return _tu;
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void ActionMenuItem::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_frame->setPalette(colors, firstcolor, ncolors);
	_txtDesc->setPalette(colors, firstcolor, ncolors);
	_txtAcc->setPalette(colors, firstcolor, ncolors);
	_txtTU->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws the bordered box.
 */
void ActionMenuItem::draw()
{
	_frame->blit(this);
	_txtDesc->blit(this);
	_txtAcc->blit(this);
	_txtTU->blit(this);
}

/**
 * Processes a mouse hover in event.
 * @param action Pointer to an action.
 * @param state Pointer to a state.
 */
void ActionMenuItem::mouseIn(Action* action, State* state)
{
	_highlighted = true;
	_frame->setBackground(Palette::blockOffset(0)+10);

	draw();
	InteractiveSurface::mouseIn(action, state);
}


/**
 * Processes a mouse hover out event.
 * @param action Pointer to an action.
 * @param state Pointer to a state.
 */
void ActionMenuItem::mouseOut(Action* action, State* state)
{
	_highlighted = false;
	_frame->setBackground(Palette::blockOffset(0)+14);

	draw();
	InteractiveSurface::mouseOut(action, state);
}

}
