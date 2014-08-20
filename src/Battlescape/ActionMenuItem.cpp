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

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleInterface.h"


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
			272,
			40,
			x + 24,
			y - (id * 40)),
		_highlighted(false),
		_action(BA_NONE),
		_tu(0)
{
	Font
		* big = game->getResourcePack()->getFont("FONT_BIG"),
		* small = game->getResourcePack()->getFont("FONT_SMALL");
	Language* lang = game->getLanguage();

	Element* actionMenu = game->getRuleset()->getInterface("battlescape")->getElement("actionMenu");

	_highlightModifier = actionMenu->TFTDMode? 12: 4;

	_frame		= new Frame(
						getWidth(),
						getHeight(),
						0,
						0);
	_txtDesc	= new Text(160, 20, 10, 13);
	_txtAcc		= new Text(63, 20, 151, 13);
	_txtTU		= new Text(50, 20, 214, 13);

	_frame->setColor(actionMenu->border); //Palette::blockOffset(0)+7);
	_frame->setBackground(actionMenu->color2); //Palette::blockOffset(0)+14);
	_frame->setHighContrast(true);
	_frame->setThickness(8);

	_txtDesc->initText(big, small, lang);
	_txtDesc->setColor(actionMenu->color); //Palette::blockOffset(0)-1);
	_txtDesc->setHighContrast(true);
	_txtDesc->setBig();
//	_txtDesc->setVisible(true);

	_txtAcc->initText(big, small, lang);
	_txtAcc->setColor(actionMenu->color); //Palette::blockOffset(0)-1);
	_txtAcc->setHighContrast(true);
	_txtAcc->setBig();

	_txtTU->initText(big, small, lang);
	_txtTU->setColor(actionMenu->color); //Palette::blockOffset(0)-1);
	_txtTU->setHighContrast(true);
	_txtTU->setBig();
}

/**
 * Deletes the ActionMenuItem.
 */
ActionMenuItem::~ActionMenuItem()
{
	delete _frame;
	delete _txtDesc;
	delete _txtTU;
	delete _txtAcc;
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
	_txtTU->setPalette(colors, firstcolor, ncolors);
	_txtAcc->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws the bordered box.
 */
void ActionMenuItem::draw()
{
	_frame->blit(this);
	_txtDesc->blit(this);
	_txtTU->blit(this);
	_txtAcc->blit(this);
}

/**
 * Processes a mouse hover in event.
 * @param action Pointer to an action.
 * @param state Pointer to a state.
 */
void ActionMenuItem::mouseIn(Action* action, State* state)
{
	_highlighted = true;
	_frame->setBackground(_frame->getBackground() - _highlightModifier); //Palette::blockOffset(0)+10);

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
	_frame->setBackground(_frame->getBackground() + _highlightModifier); //Palette::blockOffset(0)+14);

	draw();
	InteractiveSurface::mouseOut(action, state);
}

}
