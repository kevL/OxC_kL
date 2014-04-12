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

#include "BriefingState.h"

#include <sstream>

#include "AliensCrashState.h"
#include "BattlescapeState.h"
#include "InventoryState.h"
#include "NextTurnState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"
#include "../Resource/XcomResourcePack.h" // sza_MusicRules

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Briefing screen.
 * @param game Pointer to the core game.
 * @param craft Pointer to the craft in the mission.
 * @param base Pointer to the base in the mission.
 */
BriefingState::BriefingState(
		Game* game,
		Craft* craft,
		Base* base)
	:
		State(game)
{
	//Log(LOG_INFO) << "Create BriefingState";
	_screen = false;

	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(288, 17, 16, 22);

	_txtTarget		= new Text(288, 17, 16, 39);
	_txtCraft		= new Text(288, 17, 16, 56);

	_txtBriefing	= new Text(288, 97, 16, 75);

	_btnOk			= new TextButton(288, 16, 16, 177);


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_0")->getColors());

	std::string mission = _game->getSavedGame()->getSavedBattle()->getMissionType();
	if (mission == "STR_TERROR_MISSION"
		|| mission == "STR_BASE_DEFENSE")
	{
		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(2)),
					Palette::backPos,
					16);
//		_game->getResourcePack()->playMusic("GMENBASE");
		_game->getResourcePack()->getMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMENBASE)->play(); // sza_MusicRules
	}
	else if (mission == "STR_MARS_CYDONIA_LANDING"
		|| mission == "STR_MARS_THE_FINAL_ASSAULT")
	{
		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
					Palette::backPos,
					16);
//		_game->getResourcePack()->playMusic("GMNEWMAR");
		_game->getResourcePack()->getMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMNEWMAR)->play(); // sza_MusicRules
	}
	else
	{
		_game->setPalette(
					_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
					Palette::backPos,
					16);
//		_game->getResourcePack()->playMusic("GMDEFEND");
		_game->getResourcePack()->getMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMDEFEND)->play(); // sza_MusicRules
	}

	if (mission == "STR_ALIEN_BASE_ASSAULT"
		|| mission == "STR_MARS_CYDONIA_LANDING")
	{
		_txtCraft->setY(40);
		_txtBriefing->setY(56);
		_txtTarget->setVisible(false);
	}

	if (mission == "STR_MARS_THE_FINAL_ASSAULT")
		_txtCraft->setVisible(false);


	add(_window);
	add(_txtTitle);
	add(_btnOk);
	add(_txtTarget);
	add(_txtCraft);
	add(_txtBriefing);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));

	_btnOk->onMouseClick((ActionHandler)& BriefingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BriefingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BriefingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setBig();

	_txtTarget->setColor(Palette::blockOffset(8)+5);
	_txtTarget->setBig();

	_txtCraft->setColor(Palette::blockOffset(8)+5);
	_txtCraft->setBig();

	std::wstring s;
	if (craft)
	{
		if (craft->getDestination())
			_txtTarget->setText(craft->getDestination()->getName(_game->getLanguage()));

		s = tr("STR_CRAFT_").arg(craft->getName(_game->getLanguage()));
	}
	else if (base)
		s = tr("STR_BASE_UC_").arg(base->getName());

	_txtCraft->setText(s);

	_txtBriefing->setColor(Palette::blockOffset(8)+5);
	_txtBriefing->setWordWrap(true);

	// Show respective mission briefing
	if (mission == "STR_ALIEN_BASE_ASSAULT"
		|| mission == "STR_MARS_THE_FINAL_ASSAULT")
	{
		_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));
	}
	else
		_window->setBackground(_game->getResourcePack()->getSurface("BACK16.SCR"));

	_txtTitle->setText(tr(mission));

	std::ostringstream briefingtext;
	briefingtext << mission.c_str() << "_BRIEFING";
	_txtBriefing->setText(tr(briefingtext.str()));

	if (mission == "STR_BASE_DEFENSE")
		base->setRetaliationStatus(false); // And make sure the base is unmarked.
}

/**
 *
 */
BriefingState::~BriefingState()
{
	//Log(LOG_INFO) << "Delete BriefingState";
}

/**
 * Closes the window.
 * @param action Pointer to an action.
 */
void BriefingState::btnOkClick(Action*)
{
	_game->popState();

	BattlescapeState* bs = new BattlescapeState(_game); // <- ah there it is! kL_note
	int
		liveAliens = 0,
		liveSoldiers = 0;

	bs->getBattleGame()->tallyUnits(
								liveAliens,
								liveSoldiers,
								false);
	if (liveAliens > 0)
	{
		_game->pushState(bs);
		_game->getSavedGame()->getSavedBattle()->setBattleState(bs);
		_game->pushState(new NextTurnState(
										_game,
										_game->getSavedGame()->getSavedBattle(),
										bs));
		_game->pushState(new InventoryState(
										_game,
										false,
										bs));
	}
	else
	{
		delete bs;
		_game->pushState(new AliensCrashState(_game));
	}
}

}
