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

#include "BaseDefenseState.h"

//#include <sstream>

#include "BaseDestroyedState.h"
#include "GeoscapeState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Base Defense screen.
 * @param base	- pointer to a Base being attacked
 * @param ufo	- pointer to the attacking Ufo
 * @param state	- pointer to GeoscapeState
 */
BaseDefenseState::BaseDefenseState(
		Base* base,
		Ufo* ufo,
		GeoscapeState* state)
	:
		_base(base),
		_ufo(ufo),
		_state(state),
		_action(BDA_NONE),
		_row(-1),
		_passes(0),
		_attacks(0),
		_thinkcycles(0),
		_explosionCount(0)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 16, 6);
	_txtInit		= new Text(300, 10, 16, 24);
	_lstDefenses	= new TextList(300, 129, 16, 40);
	_btnOk			= new TextButton(120, 16, 100, 170);

	setPalette(
			"PAL_BASESCAPE",
			_game->getRuleset()->getInterface("baseDefense")->getElement("palette")->color); //2

	add(_window, "window", "baseDefense");
	add(_txtTitle, "text", "baseDefense");
	add(_txtInit, "text", "baseDefense");
	add(_lstDefenses, "text", "baseDefense"); // "list"
	add(_btnOk, "button", "baseDefense");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(15)+6);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

//	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseDefenseState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDefenseState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDefenseState::btnOkClick,
					Options::keyCancel);
	_btnOk->setVisible(false);

//	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_BASE_UNDER_ATTACK").arg(_base->getName()));
	_txtInit->setVisible(false);

//	_txtInit->setColor(Palette::blockOffset(13)+10);
	_txtInit->setText(tr("STR_BASE_DEFENSES_INITIATED"));

//	_lstDefenses->setColor(Palette::blockOffset(13)+10);
	_lstDefenses->setColumns(3, 134, 70, 50);

	_gravShields = _base->getGravShields();
	_defenses = _base->getDefenses()->size();

	_timer = new Timer(250);
	_timer->onTimer((StateHandler)& BaseDefenseState::nextStep);
	_timer->start();
}

/**
 * dTor.
 */
BaseDefenseState::~BaseDefenseState()
{
	delete _timer;
}

/**
 * Advance the state of the State.
 */
void BaseDefenseState::think()
{
	_timer->think(this, NULL);
}

/**
 * Advance the state of the State by doing this.
 */
void BaseDefenseState::nextStep()
{
	if (_thinkcycles == -1)
		return;

	++_thinkcycles;

	if (_thinkcycles == 1)
	{
		_txtInit->setVisible();
		return;
	}

	if (_thinkcycles > 1)
	{
		switch (_action)
		{
			case BDA_DESTROY:
				if (_explosionCount == 0)
				{
					_lstDefenses->addRow(
									3,
									tr("STR_UFO_DESTROYED").c_str(),
									L" ",
									L" ");

					++_row;
					if (_row > 14)
						_lstDefenses->scrollDown(true);
				}

				_game->getResourcePack()->getSound(
												"GEO.CAT",
												ResourcePack::UFO_EXPLODE)
											->play();

				if (++_explosionCount == 3)
					_action = BDA_END;
			return;

			case BDA_END:
				_thinkcycles = -1;
				_btnOk->setVisible();
			return;

			default:
			break;
		}

		if (_attacks == _defenses)
		{
			if (_passes == _gravShields)
			{
				_action = BDA_END;
				return;
			}
			else if (_passes < _gravShields)
			{
				_lstDefenses->addRow(
								3,
								tr("STR_GRAV_SHIELD_REPELS_UFO").c_str(),
								L" ",
								L" ");

				if (_row > 14)
					_lstDefenses->scrollDown(true);

				++_row;
				++_passes;
				_attacks = 0;

				return;
			}
		}

		const BaseFacility* const def = _base->getDefenses()->at(_attacks);

		switch (_action)
		{
			case BDA_NONE:
				_action = BDA_FIRE;

				_lstDefenses->addRow(
								3,
								tr(def->getRules()->getType()).c_str(),
								L" ",
								L" ");
				++_row;
				if (_row > 14)
					_lstDefenses->scrollDown(true);

			return;

			case BDA_FIRE:
				_lstDefenses->setCellText(
										_row,
										1,
										tr("STR_FIRING").c_str());

				_game->getResourcePack()->getSound(
												"GEO.CAT",
												def->getRules()->getFireSound())
											->play();

				_timer->setInterval(333);
				_action = BDA_RESOLVE;

			return;

			case BDA_RESOLVE:
				if (RNG::percent(def->getRules()->getHitRatio()) == true)
				{
					_game->getResourcePack()->getSound(
													"GEO.CAT",
													def->getRules()->getHitSound())
												->play();

					int power = def->getRules()->getDefenseValue();
					power = RNG::generate( // kL: vary power between 75% and 133% ( stock is 50..150% )
										power * 3 / 4,
										power * 4 / 3);
					_ufo->setDamage(_ufo->getDamage() + power);

					_lstDefenses->setCellText(
										_row,
										2,
										tr("STR_HIT").c_str());
				}
				else
					_lstDefenses->setCellText(
										_row,
										2,
										tr("STR_MISSED").c_str());

				if (_ufo->getStatus() == Ufo::DESTROYED)
					_action = BDA_DESTROY;
				else
					_action = BDA_NONE;

				++_attacks;
				_timer->setInterval(250);

			return;

			default:
			break;
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BaseDefenseState::btnOkClick(Action*)
{
	_timer->stop();
	_game->popState();

	if (_ufo->getStatus() != Ufo::DESTROYED)
	{
		_base->setDefenseResult(_ufo->getDamagePercent());
		_state->handleBaseDefense(
								_base,
								_ufo);
	}
	else
		_base->cleanupDefenses(true);
}

}
