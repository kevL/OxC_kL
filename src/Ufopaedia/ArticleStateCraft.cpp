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

#include "ArticleStateCraft.h"

//#include <sstream>

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 */
ArticleStateCraft::ArticleStateCraft(ArticleDefinitionCraft* defs)
	:
		ArticleState(defs->id)
{
	_txtTitle = new Text(210, 32, 5, 24);

	setPalette("PAL_UFOPAEDIA");

	ArticleState::initLayout();

	add(_txtTitle);

	_game->getResourcePack()->getSurface(defs->image_id)->blit(_bg);
	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnPrev->setColor(Palette::blockOffset(15)-1);
	_btnNext->setColor(Palette::blockOffset(15)-1);

	_txtTitle->setColor(Palette::blockOffset(14)+15);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();
	_txtTitle->setText(tr(defs->title));

	_txtInfo = new Text(
					defs->rect_text.width,
					defs->rect_text.height,
					defs->rect_text.x,
					defs->rect_text.y);
	add(_txtInfo);

	_txtInfo->setColor(Palette::blockOffset(14)+15);
	_txtInfo->setWordWrap();
	_txtInfo->setText(tr(defs->text));

	_txtStats = new Text(
						defs->rect_stats.width,
						defs->rect_stats.height,
						defs->rect_stats.x,
						defs->rect_stats.y);
	add(_txtStats);

	_txtStats->setColor(Palette::blockOffset(14)+15);
	_txtStats->setSecondaryColor(Palette::blockOffset(15)+4);

	const RuleCraft* const craftRule = _game->getRuleset()->getCraft(defs->id);
	int range = craftRule->getMaxFuel();
	if (craftRule->getRefuelItem().empty() == false)
		range *= craftRule->getMaxSpeed();
//	else
//		range *= 100;

	range /= 6; // six doses per hour on Geoscape.

	std::wostringstream ss;
	ss << tr("STR_MAXIMUM_SPEED_UC").arg(Text::formatNumber(craftRule->getMaxSpeed(), L"", false)) << L'\n';
	ss << tr("STR_ACCELERATION").arg(craftRule->getAcceleration()) << L'\n';
	ss << tr("STR_FUEL_CAPACITY").arg(Text::formatNumber(range, L"", false)) << L'\n';
	ss << tr("STR_WEAPON_PODS").arg(craftRule->getWeapons()) << L'\n';
	ss << tr("STR_DAMAGE_CAPACITY_UC").arg(Text::formatNumber(craftRule->getMaxDamage(), L"", false)) << L'\n';
	ss << tr("STR_CARGO_SPACE").arg(craftRule->getSoldiers()) << L'\n';
	ss << tr("STR_HWP_CAPACITY").arg(craftRule->getVehicles());
	_txtStats->setText(ss.str());

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateCraft::~ArticleStateCraft()
{
}

}
