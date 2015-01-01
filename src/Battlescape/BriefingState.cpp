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

#include "BriefingState.h"

//#include <sstream>

#include "AliensCrashState.h"
#include "BattlescapeState.h"
#include "InventoryState.h"
#include "NextTurnState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h" // sza_MusicRules

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Briefing screen.
 * @param craft	- pointer to the Craft in the mission (default NULL)
 * @param base	- pointer to the Base in the mission (default NULL)
 */
BriefingState::BriefingState(
		const Craft* const craft,
		Base* const base)
{
	_screen = true;

	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(288, 17, 16, 22);

	_txtTarget		= new Text(288, 17, 16, 39);
	_txtCraft		= new Text(288, 17, 16, 56);

	_txtBriefing	= new Text(288, 97, 16, 75);

	_btnOk			= new TextButton(288, 16, 16, 177);


	const std::string mission = _game->getSavedGame()->getSavedBattle()->getMissionType();

	int backpal;
	std::string
		background, // default: "BACK16.SCR", Ruleset/AlienDeployment.h
		music; // default: OpenXcom::res_MUSIC_GEO_BRIEFING, Ruleset/AlienDeployment.h

	const AlienDeployment* const deployment = _game->getRuleset()->getDeployment(mission);
	if (deployment == NULL) // landing site or crash site.
	{
		backpal = 0;
		background = "BACK16.SCR";

		if (mission == "STR_UFO_CRASH_RECOVERY")
			music = OpenXcom::res_MUSIC_GEO_BRIEF_UFOCRASHED;
		else if (mission == "STR_UFO_GROUND_ASSAULT")
			music = OpenXcom::res_MUSIC_GEO_BRIEF_UFOLANDED;
	}
	else
	{
		const BriefingData dataBrief = deployment->getBriefingData();

		backpal = dataBrief.palette;
		background = dataBrief.background;
		music = dataBrief.music;

//		_txtCraft->setY(_txtCraft->getY() + dataBrief.textOffset);
//		_txtBriefing->setY(_txtBriefing->getY() + dataBrief.textOffset);

//		_txtTarget->setVisible(dataBrief.showTarget);
//		_txtCraft->setVisible(dataBrief.showCraft);
	}

	setPalette(
			"PAL_GEOSCAPE",
			backpal);
	_window->setBackground(_game->getResourcePack()->getSurface(background));
	_game->getResourcePack()->playMusic(music);

/*	if (mission == "STR_ALIEN_BASE_ASSAULT"
		|| mission == "STR_MARS_CYDONIA_LANDING")
	{
		_txtBriefing->setY(56);
		_txtCraft->setY(40);
		_txtTarget->setVisible(false);
	} */

	add(_window);
	add(_txtTitle);
	add(_txtTarget);
	add(_txtCraft);
	add(_txtBriefing);
	add(_btnOk);

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
	_txtTitle->setText(tr(mission));

	std::wstring craftLabel;
	if (craft != NULL)
	{
		if (craft->getDestination() != NULL)
		{
			_txtTarget->setColor(Palette::blockOffset(8)+5);
			_txtTarget->setBig();
			_txtTarget->setText(craft->getDestination()->getName(_game->getLanguage()));
		}
		else
			_txtTarget->setVisible(false);

		craftLabel = tr("STR_CRAFT_").arg(craft->getName(_game->getLanguage()));
	}
	else if (base != NULL)
		craftLabel = tr("STR_BASE_UC_").arg(base->getName());
	else
		craftLabel = L"";

	if (craftLabel.empty() == false)
	{
		_txtCraft->setColor(Palette::blockOffset(8)+5);
		_txtCraft->setBig();
		_txtCraft->setText(craftLabel);
	}
	else
		_txtCraft->setVisible(false);


	if (_txtTarget->getVisible() == false)
	{
		_txtCraft->setY(_txtCraft->getY() - 16);
		_txtBriefing->setY(_txtBriefing->getY() - 16);
	}

	if (_txtCraft->getVisible() == false)
		_txtBriefing->setY(_txtBriefing->getY() - 16);


	_txtBriefing->setColor(Palette::blockOffset(8)+5);
	_txtBriefing->setWordWrap();
	std::ostringstream brief;
	brief << mission.c_str() << "_BRIEFING";
	_txtBriefing->setText(tr(brief.str()));

	if (mission == "STR_BASE_DEFENSE")
		base->setIsRetaliationTarget(false);
}

/**
 * dTor.
 */
BriefingState::~BriefingState()
{
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void BriefingState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 335);

	_game->popState();

	Options::baseXResolution = Options::baseXBattlescape;
	Options::baseYResolution = Options::baseYBattlescape;
	_game->getScreen()->resetDisplay(false);

	BattlescapeState* const bs = new BattlescapeState(); // <- ah there it is!

	int
		liveAliens = 0,
		liveSoldiers = 0;
	bs->getBattleGame()->tallyUnits(
								liveAliens,
								liveSoldiers);
	if (liveAliens > 0)
	{
		_game->pushState(bs);
		_game->getSavedGame()->getSavedBattle()->setBattleState(bs);
		_game->pushState(new NextTurnState(
										_game->getSavedGame()->getSavedBattle(),
										bs));
		_game->pushState(new InventoryState(
										false,
										bs));
	}
	else
	{
		Options::baseXResolution = Options::baseXGeoscape;
		Options::baseYResolution = Options::baseYGeoscape;
		_game->getScreen()->resetDisplay(false);;

		delete bs;
		_game->pushState(new AliensCrashState());
	}
}

}
