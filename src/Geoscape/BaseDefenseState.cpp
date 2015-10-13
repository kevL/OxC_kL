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

#include "GeoscapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
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
		_action(BD_NONE),
		_row(0),
		_passes(0),
		_attacks(0),
		_thinkCycles(0),
		_explosionCount(0),
		_stLen_destroyed(0),
		_stLen_initiate(0),
		_stLen_repulsed(0)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(288, 17, 16, 10);
	_txtInit		= new Text(320, 9, 0, 30);
	_lstDefenses	= new TextList(277, 121, 24, 43);
	_txtDestroyed	= new Text(320, 9, 0, 167);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("baseDefense");

	add(_window,		"window",	"baseDefense");
	add(_txtTitle,		"text",		"baseDefense");
	add(_txtInit,		"text",		"baseDefense");
	add(_lstDefenses,	"text",		"baseDefense");
	add(_txtDestroyed,	"text",		"baseDefense");
	add(_btnOk,			"button",	"baseDefense");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseDefenseState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDefenseState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDefenseState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseDefenseState::btnOkClick,
					Options::keyCancel);
	_btnOk->setVisible(false);

	_txtTitle->setText(tr("STR_BASE_UNDER_ATTACK").arg(_base->getName()));
	_txtTitle->setBig();

	_txtInit->setAlign(ALIGN_CENTER);

	_lstDefenses->setColumns(3, 130,80,67);

	_txtDestroyed->setAlign(ALIGN_CENTER);

	_timer = new Timer(TI_FAST);
	_timer->onTimer((StateHandler)& BaseDefenseState::nextStep);
	_timer->start();


	_destroyed = tr("STR_UFO_DESTROYED");
	_initiate = tr("STR_BASE_DEFENSES_INITIATED");
	_repulsed = tr("STR_GRAV_SHIELD_REPELS_UFO");

	_gravShields = _base->getGravShields();
	_defenses = _base->getDefenses()->size();

	_game->getResourcePack()->fadeMusic(_game, 863);
//	_game->getCursor()->setVisible(false);
}

/**
 * dTor.
 */
BaseDefenseState::~BaseDefenseState()
{
	delete _timer;
}

/**
 * Advances the state of the State.
 */
void BaseDefenseState::think()
{
	_timer->think(this, NULL);
}

/**
 * Advances the state of the State by doing this.
 */
void BaseDefenseState::nextStep()
{
	if (_thinkCycles != -1)
	{
		if (_stLen_initiate <= _initiate.size())
		{
			_txtInit->setText(_initiate.substr(0, _stLen_initiate++));
			return;
		}

		++_thinkCycles;
		if (_thinkCycles > 3)
		{
			switch (_action)
			{
				case BD_DESTROY:
					if (_explosionCount == 0)
					{
						if (_stLen_destroyed == 0)
						{
							_lstDefenses->addRow(3, L" ",L" ",L" ");
							++_row;
							_timer->setInterval(TI_FAST);
						}

						if (_row > 14)
							_lstDefenses->scrollDown(true);

						if (_stLen_destroyed <= _destroyed.size())
						{
							_txtDestroyed->setText(_destroyed.substr(0, _stLen_destroyed++));
							return;
						}

						_timer->setInterval(TI_MEDIUM);
					}

					_game->getResourcePack()->playSoundFX(ResourcePack::UFO_EXPLODE, true);

					if (++_explosionCount == 3)
						_action = BD_END;
				return;

				case BD_END:
					_thinkCycles = -1;
//					_game->getCursor()->setVisible();
					_btnOk->setVisible();
				return;
			} // end switch()

			if (_attacks == _defenses)
			{
				if (_passes == _gravShields)
				{
					_action = BD_END;
					return;
				}
				else if (_passes < _gravShields)
				{
					if (_stLen_repulsed == 0)
					{
						_lstDefenses->addRow(3, L" ",L" ",L" ");
						_lstDefenses->addRow(3, L" ",L" ",L" "); // <- gravShield repels UFO

						_row += 2;
						if (_row > 14)
							_lstDefenses->scrollDown(true);

						_timer->setInterval(TI_FAST);
					}

					if (_stLen_repulsed <= _repulsed.size())
					{
						_lstDefenses->setCellText(
											_row - 1, 0,
											_repulsed.substr(0, _stLen_repulsed++));

						if (_stLen_repulsed > _repulsed.size())
						{
							_lstDefenses->addRow(3, L" ",L" ",L" ");
							++_row;
						}
						return;
					}

					++_passes;
					_attacks = 0;
					_timer->setInterval(TI_MEDIUM);
					return;
				}
			}

			const BaseFacility* const defFac = _base->getDefenses()->at(_attacks);

			switch (_action)
			{
				case BD_NONE:
					_action = BD_FIRE;

					_lstDefenses->addRow(
									3,
									tr(defFac->getRules()->getType()).c_str(),
									L" ",L" ");
					++_row;
					if (_row > 14) _lstDefenses->scrollDown(true);
				return;

				case BD_FIRE:
					_lstDefenses->setCellText(
											_row - 1, 1,
											tr("STR_FIRING").c_str());
//					_lstDefenses->setCellColor(_row - 1, 1, 160, /* slate */ true);

					_game->getResourcePack()->playSoundFX(
													defFac->getRules()->getFireSound(),
													true);
					_action = BD_RESOLVE;
					_timer->setInterval(TI_SLOW);
				return;

				case BD_RESOLVE:
					if (RNG::percent(defFac->getRules()->getHitRatio()) == true)
					{
						_game->getResourcePack()->playSoundFX(defFac->getRules()->getHitSound());

						int power = defFac->getRules()->getDefenseValue();
						power = RNG::generate( // kL: vary power between 75% and 133% ( stock is 50..150% )
											power * 3 / 4,
											power * 4 / 3);
						_ufo->setUfoDamage(_ufo->getUfoDamage() + power);

						_lstDefenses->setCellText(
											_row - 1, 2,
											tr("STR_HIT").c_str());
//						_lstDefenses->setCellColor(_row - 1, 2, 32, /* green */ true);
					}
					else
					{
						_lstDefenses->setCellText(
											_row - 1, 2,
											tr("STR_MISSED").c_str());
//						_lstDefenses->setCellColor(_row - 1, 2, 144, /* brown */ true);
					}

					if (_ufo->getUfoStatus() == Ufo::DESTROYED)
						_action = BD_DESTROY;
					else
						_action = BD_NONE;

					++_attacks;
					_timer->setInterval(TI_MEDIUM);
			} // end switch()
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

	if (_ufo->getUfoStatus() != Ufo::DESTROYED) // TODO: if Status_Crashed, set up Battleship crash site near the xCom Base.
	{
		// need to handle if Defenses reduce UFO-crew to zilch but don't destroy the UFO.
		// probably handled by GeoscapeState::handleBaseDefense()-> new BriefingState
		_base->setDefenseResult(_ufo->getUfoDamagePct());
		_state->handleBaseDefense(_base, _ufo);
	}
	else
	{
		_base->cleanupDefenses(true);
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
}

}
