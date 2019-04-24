// ---
//
// $Id: time.cpp,v 1.4 2008/07/15 20:33:31 hartwork Exp $
//
// CppTest - A C++ Unit Testing Framework
// Copyright (c) 2003 Niklas Lundell
//
// ---
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// ---

#include "missing.h"
#include "cpptest-time.h"

namespace Test
{
	namespace
	{
		const unsigned int UsecPerSec = 1000000;
		
	} // anonymous namespace
	
	/// Constructs a time object with zeroed time.
	///
	Time::Time()
	:	_sec(0),
		_usec(0)
	{}
	
	/// Constructs a time object.
	///
	/// \param sec  Seconds.
	/// \param usec Micro-seconds.
	///
	Time::Time(unsigned int sec, unsigned int usec)
	:	_sec(sec),
		_usec(usec)
	{}
	
	/// \return Seconds.
	///
	unsigned int
	Time::seconds() const
	{
		return _sec;
	}
	
	/// \return Micro-seconds.
	///
	unsigned int
	Time::microseconds() const
	{
		return _usec;
	}
	
	/// \return The current time.
	///
	Time
	Time::current()
	{
		struct timeval tv;
		gettimeofday(&tv, 0);
		return Time(tv.tv_sec, tv.tv_usec);
	}
	
	/// \relates Time
	///
	/// Computes the time elapsed between two time values.
	///
	/// \param t1 Left-hand time, should be greater than \a t2.
	/// \param t2 Right-hand time, should be less than \a t1.
	///
	/// \return Computed time value.
	///
	Time
	operator-(const Time& t1, const Time& t2)
	{
		if (t2._sec > t1._sec || (t2._sec == t1._sec && t2._usec > t1._usec))
			return Time();
		
		unsigned int sec = t1._sec - t2._sec;
		unsigned int usec;
		
		if (t2._usec > t1._usec)
		{
			--sec;
			usec = UsecPerSec - (t2._usec - t1._usec);
		}
		else
			usec = t1._usec - t2._usec;
		
		return Time(sec, usec);
	}
	
	/// \relates Time
	///
	/// Adds two time values.
	///
	/// \param t1 Left-hand time.
	/// \param t2 Right-hand time.
	///
	/// \return Computed time value.
	///
	Time
	operator+(const Time& t1, const Time& t2)
	{
		unsigned int sec  = t1._sec + t2._sec;
		unsigned int usec = t1._usec + t2._usec;
		
		if (usec > UsecPerSec)
		{
			++sec;
			usec -= UsecPerSec;
		}
		return Time(sec, usec);
	}
	
	/// \relates Time
	///
	/// Outputs a time to an output stream.
	///
	/// \param os Output stream to write to.
	/// \param t  %Time to output.
	///
	/// \return A reference to the given output stream.
	///
    std::ostream&
	operator<<(std::ostream& os, const Time& t)
	{
		int old_fill(os.fill());
		int old_width(os.width());

		os << t.seconds() << '.';
		os.fill('0');
		os.width(6);
		os << t.microseconds();

		os.fill(old_fill);
		os.width(old_width);

		return os;
	}
	
} // namespace Test

