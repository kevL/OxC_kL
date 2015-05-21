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

#include "ArticleStateCraftWeapon.h"

//#include <sstream>

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionCraftWeapon (ArticleDefinition.h)
 */
ArticleStateCraftWeapon::ArticleStateCraftWeapon(ArticleDefinitionCraftWeapon* defs)
	:
		ArticleState(defs->id)
{
	const RuleCraftWeapon* const cwRule = _game->getRuleset()->getCraftWeapon(defs->id);

	_txtTitle = new Text(200, 32, 5, 24);

	setPalette("PAL_BATTLEPEDIA");

	ArticleState::initLayout();

	add(_txtTitle);

	_game->getResourcePack()->getSurface(defs->image_id)->blit(_bg);

	_btnOk->setColor(Palette::blockOffset(1));
	_btnPrev->setColor(Palette::blockOffset(1));
	_btnNext->setColor(Palette::blockOffset(1));

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(Palette::blockOffset(14)+15);
	_txtTitle->setBig();
//	_txtTitle->setWordWrap();

	_txtInfo = new Text(310, 32, 5, 160);
	add(_txtInfo);

	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(Palette::blockOffset(14)+15);
	_txtInfo->setWordWrap();

	_lstInfo = new TextList(310, 113, 5, 80);
	add(_lstInfo);

	_lstInfo->setColumns(2, 180, 130);
	_lstInfo->setColor(Palette::blockOffset(14)+15);
	_lstInfo->setBig();
	_lstInfo->setDot();

	_lstInfo->addRow(
				2,
				tr("STR_DAMAGE").c_str(),
				Text::formatNumber(cwRule->getDamage()).c_str());

	_lstInfo->addRow(
				2,
				tr("STR_RANGE").c_str(),
				tr("STR_KILOMETERS").arg(cwRule->getRange()).c_str());

	_lstInfo->addRow(
				2,
				tr("STR_ACCURACY").c_str(),
				Text::formatNumber(cwRule->getAccuracy()).c_str());

	std::wostringstream woststr;
	woststr << cwRule->getAggressiveReload()	<< L" - "
			<< cwRule->getStandardReload()		<< L" - "
			<< cwRule->getCautiousReload();

	_lstInfo->addRow(
				2,
				tr("STR_RELOAD_TIME").c_str(),
				tr("STR_SECONDS").arg(woststr.str()).c_str());
//				tr("STR_SECONDS").arg(cwRule->getStandardReload()).c_str());

	_lstInfo->addRow(
				2,
				tr("STR_ROUNDS").c_str(),
				Text::formatNumber(cwRule->getAmmoMax()).c_str());

	const size_t rows = 5;
	for (size_t
			i = 0;
			i != rows;
			++i)
	{
		_lstInfo->setCellColor(
							i,
							1,
							Palette::blockOffset(15)+4);
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateCraftWeapon::~ArticleStateCraftWeapon()
{}

}
