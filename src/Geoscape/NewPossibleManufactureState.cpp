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

#include "NewPossibleManufactureState.h"

#include "../Basescape/ManufactureState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the EndManufacture screen.
 * @param base					- pointer to the base to get info from
 * @param possibilities			- reference the vector of pointers to RuleManufacture for projects
 * @param showManufactureButton	- true to show the goto manufacture button
 */
NewPossibleManufactureState::NewPossibleManufactureState(
		Base* const base,
		const std::vector<const RuleManufacture*>& possibilities,
		bool showManufactureButton)
	:
		_base(base)
{
	_screen = false;

	_window				= new Window(this, 288, 180, 16, 10);
	_txtTitle			= new Text(288, 17, 16, 20);

	_lstPossibilities	= new TextList(253, 81, 24, 56);

	_btnManufacture		= new TextButton(160, 14, 80, 149);
	_btnOk				= new TextButton(160, 14, 80, 165);

	setInterface("geoManufacture");

	add(_window,			"window",	"geoManufacture");
	add(_txtTitle,			"text1",	"geoManufacture");
	add(_lstPossibilities,	"text2",	"geoManufacture");
	add(_btnManufacture,	"button",	"geoManufacture");
	add(_btnOk,				"button",	"geoManufacture");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_btnOk->setText(tr(showManufactureButton ? "STR_OK" : "STR_MORE"));
	_btnOk->onMouseClick((ActionHandler)& NewPossibleManufactureState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewPossibleManufactureState::btnOkClick,
					Options::keyCancel);

	_btnManufacture->setText(tr("STR_ALLOCATE_MANUFACTURE"));
	_btnManufacture->onMouseClick((ActionHandler)& NewPossibleManufactureState::btnManufactureClick);
	_btnManufacture->onKeyboardPress(
					(ActionHandler)& NewPossibleManufactureState::btnManufactureClick,
					Options::keyOk);
	_btnManufacture->setVisible(showManufactureButton && base->getAvailableWorkshops() != 0);

	_txtTitle->setText(tr("STR_WE_CAN_NOW_PRODUCE"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstPossibilities->setColumns(1, 261);
	_lstPossibilities->setAlign(ALIGN_CENTER);
	_lstPossibilities->setBig();

	for (std::vector<const RuleManufacture*>::const_iterator
			i = possibilities.begin();
			i != possibilities.end();
			++i)
	{
		_lstPossibilities->addRow(1, tr((*i)->getName()).c_str());
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void NewPossibleManufactureState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the ManufactureState so the player can dispatch available engineers.
 * @param action - pointer to an Action
 */
void NewPossibleManufactureState::btnManufactureClick(Action*)
{
	_game->popState();
	_game->pushState(new ManufactureState(_base));
}

}
