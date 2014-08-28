/**
 * \file
 * \brief threadTestCases object definition
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-08-28
 */

#include "threadTestCases.hpp"

#include "ThreadPriorityTestCase.hpp"
#include "ThreadFunctionTypesTestCase.hpp"

namespace distortos
{

namespace test
{

namespace
{

/*---------------------------------------------------------------------------------------------------------------------+
| local objects
+---------------------------------------------------------------------------------------------------------------------*/

/// ThreadPriorityTestCase instance
const ThreadPriorityTestCase priorityTestCase;

/// ThreadFunctionTypesTestCase instance
const ThreadFunctionTypesTestCase functionTypesTestCase;

/// array with references to TestCase objects related to threads
const TestCaseRange::value_type threadTestCases_[]
{
		priorityTestCase,
		functionTypesTestCase,
};

}	// namespace

/*---------------------------------------------------------------------------------------------------------------------+
| global objects
+---------------------------------------------------------------------------------------------------------------------*/

const TestCaseRange threadTestCases {threadTestCases_};

}	// namespace test

}	// namespace distortos