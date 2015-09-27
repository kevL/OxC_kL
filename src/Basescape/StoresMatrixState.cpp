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

#include "StoresMatrixState.h"

//#include <sstream>

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

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
StoresMatrixState::StoresMatrixState(const Base* base)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17,  10, 8);
	_txtBaseLabel	= new Text( 80,  9, 224, 8);

	_txtItem		= new Text(100, 9, 16, 25);
	_txtFreeStore	= new Text(100, 9, 16, 33);

	_txtBase_0		= new Text(23, 17, 116, 25);
	_txtBase_1		= new Text(23, 17, 139, 25);
	_txtBase_2		= new Text(23, 17, 162, 25);
	_txtBase_3		= new Text(23, 17, 185, 25);
	_txtBase_4		= new Text(23, 17, 208, 25);
	_txtBase_5		= new Text(23, 17, 231, 25);
	_txtBase_6		= new Text(23, 17, 254, 25);
	_txtBase_7		= new Text(23, 17, 277, 25);

	_lstMatrix		= new TextList(285, 129, 16, 44);

	_btnOk			= new TextButton(268, 16, 26, 177);

	setPalette("PAL_BASESCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtItem);
	add(_txtFreeStore);
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


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));
	_window->setColor(BLUE);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setColor(BLUE);
	_btnOk->onMouseClick((ActionHandler)& StoresMatrixState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresMatrixState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresMatrixState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresMatrixState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setText(tr("STR_MATRIX"));
	_txtTitle->setColor(BLUE);
	_txtTitle->setBig();

	_txtBaseLabel->setText(base->getName(_game->getLanguage()));
	_txtBaseLabel->setColor(BLUE);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	_txtItem->setText(tr("STR_ITEM"));
	_txtItem->setColor(WHITE);

	_txtFreeStore->setText(tr("STR_FREESTORE"));
	_txtFreeStore->setColor(WHITE);

	_lstMatrix->setColumns(9, 100,23,23,23,23,23,23,23,23);
	_lstMatrix->setColor(BLUE);
	_lstMatrix->setBackground(_window);
	_lstMatrix->setSelectable();


	const SavedGame* const gameSave = _game->getSavedGame();
	std::wstring wst;
	int
		qty[MTX_BASES] = {0,0,0,0,0,0,0,0},
		freeSpace;

	for (size_t
		i = 0;
		i != MTX_BASES;
		++i)
	{
		base = gameSave->getBases()->at(i);
		if (base != NULL)
		{
			qty[i] = base->getTotalSoldiers();

			wst = base->getName().substr(0,4);
			freeSpace = static_cast<int>(static_cast<double>(base->getAvailableStores()) - base->getUsedStores() + 0.5);

			std::wostringstream woststr;
			woststr	<< wst
					<< L"\n"
					<< freeSpace;
//					<< std::fixed
//					<< std::setprecision(1)
//					<< static_cast<double>(base->getAvailableStores()) - base->getUsedStores() + 0.05;

			switch (i)
			{
				case 0: _txtBase_0->setText(woststr.str().c_str()); break;
				case 1: _txtBase_1->setText(woststr.str().c_str()); break;
				case 2: _txtBase_2->setText(woststr.str().c_str()); break;
				case 3: _txtBase_3->setText(woststr.str().c_str()); break;
				case 4: _txtBase_4->setText(woststr.str().c_str()); break;
				case 5: _txtBase_5->setText(woststr.str().c_str()); break;
				case 6: _txtBase_6->setText(woststr.str().c_str()); break;
				case 7: _txtBase_7->setText(woststr.str().c_str());
			}
		}
		else
			break;
	}

	_txtBase_0->setColor(WHITE);
	_txtBase_1->setColor(WHITE);
	_txtBase_2->setColor(WHITE);
	_txtBase_3->setColor(WHITE);
	_txtBase_4->setColor(WHITE);
	_txtBase_5->setColor(WHITE);
	_txtBase_6->setColor(WHITE);
	_txtBase_7->setColor(WHITE);

	std::wostringstream
		woststr0,
		woststr1,
		woststr2,
		woststr3,
		woststr4,
		woststr5,
		woststr6,
		woststr7;
	size_t row = 0;

	if (qty[0] + qty[1] + qty[2] + qty[3] + qty[4] + qty[5] + qty[6] + qty[7] > 0)
	{
		if (qty[0] > 0) woststr0 << qty[0];
		if (qty[1] > 0) woststr1 << qty[1];
		if (qty[2] > 0) woststr2 << qty[2];
		if (qty[3] > 0) woststr3 << qty[3];
		if (qty[4] > 0) woststr4 << qty[4];
		if (qty[5] > 0) woststr5 << qty[5];
		if (qty[6] > 0) woststr6 << qty[6];
		if (qty[7] > 0) woststr7 << qty[7];

		_lstMatrix->addRow(
						9,
						tr("STR_SOLDIERS").c_str(),
						woststr0.str().c_str(),
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						woststr3.str().c_str(),
						woststr4.str().c_str(),
						woststr5.str().c_str(),
						woststr6.str().c_str(),
						woststr7.str().c_str());

		_lstMatrix->setRowColor(row++, BLUE);
	}

	const Ruleset* const rules = _game->getRuleset();
	const RuleItem* itRule;
//		* launchRule,
//		* clipRule;
	const RuleCraftWeapon* cwRule;
	size_t baseId = 0;
	std::string stTest;
	Uint8 color;

	const std::vector<std::string>& itemList = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = itemList.begin();
			i != itemList.end();
			++i)
	{
		itRule = rules->getItem(*i);
		stTest = itRule->getType();

		woststr0.str(L"");
		woststr1.str(L"");
		woststr2.str(L"");
		woststr3.str(L"");
		woststr4.str(L"");
		woststr5.str(L"");
		woststr6.str(L"");
		woststr7.str(L"");

		baseId = 0;

		for (std::vector<Base*>::const_iterator
				j = gameSave->getBases()->begin();
				j != gameSave->getBases()->end();
				++j,
					++baseId)
		{
			qty[baseId] = (*j)->getStorageItems()->getItemQty(*i);

			for (std::vector<Transfer*>::const_iterator // Add qty of items in transit to theMatrix.
					k = (*j)->getTransfers()->begin();
					k != (*j)->getTransfers()->end();
					++k)
			{
				if ((*k)->getTransferItems() == stTest)
					qty[baseId] += (*k)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // Add qty of items & vehicles on transport craft to da-Matrix.
					k = (*j)->getCrafts()->begin();
					k != (*j)->getCrafts()->end();
					++k)
			{
				if ((*k)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::iterator
							l = (*k)->getCraftItems()->getContents()->begin();
							l != (*k)->getCraftItems()->getContents()->end();
							++l)
					{
						if (l->first == stTest)
							qty[baseId] += l->second;
					}
				}

				if ((*k)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							l = (*k)->getVehicles()->begin();
							l != (*k)->getVehicles()->end();
							++l)
					{
						if ((*l)->getRules()->getType() == stTest)
							++qty[baseId];

						if ((*l)->getAmmo() != 255)
						{
							const RuleItem* const ammoRule = rules->getItem(
																rules->getItem((*l)->getRules()->getType())
															->getCompatibleAmmo()->front());

							if (ammoRule->getType() == stTest)
								qty[baseId] += (*l)->getAmmo();
						}
					}
				}
			}
		}

		if (qty[0] + qty[1] + qty[2] + qty[3] + qty[4] + qty[5] + qty[6] + qty[7] > 0)
		{
			if (qty[0] > 0) woststr0 << qty[0];
			if (qty[1] > 0) woststr1 << qty[1];
			if (qty[2] > 0) woststr2 << qty[2];
			if (qty[3] > 0) woststr3 << qty[3];
			if (qty[4] > 0) woststr4 << qty[4];
			if (qty[5] > 0) woststr5 << qty[5];
			if (qty[6] > 0) woststr6 << qty[6];
			if (qty[7] > 0) woststr7 << qty[7];


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

				if (itRule == rules->getItem(cwRule->getLauncherItem())
					|| itRule == rules->getItem(cwRule->getClipItem()))
				{
					craftOrdnance = true;
				}
			}

/*				launchRule = rules->getItem(cwRule->getLauncherItem());
				clipRule = rules->getItem(cwRule->getClipItem());
				if (launchRule == itRule)
				{
					craftOrdnance = true;
					int clipSize = cwRule->getAmmoMax(); // Launcher
					if (clipSize > 0)
						item = item + L" (" + Text::formatNumber(clipSize) + L")";
				}
				else if (clipRule == itRule)
				{
					craftOrdnance = true;
					int clipSize = clipRule->getClipSize(); // launcher Ammo
					if (clipSize > 1)
						item = item + L"s (" + Text::formatNumber(clipSize) + L")";
				} */
/*			if (itRule->getBattleType() == BT_AMMO
				&& itRule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
			{
				int clipSize = itRule->getClipSize();
				if (clipSize > 1)
					item = item + L" (" + Text::formatNumber(clipSize) + L")";
			} */
/*			if (itRule->isFixed() // tank w/ Ordnance.
				&& !itRule->getCompatibleAmmo()->empty())
			{
				clipRule = _game->getRuleset()->getItem(itRule->getCompatibleAmmo()->front());
				int clipSize = clipRule->getClipSize();
				if (clipSize > 0)
					item = item + L" (" + Text::formatNumber(clipSize) + L")";
			} */

			std::wstring item = tr(*i);
			color = BLUE;

			if ((itRule->getBattleType() == BT_AMMO
					|| (itRule->getBattleType() == BT_NONE
						&& itRule->getClipSize() > 0))
				&& itRule->getType() != "STR_ELERIUM_115")
			{
				item.insert(0, L"  ");
				color = PURPLE;
			}

			if (gameSave->isResearched(itRule->getType()) == false				// not researched or is research exempt
				&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use but not been researched
					|| rules->getItem(*i)->isAlien() == true						// or is an alien
					|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
					|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
				&& craftOrdnance == false)										// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = YELLOW;
			}

			_lstMatrix->addRow(
							9,
							item.c_str(),
							woststr0.str().c_str(),
							woststr1.str().c_str(),
							woststr2.str().c_str(),
							woststr3.str().c_str(),
							woststr4.str().c_str(),
							woststr5.str().c_str(),
							woststr6.str().c_str(),
							woststr7.str().c_str());

			_lstMatrix->setRowColor(row++, color);
		}
	}

	_lstMatrix->scrollTo(gameSave->getCurrentRowMatrix());
//	_lstMatrix->draw(); // only needed if list changes while state is active. Eg, on re-inits
}

/**
 * dTor.
 */
StoresMatrixState::~StoresMatrixState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void StoresMatrixState::btnOkClick(Action*)
{
	_game->getSavedGame()->setCurrentRowMatrix(_lstMatrix->getScroll());
	_game->popState();
}

}
