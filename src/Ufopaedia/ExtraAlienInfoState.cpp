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

#include "ArticleState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleArmor.h"


namespace OpenXcom
{

/**
 * Displays alien properties such as resistances.
 * @param defs - pointer to an ArticleDefinitionTextImage
 */
ExtraAlienInfoState::ExtraAlienInfoState(ArticleDefinitionTextImage* defs)
{
	_screen = false;

	_window		= new Window(this, 224, 168, 48, 16, POPUP_VERTICAL);
	_btnExit	= new TextButton(62, 16, 129, 159);
	_lstInfo	= new TextList(150, 129, 64, 30);

	setPalette("PAL_UFOPAEDIA");

	add(_window);
	add(_btnExit);
	add(_lstInfo);

	centerAllSurfaces();


	const ResourcePack* const rp = _game->getResourcePack();
	_window->setBackground(rp->getSurface(rp->getRandomBackground()));
	_window->setColor(Palette::blockOffset(10)+1); // brown

	_btnExit->setText(tr("STR_OK"));
	_btnExit->setColor(Palette::blockOffset(15)+4); // bluish
	_btnExit->onMouseClick((ActionHandler)& ExtraAlienInfoState::btnExit);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyCancel);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyOk);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					SDLK_KP_ENTER);

	_lstInfo->setColumns(2, 125, 25);
	_lstInfo->setColor(Palette::blockOffset(14)+15); // bluish purple
	_lstInfo->setDot();


	std::string alienId;
	if (defs->id.find("_AUTOPSY") != std::string::npos)
		alienId = defs->id.substr(0, defs->id.length() - 8);
	else
		alienId = defs->id;

	alienId = getBasicUnit(alienId);

	const RuleUnit* const unitRule = _game->getRuleset()->getUnit(alienId);
	if (unitRule != NULL)
	{
		const std::string stArmor = unitRule->getArmor();
		const RuleArmor* const armorRule = _game->getRuleset()->getArmor(stArmor);

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
								Text::formatPercentage(vulnr).c_str());
				_lstInfo->setCellColor(
								row++,
								1,
								Palette::blockOffset(15)+4); // bluish
			}
		}
	}
	else Log(LOG_INFO) << "ERROR: rules not found for unit - " << alienId;
}

/**
 * dTor.
 */
ExtraAlienInfoState::~ExtraAlienInfoState()
{}

/**
 * Closes state.
 * @param action - pointer to an Action
 */
void ExtraAlienInfoState::btnExit(Action*) // private.
{
	_game->popState();
}

/**
 * Gets the id of a Soldier or Terrorist from a race-string.
 * @note Terrorists are sometimes considered members of the race they belong to. *quirgk
 * @param alienId - the alien race
 * @return, basic unit id
 */
std::string ExtraAlienInfoState::getBasicUnit(std::string alienId) const // private.
{
	if (   alienId == "STR_CYBERDISC"
		|| alienId == "STR_REAPER"
		|| alienId == "STR_CHRYSSALID"
		|| alienId == "STR_SILACOID"
		|| alienId == "STR_CELATID"
		|| alienId == "STR_SECTOPOD"
		|| alienId == "STR_CYBERMITE")
	{
		return alienId + "_TERRORIST";
	}

	return alienId + "_SOLDIER";
}

}
