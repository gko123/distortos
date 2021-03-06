/**
 * \file
 * \brief statistics namespace implementation
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-11-04
 */

#include "distortos/statistics.hpp"

#include "distortos/scheduler/getScheduler.hpp"
#include "distortos/scheduler/Scheduler.hpp"

namespace distortos
{

namespace statistics
{

/*---------------------------------------------------------------------------------------------------------------------+
| global functions
+---------------------------------------------------------------------------------------------------------------------*/

uint64_t getContextSwitchCount()
{
	return scheduler::getScheduler().getContextSwitchCount();
}

}	// namespace statistics

}	// namespace distortos
