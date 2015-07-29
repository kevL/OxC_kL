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

#include "ArticleStateItem.h"

//#include <algorithm>
//#include <sstream>

#include "Ufopaedia.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Surface.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleItem.h"
//#include "../Ruleset/Ruleset.h"

//#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionItem (ArticleDefinition.h)
 */
ArticleStateItem::ArticleStateItem(ArticleDefinitionItem* defs)
	:
		ArticleState(defs->id)
{
	_txtTitle = new Text(148, 32, 5, 24);

	setPalette("PAL_BATTLEPEDIA");

	ArticleState::initLayout();

	add(_txtTitle);

	_game->getResourcePack()->getSurface("BACK08.SCR")->blit(_bg);

	_btnOk->setColor(Palette::blockOffset(9));
	_btnPrev->setColor(Palette::blockOffset(9));
	_btnNext->setColor(Palette::blockOffset(9));

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(Palette::blockOffset(14)+15);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();


	_image = new Surface(32, 48, 157, 5);
	add(_image);


	const RuleItem* const itRule = _game->getRuleset()->getItem(defs->id);

	itRule->drawHandSprite(
					_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
					_image);

	if (itRule->isTwoHanded() == true)
	{
		_txtTwoHand = new Text(13, 9, 138, 52);
		add(_txtTwoHand);
		_txtTwoHand->setText(tr("STR_2HAND"));
		_txtTwoHand->setColor(Palette::blockOffset(14)+15);
	}

	const std::vector<std::string>* const ammo_data = itRule->getCompatibleAmmo();

	// SHOT STATS TABLE (for firearms only)
	if (itRule->getBattleType() == BT_FIREARM)
	{
		_txtShotType = new Text(100, 17, 8, 66);
		add(_txtShotType);
		_txtShotType->setText(tr("STR_SHOT_TYPE"));
		_txtShotType->setColor(Palette::blockOffset(14)+15);
//		_txtShotType->setWordWrap();

		_txtAccuracy = new Text(50, 17, 108, 66);
		add(_txtAccuracy);
		_txtAccuracy->setText(tr("STR_ACCURACY_UC"));
		_txtAccuracy->setColor(Palette::blockOffset(14)+15);
//		_txtAccuracy->setWordWrap();

		_txtTuCost = new Text(60, 17, 160, 66);
		add(_txtTuCost);
		_txtTuCost->setText(tr("STR_TIME_UNIT_COST"));
		_txtTuCost->setColor(Palette::blockOffset(14)+15);
//		_txtTuCost->setWordWrap();

		_lstInfo = new TextList(204, 57, 8, 82);
		add(_lstInfo);
		_lstInfo->setColor(Palette::blockOffset(15)+4); // color for %-data!
		_lstInfo->setColumns(3, 100, 52, 52);
		_lstInfo->setBig();


		std::wstring tu;

		int current_row = 0;
		if (itRule->getTUAuto() != 0)
		{
			if (itRule->getFlatRate() == true)
				tu = Text::formatNumber(itRule->getTUAuto());
			else
				tu = Text::formatPercentage(itRule->getTUAuto());

			_lstInfo->addRow(
							3,
							tr("STR_SHOT_TYPE_AUTO").c_str(),
							Text::formatNumber(itRule->getAccuracyAuto()).c_str(),
							tu.c_str());
			_lstInfo->setCellColor(current_row++, 0, Palette::blockOffset(14)+15);
		}

		if (itRule->getTUSnap() != 0)
		{
			if (itRule->getFlatRate() == true)
				tu = Text::formatNumber(itRule->getTUSnap());
			else
				tu = Text::formatPercentage(itRule->getTUSnap());

			_lstInfo->addRow(
							3,
							tr("STR_SHOT_TYPE_SNAP").c_str(),
							Text::formatNumber(itRule->getAccuracySnap()).c_str(),
							tu.c_str());
			_lstInfo->setCellColor(current_row++, 0, Palette::blockOffset(14)+15);
		}

		if (itRule->getTUAimed() != 0)
		{
			if (itRule->getFlatRate() == true)
				tu = Text::formatNumber(itRule->getTUAimed());
			else
				tu = Text::formatPercentage(itRule->getTUAimed());

			_lstInfo->addRow(
							3,
							tr("STR_SHOT_TYPE_AIMED").c_str(),
							Text::formatNumber(itRule->getAccuracyAimed()).c_str(),
							tu.c_str());
			_lstInfo->setCellColor(current_row++, 0, Palette::blockOffset(14)+15);
		}

		if (itRule->getTULaunch() != 0)
		{
			if (itRule->getFlatRate() == true)
				tu = Text::formatNumber(itRule->getTULaunch());
			else
				tu = Text::formatPercentage(itRule->getTULaunch());

			_lstInfo->addRow(
							3,
							tr("STR_SHOT_TYPE_LAUNCH").c_str(),
							L"",
							tu.c_str());
			_lstInfo->setCellColor(current_row, 0, Palette::blockOffset(14)+15);
		}

		// text_info is BELOW the info table
		_txtInfo = new Text((ammo_data->size() < 3 ? 300 : 180), 56, 8, 138);
	}
	else // text_info is larger and starts on top
		_txtInfo = new Text(300, 125, 8, 67);

	add(_txtInfo);
	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(Palette::blockOffset(14)+15);
	_txtInfo->setWordWrap();


	for (size_t // AMMO column
			i = 0;
			i != 3;
			++i)
	{
		_txtAmmoType[i] = new Text(90, 9, 189, 24 + (static_cast<int>(i) * 49));
		add(_txtAmmoType[i]);
		_txtAmmoType[i]->setColor(Palette::blockOffset(14)+15);
		_txtAmmoType[i]->setAlign(ALIGN_CENTER);
		_txtAmmoType[i]->setVerticalAlign(ALIGN_MIDDLE);
		_txtAmmoType[i]->setWordWrap();

		_txtAmmoDamage[i] = new Text(90, 16, 190, 40 + (static_cast<int>(i) * 49));
		add(_txtAmmoDamage[i]);
		_txtAmmoDamage[i]->setColor(Palette::blockOffset(2)+5);
		_txtAmmoDamage[i]->setAlign(ALIGN_CENTER);
		_txtAmmoDamage[i]->setBig();

		_imageAmmo[i] = new Surface(32, 48, 280, 16 + static_cast<int>(i) * 49);
		add(_imageAmmo[i]);
	}

	std::wostringstream woststr;

	switch (itRule->getBattleType())
	{
		case BT_FIREARM:
			_txtDamage = new Text(80, 10, 199, 7);
			add(_txtDamage);
			_txtDamage->setText(tr("STR_DAMAGE_UC"));
			_txtDamage->setColor(Palette::blockOffset(14)+15);
			_txtDamage->setAlign(ALIGN_CENTER);

			_txtAmmo = new Text(45, 10, 271, 7);
			add(_txtAmmo);
			_txtAmmo->setText(tr("STR_AMMO"));
			_txtAmmo->setColor(Palette::blockOffset(14)+15);
			_txtAmmo->setAlign(ALIGN_CENTER);

			if (ammo_data->empty() == true)
			{
				_txtAmmoType[0]->setText(tr(getDamageTypeText(itRule->getDamageType())));

				woststr.str(L"");
				woststr.clear();
				woststr << itRule->getPower();
				if (itRule->getShotgunPellets() != 0)
					woststr << L"x" << itRule->getShotgunPellets();

				_txtAmmoDamage[0]->setText(woststr.str());
			}
			else
			{
				const size_t ammoSize = std::min(
											ammo_data->size(),
											static_cast<size_t>(3)); // yeh right.
				for (size_t
						i = 0;
						i != ammoSize;
						++i)
				{
					const ArticleDefinition* const ammo_article = _game->getRuleset()->getUfopaediaArticle((*ammo_data)[i]);
					if (Ufopaedia::isArticleAvailable(
												_game->getSavedGame(),
												ammo_article) == true)
					{
						const RuleItem* const ammo_rule = _game->getRuleset()->getItem((*ammo_data)[i]);
						_txtAmmoType[i]->setText(tr(getDamageTypeText(ammo_rule->getDamageType())));

						woststr.str(L"");
						woststr.clear();
						woststr << ammo_rule->getPower();
						if (ammo_rule->getShotgunPellets() != 0)
							woststr << L"x" << ammo_rule->getShotgunPellets();

						_txtAmmoDamage[i]->setText(woststr.str());

						ammo_rule->drawHandSprite(
											_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
											_imageAmmo[i]);
					}
				}
			}
		break;

		case BT_AMMO:
		case BT_GRENADE:
		case BT_PROXYGRENADE:
		case BT_MELEE:
/*			_txtDamage = new Text(82, 10, 194, 7);
			add(_txtDamage);
			_txtDamage->setColor(Palette::blockOffset(14)+15);
			_txtDamage->setAlign(ALIGN_CENTER);
			_txtDamage->setText(tr("STR_DAMAGE_UC")); */

			_txtAmmoType[0]->setText(tr(getDamageTypeText(itRule->getDamageType())));

			woststr.str(L"");
			woststr.clear();
			woststr << itRule->getPower();
			_txtAmmoDamage[0]->setText(woststr.str());
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateItem::~ArticleStateItem()
{}

}
