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

#ifndef OPENXCOM_ALLOCATEPSITRAININGSTATE_H
#define OPENXCOM_ALLOCATEPSITRAININGSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Soldier;
class Window;


/**
 * Screen that allocates soldiers to psionic training.
 */
class AllocatePsiTrainingState
	:
		public State
{

private:
	int _labSpace;
	unsigned int _sel;

	Base* _base;
	Text
		* _txtCraft,
		* _txtName,
		* _txtPsiSkill,
		* _txtPsiStrength,
		* _txtRemaining,
		* _txtTitle,
		* _txtTraining;
	TextButton* _btnOk;
	TextList* _lstSoldiers;
	Window* _window;

	std::vector<Soldier*> _soldiers;


	public:
		/// Creates the Psi Training state.
		AllocatePsiTrainingState(
				Game* game,
				Base* base);
		/// Cleans up the Psi Training state.
		~AllocatePsiTrainingState();

		/// Updates the palette.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
//kL		void btnBase1Click();
//kL		void lstSoldiersPress(Action* action);
//kL		void lstSoldiersRelease(Action* action);
		void lstSoldiersClick(Action* action);

		// kL_begin: re-order soldiers, taken from CraftSoldiersState.
		/// Handler for clicking the Soldiers reordering button.
		void lstItemsLeftArrowClick_Psi(Action* action);
		/// Handler for clicking the Soldiers reordering button.
		void lstItemsRightArrowClick_Psi(Action* action);
		// kL_end.
};

}

#endif
