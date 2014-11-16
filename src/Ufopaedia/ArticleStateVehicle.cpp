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

#include "ArticleStateVehicle.h"

//#include <sstream>

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Resource/ResourcePack.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/Unit.h"


namespace OpenXcom
{

/**
 * ArticleStateVehicle has a caption, text, and a stats block.
 * @param defs - pointer to ArticleDefinitionVehicle
 */
ArticleStateVehicle::ArticleStateVehicle(ArticleDefinitionVehicle* defs)
	:
		ArticleState(defs->id)
{
	Unit* const unit = _game->getRuleset()->getUnit(defs->id);
	const Armor* const armor = _game->getRuleset()->getArmor(unit->getArmor());
	RuleItem* const item = _game->getRuleset()->getItem(defs->id);

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

	_txtTitle->setColor(Palette::blockOffset(15)+4);
	_txtTitle->setBig();
	_txtTitle->setText(tr(defs->title));

	_txtInfo->setColor(Palette::blockOffset(15)-1);
	_txtInfo->setWordWrap();
	_txtInfo->setText(tr(defs->text));

	_lstStats->setColor(Palette::blockOffset(15)+4);
	_lstStats->setColumns(2, 175, 145);
	_lstStats->setDot();

	std::wostringstream
		ss,
		ss2,
		ss3,
		ss4,
		ss5,
		ss6,
		ss7,
		ss8;

	ss << unit->getStats()->tu;
	_lstStats->addRow(
					2,
					tr("STR_TIME_UNITS").c_str(),
					ss.str().c_str());

	ss2 << unit->getStats()->health;
	_lstStats->addRow(
					2,
					tr("STR_HEALTH").c_str(),
					ss2.str().c_str());

	ss3 << armor->getFrontArmor();
	_lstStats->addRow(
					2,
					tr("STR_FRONT_ARMOR").c_str(),
					ss3.str().c_str());

	ss4 << armor->getSideArmor();
	_lstStats->addRow(
					2,
					tr("STR_LEFT_ARMOR").c_str(),
					ss4.str().c_str());

	ss5 << armor->getSideArmor();
	_lstStats->addRow(
					2,
					tr("STR_RIGHT_ARMOR").c_str(),
					ss5.str().c_str());

	ss6 << armor->getRearArmor();
	_lstStats->addRow(
					2,
					tr("STR_REAR_ARMOR").c_str(),
					ss6.str().c_str());

	ss7 << armor->getUnderArmor();
	_lstStats->addRow(
					2,
					tr("STR_UNDER_ARMOR").c_str(),
					ss7.str().c_str());

	_lstStats->addRow(
					2,
					tr("STR_WEAPON_LC").c_str(), // was "STR_WEAPON"
					tr(defs->weapon).c_str());

	if (item->getCompatibleAmmo()->empty() == false)
	{
		const RuleItem* const ammo = _game->getRuleset()->getItem(item->getCompatibleAmmo()->front());

		ss8 << ammo->getPower();
		_lstStats->addRow(
						2,
						tr("STR_WEAPON_POWER").c_str(),
						ss8.str().c_str());

		_lstStats->addRow(
						2,
						tr("STR_ORDNANCE_LC").c_str(), // was "STR_AMMUNITION"
						tr(ammo->getName()).c_str());

		std::wostringstream ss9;
		ss9 << ammo->getClipSize();
		_lstStats->addRow(
						2,
						tr("STR_ROUNDS").c_str(),
						ss9.str().c_str());

		_txtInfo->setY(138);
	}
	else
	{
		ss8 << item->getPower();
		_lstStats->addRow(
						2,
						tr("STR_WEAPON_POWER").c_str(),
						ss8.str().c_str());
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
 ArticleStateVehicle::~ArticleStateVehicle()
{
}

}
