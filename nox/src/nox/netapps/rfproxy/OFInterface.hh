/* Copyright 2012 (C) Victoria University of Waikato.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __OFINTERFACE_HH__
#define __OFINTERFACE_HH__

#include <vector>
#include <boost/shared_array.hpp>

#include "config.h"
#include "vlog.hh"

#include "types/Match.hh"
#include "types/Action.hh"
#include "types/Option.hh"

boost::shared_array<uint8_t> create_flow_mod(uint8_t mod,
            std::vector<Match>, std::vector<Action>, std::vector<Option>);

#endif /*__OFINTERFACE_HH__ */
