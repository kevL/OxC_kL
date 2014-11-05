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

#include "StoresMatrixState.h"

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

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Matrix window.
 * @param base - pointer to the accessing Base
 */
StoresMatrixState::StoresMatrixState(Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtItem		= new Text(100, 9, 16, 25);

	_txtBase_0		= new Text(23, 9, 116, 25);
	_txtBase_1		= new Text(23, 9, 139, 25);
	_txtBase_2		= new Text(23, 9, 162, 25);
	_txtBase_3		= new Text(23, 9, 185, 25);
	_txtBase_4		= new Text(23, 9, 208, 25);
	_txtBase_5		= new Text(23, 9, 231, 25);
	_txtBase_6		= new Text(23, 9, 254, 25);
	_txtBase_7		= new Text(23, 9, 277, 25);

	_lstMatrix		= new TextList(285, 137, 16, 36);

	_btnOk			= new TextButton(268, 16, 26, 177);

	setPalette("PAL_BASESCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtItem);
	add(_txtBase_0);
	add(_txtBase_1);
	add(_txtBase_2);
	add(_txtBase_3);
	add(_txtBase_4);
	add(_txtBase_5);
	add(_txtBase_6);
	add(_txtBase_7);
	add(_lstMatrix);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& StoresMatrixState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresMatrixState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresMatrixState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MATRIX"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtItem->setColor(Palette::blockOffset(13)+10);
	_txtItem->setText(tr("STR_ITEM"));


	SavedGame* save = _game->getSavedGame();
	std::wstring wstr;

	if (save->getBases()->at(0)) // always true, but hey.
	{
		_txtBase_0->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(0)->getName().substr(0, 4);
		_txtBase_0->setText(wstr);
	}

	if (save->getBases()->at(1))
	{
		_txtBase_1->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(1)->getName().substr(0, 4);
		_txtBase_1->setText(wstr);
	}

	if (save->getBases()->at(2))
	{
		_txtBase_2->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(2)->getName().substr(0, 4);
		_txtBase_2->setText(wstr);
	}

	if (save->getBases()->at(3))
	{
		_txtBase_3->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(3)->getName().substr(0, 4);
		_txtBase_3->setText(wstr);
	}

	if (save->getBases()->at(4))
	{
		_txtBase_4->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(4)->getName().substr(0, 4);
		_txtBase_4->setText(wstr);
	}

	if (save->getBases()->at(5))
	{
		_txtBase_5->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(5)->getName().substr(0, 4);
		_txtBase_5->setText(wstr);
	}

	if (save->getBases()->at(6))
	{
		_txtBase_6->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(6)->getName().substr(0, 4);
		_txtBase_6->setText(wstr);
	}

	if (save->getBases()->at(7))
	{
		_txtBase_7->setColor(Palette::blockOffset(13)+10);

		wstr = save->getBases()->at(7)->getName().substr(0, 4);
		_txtBase_7->setText(wstr);
	}

	_lstMatrix->setColor(Palette::blockOffset(13)+10);
	_lstMatrix->setColumns(9, 100, 23, 23, 23, 23, 23, 23, 23, 23);
	_lstMatrix->setSelectable();
	_lstMatrix->setBackground(_window);


	int qty[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	size_t
		ent = 0,
		row = 0;

	std::wostringstream
		ss0,
		ss1,
		ss2,
		ss3,
		ss4,
		ss5,
		ss6,
		ss7;

	Ruleset* rules = _game->getRuleset();
	RuleItem* rule;
//		* launchRule,
//		* clipRule;
	RuleCraftWeapon* cwRule;

	const std::vector<std::string>& items = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		rule = rules->getItem(*i);
		std::string test = rule->getType();

		ss0.str(L"");
		ss1.str(L"");
		ss2.str(L"");
		ss3.str(L"");
		ss4.str(L"");
		ss5.str(L"");
		ss6.str(L"");
		ss7.str(L"");

		ent = 0;

		for (std::vector<Base*>::const_iterator
				j = save->getBases()->begin();
				j != save->getBases()->end();
				++j)
		{
			qty[ent] = (*j)->getItems()->getItem(*i);

			for (std::vector<Transfer*>::const_iterator // Add qty of items in transit to theMatrix.
					k = (*j)->getTransfers()->begin();
					k != (*j)->getTransfers()->end();
					++k)
			{
				if ((*k)->getItems() == test)
					qty[ent] += (*k)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // Add qty of items & vehicles on transport craft to da-Matrix.
					k = (*j)->getCrafts()->begin();
					k != (*j)->getCrafts()->end();
					++k)
			{
				if ((*k)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::iterator
							l = (*k)->getItems()->getContents()->begin();
							l != (*k)->getItems()->getContents()->end();
							++l)
					{
						if (l->first == test)
							qty[ent] += l->second;
					}
				}

				if ((*k)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							l = (*k)->getVehicles()->begin();
							l != (*k)->getVehicles()->end();
							++l)
					{
						if ((*l)->getRules()->getType() == test)
							qty[ent]++;

						if ((*l)->getAmmo() != 255)
						{
							RuleItem
								* tankRule = rules->getItem((*l)->getRules()->getType()),
								* ammoRule = rules->getItem(tankRule->getCompatibleAmmo()->front());

							if (ammoRule->getType() == test)
								qty[ent] += (*l)->getAmmo();
						}
					}
				}
			}

			ent++;
		}

		if (qty[0] + qty[1] + qty[2] + qty[3] + qty[4] + qty[5] + qty[6] + qty[7] > 0)
		{
			if (qty[0] > 0) ss0 << qty[0];
			if (qty[1] > 0) ss1 << qty[1];
			if (qty[2] > 0) ss2 << qty[2];
			if (qty[3] > 0) ss3 << qty[3];
			if (qty[4] > 0) ss4 << qty[4];
			if (qty[5] > 0) ss5 << qty[5];
			if (qty[6] > 0) ss6 << qty[6];
			if (qty[7] > 0) ss7 << qty[7];


			bool craftOrdnance = false;
			const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
			for (std::vector<std::string>::const_iterator
					j = cwList.begin();
					j != cwList.end()
						&& craftOrdnance == false;
					++j)
			{
				// Special handling for treating craft weapons as items
				cwRule = rules->getCraftWeapon(*j);

				if (rule == rules->getItem(cwRule->getLauncherItem())
					|| rule == rules->getItem(cwRule->getClipItem()))
				{
					craftOrdnance = true;
				}
			}

/*				launchRule = rules->getItem(cwRule->getLauncherItem());
				clipRule = rules->getItem(cwRule->getClipItem());
				if (launchRule == rule)
				{
					craftOrdnance = true;
					int clipSize = cwRule->getAmmoMax(); // Launcher
					if (clipSize > 0)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
				}
				else if (clipRule == rule)
				{
					craftOrdnance = true;
					int clipSize = clipRule->getClipSize(); // launcher Ammo
					if (clipSize > 1)
						item = item + L"s (" + Text::formatNumber(clipSize) + L")";
				} */
/*			if (rule->getBattleType() == BT_AMMO
				&& rule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
			{
				int clipSize = rule->getClipSize();
				if (clipSize > 1)
					item = item + L" (" + Text::formatNumber(clipSize) + L")";
			} */
/*			if (rule->isFixed() // tank w/ Ordnance.
				&& !rule->getCompatibleAmmo()->empty())
			{
				clipRule = _game->getRuleset()->getItem(rule->getCompatibleAmmo()->front());
				int clipSize = clipRule->getClipSize();
				if (clipSize > 0)
					item = item + L" (" + Text::formatNumber(clipSize) + L")";
			} */

			std::wstring item = tr(*i);

			Uint8 color = Palette::blockOffset(13)+10; // blue

			if ((rule->getBattleType() == BT_AMMO
					|| (rule->getBattleType() == BT_NONE
						&& rule->getClipSize() > 0))
				&& rule->getType() != "STR_ELERIUM_115")
			{
				item.insert(0, L"  ");
				color = Palette::blockOffset(15)+6; // purple
			}

			if (save->isResearched(rule->getType()) == false				// not researched
				&& (save->isResearched(rule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(*i)->getAlien() == true					// or is an alien
					|| rule->getBattleType() == BT_CORPSE						// or is a corpse
					|| rule->getBattleType() == BT_NONE)						// or is not a battlefield item
				&& craftOrdnance == false									// and is not craft ordnance
				&& rule->isResearchExempt() == false)						// and is not research exempt
			{
				// well, that was !NOT! easy.
				color = Palette::blockOffset(13)+5; // yellow
			}

			_lstMatrix->addRow(
							9,
							item.c_str(),
							ss0.str().c_str(),
							ss1.str().c_str(),
							ss2.str().c_str(),
							ss3.str().c_str(),
							ss4.str().c_str(),
							ss5.str().c_str(),
							ss6.str().c_str(),
							ss7.str().c_str());

			_lstMatrix->setRowColor(row, color);

			row++;
		}
	}

	_lstMatrix->scrollTo(save->getCurrentRowMatrix());
/*	if (row > 0 // all taken care of in TextList
		&& save->getCurrentRowMatrix() >= row)
	{
		_lstMatrix->scrollTo(0);
	}
	else if (save->getCurrentRowMatrix() > 0)
		_lstMatrix->scrollTo(save->getCurrentRowMatrix()); */

//	_lstMatrix->draw(); // only needed when list changes while state is active. Eg, on re-inits
}

/**
 * dTor.
 */
StoresMatrixState::~StoresMatrixState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an action
 */
void StoresMatrixState::btnOkClick(Action*)
{
	_game->getSavedGame()->setCurrentRowMatrix(_lstMatrix->getScroll());

	_game->popState();
}

}
