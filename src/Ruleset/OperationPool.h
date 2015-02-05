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

#ifndef OPENXCOM_OPERATIONPOOL_H
#define OPENXCOM_OPERATIONPOOL_H

//#include <string>
//#include <vector>


namespace OpenXcom
{

/**
 * Pool of operation names to generate random mission operation titles.
 * Each pool contains a set of first and last words.
 * The first word defines the operation's adjective and are randomly associated
 * with a last word that defines the operation's noun.
 */
class OperationPool
{

private:
	std::vector<std::wstring>
		_operaFirst,
		_operaLast;


	public:
		/// Creates a blank pool.
		OperationPool();
		/// Cleans up the pool.
		~OperationPool();

		/// Loads the pool from YAML.
		void load(const std::string& filename);

		/// Generates a new operation title from the pool.
		std::wstring genOperation() const;
};

}

#endif
