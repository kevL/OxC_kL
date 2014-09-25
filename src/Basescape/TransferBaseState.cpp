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

#include "TransferBaseState.h"

#include <sstream>

#include "TransferItemsState.h"

#include "../Basescape/StoresMatrixState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

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
 * @param base Pointer to the base to get info from.
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

	setPalette("PAL_BASESCAPE", 4);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtFunds);
	add(_txtName);
	add(_txtArea);
	add(_lstBases);
	add(_btnMatrix);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnCancel->setColor(Palette::blockOffset(13)+5);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& TransferBaseState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& TransferBaseState::btnCancelClick,
					Options::keyCancel);

	_btnMatrix->setColor(Palette::blockOffset(13)+5);
	_btnMatrix->setText(tr("STR_MATRIX"));
	_btnMatrix->onMouseClick((ActionHandler)& TransferBaseState::btnMatrixClick);

	_txtTitle->setColor(Palette::blockOffset(13)+5);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_RIGHT);
	_txtTitle->setText(tr("STR_SELECT_DESTINATION_BASE"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+5);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtFunds->setColor(Palette::blockOffset(13)+5);
	_txtFunds->setSecondaryColor(Palette::blockOffset(13));
	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtName->setColor(Palette::blockOffset(13)+5);
	_txtName->setText(tr("STR_BASE_KL"));
	_txtName->setBig();

	_txtArea->setColor(Palette::blockOffset(13)+5);
	_txtArea->setText(tr("STR_AREA"));
	_txtArea->setBig();

	_lstBases->setColor(Palette::blockOffset(15)+1);
	_lstBases->setArrowColor(Palette::blockOffset(13)+5);
	_lstBases->setColumns(2, 128, 80);
	_lstBases->setSelectable();
	_lstBases->setBackground(_window);
	_lstBases->setMargin();
	_lstBases->onMouseClick((ActionHandler)& TransferBaseState::lstBasesClick);

	int row = 0;
	for (std::vector<Base*>::iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (*i != _base)
		{
			std::wstring area;

			for (std::vector<Region*>::iterator
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

			_lstBases->addRow(
							2,
							(*i)->getName().c_str(),
							area.c_str());
			_lstBases->setCellColor(
								row,
								1,
								Palette::blockOffset(13)+5);
			_bases.push_back(*i);

			row++;
		}
	}
}

/**
 * dTor.
 */
TransferBaseState::~TransferBaseState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TransferBaseState::btnCancelClick(Action*)
{
	_game->popState(); // pop choose Destination (this)
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void TransferBaseState::btnMatrixClick(Action*)
{
	_game->pushState(new StoresMatrixState(_base));
}

/**
 * Shows the Transfer screen for the selected base.
 * @param action Pointer to an action.
 */
void TransferBaseState::lstBasesClick(Action*)
{
	_game->pushState(new TransferItemsState(
										_base,
										_bases[_lstBases->getSelectedRow()]));
}

}
