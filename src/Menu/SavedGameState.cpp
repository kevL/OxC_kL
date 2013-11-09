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

#include "SavedGameState.h"
#include "../Engine/Logger.h"
#include "../Savegame/SavedGame.h"
#include "../Engine/Game.h"
#include "../Engine/Exception.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Saved Game screen.
 * @param game, Pointer to the core game.
 * @param origin, Game section that originated this state.
 */
SavedGameState::SavedGameState(Game* game, OptionsOrigin origin, int firstValidRow)
	:
		State(game),
		_origin(origin),
		_firstValidRow(firstValidRow),
		_showMsg(true),
		_noUI(false)
{
	_screen = false;

	_window		= new Window(this, 320, 200, 0, 0, POPUP_BOTH);

	_txtTitle	= new Text(310, 17, 5, 8);

	_txtDelete	= new Text(310, 9, 5, 24);

	_txtName	= new Text(150, 9, 16, 32);
	_txtTime	= new Text(30, 9, 184, 32);
	_txtDate	= new Text(38, 9, 214, 32);

	_lstSaves	= new TextList(294, 112, 8, 40);

	_txtStatus	= new Text(320, 17, 0, 92);
	_txtDetails = new Text(288, 9, 16, 160);

	_btnCancel	= new TextButton(288, 16, 16, 177);

	if (_origin != OPT_BATTLESCAPE)
	{
		_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)), Palette::backPos, 16);
	}

	add(_window);
	add(_btnCancel);
	add(_txtTitle);
	add(_txtDelete);
	add(_txtName);
	add(_txtTime);
	add(_txtDate);
	add(_lstSaves);
	add(_txtStatus);
	add(_txtDetails);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnCancel->setColor(Palette::blockOffset(8)+5);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& SavedGameState::btnCancelClick);
	_btnCancel->onKeyboardPress((ActionHandler)& SavedGameState::btnCancelClick, (SDLKey) Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtDelete->setColor(Palette::blockOffset(15)-1);
	_txtDelete->setAlign(ALIGN_CENTER);
	_txtDelete->setText(tr("STR_RIGHT_CLICK_TO_DELETE"));

	_txtName->setColor(Palette::blockOffset(15)-1);
	_txtName->setText(tr("STR_NAME"));

	_txtTime->setColor(Palette::blockOffset(15)-1);
	_txtTime->setText(tr("STR_TIME"));

	_txtDate->setColor(Palette::blockOffset(15)-1);
	_txtDate->setText(tr("STR_DATE"));

	_txtStatus->setColor(Palette::blockOffset(11)+1);
	_txtStatus->setBig();
	_txtStatus->setAlign(ALIGN_CENTER);

	_lstSaves->setColor(Palette::blockOffset(8)+10);
	_lstSaves->setArrowColor(Palette::blockOffset(8)+5);
	_lstSaves->setColumns(5, 168, 30, 30, 30, 30);
	_lstSaves->setSelectable(true);
	_lstSaves->setBackground(_window);
	_lstSaves->setMargin(8);
	_lstSaves->onMouseOver((ActionHandler)& SavedGameState::lstSavesMouseOver);
	_lstSaves->onMouseOut((ActionHandler)& SavedGameState::lstSavesMouseOut);

	_txtDetails->setColor(Palette::blockOffset(15)-1);
	_txtDetails->setSecondaryColor(Palette::blockOffset(8)+10);
	_txtDetails->setText(L"");
}

/**
 * Initializes the Quicksave notice.
 * @param game, Pointer to the core game.
 * @param origin, Game section that originated this state.
 * @param showMsg, True if need to show messages like "Loading game" or "Saving game".
 */
SavedGameState::SavedGameState(Game* game, OptionsOrigin origin, int firstValidRow, bool showMsg)
	:
		State(game),
		_origin(origin),
		_firstValidRow(firstValidRow),
		_showMsg(showMsg),
		_noUI(true)
{
	if (_showMsg)
	{
		_txtStatus = new Text(320, 17, Screen::getDX(), Screen::getDY() + 64);
		add(_txtStatus);

		_txtStatus->setBig();
		_txtStatus->setAlign(ALIGN_CENTER);

		if (origin == OPT_BATTLESCAPE)
		{
			_txtStatus->setColor(Palette::blockOffset(13)+5);	// kL, blue
			_txtStatus->setHighContrast(true);
		}
		else
		{
			_txtStatus->setColor(Palette::blockOffset(11+5));	// kL, purple
		}
	}
}

/**
 *
 */
SavedGameState::~SavedGameState()
{
}

/**
 * Resets the palette and refreshes saves.
 */
void SavedGameState::init()
{
	if (_noUI)
	{
		_game->popState();

		return;
	}

	_txtStatus->setText(L"");

	if (_origin != OPT_BATTLESCAPE)
	{
		_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)), Palette::backPos, 16);
	}
	else
	{
		applyBattlescapeTheme();
	}

	try
	{
		updateList();
	}
	catch (Exception &e)
	{
		Log(LOG_ERROR) << e.what();
	}
}

/**
 * Updates the save game list with a current list of available savegames.
 */
void SavedGameState::updateList()
{
	_lstSaves->clearList();
	_saves = SavedGame::getList(_lstSaves, _game->getLanguage());
}

/**
 * Updates the status message in the center of the screen.
 * @param msg New message ID.
 */
void SavedGameState::updateStatus(const std::string& msg)
{
	_txtStatus->setText(tr(msg));
	blit();
	_game->getScreen()->flip();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SavedGameState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 *
 */
void SavedGameState::lstSavesMouseOver(Action*)
{
	int sel = _lstSaves->getSelectedRow() - _firstValidRow;
	if (0 > sel)
	{
		_txtDetails->setText(L"");
		return;
	}

	std::string fullname = Options::getUserFolder() + _saves[sel];
	std::wstring wstr=L"";
	try
	{
		YAML::Node doc = YAML::LoadFile(fullname);
		if (doc["turn"])
		{
			wstr = tr("STR_BATTLESCAPE"); wstr += L": ";
			wstr += tr(doc["mission"].as<std::string>()); wstr += L", ";
			wstr += tr("STR_TURN").arg(Language::utf8ToWstr(doc["turn"].as<std::string>()));
		}
		else wstr = tr("STR_GEOSCAPE");
	}
	catch (Exception &e)
	{
		Log(LOG_ERROR) << e.what();
	}
	catch (YAML::Exception &e)
	{
		Log(LOG_ERROR) << e.what();
	}

	_txtDetails->setText(tr("STR_DETAILS").arg(wstr));
}

/**
 *
 */
void SavedGameState::lstSavesMouseOut(Action*)
{
	_txtDetails->setText(L"");
}

}
