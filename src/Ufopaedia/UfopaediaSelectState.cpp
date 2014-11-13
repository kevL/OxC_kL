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

#include "UfopaediaSelectState.h"

#include "ArticleState.h"
#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

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

	_window			= new Window(this, 256, 180, 32, 10, POPUP_NONE);
	_txtTitle		= new Text(224, 17, 48, 26);
	_btnOk			= new TextButton(224, 16, 48, 166);
	_lstSelection	= new TextList(224, 105, 40, 50);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_btnOk);
	add(_lstSelection);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setColor(Palette::blockOffset(8)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SELECT_ITEM"));

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& UfopaediaSelectState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& UfopaediaSelectState::btnOkClick,
					Options::keyCancel);

	_lstSelection->setColor(Palette::blockOffset(8)+5);
	_lstSelection->setArrowColor(Palette::blockOffset(15)-1);
	_lstSelection->setColumns(1, 206);
	_lstSelection->setSelectable();
	_lstSelection->setBackground(_window);
	_lstSelection->setMargin(18);
	_lstSelection->setAlign(ALIGN_CENTER);
	_lstSelection->onMouseClick((ActionHandler)& UfopaediaSelectState::lstSelectionClick);

	loadSelectionList();
}

/**
 * dTor.
 */
UfopaediaSelectState::~UfopaediaSelectState()
{
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

	ArticleDefinitionList::iterator it;
	for (
			it = _article_list.begin();
			it != _article_list.end();
			++it)
	{
		_lstSelection->addRow(
							1,
							tr((*it)->title).c_str());
	}
}

}
