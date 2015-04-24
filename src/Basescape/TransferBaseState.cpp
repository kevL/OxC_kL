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

#include "TransferBaseState.h"

//#include <sstream>

#include "StoresMatrixState.h"
#include "TransferItemsState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleRegion.h"

#include "../Savegame/Base.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Transfer to Base window.
 * @param base - pointer to the base to get info from
 */
TransferBaseState::TransferBaseState(Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 260, 140, 30, 30);
	_txtTitle		= new Text(228, 16, 46, 40);
	_txtBaseLabel	= new Text(80, 9, 46, 40);

	_txtFunds		= new Text(100, 9, 46, 54);

	_txtName		= new Text(80, 17, 46, 65);
	_txtArea		= new Text(60, 17, 182, 65);

	_lstBases		= new TextList(228, 57, 46, 82);

	_btnMatrix		= new TextButton(112, 16, 46, 146);
	_btnCancel		= new TextButton(112, 16, 162, 146);

	setInterface("transferBaseSelect");

	add(_window,		"window",	"transferBaseSelect");
	add(_txtTitle,		"text",		"transferBaseSelect");
	add(_txtBaseLabel,	"text",		"transferBaseSelect");
	add(_txtFunds,		"text",		"transferBaseSelect");
	add(_txtName,		"text",		"transferBaseSelect");
	add(_txtArea,		"text",		"transferBaseSelect");
	add(_lstBases,		"list",		"transferBaseSelect");
	add(_btnMatrix,		"button",	"transferBaseSelect");
	add(_btnCancel,		"button",	"transferBaseSelect");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& TransferBaseState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferBaseState::btnCancelClick,
					Options::keyCancel);

	_btnMatrix->setText(tr("STR_MATRIX"));
	_btnMatrix->onMouseClick((ActionHandler)& TransferBaseState::btnMatrixClick);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_RIGHT);
	_txtTitle->setText(tr("STR_SELECT_DESTINATION_BASE"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtName->setText(tr("STR_BASE_KL"));
	_txtName->setBig();

	_txtArea->setText(tr("STR_AREA"));
	_txtArea->setBig();

	_lstBases->setBackground(_window);
	_lstBases->setColumns(2, 128, 80);
	_lstBases->setSelectable();
	_lstBases->setMargin();
	_lstBases->onMouseClick((ActionHandler)& TransferBaseState::lstBasesClick);

	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (*i != _base)
		{
			std::wstring area;

			for (std::vector<Region*>::const_iterator
					j = _game->getSavedGame()->getRegions()->begin();
					j != _game->getSavedGame()->getRegions()->end();
					++j)
			{
				if ((*j)->getRules()->insideRegion(
												(*i)->getLongitude(),
												(*i)->getLatitude()))
				{
					area = tr((*j)->getRules()->getType());
					break;
				}
			}

			std::wostringstream woststr;
			woststr << L'\x01' << area;
			_lstBases->addRow(
							2,
							(*i)->getName().c_str(),
							woststr.str().c_str());
			_bases.push_back(*i);
		}
	}
}

/**
 * dTor.
 */
TransferBaseState::~TransferBaseState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void TransferBaseState::btnCancelClick(Action*)
{
	_game->popState(); // pop choose Destination (this)
}

/**
 * Shows da Matrix.
 * @param action - pointer to an Action
 */
void TransferBaseState::btnMatrixClick(Action*)
{
	_game->pushState(new StoresMatrixState(_base));
}

/**
 * Shows the Transfer screen for the selected base.
 * @param action - pointer to an Action
 */
void TransferBaseState::lstBasesClick(Action*)
{
	_game->pushState(new TransferItemsState(
										_base,
										_bases[_lstBases->getSelectedRow()]));
}

}
