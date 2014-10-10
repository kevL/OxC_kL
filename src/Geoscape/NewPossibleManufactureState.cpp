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

#include "NewPossibleManufactureState.h"

#include "../Basescape/ManufactureState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/Base.h" // kL


namespace OpenXcom
{
/**
 * Initializes all the elements in the EndManufacture screen.
 * @param base					- pointer to the base to get info from
 * @param possibilities			- reference the vector of pointers to RuleManufacture for projects
 * @param showManufactureButton	- true to show the goto manufacture button
 */
NewPossibleManufactureState::NewPossibleManufactureState(
		Base* base,
		const std::vector<RuleManufacture*>& possibilities,
		bool showManufactureButton) // myk002_add.
	:
		_base(base)
{
	_screen = false;

	_window				= new Window(this, 288, 180, 16, 10);
	_txtTitle			= new Text(288, 40, 16, 20);

	_lstPossibilities	= new TextList(288, 81, 16, 56);

	_btnOk				= new TextButton(160, 14, 80, 149);
	_btnManufacture		= new TextButton(160, 14, 80, 165);

	setPalette("PAL_GEOSCAPE", 6);

	add(_window);
	add(_btnOk);
	add(_btnManufacture);
	add(_txtTitle);
	add(_lstPossibilities);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
//myk002	_btnOk->setText(tr("STR_OK"));
	_btnOk->setText(tr(showManufactureButton? "STR_OK": "STR_MORE")); // myk002
	_btnOk->onMouseClick((ActionHandler)& NewPossibleManufactureState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewPossibleManufactureState::btnOkClick,
					Options::keyCancel);

	_btnManufacture->setColor(Palette::blockOffset(8)+5);
	_btnManufacture->setText(tr("STR_ALLOCATE_MANUFACTURE"));
	_btnManufacture->setVisible(showManufactureButton); // myk002
	_btnManufacture->onMouseClick((ActionHandler)& NewPossibleManufactureState::btnManufactureClick);
	_btnManufacture->onKeyboardPress(
					(ActionHandler)& NewPossibleManufactureState::btnManufactureClick,
					Options::keyOk);
	_btnManufacture->setVisible(base->getAvailableWorkshops() > 0); // kL

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_WE_CAN_NOW_PRODUCE"));

	_lstPossibilities->setColor(Palette::blockOffset(8)+10);
	_lstPossibilities->setColumns(1, 288);
	_lstPossibilities->setBig();
	_lstPossibilities->setAlign(ALIGN_CENTER);

	for (std::vector<RuleManufacture*>::const_iterator
			iter = possibilities.begin();
			iter != possibilities.end();
			++iter)
	{
		_lstPossibilities->addRow(
								1,
								tr((*iter)->getName()).c_str());
	}
}

/**
 * return to the previous screen
 * @param action - pointer to an action
 */
void NewPossibleManufactureState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Open the ManufactureState so the player can dispatch available engineers.
 * @param action - pointer to an action
 */
void NewPossibleManufactureState::btnManufactureClick(Action*)
{
	_game->popState();
	_game->pushState(new ManufactureState(
										_base,
										NULL));
}

}
