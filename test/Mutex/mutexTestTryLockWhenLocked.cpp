/**
 * \file
 * \brief mutexTestTryLockWhenLocked() implementation
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-09-28
 */

#include "mutexTestTryLockWhenLocked.hpp"

#include "waitForNextTick.hpp"

#include "distortos/scheduler/StaticThread.hpp"
#include "distortos/scheduler/Mutex.hpp"

#include <cerrno>

namespace distortos
{

namespace test
{

namespace
{

/*---------------------------------------------------------------------------------------------------------------------+
| local constants
+---------------------------------------------------------------------------------------------------------------------*/

/// size of stack for test thread, bytes
constexpr size_t testThreadStackSize {256};

}	// namespace

/*---------------------------------------------------------------------------------------------------------------------+
| global functions
+---------------------------------------------------------------------------------------------------------------------*/

bool mutexTestTryLockWhenLocked(distortos::scheduler::Mutex& mutex)
{
	using distortos::scheduler::makeStaticThread;

	bool sharedRet {};
	auto tryLockThreadObject = makeStaticThread<testThreadStackSize>(UINT8_MAX,
			[&mutex, &sharedRet]()
			{
				using distortos::scheduler::TickClock;

				const auto start = TickClock::now();
				const auto ret = mutex.tryLock();
				sharedRet = ret == EBUSY && start == TickClock::now();
			});
	waitForNextTick();
	tryLockThreadObject.start();
	tryLockThreadObject.join();

	return sharedRet;
}

}	// namespace test

}	// namespace distortos