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

#include "ListSaveState.h"

#include "SaveGameState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Save Game screen.
 * @param origin - game section that originated this state
 */
ListSaveState::ListSaveState(OptionsOrigin origin)
	:
		ListGamesState(
			origin,
			1, false),
		_selectedRowPre(-1),
		_selectedRow(-1)
{
	_edtSave		= new TextEdit(this, 168, 9);
	_btnSaveGame	= new TextButton(134, 16, 170, 177);
//	_btnSaveGame	= new TextButton(_game->getSavedGame()->isIronman()? 200: 80, 16, 60, 172);

	add(_edtSave);
	add(_btnSaveGame, "button", "saveMenus");

	_txtTitle->setText(tr("STR_SELECT_SAVE_POSITION"));

/*	if (_game->getSavedGame()->isIronman())
		_btnCancel->setVisible(false);
	else
		_btnCancel->setX(180); */

	// note: selected SaveSlot for Battlescape is grayscaled.
	_edtSave->setColor(Palette::blockOffset(10));
	_edtSave->setHighContrast();
	_edtSave->setVisible(false);
	_edtSave->onKeyboardPress((ActionHandler)& ListSaveState::edtSaveKeyPress);

	_btnSaveGame->setText(tr("STR_OK"));
	_btnSaveGame->onMouseClick((ActionHandler)& ListSaveState::btnSaveGameClick);
	_btnSaveGame->setVisible(false);

	centerAllSurfaces();
}

/**
 * dTor.
 */
ListSaveState::~ListSaveState()
{}

/**
 * Updates the save game list with the current list of available savedgames.
 */
void ListSaveState::updateList()
{
	_lstSaves->addRow(1, tr("STR_NEW_SAVED_GAME_SLOT").c_str());

	if (_origin != OPT_BATTLESCAPE)
		_lstSaves->setRowColor(0, _lstSaves->getSecondaryColor());

	ListGamesState::updateList();
}

/**
 * Names the selected save.
 * @param action - pointer to an Action
 */
void ListSaveState::lstSavesPress(Action* action)
{
/*	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT && _edtSave->isFocused())
	{
		_edtSave->setText(L"");
		_edtSave->setVisible(false);
		_edtSave->setFocus(false, false);
		_lstSaves->setScrolling(true);
	} */
	if (_inEditMode == true)
		return;

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_inEditMode = true;					// kL
		_btnSaveGame->setVisible();			// kL
		_lstSaves->setSelectable(false);	// kL
		_lstSaves->setScrollable(false);


		_selectedRowPre = _selectedRow;
		_selectedRow = _lstSaves->getSelectedRow();

		switch (_selectedRowPre)
		{
			case -1: // first click on the savegame list
			break;

			case 0:
				_lstSaves->setCellText(
									_selectedRowPre,
									0,
									tr("STR_NEW_SAVED_GAME_SLOT"));
			break;

			default:
				_lstSaves->setCellText(
									_selectedRowPre,
									0,
									_selected);
		}

		_selected = _lstSaves->getCellText(
										_lstSaves->getSelectedRow(),
										0);
		_lstSaves->setCellText(
							_lstSaves->getSelectedRow(),
							0,
							L"");

		_edtSave->setTextStored(_selected);

		if (_lstSaves->getSelectedRow() == 0)
			_selected = L"";

		_edtSave->setText(_selected);

		_edtSave->setX(_lstSaves->getColumnX(0));
		_edtSave->setY(_lstSaves->getRowY(_selectedRow));
		_edtSave->setVisible();
		_edtSave->setFocus(true, false);

		ListGamesState::disableSort();
	}
	else
		ListGamesState::lstSavesPress(action); // RMB -> delete file
}

/**
 * Saves the selected slot or cancels it.
 * @param action - pointer to an Action
 */
void ListSaveState::edtSaveKeyPress(Action* action)
{
	if (_inEditMode == true)
	{
		if (action->getDetails()->key.keysym.sym == SDLK_RETURN
			|| action->getDetails()->key.keysym.sym == SDLK_KP_ENTER)
		{
			saveGame();
		}
		else if (action->getDetails()->key.keysym.sym == SDLK_ESCAPE) // note this should really be the keyCancel option
		{
//			_inEditMode = false; // done in ListGamesState::btnCancelKeypress()
			_btnSaveGame->setVisible(false);
			_lstSaves->setSelectable();
			_lstSaves->setScrollable();

			_edtSave->setVisible(false);
			_edtSave->setFocus(false);

			_lstSaves->setCellText(
								_lstSaves->getSelectedRow(),
								0,
								_edtSave->getTextStored());
		}
	}
}

/**
 * Saves the selected slot.
 * @param action - pointer to an Action
 */
void ListSaveState::btnSaveGameClick(Action*)
{
	if (_inEditMode == true
		&& _selectedRow != -1)
	{
		saveGame();
	}
}

/**
 * Saves the selected slot.
 */
void ListSaveState::saveGame()
{
//	_inEditMode = false;				// safeties. Should not need these three <- ie.
//	_btnSaveGame->setVisible(false);	// SaveGameState() below_ pops current state(s) all the way back to play.
//	_lstSaves->setSelectable();
//	_lstSaves->setScrollable();			// don't need this either ....

	hideElements();
	_game->getSavedGame()->setName(_edtSave->getText());

	std::string
		file = CrossPlatform::sanitizeFilename(Language::wstrToFs(_edtSave->getText())),
		fileOld;

	if (_selectedRow > 0)
	{
		fileOld = _saves[_selectedRow - 1].file;
		if (fileOld != file + ".sav")
		{
			while (CrossPlatform::fileExists(Options::getUserFolder() + file + ".sav") == true)
				file += "_";

			CrossPlatform::moveFile(
								Options::getUserFolder() + fileOld,
								Options::getUserFolder() + file + ".sav");
		}
	}
	else
	{
		while (CrossPlatform::fileExists(Options::getUserFolder() + file + ".sav") == true)
			file += "_";
	}

	file += ".sav";
	_game->pushState(new SaveGameState(_origin, file, _palette));
}

/**
 * Hides textlike elements of this state.
 */
void ListSaveState::hideElements() // private.
{
	_txtTitle->setVisible(false);
	_txtDelete->setVisible(false);
	_txtName->setVisible(false);
	_txtDate->setVisible(false);
	_sortName->setVisible(false);
	_sortDate->setVisible(false);
	_lstSaves->setVisible(false);
	_edtSave->setVisible(false);
	_txtDetails->setVisible(false);
	_btnCancel->setVisible(false);
	_btnSaveGame->setVisible(false);
}

}
