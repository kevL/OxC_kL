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

#ifndef OPENXCOM_PSITRAININGSTATE_H
#define OPENXCOM_PSITRAININGSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class Window;


/**
 * Report screen shown monthly to display
 * changes in the player's performance and funding.
 */
class PsiTrainingState
	:
		public State
{

private:
	Window* _window;
	Text* _txtTitle;
	TextButton* _btnOk;

	std::vector<Base*> _bases;


	public:
		/// Creates the Psi Training state.
		PsiTrainingState(Game* game);
		/// Cleans up the Psi Training state.
		~PsiTrainingState();

		/// Updates the palette.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		void btnBaseXClick(Action* action);
};

}

#endif
