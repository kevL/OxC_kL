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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "BaseDestroyedState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleRegion.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

BaseDestroyedState::BaseDestroyedState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_screen = false;

	_window		= new Window(this, 256, 160, 32, 20);
	_txtMessage	= new Text(224, 120, 48, 30);
	_btnOk		= new TextButton(100, 16, 110, 156);

	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtMessage);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(8)+5);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseDestroyedState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDestroyedState::btnOkClick,
					(SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDestroyedState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap(true);
	_txtMessage->setColor(Palette::blockOffset(8)+5);

	_txtMessage->setText(tr("STR_THE_ALIENS_HAVE_DESTROYED_THE_UNDEFENDED_BASE")
							.arg(_base->getName()));


	std::vector<Region*>::iterator r = _game->getSavedGame()->getRegions()->begin();
	for (
			;
			r != _game->getSavedGame()->getRegions()->end();
			++r)
	{
		if ((*r)->getRules()->insideRegion((base)->getLongitude(), (base)->getLatitude()))
		{
			break;
		}
	}

	AlienMission* a = _game->getSavedGame()->getAlienMission((*r)->getRules()->getType(), "STR_ALIEN_RETALIATION");
	for (std::vector<Ufo*>::iterator
			u = _game->getSavedGame()->getUfos()->begin();
			u != _game->getSavedGame()->getUfos()->end();)
	{
		if ((*u)->getMission() == a)
		{
			delete *u;
			u = _game->getSavedGame()->getUfos()->erase(u);
		}
		else
		{
			++u;
		}
	}

	for (std::vector<AlienMission*>::iterator
			m = _game->getSavedGame()->getAlienMissions().begin();
			m != _game->getSavedGame()->getAlienMissions().end();
			++m)
	{
		if ((AlienMission*)(*m) == a)
		{
			delete (*m);
			_game->getSavedGame()->getAlienMissions().erase(m);

			break;
		}
	}
}

/**
 *
 */
BaseDestroyedState::~BaseDestroyedState()
{
}

/**
 * Resets the palette.
 */
void BaseDestroyedState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void BaseDestroyedState::btnOkClick(Action*)
{
	_game->popState();

	for (std::vector<Base*>::iterator
			b = _game->getSavedGame()->getBases()->begin();
			b != _game->getSavedGame()->getBases()->end();
			++b)
	{
		if ((*b) == _base)
		{
			delete (*b);
			_game->getSavedGame()->getBases()->erase(b);

			break;
		}
	}
}

}
