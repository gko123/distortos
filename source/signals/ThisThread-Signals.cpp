/**
 * \file
 * \brief ThisThread::Signals namespace implementation
 *
 * \author Copyright (C) 2015 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2015-02-21
 */

#include "distortos/ThisThread-Signals.hpp"

#include "distortos/ThisThread.hpp"
#include "distortos/ThreadBase.hpp"

#include "distortos/scheduler/getScheduler.hpp"
#include "distortos/scheduler/Scheduler.hpp"

#include "distortos/architecture/InterruptMaskingLock.hpp"

#include <cerrno>

namespace distortos
{

namespace ThisThread
{

namespace Signals
{

namespace
{

/*---------------------------------------------------------------------------------------------------------------------+
| local types
+---------------------------------------------------------------------------------------------------------------------*/

/// SignalsWaitUnblockFunctor is a functor executed when unblocking a thread that is waiting for signal
class SignalsWaitUnblockFunctor : public scheduler::ThreadControlBlock::UnblockFunctor
{
public:

	/**
	 * \brief SignalsWaitUnblockFunctor's constructor
	 *
	 * \param [in] pendingSignalSet is a reference to SignalSet that will be used to return saved pending signal set
	 */

	constexpr explicit SignalsWaitUnblockFunctor(SignalSet& pendingSignalSet) :
			pendingSignalSet_(pendingSignalSet)
	{

	}

	/**
	 * \brief SignalsWaitUnblockFunctor's function call operator
	 *
	 * Saves pending signal set of unblocked thread and clears pointer to set of signals that were "waited for".
	 *
	 * \param [in] threadControlBlock is a reference to ThreadControlBlock that is being unblocked
	 */

	void operator()(scheduler::ThreadControlBlock& threadControlBlock) const override
	{
		pendingSignalSet_ = threadControlBlock.getPendingSignalSet();
		threadControlBlock.setWaitingSignalSet(nullptr);
	}

private:

	/// reference to SignalSet that will be used to return saved pending signal set
	SignalSet& pendingSignalSet_;
};

/*---------------------------------------------------------------------------------------------------------------------+
| local functions
+---------------------------------------------------------------------------------------------------------------------*/

/**
 * \brief Implementation of distortos::ThisThread::Signals::wait(), distortos::ThisThread::Signals::tryWait() and
 * distortos::ThisThread::Signals::tryWaitUntil().
 *
 * \param [in] signalSet is a reference to set of signals that will be waited for
 * \param [in] nonBlocking selects whether this function operates in blocking mode (false) or non-blocking mode (true)
 * \param [in] timePoint is a pointer to time point at which the wait for signals will be terminated, used only if
 * blocking mode is selected, nullptr to block without timeout
 *
 * \return pair with return code (0 on success, error code otherwise) and signal number of signal that was accepted;
 * error codes:
 * - EAGAIN - no signal specified by \a signalSet was pending and non-blocking mode was selected;
 * - ETIMEDOUT - no signal specified by \a signalSet was generated before specified \a timePoint;
 */

std::pair<int, uint8_t> waitImplementation(const SignalSet& signalSet, const bool nonBlocking,
		const TickClock::time_point* const timePoint)
{
	architecture::InterruptMaskingLock interruptMaskingLock;

	auto& scheduler = scheduler::getScheduler();
	auto& currentThreadControlBlock = scheduler.getCurrentThreadControlBlock();

	const auto bitset = signalSet.getBitset();
	auto pendingSignalSet = currentThreadControlBlock.getPendingSignalSet();
	auto pendingBitset = pendingSignalSet.getBitset();
	auto intersection = bitset & pendingBitset;

	if (intersection.none() == true)	// none of desired signals is pending, so current thread must be blocked
	{
		if (nonBlocking == true)
			return {EAGAIN, {}};

		scheduler::ThreadControlBlockList waitingList {scheduler.getThreadControlBlockListAllocator(),
				scheduler::ThreadControlBlock::State::WaitingForSignal};

		currentThreadControlBlock.setWaitingSignalSet(&signalSet);
		const SignalsWaitUnblockFunctor signalsWaitUnblockFunctor {pendingSignalSet};
		const auto ret = timePoint == nullptr ? scheduler.block(waitingList, &signalsWaitUnblockFunctor) :
				scheduler.blockUntil(waitingList, *timePoint, &signalsWaitUnblockFunctor);
		if (ret != 0)
			return {ret, {}};

		pendingBitset = pendingSignalSet.getBitset();
		intersection = bitset & pendingBitset;
	}

	const auto intersectionValue = intersection.to_ulong();
	// GCC builtin - "find first set" - https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
	const auto signalNumber = __builtin_ffsl(intersectionValue) - 1;
	const auto ret = currentThreadControlBlock.acceptPendingSignal(signalNumber);
	return {ret, signalNumber};
}

}	// namespace

/*---------------------------------------------------------------------------------------------------------------------+
| global functions
+---------------------------------------------------------------------------------------------------------------------*/

int generateSignal(const uint8_t signalNumber)
{
	return ThisThread::get().generateSignal(signalNumber);
}

SignalSet getPendingSignalSet()
{
	return ThisThread::get().getPendingSignalSet();
}

std::pair<int, uint8_t> tryWait(const SignalSet& signalSet)
{
	return waitImplementation(signalSet, true, nullptr);	// non-blocking mode
}

std::pair<int, uint8_t> tryWaitFor(const SignalSet& signalSet, const TickClock::duration duration)
{
	return tryWaitUntil(signalSet, TickClock::now() + duration + TickClock::duration{1});
}

std::pair<int, uint8_t> tryWaitUntil(const SignalSet& signalSet, const TickClock::time_point timePoint)
{
	return waitImplementation(signalSet, false, &timePoint);	// blocking mode, with timeout
}

std::pair<int, uint8_t> wait(const SignalSet& signalSet)
{
	return waitImplementation(signalSet, false, nullptr);	// blocking mode, no timeout
}

}	// namespace Signals

}	// namespace ThisThread

}	// namespace distortos
