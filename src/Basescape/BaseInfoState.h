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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BASEINFOSTATE_H
#define OPENXCOM_BASEINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class Base;
class BasescapeState;
class MiniBaseView;
class Surface;
class Text;
class TextButton;
class TextEdit;


/**
 * Base Info screen that shows all the
 * stats of a base from the Basescape.
 */
class BaseInfoState
	:
		public State
{

private:
	bool _containmentLimit;

	Base* _base;
	BasescapeState* _state;
	MiniBaseView* _mini;
	Surface* _bg;
	Text
		* _txtPersonnel,
		* _txtSpace;
	TextButton
		* _btnMonthlyCosts,
		* _btnOk,
		* _btnStores,
		* _btnTransfers;
	TextEdit* _edtBase;

	Text
		* _txtSoldiers,
		* _txtEngineers,
		* _txtScientists,
		* _numSoldiers,
		* _numEngineers,
		* _numScientists;
	Bar
		* _barSoldiers,
		* _barEngineers,
		* _barScientists;

	Text
		* _txtQuarters,
		* _txtStores,
		* _txtLaboratories,
		* _txtWorkshops,
		* _txtContainment,
		* _txtHangars,
		* _numQuarters,
		* _numStores,
		* _numLaboratories,
		* _numWorkshops,
		* _numContainment,
		* _numHangars;
	Bar
		* _barQuarters,
		* _barStores,
		* _barLaboratories,
		* _barWorkshops,
		* _barContainment,
		* _barHangars;

	Text
		* _txtDefense,
		* _txtShortRange,
		* _txtLongRange,
		* _numDefense,
		* _numShortRange,
		* _numLongRange;
	Bar
		* _barDefense,
		* _barShortRange,
		* _barLongRange;


	public:
		/// Creates the Base Info state.
		BaseInfoState(
				Game* game,
				Base* base,
				BasescapeState* state);
		/// Cleans up the Base Info state.
		~BaseInfoState();

		/// Updates the base stats.
		void init();

		/// Handler for pressing a key on the Name edit.
		void edtBaseKeyPress(Action* action);
		/// Handler for clicking the mini base view.
		void miniClick(Action* action);
		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Transfers button.
		void btnTransfersClick(Action* action);
		/// Handler for clicking the Stores button.
		void btnStoresClick(Action* action);
		/// Handler for clicking the Monthly Costs button.
		void btnMonthlyCostsClick(Action* action);
};

}

#endif
