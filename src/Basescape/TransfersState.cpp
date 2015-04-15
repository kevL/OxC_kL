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

#include "TransfersState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Transfers window.
 * @param base - pointer to the Base to get info from
 */
TransfersState::TransfersState(Base* base)
	:
		_base(base)
{
	_screen = false;

	_window			= new Window(this, 320, 184, 0, 8, POPUP_BOTH);
	_txtTitle		= new Text(288, 17, 16, 17);
	_txtBaseLabel	= new Text(80, 9, 16, 17);

	_txtItem		= new Text(114, 9, 16, 34);
	_txtQuantity	= new Text(54, 9, 179, 34);
	_txtArrivalTime	= new Text(28, 9, 254, 34);

	_lstTransfers	= new TextList(285, 121, 16, 45);

	_btnOk			= new TextButton(288, 16, 16, 169);

	std::string pal = "PAL_BASESCAPE";
	int bgHue = 6; // oxide by default in ufo palette
	const Element* const element = _game->getRuleset()->getInterface("transferInfo")->getElement("palette");
	if (element != NULL)
	{
		if (element->TFTDMode == true)
			pal = "PAL_GEOSCAPE";

		if (element->color != std::numeric_limits<int>::max())
			bgHue = element->color;
	}
	setPalette(pal, bgHue);

	add(_window,			"window",	"transferInfo");
	add(_txtTitle,			"text",		"transferInfo");
	add(_txtBaseLabel,		"text",		"transferInfo");
	add(_txtItem,			"text",		"transferInfo");
	add(_txtQuantity,		"text",		"transferInfo");
	add(_txtArrivalTime,	"text",		"transferInfo");
	add(_lstTransfers,		"list",		"transferInfo");
	add(_btnOk,				"button",	"transferInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& TransfersState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransfersState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& TransfersState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_TRANSFERS_UC"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtArrivalTime->setText(tr("STR_ARRIVAL_TIME_HOURS"));

	_lstTransfers->setColumns(3, 155, 75, 28);
	_lstTransfers->setBackground(_window);
	_lstTransfers->setSelectable();
	_lstTransfers->setMargin();

	for (std::vector<Transfer*>::const_iterator
			i = _base->getTransfers()->begin();
			i != _base->getTransfers()->end();
			++i)
	{
		std::wostringstream
			woststr1,
			woststr2;
		woststr1 << (*i)->getQuantity();
		woststr2 << (*i)->getHours();

		_lstTransfers->addRow(
							3,
							(*i)->getName(_game->getLanguage()).c_str(),
							woststr1.str().c_str(),
							woststr2.str().c_str());
	}
}

/**
 * dTor.
 */
TransfersState::~TransfersState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void TransfersState::btnOkClick(Action*)
{
	_game->popState();
}

}
