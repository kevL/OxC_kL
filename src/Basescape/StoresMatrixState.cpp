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
 * @param game - pointer to the core game
 * @param base - pointer to the accessing base
 */
StoresMatrixState::StoresMatrixState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtItem		= new Text(100, 9, 16, 25);

	_txtBase_0		= new Text(21, 9, 116, 25);
	_txtBase_1		= new Text(21, 9, 138, 25);
	_txtBase_2		= new Text(21, 9, 160, 25);
	_txtBase_3		= new Text(21, 9, 182, 25);
	_txtBase_4		= new Text(21, 9, 204, 25);
	_txtBase_5		= new Text(21, 9, 226, 25);
	_txtBase_6		= new Text(21, 9, 248, 25);
	_txtBase_7		= new Text(21, 9, 270, 25);

	_lstMatrix		= new TextList(285, 136, 16, 36);

	_btnOk			= new TextButton(288, 16, 16, 177);

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


	std::wstring wstr;

	if (_game->getSavedGame()->getBases()->at(0)) // always true, but hey.
	{
		_txtBase_0->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(0)->getName().substr(0, 3);
		_txtBase_0->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(1)) // always true, but hey.
	{
		_txtBase_1->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(1)->getName().substr(0, 3);
		_txtBase_1->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(2))
	{
		_txtBase_2->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(2)->getName().substr(0, 3);
		_txtBase_2->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(3))
	{
		_txtBase_3->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(3)->getName().substr(0, 3);
		_txtBase_3->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(4))
	{
		_txtBase_4->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(4)->getName().substr(0, 3);
		_txtBase_4->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(5))
	{
		_txtBase_5->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(5)->getName().substr(0, 3);
		_txtBase_5->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(6))
	{
		_txtBase_6->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(6)->getName().substr(0, 3);
		_txtBase_6->setText(wstr);
	}

	if (_game->getSavedGame()->getBases()->at(7))
	{
		_txtBase_7->setColor(Palette::blockOffset(13)+10);

		wstr = _game->getSavedGame()->getBases()->at(7)->getName().substr(0, 3);
		_txtBase_7->setText(wstr);
	}

	_lstMatrix->setColor(Palette::blockOffset(13)+10);
	_lstMatrix->setColumns(9, 100, 22, 22, 22, 22, 22, 22, 22, 22);
	_lstMatrix->setSelectable(true);
	_lstMatrix->setBackground(_window);


	bool isAmmo = false;
	int
		row		= 0,
		iter	= 0,
		qty[8]	= { 0, 0, 0, 0, 0, 0, 0, 0 };

	std::wostringstream
		ss0,
		ss1,
		ss2,
		ss3,
		ss4,
		ss5,
		ss6,
		ss7;

	const std::vector<std::string>& items = _game->getRuleset()->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		RuleItem* rule = _game->getRuleset()->getItem(*i);
		isAmmo = false;

		std::wstring item = tr(*i);
		if (rule->getBattleType() == BT_AMMO
			 || (rule->getBattleType() == BT_NONE
				&& rule->getClipSize() > 0))
		{
			isAmmo = true;

			item.insert(0, L"  ");
		}

		ss0.str(L"");
		ss1.str(L"");
		ss2.str(L"");
		ss3.str(L"");
		ss4.str(L"");
		ss5.str(L"");
		ss6.str(L"");
		ss7.str(L"");

		iter = 0;

		for (std::vector<Base*>::const_iterator
				b = _game->getSavedGame()->getBases()->begin();
				b != _game->getSavedGame()->getBases()->end();
				++b)
		{
			qty[iter] = (*b)->getItems()->getItem(*i);

			// Add qty of items in transit to theMatrix.
			std::wstring item = tr(*i);

			for (std::vector<Transfer*>::const_iterator
					t = (*b)->getTransfers()->begin();
					t != (*b)->getTransfers()->end();
					++t)
			{
				std::wstring trItem = (*t)->getName(_game->getLanguage());
				if (trItem == item)
					qty[iter] += (*t)->getQuantity();
			}

			// Add qty of items & vehicles on transport craft to theMatrix.
			for (std::vector<Craft*>::const_iterator
					c = (*b)->getCrafts()->begin();
					c != (*b)->getCrafts()->end();
					++c)
			{
				if ((*c)->getRules()->getSoldiers() > 0) // is transport craft
				{
					for (std::map<std::string, int>::iterator
							e = (*c)->getItems()->getContents()->begin();
							e != (*c)->getItems()->getContents()->end();
							++e)
					{
						std::wstring crItem = tr(e->first);
						if (crItem == item)
							qty[iter] += e->second;
					}
				}

				if ((*c)->getRules()->getVehicles() > 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							v = (*c)->getVehicles()->begin();
							v != (*c)->getVehicles()->end();
							++v)
					{
						std::wstring crVehic = tr((*v)->getRules()->getType());
						if (crVehic == item)
							++qty[iter];

						if ((*v)->getAmmo() != 255)
						{
							RuleItem* tankRule = _game->getRuleset()->getItem((*v)->getRules()->getType());
							RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());

							std::wstring crVehic_a = tr(ammoRule->getType());

							if (crVehic_a == item)
								qty[iter] += (*v)->getAmmo();
						}
					}
				}
			}

			++iter;
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

			if (isAmmo)
				_lstMatrix->setRowColor(row, Palette::blockOffset(15)+6);

			++row;
		}
	}
}

/**
 *
 */
StoresMatrixState::~StoresMatrixState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void StoresMatrixState::btnOkClick(Action*)
{
	_game->popState();
}

}
