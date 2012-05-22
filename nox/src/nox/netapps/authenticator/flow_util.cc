/* Copyright 2008, 2009 (C) Nicira, Inc.
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

#include "flow_util.hh"

#include "cnode.hh"
#include "flow_in.hh"
#include "vlog.hh"

namespace vigil {

static Vlog_module lg("flow_classifier");

bool
Flow_expr::is_wildcard(uint32_t field) const
{
    if (field == Flow_expr::APPLY_SIDE) {
        return apply_side == ALWAYS_APPLY;
    }

    uint32_t n = m_preds.size();

    for (int i = 0; i < n; i++) {
        if ((Pred_t) field == m_preds[i].type)
            return false;
        if ((Pred_t) field < m_preds[i].type || m_preds[i].type < 0)
            return true;
    }
    return true;
}

bool
Flow_expr::splittable(uint32_t path) const
{
    if (apply_side != ALWAYS_APPLY) {
        if ((path & Cnode<Flow_expr, Flow_action>::MASKS[Flow_expr::APPLY_SIDE]) == 0) {
            return true;
        }
    }

    uint32_t n = m_preds.size();

    for (int i = 0; i < n; i++) {
        if (m_preds[i].type < 0)
            return false;
        if ((path & Cnode<Flow_expr, Flow_action>::MASKS[m_preds[i].type]) == 0)
            return true;
    }
    return false;
}

// negated predicates will have negative Pred_t value
bool
Flow_expr::get_field(uint32_t field, uint32_t& value) const
{
    if (field == Flow_expr::APPLY_SIDE) {
        if (apply_side != ALWAYS_APPLY) {
            value = apply_side;
            return true;
        }
        return false;
    }

    uint32_t n = m_preds.size();

    for (int i = 0; i < n; i++) {
        if ((Pred_t) field == m_preds[i].type) {
            value = ((uint32_t)(m_preds[i].val)) ^ ((m_preds[i].val) >> 32);
            return true;
        } else if ((Pred_t) field < m_preds[i].type || m_preds[i].type < 0) {
            return false;
        }
    }
    return false;
}

bool
Flow_expr::set_pred(uint32_t i, Pred_t type, uint64_t value)
{
    if (i >= m_preds.size())
        return false;

    m_preds[i].type = type;
    m_preds[i].val = value;
    return true;
}

bool
Flow_expr::set_pred(uint32_t i, Pred_t type, int64_t value)
{
    return set_pred(i, type, (int64_t) value);
}

bool
Flow_expr::set_fn(PyObject *fn)
{
#ifdef TWISTED_ENABLED
    if (fn == Py_None || fn == NULL) {
        Py_XDECREF(m_fn);
        m_fn = NULL;
    } else if (PyCallable_Check(fn)) {
        Py_XDECREF(m_fn);
        m_fn = fn;
        Py_INCREF(m_fn);
    } else {
        return false;
    }

    return true;
#else
    return false;
#endif
}

Flow_action::~Flow_action()
{

#ifdef TWISTED_ENABLED
    if (type == PY_FUNC) {
        PyObject* pyfn = boost::get<PyObject*>(arg);
        Py_XDECREF(pyfn);
    }
#endif
}

bool
Flow_action::set_arg(uint32_t i, uint64_t value)
{
    std::vector<uint64_t>& v = boost::get<std::vector<uint64_t> >(arg);

    if (i >= v.size()) {
        VLOG_ERR(lg, "Cannot set %"PRIu32"th arg - vector is size %zu.", i, v.size());
        return false;
    }

    v[i] = value;
    return true;
}

bool
Flow_action::set_arg(PyObject *pyfn)
{
#ifdef TWISTED_ENABLED
    if (type != PY_FUNC) {
        VLOG_ERR(lg, "Cannot set arg of action type %u to PyObject*.", type);
        return false;
    }
    Py_INCREF(pyfn);
    arg = pyfn;
    return true;
#else
    VLOG_ERR(lg, "Twisted disabled - cannot set action arg to PyObject*.");
    return false;
#endif
}

bool
Flow_action::set_arg(uint64_t value)
{
    arg = value;
    return true;
}

bool
Flow_action::set_arg(const C_func_t& fn)
{
    arg = fn;
    return true;
}

Flow_util::Flow_util(const container::Context* c,
                     const json_object*)
    : Component(c)
{}

void
Flow_util::getInstance(const container::Context* ctxt,
                       Flow_util*& f)
{
    f = dynamic_cast<Flow_util*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Flow_util).name())));
}

void
Flow_util::configure(const container::Configuration*)
{
    register_event(Flow_in_event::static_get_name());
}

} // namespace vigil

using namespace vigil;
using namespace vigil::container;

REGISTER_COMPONENT(Simple_component_factory<Flow_util>, Flow_util);
