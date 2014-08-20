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

#include "StoresState.h"

#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Stores window.
 * @param base Pointer to the base to get info from.
 */
StoresState::StoresState(Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtItem		= new Text(162, 9, 16, 25);
	_txtQuantity	= new Text(84, 9, 178, 25);
	_txtSpaceUsed	= new Text(26, 9, 262, 25);

	_lstStores		= new TextList(285, 136, 16, 36);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_BASESCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtItem);
	add(_txtQuantity);
	add(_txtSpaceUsed);
	add(_lstStores);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& StoresState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_STORES"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setColor(Palette::blockOffset(13)+10);
	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setColor(Palette::blockOffset(13)+10);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

	_txtSpaceUsed->setColor(Palette::blockOffset(13)+10);
//kL	_txtSpaceUsed->setText(tr("STR_SPACE_USED_UC"));
	_txtSpaceUsed->setText(tr("STR_VOLUME")); // kL

	_lstStores->setColor(Palette::blockOffset(13)+10);
	_lstStores->setColumns(3, 154, 84, 26);
	_lstStores->setSelectable(true);
	_lstStores->setBackground(_window);
	_lstStores->setMargin(8);


	SavedGame* sg = _game->getSavedGame();
	Ruleset* rules = _game->getRuleset();
	RuleItem
		* itRule,
		* laRule,
		* clRule;
	RuleCraftWeapon* cwRule;

	int
		row = 0,
		clipSize;

	const std::vector<std::string>& items = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		int qty = _base->getItems()->getItem(*i);
		if (qty > 0)
		{
			itRule = rules->getItem(*i);
			Uint8 color = Palette::blockOffset(13)+10; // blue

			std::wostringstream
				ss,
				ss2;
			ss << qty;
			ss2 << qty * itRule->getSize();

			std::wstring item = tr(*i);
			if (itRule->getBattleType() == BT_AMMO
				 || (itRule->getBattleType() == BT_NONE
					&& itRule->getClipSize() > 0))
			{
				if (itRule->getBattleType() == BT_AMMO
					&& itRule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
				{
					clipSize = itRule->getClipSize();
					if (clipSize > 1)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
				}

				item.insert(0, L"  ");
				color = Palette::blockOffset(15)+6; // purple
			}

			bool craftOrdnance = false;
			const std::vector<std::string>& craftWeaps = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					j = craftWeaps.begin();
					j != craftWeaps.end()
						&& craftOrdnance == false;
					++j)
			{
				// Special handling for treating craft weapons as items
				cwRule = rules->getCraftWeapon(*j);

				laRule = rules->getItem(cwRule->getLauncherItem());
				clRule = rules->getItem(cwRule->getClipItem());

				if (laRule == itRule)
				{
					craftOrdnance = true;
					clipSize = cwRule->getAmmoMax(); // Launcher capacity
					if (clipSize > 0)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
				}
				else if (clRule == itRule)
				{
					craftOrdnance = true;
					clipSize = clRule->getClipSize(); // launcher Ammo quantity
					if (clipSize > 1)
						item = item + L"s (" + Text::formatNumber(clipSize) + L")";
				}
			}

			if (itRule->isFixed() // tank w/ Ordnance.
				&& !itRule->getCompatibleAmmo()->empty())
			{
				clRule = rules->getItem(itRule->getCompatibleAmmo()->front());
				clipSize = clRule->getClipSize();
				if (clipSize > 0)
					item = item + L" (" + Text::formatNumber(clipSize) + L")";
			}

			if (!sg->isResearched(itRule->getType())				// not researched
				&& (!sg->isResearched(itRule->getRequirements())	// and has requirements to use but not been researched
					|| rules->getItem(*i)->getAlien()							// or is an alien
					|| itRule->getBattleType() == BT_CORPSE					// or is a corpse
					|| itRule->getBattleType() == BT_NONE)					// or is not a battlefield item
				&& craftOrdnance == false)									// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = Palette::blockOffset(13)+5; // yellow
			}

			_lstStores->addRow(
							3,
//							tr(*i).c_str(),
							item.c_str(),
							ss.str().c_str(),
							ss2.str().c_str());

			_lstStores->setRowColor(row, color);

			row++;
		}
	}
}

/**
 *
 */
StoresState::~StoresState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StoresState::btnOkClick(Action*)
{
	_game->popState();
}

}
