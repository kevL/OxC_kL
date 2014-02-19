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

#ifndef OPENXCOM_RNG_H
#define OPENXCOM_RNG_H

#include <algorithm>


namespace OpenXcom
{

/**
 * Random Number Generator used throughout the game
 * for all your random needs. Uses a Mersenne Twister
 * pseudorandom number generator based on the code sample
 * from Game Coding Complete 4.
 * @note http://www.mcshaffry.com/GameCode/
 */
namespace RNG
{
	/// Initializes the generator.
	void init();
	/// Initializes the generator.
	void init(unsigned int seed);

	/// Gets the seed in use.
	unsigned int getSeed();

	/// Generates a random integer number, inclusive.
	int generate(
			int min,
			int max);
	/// Generates a random floating-point number.
	double generate(
			double min,
			double max);

	/// Get normally distributed value.
	double boxMuller(
			double mean = 0,
			double standardDeviation = 1);

	/// Generates a percentage chance.
	bool percent(int value);

	/// Generates a random integer number, exclusive.
	int generateEx(int max);

	/// Shuffles a list randomly.
	/**
	 * Randomly changes the orders of the elements in a list.
	 * @param list The container to randomize.
	 */
	template <typename T> // Ss. Take 2!
	void shuffle(T& list)
	{
		std::random_shuffle(
						list.begin(),
						list.end(),
						generateEx);
	}
}

}

#endif
