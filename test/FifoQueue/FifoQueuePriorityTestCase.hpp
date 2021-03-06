/**
 * \file
 * \brief FifoQueuePriorityTestCase class header
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-12-14
 */

#ifndef TEST_FIFOQUEUE_FIFOQUEUEPRIORITYTESTCASE_HPP_
#define TEST_FIFOQUEUE_FIFOQUEUEPRIORITYTESTCASE_HPP_

#include "PrioritizedTestCase.hpp"

namespace distortos
{

namespace test
{

/**
 * \brief Tests priority scheduling of FIFO queue.
 *
 * Starts 10 small threads (in various order) with varying priorities which wait either for a message from the FIFO
 * queue or for free space in the FIFO queue, asserting that they start and finish in the expected order, using exact
 * number of context switches and that the data received from the FIFO queue matches what was transferred.
 */

class FifoQueuePriorityTestCase : public PrioritizedTestCase
{
	/// priority at which this test case should be executed
	constexpr static uint8_t testCasePriority_ {1};

public:

	/// internal implementation of FifoQueuePriorityTestCase
	class Implementation : public TestCase
	{
	private:

		/**
		 * \brief Runs the test case.
		 *
		 * \return true if the test case succeeded, false otherwise
		 */

		virtual bool run_() const override;
	};

	/**
	 * \brief FifoQueuePriorityTestCase's constructor
	 *
	 * \param [in] implementation is a reference to FifoQueuePriorityTestCase::Implementation object used by this
	 * instance
	 */

	constexpr explicit FifoQueuePriorityTestCase(const Implementation& implementation) :
			PrioritizedTestCase{implementation, testCasePriority_}
	{

	}
};

}	// namespace test

}	// namespace distortos

#endif	// TEST_FIFOQUEUE_FIFOQUEUEPRIORITYTESTCASE_HPP_
