/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_SOLDIERSSTATE_H
#define OPENXCOM_SOLDIERSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class TextButton;
class Window;
class Text;
class TextList;
class Base;

/**
 * Soldiers screen that lets the player
 * manage all the soldiers in a base.
 */
class SoldiersState
	:
	public State
{
	private:
		Base* _base;
//		size_t _craft;		//kL

		TextButton* _btnOk, * _btnPsiTrain, * _btnArmor;	// kL: add _btnArmor
		Window* _window;
		Text* _txtTitle, * _txtName, * _txtRank, * _txtCraft;
		TextList* _lstSoldiers;

	public:
		/// Creates the Soldiers state.
		SoldiersState(Game* game, Base* base);
//		SoldiersState(Game* game, Base* base, Craft* craft);		// kL
		/// Cleans up the Soldiers state.
		~SoldiersState();
		/// Updates the soldier names.

		void init();
		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Soldiers list.
		void lstSoldiersClick(Action* action);
		/// Handler for clicking the PsiTraining button.
		void btnPsiTrainingClick(Action* action);
		/// Handler for clicking the Armor button.		// kL
		void btnArmorClick_noCraft(Action* action);		// kL

		// kL_begin: re-order soldiers, taken from CraftSoldiersState.
		/// Handler for clicking the Soldiers reordering button.
		void lstItemsLeftArrowClick_noCraft(Action* action);
		/// Handler for clicking the Soldiers reordering button.
		void lstItemsRightArrowClick_noCraft(Action* action);
		// kL_end.
};

}

#endif
