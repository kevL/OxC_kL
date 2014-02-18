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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_PRIMEGRENADESTATE_H
#define OPENXCOM_PRIMEGRENADESTATE_H

#include "BattlescapeGame.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Frame;
class Game;
class InteractiveSurface;
class Surface;
class Text;


/**
 * Window that allows the player to set the timer of an explosive.
 */
class PrimeGrenadeState
	:
		public State
{

private:
	bool _inInventoryView;

	BattleAction* _action;
	BattleItem* _grenadeInInventory;
	Frame* _frame;
	InteractiveSurface* _button[24];
	Surface* _bg;
	Text
		* _number[24],
		* _title;


	public:
		/// Creates the Prime Grenade state.
		PrimeGrenadeState(
				Game* game,
				BattleAction* action,
				bool inInventoryView,
				BattleItem* grenadeInInventory);
		/// Cleans up the Prime Grenade state.
		~PrimeGrenadeState();

		/// Handler for right-clicking anything.
		void handle(Action* action);
		/// Handler for clicking a button.
		void btnClick(Action* action);
};

}

#endif
