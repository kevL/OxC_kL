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

#include "RNG.h"

/*
#include <math.h>
#include <time.h>
#include <stdlib.h>

#ifndef UINT64_MAX
	#define UINT64_MAX 0xffffffffffffffffULL
#endif */

// Or:

//#include <cmath>
//#include <ctime>


namespace OpenXcom
{

namespace RNG
{

/*	Written in 2014 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/*	This is a good generator if you're short on memory, but otherwise
	we rather suggest to use a xorshift128+ (for maximum speed) or
	xorshift1024* (for speed and very long period) generator. */


//uint64_t x = std::time(0); // The state must be seeded with a nonzero value.
uint64_t x;


uint64_t next()
{
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c

	//Log(LOG_INFO) << "rng:next(x) " << x;
	//Log(LOG_INFO) << "rng:next(ret) " << (x * 2685821657736338717ULL);
	return x * 2685821657736338717ULL;
}

/**
 * Returns the current seed in use by the generator.
 * @return, the SAVE seed
 */
uint64_t getSeed()
{
	//Log(LOG_INFO) << "rng:getSeed() " << x;
	return x;
}

/**
 * Changes the current seed in use by the generator.
 * @param seed - the LOAD seed (default 0 reset)
 */
void setSeed(uint64_t seed)
{
	//Log(LOG_INFO) << "rng:setSeed()";
	if (seed == 0)
	{
		x = std::time(NULL);
		//Log(LOG_INFO) << ". reseed = " << x;
	}
	else
	{
		x = seed;
		//Log(LOG_INFO) << ". seed = " << x;
	}
}

/**
 * Generates a random integer number within a certain range.
 * @param valMin - minimum number, inclusive
 * @param valMax - maximum number, inclusive
 * @return, generated number
 */
int generate(
		int valMin,
		int valMax)
{
	//Log(LOG_INFO) << "rng:generate(int)";
	if (valMin == valMax)
		return valMin;

	if (valMin > valMax)
		std::swap(valMin, valMax);

	return static_cast<int>(next() % (valMax - valMin + 1)) + valMin;
}

/**
 * Generates a random decimal number within a certain range.
 * @param valMin - minimum number
 * @param valMax - maximum number
 * @return, generated number
 */
double generate(
		double valMin,
		double valMax)
{
	//Log(LOG_INFO) << "rng:generate(double)";
	double delta (valMax - valMin);
	if (AreSame(delta, 0.))
		return valMin;

	delta = (static_cast<double>(std::numeric_limits<uint64_t>::max()) / delta);
	if (AreSame(delta, 0.))
		return valMin;

	return (static_cast<double>(next()) / delta) + valMin;
}

/**
 * Generates a random integer number within a certain range.
 * @note Distinct from "generate" in that it doesn't touch the seed.
 * @param min - minimum number inclusive
 * @param max - maximum number inclusive
 * @return, generated number
 */
int seedless(
		int valMin,
		int valMax)
{
	//Log(LOG_INFO) << "rng:generate(int)";
	if (valMin == valMax)
		return valMin;

	if (valMin > valMax)
		std::swap(valMin, valMax);

	return (std::rand() % (valMax - valMin + 1) + valMin);
}

/*
ftp://ftp.taygeta.com/pub/c/boxmuller.c

Implements the Polar form of the Box-Muller Transformation
(c) Copyright 1994, Everett F. Carter Jr.
	Permission is granted by the author to use this software for
	any application provided this copyright notice is preserved.
*/
/**
 * Gaussian generator.
 * @param mean		- offset of the center value (default 0.0)
 * @param deviation	- standard deviation (default 1.0)
 * @return, normally distributed value
 */
double boxMuller(
		double mean,
		double deviation)
{
	//Log(LOG_INFO) << "rng:boxMuller()";
	static bool use_last;

	static double y2;
	double y1;

	if (use_last) // use value from previous call
	{
		use_last = false;
		y1 = y2;
	}
	else
	{
		use_last = true;
		double
			x1,x2,
			w;
		do
		{
			x1 = (generate(0.,1.) * 2.) - 1.;
			x2 = (generate(0.,1.) * 2.) - 1.;
			w  = (x1 * x1) + (x2 * x2);
		}
		while (w >= 1.);

		w = std::sqrt((-2. * std::log(w)) / w);
		y1 = x1 * w;
		y2 = x2 * w;
	}

	return (mean + (y1 * deviation));
}

/**
 * Generates a random percent chance of an event occurring and returns the result.
 * @param valPct - value percentage (0-100%)
 * @return, true if the chance succeeded
 */
bool percent(int valPct)
{
	//Log(LOG_INFO) << "rng:percent()";
	if (valPct < 1)
		return false;

	if (valPct > 99)
		return true;

	return (generate(0,99) < valPct);
}

/**
 * Generates a random positive integer up to a number.
 * @param valMax - maximum number exclusive
 * @return, generated number
 */
int generateExclusive(int valMax)
{
	//Log(LOG_INFO) << "rng:generateExclusive()";
	if (valMax < 2)
		return 0;

	return static_cast<int>(next() % valMax);
}

/**
 * Picks an entry from a vector.
 * @note Don't try shoving an empty vector into here.
 * @param val - size of the vector
 * @return, picked id
 */
size_t pick(size_t valSize)
{
	return static_cast<size_t>(generate(0, static_cast<int>(valSize) - 1));
}

/**
 * Picks an entry from a vector using the seedless generator.
 * @note Don't try shoving an empty vector into here.
 * @param val			- size of the vector
 * @param useSeedless	- true/false to use the seedless (external) generator
 * @return, picked id
 */
size_t pick(
		size_t valSize,
		bool)
{
	return static_cast<size_t>(seedless(0, static_cast<int>(valSize) - 1));
}

}

}
