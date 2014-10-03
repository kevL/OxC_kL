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
		* _btnEquip,
		* _btnMemorial,
		* _btnOk,
		* _btnPsi;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates the Soldiers state.
		SoldiersState(Base* base);
		/// Cleans up the Soldiers state.
		~SoldiersState();

		/// Updates the soldier names.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the PsiTraining button.
		void btnPsiTrainingClick(Action* action);
		/// kL. Handler for clicking the Armor button.
		void btnArmorClick(Action* action); // kL
		/// Handler for clicking the Memorial button.
		void btnMemorialClick(Action* action);
		/// Handler for clicking the Soldiers list.
		void lstSoldiersPress(Action* action);

		/// kL. Handler for clicking the Soldiers reordering button.
		void lstLeftArrowClick(Action* action); // kL
		/// kL. Handler for clicking the Soldiers reordering button.
		void lstRightArrowClick(Action* action); // kL

		/// kL. Handler for clicking the Inventory button.
		void btnEquipClick(Action* action); // kL
};

}

#endif
