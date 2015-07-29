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

#include "UfopaediaSelectState.h"

#include "Ufopaedia.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param section - reference the section desired
 */
UfopaediaSelectState::UfopaediaSelectState(const std::string& section)
	:
		_section(section)
{
	_screen = false;

	_window			= new Window(this, 256, 194, 32, 6);
	_txtTitle		= new Text(224, 17, 48, 15);
	_lstSelection	= new TextList(224, 137, 40, 35);
	_btnOk			= new TextButton(224, 16, 48, 177);

	setInterface("ufopaedia");

	add(_window,		"window",	"ufopaedia");
	add(_txtTitle,		"text",		"ufopaedia");
	add(_btnOk,			"button2",	"ufopaedia");
	add(_lstSelection,	"list",		"ufopaedia");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_ITEM"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& UfopaediaSelectState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& UfopaediaSelectState::btnOkClick,
					Options::keyCancel);

	_lstSelection->setColumns(1, 206);
	_lstSelection->setBackground(_window);
	_lstSelection->setSelectable();
	_lstSelection->setMargin(18);
	_lstSelection->setAlign(ALIGN_CENTER);
	_lstSelection->onMouseClick((ActionHandler)& UfopaediaSelectState::lstSelectionClick);

	loadSelectionList();
}

/**
 * dTor.
 */
UfopaediaSelectState::~UfopaediaSelectState()
{}

/**
 * Initializes the state.
 */
void UfopaediaSelectState::init()
{
	State::init();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void UfopaediaSelectState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 *
 * @param action - pointer to an Action
 */
void UfopaediaSelectState::lstSelectionClick(Action*)
{
	Ufopaedia::openArticle(
						_game,
						_article_list[_lstSelection->getSelectedRow()]);
}

/**
 *
 */
void UfopaediaSelectState::loadSelectionList()
{
	_article_list.clear();
	Ufopaedia::list(
				_game->getSavedGame(),
				_game->getRuleset(),
				_section,
				_article_list);

	ArticleDefinitionList::const_iterator i;
	for (
			i = _article_list.begin();
			i != _article_list.end();
			++i)
	{
		_lstSelection->addRow(
							1,
							tr((*i)->title).c_str());
	}
}

}
