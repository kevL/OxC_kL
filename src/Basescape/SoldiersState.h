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

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Soldiers screen that lets the player manage all the soldiers in a base.
 */
class SoldiersState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtBaseLabel,
		* _txtCraft,
//		* _txtLost,
		* _txtName,
		* _txtRank,
//		* _txtRecruited,
		* _txtSoldiers,
		* _txtTitle;
	TextButton
		* _btnArmor,
		* _btnMemorial,
		* _btnOk,
		* _btnPsiTrain;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates the Soldiers state.
		SoldiersState(
				Game* game,
				Base* base);
		/// Cleans up the Soldiers state.
		~SoldiersState();

		/// Updates the soldier names.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the PsiTraining button.
		void btnPsiTrainingClick(Action* action);
		/// Handler for clicking the Armor button.
		void btnArmorClick_Soldier(Action* action);
		/// Handler for clicking the Memorial button.
		void btnMemorialClick(Action* action);
		/// Handler for clicking the Soldiers list.
		void lstSoldiersClick(Action* action);

		/// Handler for clicking the Soldiers reordering button.
		void lstItemsLeftArrowClick_Soldier(Action* action);
		/// Handler for clicking the Soldiers reordering button.
		void lstItemsRightArrowClick_Soldier(Action* action);
};

}

#endif
