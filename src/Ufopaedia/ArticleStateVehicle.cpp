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

#include "ArticleStateVehicle.h"

//#include <sstream>

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Resource/ResourcePack.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

/**
 * ArticleStateVehicle has a caption, text, and a stats block.
 * @param defs - pointer to ArticleDefinitionVehicle (ArticleDefinition.h)
 */
ArticleStateVehicle::ArticleStateVehicle(ArticleDefinitionVehicle* defs)
	:
		ArticleState(defs->id)
{
	RuleUnit* const unit = _game->getRuleset()->getUnit(defs->id);
	const RuleArmor* const armor = _game->getRuleset()->getArmor(unit->getArmor());
	const RuleItem* const itRule = _game->getRuleset()->getItem(defs->id);

	_txtTitle	= new Text(310, 17, 5, 23);
	_txtInfo	= new Text(300, 150, 10, 122);
	_lstStats	= new TextList(300, 89, 10, 48);

	setPalette("PAL_UFOPAEDIA");

	ArticleState::initLayout();

	add(_txtTitle);
	add(_txtInfo);
	add(_lstStats);

	_game->getResourcePack()->getSurface("BACK10.SCR")->blit(_bg);

	_btnOk->setColor(Palette::blockOffset(5));
	_btnPrev->setColor(Palette::blockOffset(5));
	_btnNext->setColor(Palette::blockOffset(5));

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(Palette::blockOffset(15)+4);
	_txtTitle->setBig();

	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(Palette::blockOffset(15)-1);
	_txtInfo->setWordWrap();

	_lstStats->setColumns(2, 175, 145);
	_lstStats->setColor(Palette::blockOffset(15)+4);
	_lstStats->setDot();

	std::wostringstream woststr;

	woststr << unit->getStats()->tu;
	_lstStats->addRow(
				2,
				tr("STR_TIME_UNITS").c_str(),
				woststr.str().c_str());

	woststr.str(L"");
	woststr << unit->getStats()->health;
	_lstStats->addRow(
				2,
				tr("STR_HEALTH").c_str(),
				woststr.str().c_str());

	woststr.str(L"");
	woststr << armor->getFrontArmor();
	_lstStats->addRow(
				2,
				tr("STR_FRONT_ARMOR").c_str(),
				woststr.str().c_str());

	woststr.str(L"");
	woststr << armor->getSideArmor();
	_lstStats->addRow(
				2,
				tr("STR_LEFT_ARMOR").c_str(),
				woststr.str().c_str());

	woststr.str(L"");
	woststr << armor->getSideArmor();
	_lstStats->addRow(
				2,
				tr("STR_RIGHT_ARMOR").c_str(),
				woststr.str().c_str());

	woststr.str(L"");
	woststr << armor->getRearArmor();
	_lstStats->addRow(
				2,
				tr("STR_REAR_ARMOR").c_str(),
				woststr.str().c_str());

	woststr.str(L"");
	woststr << armor->getUnderArmor();
	_lstStats->addRow(
				2,
				tr("STR_UNDER_ARMOR").c_str(),
				woststr.str().c_str());

	_lstStats->addRow(
				2,
				tr("STR_WEAPON_LC").c_str(),
				tr(defs->weapon).c_str());

	woststr.str(L"");
	if (itRule->getCompatibleAmmo()->empty() == false)
	{
		const RuleItem* const amRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());

		woststr << amRule->getPower();
		_lstStats->addRow(
					2,
					tr("STR_WEAPON_POWER").c_str(),
					woststr.str().c_str());

		_lstStats->addRow(
					2,
					tr("STR_ORDNANCE_LC").c_str(),
					tr(amRule->getName()).c_str());

		woststr.str(L"");
		woststr << amRule->getClipSize();
		_lstStats->addRow(
					2,
					tr("STR_ROUNDS").c_str(),
					woststr.str().c_str());

		_txtInfo->setY(138);
	}
	else
	{
		woststr << itRule->getPower();
		_lstStats->addRow(
					2,
					tr("STR_WEAPON_POWER").c_str(),
					woststr.str().c_str());
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
 ArticleStateVehicle::~ArticleStateVehicle()
{}

}
