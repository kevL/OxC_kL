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

#include "MiniMapState.h"

#include <iostream>
#include <sstream>

#include "Camera.h"
#include "MiniMapView.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"
#include "../Engine/Timer.h"

#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleInterface.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the MiniMapState screen.
 * @param camera The Battlescape camera.
 * @param battleGame The Battlescape save.
 */
MiniMapState::MiniMapState(
		Camera* camera,
		SavedBattleGame* battleGame)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_bg			= new InteractiveSurface(320, 200);
	_miniView	= new MiniMapView(
								221, 148, 48, 16,
								_game,
								camera,
								battleGame);

	_btnLvlUp	= new BattlescapeButton(18, 20, 24, 62);
	_btnLvlDwn	= new BattlescapeButton(18, 20, 24, 88);
	_btnOk		= new BattlescapeButton(32, 32, 275, 145);

	_txtLevel	= new Text(28, 16, 281, 73);

	battleGame->setPaletteByDepth(this);

	add(_bg);
	_game->getResourcePack()->getSurface("SCANBORD.PCK")->blit(_bg);

	add(_miniView);
	add(_btnLvlUp, "buttonUp", "minimap", _bg);
	add(_btnLvlDwn, "buttonDown", "minimap", _bg);
	add(_btnOk, "buttonOK", "minimap", _bg);
	add(_txtLevel, "textLevel", "minimap", _bg);

	centerAllSurfaces();

	if (_game->getScreen()->getDY() > 50)
	{
		_screen = false;

/*		SDL_Rect current;
		current.w = 223;
		current.h = 151;
		current.x = 46;
		current.y = 14;
		_bg->drawRect(&current, Palette::blockOffset(15)+15); */
		_bg->drawRect(46, 14, 223, 151, Palette::blockOffset(15)+15);
	}


	_btnLvlUp->onMouseClick((ActionHandler)& MiniMapState::btnLevelUpClick);
	_btnLvlDwn->onMouseClick((ActionHandler)& MiniMapState::btnLevelDownClick);

	_btnOk->onMouseClick((ActionHandler)& MiniMapState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MiniMapState::btnOkClick,
					Options::keyBattleMap);

	_txtLevel->setBig();
//	_txtLevel->setColor(Palette::blockOffset(4));
	_txtLevel->setHighContrast(true);
	std::wostringstream s;
	if (_game->getRuleset()->getInterface("minimap")->getElement("textLevel")->TFTDMode)
		s << tr("STR_LEVEL_SHORT");
	s << camera->getViewLevel();
	_txtLevel->setText(s.str());

	_timerAnimate = new Timer(125);
	_timerAnimate->onTimer((StateHandler)& MiniMapState::animate);
	_timerAnimate->start();

	_miniView->draw();
}

/**
 *
 */
MiniMapState::~MiniMapState()
{
	delete _timerAnimate;
}

/**
 * Handles mouse-wheeling.
 * @param action Pointer to an action.
 */
void MiniMapState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			btnLevelDownClick(action);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			btnLevelUpClick(action);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MiniMapState::btnOkClick(Action*)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_game->popState();
}

/**
 * Changes the currently displayed minimap level.
 * @param action Pointer to an action.
 */
void MiniMapState::btnLevelUpClick(Action*)
{
	std::wostringstream s;
	if (_game->getRuleset()->getInterface("minimap")->getElement("textLevel")->TFTDMode)
		s << tr("STR_LEVEL_SHORT");
	s << _miniView->up();
	_txtLevel->setText(s.str());
}

/**
 * Changes the currently displayed minimap level.
 * @param action Pointer to an action.
 */
void MiniMapState::btnLevelDownClick(Action*)
{
	std::wostringstream s;
	if (_game->getRuleset()->getInterface("minimap")->getElement("textLevel")->TFTDMode)
		s << tr("STR_LEVEL_SHORT");
	s << _miniView->down();
	_txtLevel->setText(s.str());
}

/**
 * Animation handler. Updates the minimap view animation.
 */
void MiniMapState::animate()
{
	_miniView->animate();
}

/**
 * Handles timers.
 */
void MiniMapState::think()
{
	State::think();
	_timerAnimate->think(this, NULL);
}

}
