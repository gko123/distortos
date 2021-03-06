/**
 * \file
 * \brief ThreadBase class implementation
 *
 * \author Copyright (C) 2014-2015 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2015-02-16
 */

#include "distortos/ThreadBase.hpp"

#include "distortos/scheduler/getScheduler.hpp"
#include "distortos/scheduler/Scheduler.hpp"

#include <cerrno>

namespace distortos
{

/*---------------------------------------------------------------------------------------------------------------------+
| public functions
+---------------------------------------------------------------------------------------------------------------------*/

ThreadBase::ThreadBase(void* const buffer, const size_t size, const uint8_t priority,
		const SchedulingPolicy schedulingPolicy) :
		ThreadBase{{buffer, size, threadRunner, *this}, priority, schedulingPolicy}
{

}

ThreadBase::ThreadBase(architecture::Stack&& stack, const uint8_t priority, const SchedulingPolicy schedulingPolicy) :
		threadControlBlock_{std::move(stack), priority, schedulingPolicy, *this},
		joinSemaphore_{0}
{

}

int ThreadBase::join()
{
	if (&threadControlBlock_ == &scheduler::getScheduler().getCurrentThreadControlBlock())
		return EDEADLK;

	int ret;
	while ((ret = joinSemaphore_.wait()) == EINTR);
	return ret;
}

int ThreadBase::start()
{
	if (getState() != scheduler::ThreadControlBlock::State::New)
		return EINVAL;

	scheduler::getScheduler().add(threadControlBlock_);
	return 0;
}

/*---------------------------------------------------------------------------------------------------------------------+
| private static functions
+---------------------------------------------------------------------------------------------------------------------*/

void ThreadBase::threadRunner(ThreadBase& threadBase)
{
	threadBase.run();
	scheduler::getScheduler().remove(&ThreadBase::terminationHook);

	while (1);
}

/*---------------------------------------------------------------------------------------------------------------------+
| private functions
+---------------------------------------------------------------------------------------------------------------------*/

void ThreadBase::terminationHook()
{
	joinSemaphore_.post();
}

}	// namespace distortos
