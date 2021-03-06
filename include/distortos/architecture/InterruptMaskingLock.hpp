/**
 * \file
 * \brief InterruptMaskingLock class header
 *
 * \author Copyright (C) 2014 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
 *
 * \par License
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \date 2014-08-06
 */

#ifndef INCLUDE_DISTORTOS_ARCHITECTURE_INTERRUPTMASKINGLOCK_HPP_
#define INCLUDE_DISTORTOS_ARCHITECTURE_INTERRUPTMASKINGLOCK_HPP_

#include "distortos/architecture/InterruptMaskingUnmaskingLock.hpp"

namespace distortos
{

namespace architecture
{

/// InterruptMaskingLock class is a RAII wrapper for enableInterruptMasking() / restoreInterruptMasking()
using InterruptMaskingLock = InterruptMaskingUnmaskingLock<enableInterruptMasking>;

}	// namespace architecture

}	// namespace distortos

#endif	// INCLUDE_DISTORTOS_ARCHITECTURE_INTERRUPTMASKINGLOCK_HPP_
