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

#include "ListGamesState.h"

//#include <utility>

#include "DeleteGameState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 *
 */
struct compareSaveName
	:
		public std::binary_function<SaveInfo&, SaveInfo&, bool>
{
	bool _reverse;

	///
	explicit compareSaveName(bool rev)
		:
			_reverse(rev)
			{}

	///
	bool operator()(
			const SaveInfo& a,
			const SaveInfo& b) const
	{
		if (a.reserved == b.reserved)
			return CrossPlatform::naturalCompare(
											a.displayName,
											b.displayName);
		else
			return _reverse ? b.reserved : a.reserved;
	}
};

/**
 *
 */
struct compareSaveTimestamp
	:
		public std::binary_function<SaveInfo&, SaveInfo&, bool>
{
	bool _reverse;

	///
	explicit compareSaveTimestamp(bool rev)
		:
			_reverse(rev)
			{}

	///
	bool operator()(
			const SaveInfo& a,
			const SaveInfo& b) const
	{
		if (a.reserved == b.reserved)
			return a.timestamp < b.timestamp;
		else
			return _reverse ? b.reserved : a.reserved;
	}
};


/**
 * Initializes all the elements in the Saved Game screen.
 * @param origin		- game section that originated this state
 * @param firstValidRow	- first row containing saves
 * @param autoquick		- true to show auto/quick saved games
 */
ListGamesState::ListGamesState(
		OptionsOrigin origin,
		size_t firstValidRow,
		bool autoquick)
	:
		_origin(origin),
		_firstValidRow(firstValidRow),
		_autoquick(autoquick),
		_sortable(true),
		_inEditMode(false)
{
	_screen = false;

	_window		= new Window(this, 320, 200, 0,0, POPUP_BOTH);

	_txtTitle	= new Text(310, 16, 5,  8);
	_txtDelete	= new Text(310,  9, 5, 24);

	_txtName	= new Text(176, 9,  16, 33);
	_txtDate	= new Text( 84, 9, 204, 33);
	_sortName	= new ArrowButton(ARROW_NONE, 11, 8,  16, 33);
	_sortDate	= new ArrowButton(ARROW_NONE, 11, 8, 204, 33);

	_lstSaves	= new TextList(285, 121, 16, 42);

	_txtDetails = new Text(288, 9, 16, 165);

	_btnCancel	= new TextButton(134, 16, 16, 177);

	setInterface(
			"geoscape",
			true,
			_origin == OPT_BATTLESCAPE);

	add(_window,		"window",	"saveMenus");
	add(_txtTitle,		"text",		"saveMenus");
	add(_txtDelete,		"text",		"saveMenus");
	add(_txtName,		"text",		"saveMenus");
	add(_txtDate,		"text",		"saveMenus");
	add(_sortName,		"text",		"saveMenus");
	add(_sortDate,		"text",		"saveMenus");
	add(_lstSaves,		"list",		"saveMenus");
	add(_txtDetails,	"text",		"saveMenus");
	add(_btnCancel,		"button",	"saveMenus");


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& ListGamesState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& ListGamesState::btnCancelKeypress,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtDelete->setAlign(ALIGN_CENTER);
	_txtDelete->setText(tr("STR_RIGHT_CLICK_TO_DELETE"));

	_txtName->setText(tr("STR_NAME"));

	_txtDate->setText(tr("STR_DATE"));

	_lstSaves->setBackground(_window);
	_lstSaves->setColumns(3, 188, 60, 29);
	_lstSaves->setSelectable();
	_lstSaves->setMargin();
	_lstSaves->onMouseOver((ActionHandler)& ListGamesState::lstSavesMouseOver);
	_lstSaves->onMouseOut((ActionHandler)& ListGamesState::lstSavesMouseOut);
	_lstSaves->onMousePress((ActionHandler)& ListGamesState::lstSavesPress);

	_txtDetails->setWordWrap();
	_txtDetails->setText(tr("STR_DETAILS").arg(L""));

	_sortName->setX(_sortName->getX() + _txtName->getTextWidth() + 5);
	_sortName->onMouseClick((ActionHandler)& ListGamesState::sortNameClick);

	_sortDate->setX(_sortDate->getX() + _txtDate->getTextWidth() + 5);
	_sortDate->onMouseClick((ActionHandler)& ListGamesState::sortDateClick);

	updateArrows();
}

/**
 * dTor.
 */
ListGamesState::~ListGamesState()
{}

/**
 * Refreshes the saves list.
 */
void ListGamesState::init()
{
	State::init();

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeTheme();

	try
	{
		_saves = SavedGame::getList(
								_game->getLanguage(),
								_autoquick);
		_lstSaves->clearList();
		sortList(Options::saveOrder);
	}
	catch (Exception &e)
	{
		Log(LOG_ERROR) << e.what();
	}
}

/**
 * Updates the sorting arrows based on the current setting.
 */
void ListGamesState::updateArrows()
{
	_sortName->setShape(ARROW_NONE);
	_sortDate->setShape(ARROW_NONE);

	switch (Options::saveOrder)
	{
		case SORT_NAME_ASC:
			_sortName->setShape(ARROW_SMALL_UP);
		break;
		case SORT_NAME_DESC:
			_sortName->setShape(ARROW_SMALL_DOWN);
		break;
		case SORT_DATE_ASC:
			_sortDate->setShape(ARROW_SMALL_UP);
		break;
		case SORT_DATE_DESC:
			_sortDate->setShape(ARROW_SMALL_DOWN);
	}
}

/**
 * Sorts the save game list.
 * @param order - order to sort the games in
 */
void ListGamesState::sortList(SaveSort order)
{
	switch (order)
	{
		case SORT_NAME_ASC:
			std::sort(
					_saves.begin(),
					_saves.end(),
					compareSaveName(false));
		break;
		case SORT_NAME_DESC:
			std::sort(
					_saves.rbegin(),
					_saves.rend(),
					compareSaveName(true));
		break;
		case SORT_DATE_ASC:
			std::sort(
					_saves.begin(),
					_saves.end(),
					compareSaveTimestamp(false));
		break;
		case SORT_DATE_DESC:
			std::sort(
					_saves.rbegin(),
					_saves.rend(),
					compareSaveTimestamp(true));
	}

	updateList();
}

/**
 * Updates the save game list with a current list of available savegames.
 */
void ListGamesState::updateList()
{
	size_t row = 0;
	Uint8 color = _lstSaves->getSecondaryColor();

	for (std::vector<SaveInfo>::const_iterator
			i = _saves.begin();
			i != _saves.end();
			++i,
				++row)
	{
		_lstSaves->addRow(
						3,
						i->displayName.c_str(),
						i->isoDate.c_str(),
						i->isoTime.c_str());
		if (i->reserved == true
			&& _origin != OPT_BATTLESCAPE)
		{
			_lstSaves->setRowColor(row, color);
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ListGamesState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Reverts text edit or returns to the previous screen.
 * @param action - pointer to an Action
 */
void ListGamesState::btnCancelKeypress(Action*)
{
	if (_inEditMode == true) // revert TextEdit first onEscape, see ListSaveState::edtSaveKeyPress()
		_inEditMode = false;
	else
		_game->popState(); // 2nd Escape releases state
}

/**
 * Shows the details of the currently hovered save.
 * @param action - pointer to an Action
 */
void ListGamesState::lstSavesMouseOver(Action*)
{
	if (_inEditMode == false)
	{
		std::wstring wst;

		const int sel = static_cast<int>(_lstSaves->getSelectedRow()) - static_cast<int>(_firstValidRow);
		if (sel > -1
			&& sel < static_cast<int>(_saves.size()))
		{
			wst = _saves[static_cast<size_t>(sel)].details;
		}

		_txtDetails->setText(tr("STR_DETAILS").arg(wst));
	}
}

/**
 * Hides details of saves.
 */
void ListGamesState::lstSavesMouseOut(Action*)
{
	if (_inEditMode == false)
		_txtDetails->setText(tr("STR_DETAILS").arg(L""));
}

/**
 * Deletes the selected save.
 * @param action - pointer to an Action
 */
void ListGamesState::lstSavesPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		&& _lstSaves->getSelectedRow() >= _firstValidRow)
	{
		_game->pushState(new DeleteGameState(
										_origin,
										_saves[_lstSaves->getSelectedRow() - _firstValidRow].fileName));
	}
}

/**
 * Sorts the saves by name.
 * @param action - pointer to an Action
 */
void ListGamesState::sortNameClick(Action*)
{
	if (_sortable == true)
	{
		if (Options::saveOrder == SORT_NAME_ASC)
			Options::saveOrder = SORT_NAME_DESC;
		else
			Options::saveOrder = SORT_NAME_ASC;

		updateArrows();
		_lstSaves->clearList();

		sortList(Options::saveOrder);
	}
}

/**
 * Sorts the saves by date.
 * @param action - pointer to an Action
 */
void ListGamesState::sortDateClick(Action*)
{
	if (_sortable == true)
	{
		if (Options::saveOrder == SORT_DATE_ASC)
			Options::saveOrder = SORT_DATE_DESC;
		else
			Options::saveOrder = SORT_DATE_ASC;

		updateArrows();
		_lstSaves->clearList();

		sortList(Options::saveOrder);
	}
}

/**
 * Disables sorting.
 */
void ListGamesState::disableSort()
{
	_sortable = false;
}

}
