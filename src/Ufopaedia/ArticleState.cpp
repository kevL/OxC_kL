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

#include "ArticleState.h"

//#include <sstream>

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Constructs an ArticleState.
 * @param article_id - reference the article id of this instance
 */
ArticleState::ArticleState(const std::string& article_id)
	:
		_id(article_id)
{
	_bg			= new Surface(320, 200);
	_btnOk		= new TextButton(30, 14, 5, 5);
	_btnPrev	= new TextButton(30, 14, 40, 5);
	_btnNext	= new TextButton(30, 14, 75, 5);
}

/**
 * Destructor.
 */
ArticleState::~ArticleState()
{}

/**
 * Gets damage type as a string.
 * @param dType - the ItemDamageType (RuleItem.h)
 * @return, type string
 */
std::string ArticleState::getDamageTypeText(ItemDamageType dType) const
{
	switch (dType)
	{
		case DT_AP:
			return "STR_DAMAGE_ARMOR_PIERCING";
		case DT_IN:
			return "STR_DAMAGE_INCENDIARY";
		case DT_HE:
			return "STR_DAMAGE_HIGH_EXPLOSIVE";
		case DT_LASER:
			return "STR_DAMAGE_LASER_BEAM";
		case DT_PLASMA:
			return "STR_DAMAGE_PLASMA_BEAM";
		case DT_STUN:
			return "STR_DAMAGE_STUN";
		case DT_MELEE:
			return "STR_DAMAGE_MELEE";
		case DT_ACID:
			return "STR_DAMAGE_ACID";
		case DT_SMOKE:
			return "STR_DAMAGE_SMOKE";
	}

	return "STR_UNKNOWN";
}

/**
 * Set captions and click handlers for the common control elements.
 * @param contrast - true to set buttons to high contrast (default true)
 */
void ArticleState::initLayout(bool contrast)
{
	add(_bg);
	add(_btnOk);
	add(_btnPrev);
	add(_btnNext);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ArticleState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ArticleState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ArticleState::btnOkClick,
					Options::keyCancel);

	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick((ActionHandler)& ArticleState::btnPrevClick);

	_btnNext->setText(L">");
	_btnNext->onMouseClick((ActionHandler)& ArticleState::btnNextClick);

	if (contrast == true)
	{
		_btnOk->setHighContrast();
		_btnPrev->setHighContrast();
		_btnNext->setHighContrast();
	}
}

/**
 * Returns to the previous screen.
 * @param action - Pointer to an Action
 */
void ArticleState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Shows the previous available article. Loops to the last.
 * @param action - Pointer to an Action
 */
void ArticleState::btnPrevClick(Action*)
{
	Ufopaedia::prev(_game);
}

/**
 * Shows the next available article. Loops to the first.
 * @param action - Pointer to an Action
 */
void ArticleState::btnNextClick(Action*)
{
	Ufopaedia::next(_game);
}

}
