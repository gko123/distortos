/**
 * \file
 * \brief FifoQueue class header
 *
 * \author Copyright (C) 2014-2015 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2015-01-19
 */

#ifndef INCLUDE_DISTORTOS_FIFOQUEUE_HPP_
#define INCLUDE_DISTORTOS_FIFOQUEUE_HPP_

#include "distortos/synchronization/FifoQueueBase.hpp"
#include "distortos/synchronization/CopyConstructQueueFunctor.hpp"
#include "distortos/synchronization/MoveConstructQueueFunctor.hpp"
#include "distortos/synchronization/SwapPopQueueFunctor.hpp"
#include "distortos/synchronization/SemaphoreWaitFunctor.hpp"
#include "distortos/synchronization/SemaphoreTryWaitFunctor.hpp"
#include "distortos/synchronization/SemaphoreTryWaitForFunctor.hpp"
#include "distortos/synchronization/SemaphoreTryWaitUntilFunctor.hpp"

/// GCC 4.9 is needed for all FifoQueue::*emplace*() functions - earlier versions don't support parameter pack expansion
/// in lambdas
#define DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED	__GNUC_PREREQ(4, 9)

namespace distortos
{

/**
 * \brief FifoQueue class is a simple FIFO queue for thread-thread, thread-interrupt or interrupt-interrupt
 * communication. It supports multiple readers and multiple writers. It is implemented as a wrapper for
 * synchronization::FifoQueueBase.
 *
 * \param T is the type of data in queue
 */

template<typename T>
class FifoQueue
{
public:

	/// type of uninitialized storage for data
	using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

	/**
	 * \brief FifoQueue's constructor
	 *
	 * \param [in] storage is an array of Storage elements
	 * \param [in] maxElements is the number of elements in storage array
	 */

	FifoQueue(Storage* const storage, const size_t maxElements) :
			fifoQueueBase_{storage, storage + maxElements, sizeof(T), maxElements}
	{

	}

	/**
	 * \brief FifoQueue's constructor
	 *
	 * \param N is the number of elements in \a storage array
	 *
	 * \param [in] storage is a reference to array of Storage elements
	 */

	template<size_t N>
	explicit FifoQueue(Storage (& storage)[N]) :
			FifoQueue{storage, sizeof(storage) / sizeof(*storage)}
	{

	}

	/**
	 * \brief FifoQueue's constructor
	 *
	 * \param N is the number of elements in \a storage array
	 *
	 * \param [in] storage is a reference to std::array of Storage elements
	 */

	template<size_t N>
	explicit FifoQueue(std::array<Storage, N>& storage) :
			FifoQueue{storage.data(), storage.size()}
	{

	}

#if DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

	/**
	 * \brief Emplaces the element in the queue.
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by Semaphore::wait();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename... Args>
	int emplace(Args&&... args)
	{
		const synchronization::SemaphoreWaitFunctor semaphoreWaitFunctor;
		return emplaceInternal(semaphoreWaitFunctor, std::forward<Args>(args)...);
	}

#endif	// DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

	/**
	 * \brief Pops the oldest (first) element from the queue.
	 *
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by Semaphore::wait();
	 * - error codes returned by Semaphore::post();
	 */

	int pop(T& value)
	{
		const synchronization::SemaphoreWaitFunctor semaphoreWaitFunctor;
		return popInternal(semaphoreWaitFunctor, value);
	}

	/**
	 * \brief Pushes the element to the queue.
	 *
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::wait();
	 * - error codes returned by Semaphore::post();
	 */

	int push(const T& value)
	{
		const synchronization::SemaphoreWaitFunctor semaphoreWaitFunctor;
		return pushInternal(semaphoreWaitFunctor, value);
	}

	/**
	 * \brief Pushes the element to the queue.
	 *
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::wait();
	 * - error codes returned by Semaphore::post();
	 */

	int push(T&& value)
	{
		const synchronization::SemaphoreWaitFunctor semaphoreWaitFunctor;
		return pushInternal(semaphoreWaitFunctor, std::move(value));
	}

#if DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

	/**
	 * \brief Tries to emplace the element in the queue.
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWait();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename... Args>
	int tryEmplace(Args&&... args)
	{
		const synchronization::SemaphoreTryWaitFunctor semaphoreTryWaitFunctor;
		return emplaceInternal(semaphoreTryWaitFunctor, std::forward<Args>(args)...);
	}

	/**
	 * \brief Tries to emplace the element in the queue for a given duration of time.
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] duration is the duration after which the wait will be terminated without emplacing the element
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename... Args>
	int tryEmplaceFor(const TickClock::duration duration, Args&&... args)
	{
		const synchronization::SemaphoreTryWaitForFunctor semaphoreTryWaitForFunctor {duration};
		return emplaceInternal(semaphoreTryWaitForFunctor, std::forward<Args>(args)...);
	}

	/**
	 * \brief Tries to emplace the element in the queue for a given duration of time.
	 *
	 * Template variant of FifoQueue::tryEmplaceFor(TickClock::duration, Args&&...).
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Rep is type of tick counter
	 * \param Period is std::ratio type representing the tick period of the clock, in seconds
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] duration is the duration after which the wait will be terminated without emplacing the element
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Rep, typename Period, typename... Args>
	int tryEmplaceFor(const std::chrono::duration<Rep, Period> duration, Args&&... args)
	{
		return tryEmplaceFor(std::chrono::duration_cast<TickClock::duration>(duration), std::forward<Args>(args)...);
	}

	/**
	 * \brief Tries to emplace the element in the queue until a given time point.
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without emplacing the element
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename... Args>
	int tryEmplaceUntil(const TickClock::time_point timePoint, Args&&... args)
	{
		const synchronization::SemaphoreTryWaitUntilFunctor semaphoreTryWaitUntilFunctor {timePoint};
		return emplaceInternal(semaphoreTryWaitUntilFunctor, std::forward<Args>(args)...);
	}

	/**
	 * \brief Tries to emplace the element in the queue until a given time point.
	 *
	 * Template variant of FifoQueue::tryEmplaceUntil(TickClock::time_point, Args&&...).
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Duration is a std::chrono::duration type used to measure duration
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without emplacing the element
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Duration, typename... Args>
	int tryEmplaceUntil(const std::chrono::time_point<TickClock, Duration> timePoint, Args&&... args)
	{
		return tryEmplaceUntil(std::chrono::time_point_cast<TickClock::duration>(timePoint),
				std::forward<Args>(args)...);
	}

#endif	// DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

	/**
	 * \brief Tries to pop the oldest (first) element from the queue.
	 *
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWait();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPop(T& value)
	{
		synchronization::SemaphoreTryWaitFunctor semaphoreTryWaitFunctor;
		return popInternal(semaphoreTryWaitFunctor, value);
	}

	/**
	 * \brief Tries to pop the oldest (first) element from the queue for a given duration of time.
	 *
	 * \param [in] duration is the duration after which the call will be terminated without popping the element
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPopFor(const TickClock::duration duration, T& value)
	{
		const synchronization::SemaphoreTryWaitForFunctor semaphoreTryWaitForFunctor {duration};
		return popInternal(semaphoreTryWaitForFunctor, value);
	}

	/**
	 * \brief Tries to pop the oldest (first) element from the queue for a given duration of time.
	 *
	 * Template variant of tryPopFor(TickClock::duration, T&).
	 *
	 * \param Rep is type of tick counter
	 * \param Period is std::ratio type representing the tick period of the clock, in seconds
	 *
	 * \param [in] duration is the duration after which the call will be terminated without popping the element
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Rep, typename Period>
	int tryPopFor(const std::chrono::duration<Rep, Period> duration, T& value)
	{
		return tryPopFor(std::chrono::duration_cast<TickClock::duration>(duration), value);
	}

	/**
	 * \brief Tries to pop the oldest (first) element from the queue until a given time point.
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without popping the element
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPopUntil(const TickClock::time_point timePoint, T& value)
	{
		const synchronization::SemaphoreTryWaitUntilFunctor semaphoreTryWaitUntilFunctor {timePoint};
		return popInternal(semaphoreTryWaitUntilFunctor, value);
	}

	/**
	 * \brief Tries to pop the oldest (first) element from the queue until a given time point.
	 *
	 * Template variant of tryPopUntil(TickClock::time_point, T&).
	 *
	 * \param Duration is a std::chrono::duration type used to measure duration
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without popping the element
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Duration>
	int tryPopUntil(const std::chrono::time_point<TickClock, Duration> timePoint, T& value)
	{
		return tryPopUntil(std::chrono::time_point_cast<TickClock::duration>(timePoint), value);
	}

	/**
	 * \brief Tries to push the element to the queue.
	 *
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWait();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPush(const T& value)
	{
		const synchronization::SemaphoreTryWaitFunctor semaphoreTryWaitFunctor;
		return pushInternal(semaphoreTryWaitFunctor, value);
	}

	/**
	 * \brief Tries to push the element to the queue.
	 *
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWait();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPush(T&& value)
	{
		const synchronization::SemaphoreTryWaitFunctor semaphoreTryWaitFunctor;
		return pushInternal(semaphoreTryWaitFunctor, std::move(value));
	}

	/**
	 * \brief Tries to push the element to the queue for a given duration of time.
	 *
	 * \param [in] duration is the duration after which the wait will be terminated without pushing the element
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPushFor(const TickClock::duration duration, const T& value)
	{
		const synchronization::SemaphoreTryWaitForFunctor semaphoreTryWaitForFunctor {duration};
		return pushInternal(semaphoreTryWaitForFunctor, value);
	}

	/**
	 * \brief Tries to push the element to the queue for a given duration of time.
	 *
	 * Template variant of tryPushFor(TickClock::duration, const T&).
	 *
	 * \param Rep is type of tick counter
	 * \param Period is std::ratio type representing the tick period of the clock, in seconds
	 *
	 * \param [in] duration is the duration after which the wait will be terminated without pushing the element
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Rep, typename Period>
	int tryPushFor(const std::chrono::duration<Rep, Period> duration, const T& value)
	{
		return tryPushFor(std::chrono::duration_cast<TickClock::duration>(duration), value);
	}

	/**
	 * \brief Tries to push the element to the queue for a given duration of time.
	 *
	 * \param [in] duration is the duration after which the call will be terminated without pushing the element
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPushFor(const TickClock::duration duration, T&& value)
	{
		const synchronization::SemaphoreTryWaitForFunctor semaphoreTryWaitForFunctor {duration};
		return pushInternal(semaphoreTryWaitForFunctor, std::move(value));
	}

	/**
	 * \brief Tries to push the element to the queue for a given duration of time.
	 *
	 * Template variant of tryPushFor(TickClock::duration, T&&).
	 *
	 * \param Rep is type of tick counter
	 * \param Period is std::ratio type representing the tick period of the clock, in seconds
	 *
	 * \param [in] duration is the duration after which the call will be terminated without pushing the element
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitFor();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Rep, typename Period>
	int tryPushFor(const std::chrono::duration<Rep, Period> duration, T&& value)
	{
		return tryPushFor(std::chrono::duration_cast<TickClock::duration>(duration), std::move(value));
	}

	/**
	 * \brief Tries to push the element to the queue until a given time point.
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without pushing the element
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPushUntil(const TickClock::time_point timePoint, const T& value)
	{
		const synchronization::SemaphoreTryWaitUntilFunctor semaphoreTryWaitUntilFunctor {timePoint};
		return pushInternal(semaphoreTryWaitUntilFunctor, value);
	}

	/**
	 * \brief Tries to push the element to the queue until a given time point.
	 *
	 * Template variant of tryPushUntil(TickClock::time_point, const T&).
	 *
	 * \param Duration is a std::chrono::duration type used to measure duration
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without pushing the element
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Duration>
	int tryPushUntil(const std::chrono::time_point<TickClock, Duration> timePoint, const T& value)
	{
		return tryPushUntil(std::chrono::time_point_cast<TickClock::duration>(timePoint), value);
	}

	/**
	 * \brief Tries to push the element to the queue until a given time point.
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without pushing the element
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	int tryPushUntil(const TickClock::time_point timePoint, T&& value)
	{
		const synchronization::SemaphoreTryWaitUntilFunctor semaphoreTryWaitUntilFunctor {timePoint};
		return pushInternal(semaphoreTryWaitUntilFunctor, std::move(value));
	}

	/**
	 * \brief Tries to push the element to the queue until a given time point.
	 *
	 * Template variant of tryPushUntil(TickClock::time_point, T&&).
	 *
	 * \param Duration is a std::chrono::duration type used to measure duration
	 *
	 * \param [in] timePoint is the time point at which the call will be terminated without pushing the element
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by Semaphore::tryWaitUntil();
	 * - error codes returned by Semaphore::post();
	 */

	template<typename Duration>
	int tryPushUntil(const std::chrono::time_point<TickClock, Duration> timePoint, T&& value)
	{
		return tryPushUntil(std::chrono::time_point_cast<TickClock::duration>(timePoint), std::move(value));
	}

private:

	/**
	 * \brief BoundedFunctor is a type-erased synchronization::QueueFunctor which calls its bounded functor to execute
	 * actions on queue's storage
	 *
	 * \param F is the type of bounded functor, it will be called with <em>void*</em> as only argument
	 */

	template<typename F>
	class BoundedFunctor : public synchronization::QueueFunctor
	{
	public:

		/**
		 * \brief BoundedFunctor's constructor
		 *
		 * \param [in] boundedFunctor is a rvalue reference to bounded functor which will be used to move-construct
		 * internal bounded functor
		 */

		constexpr explicit BoundedFunctor(F&& boundedFunctor) :
				boundedFunctor_{std::move(boundedFunctor)}
		{

		}

		/**
		 * \brief Calls the bounded functor which will execute some action on queue's storage (like copy-constructing,
		 * swapping, destroying, emplacing, ...)
		 *
		 * \param [in,out] storage is a pointer to storage with/for element
		 */

		virtual void operator()(void* storage) const override
		{
			boundedFunctor_(storage);
		}

	private:

		/// bounded functor
		F boundedFunctor_;
	};

	/**
	 * \brief Helper factory function to make BoundedFunctor object with partially deduced template arguments
	 *
	 * \param F is the type of bounded functor, it will be called with <em>void*</em> as only argument
	 *
	 * \param [in] boundedFunctor is a rvalue reference to bounded functor which will be used to move-construct internal
	 * bounded functor
	 *
	 * \return BoundedFunctor object with partially deduced template arguments
	 */

	template<typename F>
	constexpr static BoundedFunctor<F> makeBoundedFunctor(F&& boundedFunctor)
	{
		return BoundedFunctor<F>{std::move(boundedFunctor)};
	}

#if DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

	/**
	 * \brief Emplaces the element in the queue.
	 *
	 * Internal version - builds the Functor object.
	 *
	 * \note This function requires GCC 4.9.
	 *
	 * \param Args are types of arguments for constructor of T
	 *
	 * \param [in] waitSemaphoreFunctor is a reference to SemaphoreFunctor which will be executed with \a pushSemaphore_
	 * \param [in] args are arguments for constructor of T
	 *
	 * \return zero if element was emplaced successfully, error code otherwise:
	 * - error codes returned by \a waitSemaphoreFunctor's operator() call;
	 * - error codes returned by Semaphore::post();
	 */

	template<typename... Args>
	int emplaceInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, Args&&... args);

#endif	// DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

	/**
	 * \brief Pops the oldest (first) element from the queue.
	 *
	 * Internal version - builds the Functor object.
	 *
	 * \param [in] waitSemaphoreFunctor is a reference to SemaphoreFunctor which will be executed with \a popSemaphore_
	 * \param [out] value is a reference to object that will be used to return popped value, its contents are swapped
	 * with the value in the queue's storage and destructed when no longer needed
	 *
	 * \return zero if element was popped successfully, error code otherwise:
	 * - error codes returned by \a waitSemaphoreFunctor's operator() call;
	 * - error codes returned by Semaphore::post();
	 */

	int popInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, T& value);

	/**
	 * \brief Pushes the element to the queue.
	 *
	 * Internal version - builds the Functor object.
	 *
	 * \param [in] waitSemaphoreFunctor is a reference to SemaphoreFunctor which will be executed with \a pushSemaphore_
	 * \param [in] value is a reference to object that will be pushed, value in queue's storage is copy-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by \a waitSemaphoreFunctor's operator() call;
	 * - error codes returned by Semaphore::post();
	 */

	int pushInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, const T& value);

	/**
	 * \brief Pushes the element to the queue.
	 *
	 * Internal version - builds the Functor object.
	 *
	 * \param [in] waitSemaphoreFunctor is a reference to SemaphoreFunctor which will be executed with \a pushSemaphore_
	 * \param [in] value is a rvalue reference to object that will be pushed, value in queue's storage is
	 * move-constructed
	 *
	 * \return zero if element was pushed successfully, error code otherwise:
	 * - error codes returned by \a waitSemaphoreFunctor's operator() call;
	 * - error codes returned by Semaphore::post();
	 */

	int pushInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, T&& value);

	/// contained synchronization::FifoQueueBase object which implements whole functionality
	synchronization::FifoQueueBase fifoQueueBase_;
};

#if DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

template<typename T>
template<typename... Args>
int FifoQueue<T>::emplaceInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, Args&&... args)
{
	const auto emplaceFunctor = makeBoundedFunctor(
			[&args...](void* const storage)
			{
				new (storage) T{std::forward<Args>(args)...};
			});
	return fifoQueueBase_.push(waitSemaphoreFunctor, emplaceFunctor);
}

#endif	// DISTORTOS_FIFOQUEUE_EMPLACE_SUPPORTED == 1 || DOXYGEN == 1

template<typename T>
int FifoQueue<T>::popInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, T& value)
{
	const synchronization::SwapPopQueueFunctor<T> swapPopQueueFunctor {value};
	return fifoQueueBase_.pop(waitSemaphoreFunctor, swapPopQueueFunctor);
}

template<typename T>
int FifoQueue<T>::pushInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, const T& value)
{
	const synchronization::CopyConstructQueueFunctor<T> copyConstructQueueFunctor {value};
	return fifoQueueBase_.push(waitSemaphoreFunctor, copyConstructQueueFunctor);
}

template<typename T>
int FifoQueue<T>::pushInternal(const synchronization::SemaphoreFunctor& waitSemaphoreFunctor, T&& value)
{
	const synchronization::MoveConstructQueueFunctor<T> moveConstructQueueFunctor {std::move(value)};
	return fifoQueueBase_.push(waitSemaphoreFunctor, moveConstructQueueFunctor);
}

}	// namespace distortos

#endif	// INCLUDE_DISTORTOS_FIFOQUEUE_HPP_
