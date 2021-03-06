/**
 * \file
 * \brief SignalsWaitOperationsTestCase class implementation
 *
 * \author Copyright (C) 2015 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2015-02-21
 */

#include "SignalsWaitOperationsTestCase.hpp"

#include "waitForNextTick.hpp"

#include "distortos/ThisThread-Signals.hpp"
#include "distortos/ThisThread.hpp"
#include "distortos/ThreadBase.hpp"
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

/// expected number of context switches in waitForNextTick(): main -> idle -> main
constexpr decltype(statistics::getContextSwitchCount()) waitForNextTickContextSwitchCount {2};

/// expected number of context switches in phase1 block involving timed-out tryWaitFor() or tryWaitUntil() (excluding
/// waitForNextTick()): 1 - main thread blocks waiting for signals (main -> idle), 2 - main thread wakes up
/// (idle -> main)
constexpr decltype(statistics::getContextSwitchCount()) phase1TimedOutWaitContextSwitchCount {2};

/// expected number of context switches in phase2 block involving software timer (excluding waitForNextTick()): 1 - main
/// thread blocks waiting for signals (main -> idle), 2 - main thread is unblocked by interrupt (idle -> main)
constexpr decltype(statistics::getContextSwitchCount()) phase2SoftwareTimerContextSwitchCount {2};

/*---------------------------------------------------------------------------------------------------------------------+
| local functions
+---------------------------------------------------------------------------------------------------------------------*/

/**
 * \brief Tests whether no signals are pending for current thread.
 *
 * \return true if test succeeded, false otherwise
 */

bool testSelfNoSignalsPending()
{
	const auto pendingSignalSet = ThisThread::Signals::getPendingSignalSet();
	const auto pendingBitset = pendingSignalSet.getBitset();
	return pendingBitset.none();
}

/**
 * \brief Tests generation of signal for current thread.
 *
 * Initially no signals may be pending for current thread. After call to ThisThread::Signals::generateSignal() exactly
 * one signal - \a signalNumber - must be pending.
 *
 * \param [in] signalNumber is the signal number that will be generated
 *
 * \return true if test succeeded, false otherwise
 */

bool testSelfGenerateSignal(const uint8_t signalNumber)
{
	{
		const auto ret = testSelfNoSignalsPending();	// initially no signals may be pending
		if (ret != true)
			return ret;
	}

	{
		const auto ret = ThisThread::Signals::generateSignal(signalNumber);
		if (ret != 0)
			return false;
	}

	auto pendingSignalSet = ThisThread::Signals::getPendingSignalSet();
	const auto testResult = pendingSignalSet.test(signalNumber);
	if (testResult.first != 0 || testResult.second != true)	// selected signal number must be pending
		return false;
	// no other signal may be pending
	const auto ret = pendingSignalSet.remove(signalNumber);
	if (ret != 0)
		return false;
	const auto pendingBitset = pendingSignalSet.getBitset();
	return pendingBitset.none();
}

/**
 * \brief Phase 1 of test case.
 *
 * Tests whether all ThisThread::Signals::tryWait*() functions properly ignore pending signal that is not included in
 * SignalSet and accept it otherwise.
 *
 * \return true if test succeeded, false otherwise
 */

bool phase1()
{
	const SignalSet fullSignalSet {SignalSet::full};

	{
		constexpr uint8_t testSignalNumber {19};

		{
			const auto ret = testSelfGenerateSignal(testSignalNumber);
			if (ret != true)
				return ret;
		}

		{
			SignalSet excludingSignalSet {SignalSet::full};
			const auto ret = excludingSignalSet.remove(testSignalNumber);
			if (ret != 0)
				return false;

			waitForNextTick();

			// no signals are pending, so ThisThread::Signals::tryWait() should fail immediately
			const auto start = TickClock::now();
			const auto tryWaitResult = ThisThread::Signals::tryWait(excludingSignalSet);
			if (tryWaitResult.first != EAGAIN || TickClock::now() != start)
				return false;
		}

		{
			// one signal is pending, so ThisThread::Signals::tryWait() must succeed immediately
			waitForNextTick();
			const auto start = TickClock::now();
			const auto tryWaitResult = ThisThread::Signals::tryWait(fullSignalSet);
			if (tryWaitResult.first != 0 || tryWaitResult.second != testSignalNumber || start != TickClock::now())
				return false;
		}
	}

	{
		constexpr uint8_t testSignalNumber {8};

		{
			const auto ret = testSelfGenerateSignal(testSignalNumber);
			if (ret != true)
				return ret;
		}

		{
			SignalSet excludingSignalSet {SignalSet::full};
			const auto ret = excludingSignalSet.remove(testSignalNumber);
			if (ret != 0)
				return false;

			waitForNextTick();

			const auto contextSwitchCount = statistics::getContextSwitchCount();

			// no signals are pending, so ThisThread::Signals::tryWaitFor() should time-out at expected time
			const auto start = TickClock::now();
			const auto tryWaitResult = ThisThread::Signals::tryWaitFor(excludingSignalSet, singleDuration);
			const auto realDuration = TickClock::now() - start;
			if (tryWaitResult.first != ETIMEDOUT || realDuration != singleDuration + decltype(singleDuration){1} ||
					statistics::getContextSwitchCount() - contextSwitchCount != phase1TimedOutWaitContextSwitchCount)
				return false;
		}

		{
			// one signal is pending, so ThisThread::Signals::tryWaitFor() must succeed immediately
			waitForNextTick();
			const auto start = TickClock::now();
			const auto tryWaitResult = ThisThread::Signals::tryWaitFor(fullSignalSet, singleDuration);
			if (tryWaitResult.first != 0 || tryWaitResult.second != testSignalNumber || start != TickClock::now())
				return false;
		}
	}

	{
		constexpr uint8_t testSignalNumber {22};

		{
			const auto ret = testSelfGenerateSignal(testSignalNumber);
			if (ret != true)
				return ret;
		}

		{
			SignalSet excludingSignalSet {SignalSet::full};

			{
				const auto ret = excludingSignalSet.remove(testSignalNumber);
				if (ret != 0)
					return false;
			}

			waitForNextTick();

			const auto contextSwitchCount = statistics::getContextSwitchCount();

			// no signals are pending, so ThisThread::Signals::tryWaitUntil() should time-out at exact expected time
			const auto requestedTimePoint = TickClock::now() + singleDuration;
			const auto tryWaitResult = ThisThread::Signals::tryWaitUntil(excludingSignalSet, requestedTimePoint);
			if (tryWaitResult.first != ETIMEDOUT || requestedTimePoint != TickClock::now() ||
					statistics::getContextSwitchCount() - contextSwitchCount != phase1TimedOutWaitContextSwitchCount)
				return false;
		}

		{
			// one signal is pending, so ThisThread::Signals::tryWaitUntil() must succeed immediately
			waitForNextTick();
			const auto start = TickClock::now();
			const auto tryWaitResult = ThisThread::Signals::tryWaitUntil(fullSignalSet, start + singleDuration);
			if (tryWaitResult.first != 0 || tryWaitResult.second != testSignalNumber || start != TickClock::now())
				return false;
		}
	}

	return testSelfNoSignalsPending();
}

/**
 * \brief Phase 2 of test case.
 *
 * Tests interrupt -> thread communication scenario. Main (current) thread waits for signals. Software timer generates
 * signal for main thread at specified time point from interrupt context, main thread is expected to accept this signal
 * (with ThisThread::Signals::wait(), ThisThread::Signals::tryWaitFor() and ThisThread::Signals::tryWaitUntil()) in the
 * same moment.
 *
 * \return true if test succeeded, false otherwise
 */

bool phase2()
{
	const SignalSet fullSignalSet {SignalSet::full};
	auto& mainThread = ThisThread::get();
	uint8_t sharedSignalNumber {};
	auto softwareTimer = makeSoftwareTimer(
			[&mainThread, &sharedSignalNumber]()
			{
				mainThread.generateSignal(sharedSignalNumber);
			});

	{
		waitForNextTick();

		const auto contextSwitchCount = statistics::getContextSwitchCount();
		const auto wakeUpTimePoint = TickClock::now() + longDuration;
		sharedSignalNumber = 3;
		softwareTimer.start(wakeUpTimePoint);

		// no signals are currently pending, but ThisThread::Signals::wait() should succeed at expected time
		const auto waitResult = ThisThread::Signals::wait(fullSignalSet);
		const auto wokenUpTimePoint = TickClock::now();
		if (waitResult.first != 0 || waitResult.second != sharedSignalNumber || wakeUpTimePoint != wokenUpTimePoint ||
				statistics::getContextSwitchCount() - contextSwitchCount != phase2SoftwareTimerContextSwitchCount)
			return false;
	}

	{
		const auto ret = testSelfNoSignalsPending();	// no signals may be pending
		if (ret != true)
			return ret;
	}

	{
		waitForNextTick();

		const auto contextSwitchCount = statistics::getContextSwitchCount();
		const auto wakeUpTimePoint = TickClock::now() + longDuration;
		sharedSignalNumber = 26;
		softwareTimer.start(wakeUpTimePoint);

		// no signals are currently pending, but ThisThread::Signals::tryWaitFor() should succeed at expected time
		const auto tryWaitResult = ThisThread::Signals::tryWaitFor(fullSignalSet, wakeUpTimePoint - TickClock::now() +
				longDuration);
		const auto wokenUpTimePoint = TickClock::now();
		if (tryWaitResult.first != 0 || tryWaitResult.second != sharedSignalNumber ||
				wakeUpTimePoint != wokenUpTimePoint ||
				statistics::getContextSwitchCount() - contextSwitchCount != phase2SoftwareTimerContextSwitchCount)
			return false;
	}

	{
		const auto ret = testSelfNoSignalsPending();	// no signals may be pending
		if (ret != true)
			return ret;
	}

	{
		waitForNextTick();

		const auto contextSwitchCount = statistics::getContextSwitchCount();
		const auto wakeUpTimePoint = TickClock::now() + longDuration;
		sharedSignalNumber = 5;
		softwareTimer.start(wakeUpTimePoint);

		// no signals are currently pending, but ThisThread::Signals::tryWaitUntil() should succeed at expected time
		const auto tryWaitResult = ThisThread::Signals::tryWaitUntil(fullSignalSet, wakeUpTimePoint + longDuration);
		const auto wokenUpTimePoint = TickClock::now();
		if (tryWaitResult.first != 0 || tryWaitResult.second != sharedSignalNumber ||
				wakeUpTimePoint != wokenUpTimePoint ||
				statistics::getContextSwitchCount() - contextSwitchCount != phase2SoftwareTimerContextSwitchCount)
			return false;
	}

	{
		const auto ret = testSelfNoSignalsPending();	// no signals may be pending
		if (ret != true)
			return ret;
	}

	return true;
}

}	// namespace

/*---------------------------------------------------------------------------------------------------------------------+
| private functions
+---------------------------------------------------------------------------------------------------------------------*/

bool SignalsWaitOperationsTestCase::run_() const
{
	constexpr auto phase1ExpectedContextSwitchCount = 6 * waitForNextTickContextSwitchCount +
			2 * phase1TimedOutWaitContextSwitchCount;
	constexpr auto phase2ExpectedContextSwitchCount = 3 * waitForNextTickContextSwitchCount +
			3 * phase2SoftwareTimerContextSwitchCount;
	constexpr auto expectedContextSwitchCount = phase1ExpectedContextSwitchCount + phase2ExpectedContextSwitchCount;

	const auto contextSwitchCount = statistics::getContextSwitchCount();

	for (const auto& function : {phase1, phase2})
	{
		{
			const auto ret = testSelfNoSignalsPending();	// initially no signals may be pending
			if (ret != true)
				return ret;
		}

		const auto ret = function();
		if (ret != true)
			return ret;
	}

	const auto ret = testSelfNoSignalsPending();	// after the test no signals may be pending
	if (ret != true)
		return ret;

	if (statistics::getContextSwitchCount() - contextSwitchCount != expectedContextSwitchCount)
		return false;

	return true;
}

}	// namespace test

}	// namespace distortos
