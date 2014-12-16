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

#include "ListLoadState.h"

#include "ConfirmLoadState.h"
#include "ListLoadOriginalState.h"
#include "LoadGameState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Load Game screen.
 * @param origin - game section that originated this state
 */
ListLoadState::ListLoadState(OptionsOrigin origin)
	:
		ListGamesState(
			origin,
			0,
			true)
{
/*	_btnOld = new TextButton(80, 16, 60, 172);
	add(_btnOld);
	if (origin != OPT_MENU)
		_btnOld->setVisible(false);
	else
		_btnCancel->setX(180); */

	_btnOld->setColor(Palette::blockOffset(8)+5);
	_btnOld->setText(L"original");
	_btnOld->onMouseClick((ActionHandler)& ListLoadState::btnOldClick);

	_txtTitle->setText(tr("STR_SELECT_GAME_TO_LOAD"));

	centerAllSurfaces();
}

/**
 * dTor.
 */
ListLoadState::~ListLoadState()
{
}

/**
 * Switches to Original X-Com saves.
 * @param action - pointer to an Action
 */
void ListLoadState::btnOldClick(Action*)
{
	_game->pushState(new ListLoadOriginalState);
}

/**
 * Loads the selected save.
 * @param action - pointer to an Action
 */
void ListLoadState::lstSavesPress(Action* action)
{
	ListGamesState::lstSavesPress(action);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_origin == OPT_MENU
			|| (_origin == OPT_GEOSCAPE
				&& _saves[_lstSaves->getSelectedRow()].mode == MODE_BATTLESCAPE)
			|| (_origin == OPT_BATTLESCAPE
				&& _saves[_lstSaves->getSelectedRow()].mode == MODE_GEOSCAPE))
		{
			_game->getResourcePack()->fadeMusic(_game, 1225);
		}


		bool confirm = false;
		for (std::vector<std::string>::const_iterator
				i = _saves[_lstSaves->getSelectedRow()].rulesets.begin();
				i != _saves[_lstSaves->getSelectedRow()].rulesets.end();
				++i)
		{
			if (std::find(
						Options::rulesets.begin(),
						Options::rulesets.end(),
						*i) == Options::rulesets.end())
			{
				confirm = true;
				break;
			}
		}

		if (confirm == true)
				_game->pushState(new ConfirmLoadState(
													_origin,
													_saves[_lstSaves->getSelectedRow()].fileName));
		else
			_game->pushState(new LoadGameState(
											_origin,
											_saves[_lstSaves->getSelectedRow()].fileName,
											_palette));
	}
}

}
