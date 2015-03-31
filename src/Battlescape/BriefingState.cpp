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

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Briefing screen.
 * @param craft	- pointer to the xCom Craft in the mission (default NULL)
 * @param base	- pointer to the xCom Base in the mission (default NULL)
 */
BriefingState::BriefingState(
		const Craft* const craft,
		Base* const base)
{
//	_screen = true;

	_window			= new Window(this, 320, 200);
	_txtTitle		= new Text(288, 17, 16, 22);

	_txtTarget		= new Text(288, 17, 16, 39);
	_txtCraft		= new Text(288, 17, 16, 56);

	_txtBriefing	= new Text(288, 97, 16, 75);

	_btnOk			= new TextButton(288, 16, 16, 177);


	const std::string missionType = _game->getSavedGame()->getSavedBattle()->getMissionType();

	int bgColor;
	std::string
		bg,		// default defined in Ruleset/AlienDeployment.h: "BACK16.SCR",
		music;	// default defined in Ruleset/AlienDeployment.h: OpenXcom::res_MUSIC_GEO_BRIEFING,

	const AlienDeployment* deployRule = _game->getRuleset()->getDeployment(missionType); // check, Xcom1Ruleset->alienDeployments for a missionType
	if (deployRule == NULL) // landing site or crash site -> define BG & Music by ufoType instead
	{
		const Ufo* const ufo = dynamic_cast<Ufo*>(craft->getDestination());
		if (ufo != NULL)
			deployRule = _game->getRuleset()->getDeployment(ufo->getRules()->getType()); // check, Xcom1Ruleset->alienDeployments for a ufoType
	}

	if (deployRule == NULL) // should never happen
	{
		bg = "BACK16.SCR";
		bgColor = _game->getRuleset()->getInterface("briefing")->getElement("palette")->color;
		music = OpenXcom::res_MUSIC_GEO_BRIEFING;
	}
	else
	{
		const BriefingData dataBrief = deployRule->getBriefingData();

		bg = dataBrief.background;
		bgColor = dataBrief.palette;

		if (missionType == "STR_UFO_CRASH_RECOVERY")
			music = OpenXcom::res_MUSIC_GEO_BRIEF_UFOCRASHED;
		else if (missionType == "STR_UFO_GROUND_ASSAULT")
			music = OpenXcom::res_MUSIC_GEO_BRIEF_UFOLANDED;
		else
			music = dataBrief.music;	// note This currently conflicts w/ UFO Recovery/Assault.
										// that is, the music assigned to a UFO will be overridden ...

//		_txtBriefing->setY(_txtBriefing->getY() + dataBrief.textOffset);
//		_txtCraft->setY(_txtCraft->getY() + dataBrief.textOffset);
//		_txtTarget->setVisible(dataBrief.showTarget);
//		_txtCraft->setVisible(dataBrief.showCraft);
	}
	_game->getResourcePack()->playMusic(music);

	setPalette(
			"PAL_GEOSCAPE",
			bgColor);
	_window->setBackground(_game->getResourcePack()->getSurface(bg));

	add(_window,		"window",	"briefing");
	add(_txtTitle,		"text",		"briefing");
	add(_txtTarget,		"text",		"briefing");
	add(_txtCraft,		"text",		"briefing");
	add(_txtBriefing,	"text",		"briefing");
	add(_btnOk,			"button",	"briefing");

	centerAllSurfaces();


	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BriefingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BriefingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BriefingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setText(tr(missionType));

	std::wstring craftLabel;
	if (craft != NULL)
	{
		if (craft->getDestination() != NULL)
		{
			_txtTarget->setText(craft->getDestination()->getName(_game->getLanguage()));
			_txtTarget->setBig();
		}
		else
			_txtTarget->setVisible(false);

		craftLabel = tr("STR_CRAFT_").arg(craft->getName(_game->getLanguage()));
	}
	else if (base != NULL)
	{
		craftLabel = tr("STR_BASE_UC_").arg(base->getName());
		_txtTarget->setVisible(false);
	}

	if (craftLabel.empty() == false)
	{
		_txtCraft->setText(craftLabel);
		_txtCraft->setBig();
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


	std::ostringstream brief;
	brief << missionType.c_str() << "_BRIEFING";
	_txtBriefing->setText(tr(brief.str()));
	_txtBriefing->setWordWrap();


	if (missionType == "STR_BASE_DEFENSE")
		base->setIsRetaliationTarget(false);
}

/**
 * dTor.
 */
BriefingState::~BriefingState()
{}

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

	BattlescapeState* const battleState = new BattlescapeState(); // <- ah there it is!

	int
		liveAliens,
		liveSoldiers;
	battleState->getBattleGame()->tallyUnits(
										liveAliens,
										liveSoldiers);
	if (liveAliens > 0)
	{
		_game->pushState(battleState);
		_game->getSavedGame()->getSavedBattle()->setBattleState(battleState);
		_game->pushState(new NextTurnState(
										_game->getSavedGame()->getSavedBattle(),
										battleState));
		_game->pushState(new InventoryState(
										false,
										battleState));
	}
	else
	{
		Options::baseXResolution = Options::baseXGeoscape;
		Options::baseYResolution = Options::baseYGeoscape;
		_game->getScreen()->resetDisplay(false);;

		delete battleState;
		_game->pushState(new AliensCrashState());
	}
}

}
