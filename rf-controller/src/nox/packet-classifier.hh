/* Copyright 2008 (C) Nicira, Inc.
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
#ifndef PACKET_CLASSIFIER_HH
#define PACKET_CLASSIFIER_HH 1

#include "classifier.hh"
#include "event.hh"
#include "expr.hh"

namespace vigil {

typedef boost::function<void(const Event&)> Pexpr_action;

class Packet_classifier
    : public Classifier<Packet_expr, Pexpr_action>
{
public:
    Packet_classifier(uint32_t split_field, int n_buckets)
        : Classifier<Packet_expr, Pexpr_action>(split_field, n_buckets), result(NULL) { }
    Packet_classifier()
        : Classifier<Packet_expr, Pexpr_action>(), result(NULL) { }
    ~Packet_classifier() { }

    void register_packet_in();
    Disposition handle_packet_in(const Event& e);

private:
    Cnode_result<Packet_expr, Pexpr_action, Flow> result;

    Packet_classifier(const Packet_classifier&);
    Packet_classifier& operator=(const Packet_classifier&);
};

} // namespace vigil

#endif /* packet-classifier.hh */
