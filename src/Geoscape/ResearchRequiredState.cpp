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

#include "ResearchRequiredState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Research Required screen.
 * @param item - pointer to a researched weapon
 */
ResearchRequiredState::ResearchRequiredState(RuleItem* item)
{
	_screen = false;

	_window		= new Window(this, 288, 180, 16, 10);
	_btnOk		= new TextButton(160, 18, 80, 150);
	_txtTitle	= new Text(288, 80, 16, 50);

	setInterface("geoResearch");

	add(_window,	"window",	"geoResearch");
	add(_btnOk,		"button",	"geoResearch");
	add(_txtTitle,	"text1",	"geoResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ResearchRequiredState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchRequiredState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchRequiredState::btnOkClick,
					Options::keyOk);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	const std::string
		weapon = item->getType(),
		clip = item->getCompatibleAmmo()->front();
	_txtTitle->setText(tr("STR_YOU_NEED_TO_RESEARCH_ITEM_TO_PRODUCE_ITEM")
					   .arg(tr(clip))
					   .arg(tr(weapon)));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchRequiredState::btnOkClick(Action*)
{
	_game->popState();
}

}
