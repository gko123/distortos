/**
 * \file
 * \brief StaticThread class header
 *
 * \author Copyright (C) 2014-2015 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2015-02-16
 */

#ifndef INCLUDE_DISTORTOS_STATICTHREAD_HPP_
#define INCLUDE_DISTORTOS_STATICTHREAD_HPP_

#include "distortos/Thread.hpp"

namespace distortos
{

/**
 * \brief StaticThread class is a templated interface for thread that has automatic storage of stack.
 *
 * \param StackSize is the size of stack, bytes
 * \param Function is the function that will be executed in separate thread
 * \param Args are the arguments for Function
 */

template<size_t StackSize, typename Function, typename... Args>
class StaticThread : public Thread<Function, Args...>
{
public:

	/// base of StaticThread
	using Base = Thread<Function, Args...>;

	/**
	 * \brief StaticThread's constructor
	 *
	 * \param [in] priority is the thread's priority, 0 - lowest, UINT8_MAX - highest
	 * \param [in] schedulingPolicy is the scheduling policy of the thread
	 * \param [in] function is a function that will be executed in separate thread
	 * \param [in] args are arguments for function
	 */

	StaticThread(const uint8_t priority, const SchedulingPolicy schedulingPolicy, Function&& function, Args&&... args) :
			Base{&stack_, sizeof(stack_), priority, schedulingPolicy, std::forward<Function>(function),
					std::forward<Args>(args)...}
	{

	}

	/**
	 * \brief StaticThread's constructor
	 *
	 * \param [in] priority is the thread's priority, 0 - lowest, UINT8_MAX - highest
	 * \param [in] function is a function that will be executed in separate thread
	 * \param [in] args are arguments for function
	 */

	StaticThread(const uint8_t priority, Function&& function, Args&&... args) :
			StaticThread{priority, SchedulingPolicy::RoundRobin, std::forward<Function>(function),
					std::forward<Args>(args)...}
	{

	}

	StaticThread(const StaticThread&) = delete;
	StaticThread(StaticThread&&) = default;
	const StaticThread& operator=(const StaticThread&) = delete;
	StaticThread& operator=(StaticThread&&) = delete;

private:

	/// stack buffer
	typename std::aligned_storage<StackSize>::type stack_;
};

/**
 * \brief Helper factory function to make StaticThread object with partially deduced template arguments
 *
 * \param StackSize is the size of stack, bytes
 * \param Function is the function that will be executed
 * \param Args are the arguments for Function
 *
 * \param [in] priority is the thread's priority, 0 - lowest, UINT8_MAX - highest
 * \param [in] schedulingPolicy is the scheduling policy of the thread
 * \param [in] function is a function that will be executed in separate thread
 * \param [in] args are arguments for function
 *
 * \return StaticThread object with partially deduced template arguments
 */

template<size_t StackSize, typename Function, typename... Args>
StaticThread<StackSize, Function, Args...> makeStaticThread(const uint8_t priority,
		const SchedulingPolicy schedulingPolicy, Function&& function, Args&&... args)
{
	return {priority, schedulingPolicy, std::forward<Function>(function), std::forward<Args>(args)...};
}

/**
 * \brief Helper factory function to make StaticThread object with partially deduced template arguments
 *
 * \param StackSize is the size of stack, bytes
 * \param Function is the function that will be executed
 * \param Args are the arguments for Function
 *
 * \param [in] priority is the thread's priority, 0 - lowest, UINT8_MAX - highest
 * \param [in] function is a function that will be executed in separate thread
 * \param [in] args are arguments for function
 *
 * \return StaticThread object with partially deduced template arguments
 */

template<size_t StackSize, typename Function, typename... Args>
StaticThread<StackSize, Function, Args...> makeStaticThread(const uint8_t priority, Function&& function, Args&&... args)
{
	return {priority, std::forward<Function>(function), std::forward<Args>(args)...};
}

}	// namespace distortos

#endif	// INCLUDE_DISTORTOS_STATICTHREAD_HPP_
