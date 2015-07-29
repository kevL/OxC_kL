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

#include "ArticleStateArmor.h"

//#include <sstream>
//#include "../fmath.h"

//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionArmor (ArticleDefinition.h)
 */
ArticleStateArmor::ArticleStateArmor(ArticleDefinitionArmor* defs)
	:
		ArticleState(defs->id),
		_row(0)
{
	const RuleArmor* const armorRule = _game->getRuleset()->getArmor(defs->id);

	_txtTitle = new Text(300, 17, 5, 24);

	setPalette("PAL_BATTLEPEDIA");

	ArticleState::initLayout();

	add(_txtTitle);

	_btnOk->setColor(Palette::blockOffset(0)+15);
	_btnPrev->setColor(Palette::blockOffset(0)+15);
	_btnNext->setColor(Palette::blockOffset(0)+15);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(Palette::blockOffset(14)+15);
	_txtTitle->setBig();

	_image = new Surface(320, 200);
	add(_image);

	std::string look = armorRule->getSpriteInventory();
	look += "M0.SPK";
	if (CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look)) == false
		&& _game->getResourcePack()->getSurface(look) == NULL)
	{
		look = armorRule->getSpriteInventory() + ".SPK";
	}
	_game->getResourcePack()->getSurface(look)->blit(_image);


	_lstInfo = new TextList(150, 129, 150, 12);
	add(_lstInfo);
	_lstInfo->setColumns(2, 125, 25);
	_lstInfo->setColor(Palette::blockOffset(14)+15);
	_lstInfo->setDot();

	_txtInfo = new Text(300, 56, 8, 150);
	add(_txtInfo);
	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(Palette::blockOffset(14)+15);
	_txtInfo->setWordWrap();


	addStat("STR_FRONT_ARMOR",	armorRule->getFrontArmor());
	addStat("STR_LEFT_ARMOR",	armorRule->getSideArmor());
	addStat("STR_RIGHT_ARMOR",	armorRule->getSideArmor());
	addStat("STR_REAR_ARMOR",	armorRule->getRearArmor());
	addStat("STR_UNDER_ARMOR",	armorRule->getUnderArmor());

	_lstInfo->addRow(0);
	++_row;


	for (size_t
			i = 0;
			i != RuleArmor::DAMAGE_TYPES;
			++i)
	{
		const ItemDamageType dType = static_cast<ItemDamageType>(i);
		const std::string st = getDamageTypeText(dType);
		if (st != "STR_UNKNOWN")
		{
			const int vulnr = static_cast<int>(Round(static_cast<double>(armorRule->getDamageModifier(dType)) * 100.));
			addStat(
				st,
				Text::formatPercentage(vulnr));
		}
	}

	_lstInfo->addRow(0);
	++_row;

	// Add unit stats
	addStat("STR_TIME_UNITS",			armorRule->getStats()->tu,			true);
	addStat("STR_STAMINA",				armorRule->getStats()->stamina,		true);
	addStat("STR_HEALTH",				armorRule->getStats()->health,		true);
	addStat("STR_BRAVERY",				armorRule->getStats()->bravery,		true);
	addStat("STR_REACTIONS",			armorRule->getStats()->reactions,	true);
	addStat("STR_FIRING_ACCURACY",		armorRule->getStats()->firing,		true);
	addStat("STR_THROWING_ACCURACY",	armorRule->getStats()->throwing,	true);
	addStat("STR_MELEE_ACCURACY",		armorRule->getStats()->melee,		true);
	addStat("STR_STRENGTH",				armorRule->getStats()->strength,	true);
	addStat("STR_PSIONIC_STRENGTH",		armorRule->getStats()->psiStrength,	true);
	addStat("STR_PSIONIC_SKILL",		armorRule->getStats()->psiSkill,	true);

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateArmor::~ArticleStateArmor()
{}

/**
 *
 * @param stLabel	-
 * @param iStat		-
 * @param addSign	-
 */
void ArticleStateArmor::addStat( // private.
		const std::string& stLabel,
		int iStat,
		bool addSign)
{
	if (iStat != 0)
	{
		std::wostringstream woststr;
		if (addSign == true
			&& iStat > 0)
		{
			woststr << L'+';
		}
		woststr << iStat;

		_lstInfo->addRow(
						2,
						tr(stLabel).c_str(),
						woststr.str().c_str());
		_lstInfo->setCellColor(
						_row++,
						1,
						Palette::blockOffset(15)+4);
	}
}

/**
 *
 * @param stLabel -
 * @param wstStat -
 */
void ArticleStateArmor::addStat( // private.
		const std::string& stLabel,
		const std::wstring& wstStat)
{
	_lstInfo->addRow(
					2,
					tr(stLabel).c_str(),
					wstStat.c_str());
	_lstInfo->setCellColor(
					_row++,
					1,
					Palette::blockOffset(15)+4);
}

}
