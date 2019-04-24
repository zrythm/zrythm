// ---
//
// $Id: cpptest-assert.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

/** \file */

#ifndef CPPTEST_ASSERT_H
#define CPPTEST_ASSERT_H

/// Unconditional failure.
///
/// Used in conjunction with Test::Suite.
///
/// \param msg Provided message.
///
/// \par Example:
/// \code
/// void MySuite::test()
/// {
/// 	// ...
///
/// 	switch (flag)
/// 	{
/// 		// handling valid cases ...
///
/// 		default:
/// 			TEST_FAIL("This should not happen")
/// 	}
/// }
/// \endcode
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_FAIL(msg) \
	{																\
		assertment(::Test::Source(__FILE__, __LINE__, (msg) != 0 ? #msg : "")); \
		if (!continue_after_failure()) return;						\
	}

/// Verify an expression and issues an assertment if it fails.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
///
/// \see TEST_ASSERT_MSG(expr, msg)
///
/// \par Example:
/// \code
/// void MySuite::test()
/// {
/// 	int i;
///
/// 	// ...
///
/// 	TEST_ASSERT(i == 13)
/// }
/// \endcode
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_ASSERT(expr)  											\
	{  																\
		if (!(expr))  												\
		{ 															\
			assertment(::Test::Source(__FILE__, __LINE__, #expr));	\
			if (!continue_after_failure()) return;					\
		} 															\
	}

/// Verify an expression and issues an assertment if it fails.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
/// \param msg  User message.
///
/// \see TEST_ASSERT(expr)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_ASSERT_MSG(expr, msg)									\
	{  																\
		if (!(expr))  												\
		{ 															\
			assertment(::Test::Source(__FILE__, __LINE__, msg));	\
			if (!continue_after_failure()) return;					\
		} 															\
	}

/// Verify that two expressions are equal up to a constant, issues an
/// assertment if it fails.
///
/// Used in conjunction with Test::Suite.
///
/// \param a     First expression to test.
/// \param b     Second expression to test.
/// \param delta Constant.
///
/// \see TEST_ASSERT_DELTA_MSG(a, b, delta, msg)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_ASSERT_DELTA(a, b, delta)								\
	{																\
		if (((b) < (a) - (delta)) || ((b) > (a) + (delta)))			\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, 			\
					   "delta(" #a ", " #b ", " #delta ")" ));		\
			if (!continue_after_failure()) return;					\
		}															\
	}

/// Verify that two expressions are equal up to a constant, issues an
/// assertment if it fails.
///
/// Used in conjunction with Test::Suite.
///
/// \param a     First expression to test.
/// \param b     Second expression to test.
/// \param delta Constant.
/// \param msg   User message.	
///
/// \see TEST_ASSERT_DELTA(a, b, delta)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_ASSERT_DELTA_MSG(a, b, delta, msg)						\
	{																\
		if (((b) < (a) - (delta)) || ((b) > (a) + (delta)))			\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, msg));	\
			if (!continue_after_failure()) return;					\
		}															\
	}
	
/// Verify an expression and expects an exception in return.
/// An assertment is issued if the exception is not thrown.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
/// \param x    Expected exception.
///
/// \see TEST_THROWS_MSG(expr, x, msg)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_THROWS(expr, x)										\
	{																\
		bool __expected = false;									\
		try { expr; } 												\
		catch (x)			{ __expected = true; }					\
		catch (...)			{}										\
		if (!__expected)											\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, #expr));	\
			if (!continue_after_failure()) return;					\
		}															\
	}

/// Verify an expression and expects an exception in return.
/// An assertment is issued if the exception is not thrown.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
/// \param x    Expected exception.
/// \param msg  User message.
///
/// \see TEST_THROWS(expr, x)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_THROWS_MSG(expr, x, msg)								\
	{																\
		bool __expected = false;									\
		try { expr; } 												\
		catch (x)			{ __expected = true; }					\
		catch (...)			{}										\
		if (!__expected)											\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, msg));	\
			if (!continue_after_failure()) return;					\
		}															\
	}

/// Verify an expression and expects any exception in return.
/// An assertment is issued if no exception is thrown.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
///
/// \see TEST_THROWS_ANYTHING_MSG(expr, msg)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_THROWS_ANYTHING(expr)									\
	{																\
		bool __expected = false;									\
		try { expr; } 												\
		catch (...) { __expected = true; }							\
		if (!__expected)											\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, #expr));	\
			if (!continue_after_failure()) return;					\
		}															\
	}

/// Verify an expression and expects any exception in return.
/// An assertment is issued if no exception is thrown.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
/// \param msg  User message.
///
/// \see TEST_THROWS_ANYTHING(expr)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_THROWS_ANYTHING_MSG(expr, msg)							\
	{																\
		bool __expected = false;									\
		try { expr; }		 										\
		catch (...) { __expected = true; }							\
		if (!__expected)											\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, msg));	\
			if (!continue_after_failure()) return;					\
		}															\
	}
	
/// Verify an expression and expects no exception in return.
/// An assertment is issued if any exception is thrown.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
///
/// \see TEST_THROWS_NOTHING_MSG(expr, msg)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_THROWS_NOTHING(expr)									\
	{																\
		bool __expected = true;										\
		try { expr; } 												\
		catch (...) { __expected = false; }							\
		if (!__expected)											\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, #expr));	\
			if (!continue_after_failure()) return;					\
		}															\
	}

/// Verify an expression and expects no exception in return.
/// An assertment is issued if any exception is thrown.
///
/// Used in conjunction with Test::Suite.
///
/// \param expr Expression to test.
/// \param msg  User message.
///
/// \see TEST_THROWS_NOTHING(expr)
///
/// For a description of all asserts, see \ref asserts.
///
#define TEST_THROWS_NOTHING_MSG(expr, msg)							\
	{																\
		bool __expected = true;										\
		try { expr; } 												\
		catch (...) { __expected = false; }							\
		if (!__expected)											\
		{															\
			assertment(::Test::Source(__FILE__, __LINE__, msg));	\
			if (!continue_after_failure()) return;					\
		}															\
	}

/// \page asserts Available asserts
///
/// Normally, assert macros issues an assert if the given expression, if any,
/// fails (as defined by the macro). Assertments include information about the
/// current test suite, test function, source file, source file line, and a
/// message. The message is normally the offending expression, however, for
/// macros ending in _MSG it is possibly to supply a user defined message.
///
/// <b>General asserts</b>
/// - TEST_FAIL(msg)
///
/// <b>Comparision asserts</b>
/// - TEST_ASSERT(expr)
/// - TEST_ASSERT_MSG(expr, msg)
/// - TEST_ASSERT_DELTA(a, b, delta)
/// - TEST_ASSERT_DELTA_MSG(a, b, delta, msg)
///
/// <b>Exception asserts</b>
/// - TEST_THROWS(expr, x)
/// - TEST_THROWS_MSG(expr, x, msg)
/// - TEST_THROWS_ANYTHING(expr)
/// - TEST_THROWS_ANYTHING_MSG(expr, msg)
/// - TEST_THROWS_NOTHING(expr)
/// - TEST_THROWS_NOTHING_MSG(expr, msg)
///

#endif // #ifndef CPPTEST_ASSERT_H

