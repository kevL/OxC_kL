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

#include "ExtraAlienInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Displays alien properties such as vulnerabilities and fixed-weapon damage.
 * @param defs - pointer to an ArticleDefinitionTextImage
 */
ExtraAlienInfoState::ExtraAlienInfoState(const ArticleDefinitionTextImage* const defs)
	:
		ArticleState(defs->id)
{
	_screen = false;

	_window		= new Window(this, 224, 168, 48, 16, POPUP_VERTICAL);
	_btnExit	= new TextButton(62, 16, 129, 159);
	_lstInfo	= new TextList(150, 73, 84,  28);
	_lstWeapon	= new TextList(150,  9, 84, 110);

	setPalette("PAL_UFOPAEDIA");

	add(_window);
	add(_btnExit);
	add(_lstInfo);
	add(_lstWeapon);

	centerAllSurfaces();


	const ResourcePack* const rp = _game->getResourcePack();
	_window->setBackground(rp->getSurface(rp->getRandomBackground()));
	_window->setColor(uPed_PINK);

	_btnExit->setText(tr("STR_OK"));
	_btnExit->setColor(uPed_GREEN_SLATE);
	_btnExit->onMouseClick((ActionHandler)& ExtraAlienInfoState::btnExit);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyCancel);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyOk);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyOkKeypad);

	_lstInfo->setColumns(2, 125, 25);
	_lstInfo->setColor(uPed_BLUE_SLATE);
	_lstInfo->setDot();

	_lstWeapon->setColumns(3, 100, 25, 25);
	_lstWeapon->setColor(uPed_BLUE_SLATE);
	_lstWeapon->setDot();


	std::string alienId;
	if (defs->id.find("_AUTOPSY") != std::string::npos)
		alienId = defs->id.substr(0, defs->id.length() - 8);
	else
		alienId = defs->id;

	const RuleUnit* unitRule;
	if (_game->getRuleset()->getUnit(alienId + "_SOLDIER") != NULL)
		unitRule = _game->getRuleset()->getUnit(alienId + "_SOLDIER");
	else if (_game->getRuleset()->getUnit(alienId + "_TERRORIST") != NULL)
		unitRule = _game->getRuleset()->getUnit(alienId + "_TERRORIST");
	else
	{
		unitRule = NULL;
		Log(LOG_INFO) << "ERROR: rules not found for unit - " << alienId;
	}

	if (unitRule != NULL)
	{
		const RuleArmor* const armorRule = _game->getRuleset()->getArmor(unitRule->getArmor());

		size_t row = 0;

		for (size_t
				i = 0;
				i != RuleArmor::DAMAGE_TYPES;
				++i)
		{
			const ItemDamageType dType = static_cast<ItemDamageType>(i);
			const std::string st = ArticleState::getDamageTypeText(dType);
			if (st != "STR_UNKNOWN")
			{
				const int vulnr = static_cast<int>(Round(static_cast<double>(armorRule->getDamageModifier(dType)) * 100.));
				_lstInfo->addRow(
								2,
								tr(st).c_str(),
								Text::formatPct(vulnr).c_str());
				_lstInfo->setCellColor(row++, 1, uPed_GREEN_SLATE);
			}
		}

		if (unitRule->isLivingWeapon() == true)
		{
			std::string terrorWeapon = unitRule->getRace().substr(4) + "_WEAPON";
			const RuleItem* const itRule = _game->getRuleset()->getItem(terrorWeapon);
			if (itRule != NULL)
			{
				const ItemDamageType dType = itRule->getDamageType();
				const std::string stType = ArticleState::getDamageTypeText(dType);
				if (stType != "STR_UNKNOWN")
				{
					std::wstring wstPower;
					if (itRule->isStrengthApplied() == true)
						wstPower = tr("STR_VARIABLE");
					else
						wstPower = Text::formatNumber(itRule->getPower());

					_lstWeapon->addRow(
									3,
									tr("STR_WEAPON").c_str(),
									wstPower.c_str(),
									tr(stType).c_str());
					_lstWeapon->setCellColor(0,1, uPed_GREEN_SLATE);
					_lstWeapon->setCellColor(0,2, uPed_GREEN_SLATE);
				}
			}
		}
	}
}

/**
 * dTor.
 */
ExtraAlienInfoState::~ExtraAlienInfoState() // virtual.
{}

/**
 * Closes state.
 * @param action - pointer to an Action
 */
void ExtraAlienInfoState::btnExit(Action*) // private.
{
	_game->popState();
}

}
