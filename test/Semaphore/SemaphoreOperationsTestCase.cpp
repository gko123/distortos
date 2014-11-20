/**
 * \file
 * \brief SemaphoreOperationsTestCase class implementation
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-11-19
 */

#include "SemaphoreOperationsTestCase.hpp"

#include "waitForNextTick.hpp"

#include "distortos/StaticThread.hpp"
#include "distortos/ThisThread.hpp"
#include "distortos/SoftwareTimer.hpp"
#include "distortos/statistics.hpp"

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

/// single duration used in tests
constexpr auto singleDuration = TickClock::duration{1};

/// long duration used in tests
constexpr auto longDuration = singleDuration * 10;

/*---------------------------------------------------------------------------------------------------------------------+
| local functions
+---------------------------------------------------------------------------------------------------------------------*/

/**
 * \brief Tests Semaphore::post() - it must succeed immediately
 *
 * \param [in] semaphore is a reference to semaphore that will be posted
 *
 * \return true if test succeeded, false otherwise
 */

bool testPost(Semaphore& semaphore)
{
	waitForNextTick();
	const auto start = TickClock::now();
	const auto ret = semaphore.post();
	return ret == 0 && start == TickClock::now() && semaphore.getValue() > 0;
}

/**
 * \brief Tests Semaphore::tryWait() when semaphore is locked - it must fail immediately and return EAGAIN
 *
 * \param [in] semaphore is a reference to semaphore that will be tested
 *
 * \return true if test succeeded, false otherwise
 */

bool testTryWaitWhenLocked(Semaphore& semaphore)
{
	// semaphore is locked, so tryWait() should fail immediately
	waitForNextTick();
	const auto start = TickClock::now();
	const auto ret = semaphore.tryWait();
	return ret == EAGAIN && TickClock::now() == start && semaphore.getValue() == 0;
}

/**
 * \brief Phase 1 of test case.
 *
 * Tests whether all tryWait*() functions properly return some error when dealing with locked semaphore.
 *
 * \return true if test succeeded, false otherwise
 */

bool phase1()
{
	Semaphore semaphore {0};

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		// semaphore is locked, so tryWaitFor() should time-out at expected time
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = semaphore.tryWaitFor(singleDuration);
		const auto realDuration = TickClock::now() - start;
		if (ret != ETIMEDOUT || realDuration != singleDuration + decltype(singleDuration){1} ||
				semaphore.getValue() != 0)
			return false;
	}

	{
		// semaphore is locked, so tryWaitUntil() should time-out at exact expected time
		waitForNextTick();
		const auto requestedTimePoint = TickClock::now() + singleDuration;
		const auto ret = semaphore.tryWaitUntil(requestedTimePoint);
		if (ret != ETIMEDOUT || requestedTimePoint != TickClock::now() || semaphore.getValue() != 0)
			return false;
	}

	return true;
}

/**
 * \brief Phase 2 of test case.
 *
 * Tests whether all tryWait*() functions properly lock unlocked semaphore.
 *
 * \return true if test succeeded, false otherwise
 */

bool phase2()
{
	Semaphore semaphore {1};

	{
		// semaphore is unlocked, so tryWait() must succeed immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = semaphore.tryWait();
		if (ret != 0 || start != TickClock::now() || semaphore.getValue() != 0)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto ret = testPost(semaphore);
		if (ret != true)
			return ret;
	}

	{
		// semaphore is unlocked, so tryWaitFor() should succeed immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = semaphore.tryWaitFor(singleDuration);
		if (ret != 0 || start != TickClock::now() || semaphore.getValue() != 0)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto ret = testPost(semaphore);
		if (ret != true)
			return ret;
	}

	{
		// semaphore is unlocked, so tryWaitUntil() should succeed immediately
		waitForNextTick();
		const auto start = TickClock::now();
		const auto ret = semaphore.tryWaitUntil(start + singleDuration);
		if (ret != 0 || start != TickClock::now() || semaphore.getValue() != 0)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto ret = testPost(semaphore);
		if (ret != true)
			return ret;
	}

	return true;
}

/**
 * \brief Phase 3 of test case.
 *
 * Tests thread-thread signaling scenario. Main (current) thread waits for a locked semaphore to become available. Test
 * thread posts the semaphore at specified time point, main thread is expected to acquire ownership of this semaphore
 * (with wait(), tryWaitFor() and tryWaitUntil()) in the same moment.
 *
 * \return true if test succeeded, false otherwise
 */

bool phase3()
{
	constexpr size_t testThreadStackSize {384};
	// 1 & 2 - waitForNextTick() (main -> idle -> main), 3 - test thread starts (main -> test), 4 - test thread goes to
	// sleep (test -> main), 5 - main thread blocks on semaphore (main -> idle), 6 - test thread wakes (idle -> test),
	// 7 - test thread terminates (test -> main)
	constexpr decltype(statistics::getContextSwitchCount()) expectedContextSwitchCount {7};

	Semaphore semaphore {0};

	const auto sleepUntilFunctor = [&semaphore](const TickClock::time_point timePoint)
			{
				ThisThread::sleepUntil(timePoint);
				semaphore.post();
			};

	{
		const auto contextSwitchCount = statistics::getContextSwitchCount();

		const auto wakeUpTimePoint = TickClock::now() + longDuration;
		auto thread = makeStaticThread<testThreadStackSize>(UINT8_MAX, sleepUntilFunctor, wakeUpTimePoint);

		waitForNextTick();
		thread.start();
		ThisThread::yield();

		// semaphore is currently locked, but wait() should succeed at expected time
		const auto ret = semaphore.wait();
		const auto wokenUpTimePoint = TickClock::now();
		thread.join();
		if (ret != 0 || wakeUpTimePoint != wokenUpTimePoint || semaphore.getValue() != 0 ||
				statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto contextSwitchCount = statistics::getContextSwitchCount();

		const auto wakeUpTimePoint = TickClock::now() + longDuration;
		auto thread = makeStaticThread<testThreadStackSize>(UINT8_MAX, sleepUntilFunctor, wakeUpTimePoint);

		waitForNextTick();
		thread.start();
		ThisThread::yield();

		// semaphore is currently locked, but tryWaitFor() should succeed at expected time
		const auto ret = semaphore.tryWaitFor(wakeUpTimePoint - TickClock::now() + longDuration);
		const auto wokenUpTimePoint = TickClock::now();
		thread.join();
		if (ret != 0 || wakeUpTimePoint != wokenUpTimePoint || semaphore.getValue() != 0 ||
				statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto contextSwitchCount = statistics::getContextSwitchCount();

		const auto wakeUpTimePoint = TickClock::now() + longDuration;
		auto thread = makeStaticThread<testThreadStackSize>(UINT8_MAX, sleepUntilFunctor, wakeUpTimePoint);

		waitForNextTick();
		thread.start();
		ThisThread::yield();

		// semaphore is locked, but tryWaitUntil() should succeed at expected time
		const auto ret = semaphore.tryWaitUntil(wakeUpTimePoint + longDuration);
		const auto wokenUpTimePoint = TickClock::now();
		thread.join();
		if (ret != 0 || wakeUpTimePoint != wokenUpTimePoint || semaphore.getValue() != 0 ||
				statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	return true;
}

/**
 * \brief Phase 4 of test case.
 *
 * Tests interrupt-thread signaling scenario. Main (current) thread waits for a locked semaphore to become available.
 * Software timer is used to posts the semaphore at specified time point from interrupt context, main thread is expected
 * to acquire ownership of this semaphore (with wait(), tryWaitFor() and tryWaitUntil()) in the same moment.
 *
 * \return true if test succeeded, false otherwise
 */

bool phase4()
{
	// 1 & 2 - waitForNextTick() (main -> idle -> main), 3 - main thread blocks on semaphore (main -> idle), 4 - main
	// thread is unblocked by interrupt (idle -> main)
	constexpr decltype(statistics::getContextSwitchCount()) expectedContextSwitchCount {4};

	Semaphore semaphore {0};
	auto softwareTimer = makeSoftwareTimer(&Semaphore::post, std::ref(semaphore));

	{
		const auto contextSwitchCount = statistics::getContextSwitchCount();

		const auto wakeUpTimePoint = TickClock::now() + longDuration;

		waitForNextTick();
		softwareTimer.start(wakeUpTimePoint);

		// semaphore is currently locked, but wait() should succeed at expected time
		const auto ret = semaphore.wait();
		const auto wokenUpTimePoint = TickClock::now();
		if (ret != 0 || wakeUpTimePoint != wokenUpTimePoint || semaphore.getValue() != 0 ||
				statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto contextSwitchCount = statistics::getContextSwitchCount();

		const auto wakeUpTimePoint = TickClock::now() + longDuration;

		waitForNextTick();
		softwareTimer.start(wakeUpTimePoint);

		// semaphore is currently locked, but tryWaitFor() should succeed at expected time
		const auto ret = semaphore.tryWaitFor(wakeUpTimePoint - TickClock::now() + longDuration);
		const auto wokenUpTimePoint = TickClock::now();
		if (ret != 0 || wakeUpTimePoint != wokenUpTimePoint || semaphore.getValue() != 0 ||
				statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	{
		const auto contextSwitchCount = statistics::getContextSwitchCount();

		const auto wakeUpTimePoint = TickClock::now() + longDuration;

		waitForNextTick();
		softwareTimer.start(wakeUpTimePoint);

		// semaphore is locked, but tryWaitUntil() should succeed at expected time
		const auto ret = semaphore.tryWaitUntil(wakeUpTimePoint + longDuration);
		const auto wokenUpTimePoint = TickClock::now();
		if (ret != 0 || wakeUpTimePoint != wokenUpTimePoint || semaphore.getValue() != 0 ||
				statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
			return false;
	}

	{
		const auto ret = testTryWaitWhenLocked(semaphore);
		if (ret != true)
			return ret;
	}

	return true;
}

}	// namespace

/*---------------------------------------------------------------------------------------------------------------------+
| private functions
+---------------------------------------------------------------------------------------------------------------------*/

bool SemaphoreOperationsTestCase::run_() const
{
	for (const auto& function : {phase1, phase2, phase3, phase4})
	{
		const auto ret = function();
		if (ret != true)
			return ret;
	}

	return true;
}

}	// namespace test

}	// namespace distortos