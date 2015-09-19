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

#ifndef OPENXCOM_PSITRAININGSTATE_H
#define OPENXCOM_PSITRAININGSTATE_H

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
class PsiTrainingState
	:
		public State
{

private:
	int _labSpace;
	size_t _sel;

	Base* _base;
	Text
		* _txtBaseLabel,
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
		explicit PsiTrainingState(Base* base);
		/// Cleans up the Psi Training state.
		~PsiTrainingState();

		/// Refreshes the soldier-list.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Soldiers list.
		void lstSoldiersPress(Action* action);

		/// Handler for clicking the Soldiers reordering button.
		void lstLeftArrowClick(Action* action);
		/// Handler for clicking the Soldiers reordering button.
		void lstRightArrowClick(Action* action);
};

}

#endif
