/***************************************************************************
 *  durations.h - Durations of mockup actions
 *
 *  Created: Tue 14 Apr 2020 13:15:37 CEST 13:15
 *  Copyright  2020  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#pragma once

#include <array>
#include <chrono>

inline const std::chrono::milliseconds min_operation_duration_{1000};
inline const std::chrono::milliseconds duration_band_input_to_mid_{1538};
inline const std::chrono::milliseconds duration_band_mid_to_output_{2656};
//inline const std::chrono::milliseconds duration_cap_retrieval_{17660};
//inline const std::chrono::milliseconds duration_cap_mount_{17848};
inline const std::chrono::milliseconds duration_cap_op_{17500};
// TODO: get accurate timing
inline const std::chrono::milliseconds                      duration_storage_op_{17500};
inline const std::chrono::milliseconds                      duration_ring_mount_{17308};
inline const std::chrono::milliseconds                      duration_base_dispense_{1100};
inline const std::chrono::milliseconds                      duration_ready_at_output_{15000};
inline const std::array<const std::chrono::milliseconds, 3> duration_ds_slots{
  std::chrono::milliseconds{3746},
  std::chrono::milliseconds{4746},
  std::chrono::milliseconds{3983}};
