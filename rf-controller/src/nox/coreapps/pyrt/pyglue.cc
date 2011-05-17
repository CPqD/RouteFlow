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
#include <config.h>

#ifdef TWISTED_ENABLED

#include "pyglue.hh"
#include "buffer.hh"
#include "flow.hh"
#include "flow-stats-in.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ipaddr.hh"
#include "packets.h"
#include "port.hh"
#include "table-stats-in.hh"
#include "port-stats-in.hh"
#include "pycontext.hh"
#include "pyrt.hh"
#include "vlog.hh"

#ifndef SWIG
#include "swigpyrun.h"
#endif // SWIG

using namespace std;

namespace vigil {

static Vlog_module lg("pyglue");

/* Convert 'obj' into a long int, clamp the value between 'maximum' and
 * 'minimum', reporting any error, and return the final value. */
static inline long int
long_from_python(PyObject* obj, const char *type_name,
                 long int minimum, long int maximum)
{
    long int value = PyInt_AsLong(obj);
    if (value == -1 && PyErr_Occurred()) {
        PyErr_Print();
    } else if (value < minimum) {
        PyErr_Format(PyExc_RuntimeError,
                     "Value %ld is less than minimum for %s (%ld)",
                     value, type_name, minimum);
        value = minimum;
    } else if (value > maximum) {
        PyErr_Format(PyExc_RuntimeError,
                     "Value %ld is greater than maximum for %s (%ld)",
                     value, type_name, maximum);
        value = maximum;
    }
    return value;
}

#define FROM_PYTHON_LONG(TYPE, MIN, MAX)                        \
        template <>                                             \
        TYPE                                                    \
        from_python(PyObject* obj)                              \
        {                                                       \
            return long_from_python(obj, #TYPE, MIN, MAX);      \
        }

/* Define from_python for all the integer types whose values are a subrange of
 * 'long int'. */
FROM_PYTHON_LONG(signed char,           SCHAR_MIN,      SCHAR_MAX);
FROM_PYTHON_LONG(short int,             SHRT_MIN,       SHRT_MAX);
FROM_PYTHON_LONG(int,                   INT_MIN,        INT_MAX);
FROM_PYTHON_LONG(long int,              LONG_MIN,       LONG_MAX);

FROM_PYTHON_LONG(unsigned char,         0,              UCHAR_MAX);
FROM_PYTHON_LONG(unsigned short int,    0,              USHRT_MAX);
FROM_PYTHON_LONG(unsigned int,          0,              UINT_MAX);

FROM_PYTHON_LONG(char,                  CHAR_MIN,       CHAR_MAX);

/* Define from_python for all the other integer types.  Python doesn't provide
 * very good support for this, so our error checking is not so great. */
#define FROM_PYTHON_LONG_LONG(TYPE)                     \
        template <>                                     \
        TYPE                                            \
        from_python(PyObject* obj)                      \
        {                                               \
            return PyInt_AsUnsignedLongLongMask(obj);   \
        }
FROM_PYTHON_LONG_LONG(long long int);
FROM_PYTHON_LONG_LONG(unsigned long int);
FROM_PYTHON_LONG_LONG(unsigned long long int);

template <>
const container::Context*
from_python(PyObject* ctxt)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    applications::PyContext* pyctxt = (applications::PyContext*)
        SWIG_Python_GetSwigThis(ctxt)->ptr;

    return pyctxt->ctxt;
}

template <>
datapathid
from_python(PyObject *pydp)
{
    static PyObject *method = PyString_FromString("as_host");
    if (method == NULL) {
        VLOG_ERR(lg, "Could not create as_host string.");
    } else {
        PyObject *as_host = PyObject_CallMethodObjArgs(pydp, method, NULL);
        if (as_host == NULL) {
            const string exc = applications::pretty_print_python_exception();
            VLOG_ERR(lg, "Could not get as_host() datapathid value: %s",
                     exc.c_str());
        } else {
            datapathid dp = datapathid::from_host(from_python<uint64_t>(as_host));
            Py_DECREF(as_host);
            return dp;
        }
    }
    return datapathid::from_host(0);
}

template <>
ofp_match
from_python(PyObject *pymatch)
{
    ofp_match match;
    memset(&match, 0, sizeof match);
    match.wildcards = htonl(OFPFW_ALL);

    PyObject* in_port = PyDict_GetItemString(pymatch, "in_port");
    if (in_port) {
        match.in_port = htons(from_python<uint16_t>(in_port));
        match.wildcards &= htonl(~OFPFW_IN_PORT);
    }

    PyObject* dl_src = PyDict_GetItemString(pymatch, "dl_src");
    if (dl_src) {
        /* XXX this should accept ethernetaddr objects also. */
        eth_addr_from_uint64(from_python<uint64_t>(dl_src), match.dl_src);
        match.wildcards &= htonl(~OFPFW_DL_SRC);
    }

    PyObject* dl_dst = PyDict_GetItemString(pymatch, "dl_dst");
    if (dl_dst) {
        /* XXX this should accept ethernetaddr objects also. */
        eth_addr_from_uint64(from_python<uint64_t>(dl_dst), match.dl_dst);
        match.wildcards &= htonl(~OFPFW_DL_DST);
    }

    PyObject* dl_vlan = PyDict_GetItemString(pymatch, "dl_vlan");
    if (dl_vlan) {
        match.dl_vlan = htons(from_python<uint16_t>(dl_vlan));
        match.wildcards &= htonl(~OFPFW_DL_VLAN);
    }

    PyObject* dl_vlan_pcp = PyDict_GetItemString(pymatch, "dl_vlan_pcp");
    if (dl_vlan_pcp) {
        match.dl_vlan_pcp = from_python<uint8_t>(dl_vlan_pcp);
        match.wildcards &= htonl(~OFPFW_DL_VLAN_PCP);
    }

    PyObject* dl_type = PyDict_GetItemString(pymatch, "dl_type");
    if (dl_type) {
        match.dl_type = htons(from_python<uint16_t>(dl_type));
        match.wildcards &= htonl(~OFPFW_DL_TYPE);
    }

    PyObject* nw_src = PyDict_GetItemString(pymatch, "nw_src");
    if (nw_src) {
        match.nw_src = htons(from_python<uint32_t>(nw_src));
        match.wildcards &= htonl(~OFPFW_NW_SRC_MASK);
        PyObject* nw_src_n_wild = PyDict_GetItemString(pymatch,
                                                       "nw_src_n_wild");
        if (nw_src_n_wild) {
            unsigned int n_wild = from_python<unsigned int>(nw_src_n_wild);
            if (n_wild > 0) {
                if (n_wild > 32) {
                    n_wild = 32;
                }
                match.wildcards |= htonl(n_wild << OFPFW_NW_SRC_SHIFT);
            }
        }
    }

    PyObject* nw_dst = PyDict_GetItemString(pymatch, "nw_dst");
    if (nw_dst) {
        match.nw_dst = htons(from_python<uint32_t>(nw_dst));
        match.wildcards &= htonl(~OFPFW_NW_DST_MASK);
        PyObject* nw_dst_n_wild = PyDict_GetItemString(pymatch,
                                                       "nw_dst_n_wild");
        if (nw_dst_n_wild) {
            unsigned int n_wild = from_python<unsigned int>(nw_dst_n_wild);
            if (n_wild > 0) {
                if (n_wild > 32) {
                    n_wild = 32;
                }
                match.wildcards |= htonl(n_wild << OFPFW_NW_DST_SHIFT);
            }
        }
    }

    PyObject* nw_tos = PyDict_GetItemString(pymatch, "nw_tos");
    if (nw_tos) {
        match.nw_tos = from_python<uint8_t>(nw_tos);
        match.wildcards &= htonl(~OFPFW_NW_TOS);
    }

    PyObject* nw_proto = PyDict_GetItemString(pymatch, "nw_proto");
    if (nw_proto) {
        match.nw_proto = from_python<uint8_t>(nw_proto);
        match.wildcards &= htonl(~OFPFW_NW_PROTO);
    }

    PyObject* tp_src = PyDict_GetItemString(pymatch, "tp_src");
    if (tp_src) {
        match.tp_src = htons(from_python<uint16_t>(tp_src));
        match.wildcards &= htonl(~OFPFW_TP_SRC);
    }

    PyObject* tp_dst = PyDict_GetItemString(pymatch, "tp_dst");
    if (tp_dst) {
        match.tp_dst = htons(from_python<uint16_t>(tp_dst));
        match.wildcards &= htonl(~OFPFW_TP_DST);
    }

    return match;
}

template <>
ofp_flow_stats_request
from_python(PyObject *pyfsr)
{
    ofp_flow_stats_request fsr;
    memset(&fsr, 0, sizeof fsr);
    fsr.table_id = 0xff;
    fsr.out_port = ntohs(OFPP_NONE);
    fsr.match.wildcards = htonl(OFPFW_ALL);

    PyObject* pymatch = PyDict_GetItemString(pyfsr, "match");
    if (pymatch) {
        fsr.match = from_python<ofp_match>(pymatch);
    }

    PyObject* table_id = PyDict_GetItemString(pyfsr, "table_id");
    if (table_id) {
        fsr.table_id = from_python<uint8_t>(table_id);
    }

    return fsr;
}

template <>
const std::string
from_python(PyObject* py_basestring)
{
    if (PyString_Check(py_basestring)) {
        return string(PyString_AsString(py_basestring));
    }
    else if(PyUnicode_Check(py_basestring)) {
        PyObject *sobj = PyUnicode_AsEncodedString(py_basestring,
                "utf-8", NULL);
        if (!sobj) {
            throw runtime_error("Failed to encode unicode string");
        }
        string ret = string(PyString_AsString(sobj));
        Py_DECREF(sobj);
        return ret;
    }
    else {
        throw runtime_error(
                "Invalid parameter; expected type string or unicode");
    }
}

void
pyglue_setattr_string(PyObject* src, const std::string& astr, PyObject* attr)
{
    if(PyObject_SetAttrString(src, (char*)astr.c_str(), attr) == -1){
        throw std::runtime_error("Unable to set attribute " + astr);
    }

    /* steal reference of attr */  
    Py_DECREF(attr);
}

void
pyglue_setdict_string(PyObject* dict, const std::string& key_, PyObject* value)
{
    PyObject* key = PyString_FromString(key_.c_str());
    if (!key) {
        throw std::bad_alloc();
    }

    PyDict_SetItem(dict, key, value);
    Py_DECREF(key);
    Py_DECREF(value);
}

template <>
PyObject*
to_python(const Buffer& b)
{
    return Py_BuildValue((char*)"s#",b.data(), b.size());
}

template <>
PyObject*
to_python(const boost::shared_ptr<Buffer>& p)
{
    return Py_BuildValue((char*)"s#",p->data(), p->size());
}

template <>
PyObject*
to_python(const Port& p)
{
    return Py_BuildValue((char*)"{s:h, s:l, s:l, s:l, s:l, s:l, s:l, s:l, s:s# s:s}", 
            "port_no", p.port_no, "speed", p.speed, "config", p.config, 
            "state", p.state, "curr", p.curr, "advertised", p.advertised, 
            "supported", p.supported, "peer", p.peer,
            "hw_addr", p.hw_addr.octet, ethernetaddr::LEN,
            "name", p.name.c_str()
            );
}

#define CONVERT_SWITCH_STAT(field,result) \
  do {                                \
  if(field == (uint64_t) -1)          \
    result = PyLong_FromLong(-1);      \
  else                                \
    result = PyLong_FromUnsignedLongLong((unsigned long long) field);    \
  } while(0); 

#define CONVERT_CHECK(result)\
  do {                    \
      if (!result)            \
      goto error;           \
    } while(0);          


template <>
PyObject*
to_python(const Table_stats& ts)
{
    PyObject *pyo_lookup_count = 0, *pyo_matched_count = 0, *ret = 0;
    CONVERT_SWITCH_STAT(ts.lookup_count,pyo_lookup_count)
    CONVERT_CHECK(pyo_lookup_count)
    CONVERT_SWITCH_STAT(ts.matched_count,pyo_matched_count)
    CONVERT_CHECK(pyo_matched_count)

    ret = Py_BuildValue((char*)"{s:l, s:s#, s:l, s:l, s:S, s:S}", 
            "table_id", ts.table_id, 
            "name", ts.name.c_str(), ts.name.size(),
            "max_entries",   ts.max_entries, 
            "active_count",  ts.active_count, 
            "lookup_count", pyo_lookup_count,
            "matched_count", pyo_matched_count );
error: 
    Py_XDECREF(pyo_lookup_count);
    Py_XDECREF(pyo_matched_count);
    return ret; 
}

template <>
PyObject*
to_python(const Port_stats& ts)
{
    PyObject *pyo_rx_packets = 0, *pyo_tx_packets = 0;
    PyObject *pyo_rx_bytes = 0, *pyo_tx_bytes = 0;
    PyObject *pyo_rx_dropped = 0, *pyo_tx_dropped = 0;
    PyObject *pyo_rx_errors = 0, *pyo_tx_errors = 0;
    PyObject *pyo_rx_frame_err = 0, *pyo_rx_over_err = 0;
    PyObject *pyo_rx_crc_err = 0, *pyo_collisions = 0;
    PyObject *ret = 0;

    CONVERT_SWITCH_STAT(ts.rx_packets,pyo_rx_packets)
    CONVERT_CHECK(pyo_rx_packets)
    CONVERT_SWITCH_STAT(ts.tx_packets,pyo_tx_packets)
    CONVERT_CHECK(pyo_tx_packets)
    
    CONVERT_SWITCH_STAT(ts.rx_bytes,pyo_rx_bytes)
    CONVERT_CHECK(pyo_rx_bytes)
    CONVERT_SWITCH_STAT(ts.tx_bytes,pyo_tx_bytes)
    CONVERT_CHECK(pyo_tx_bytes)
    
    CONVERT_SWITCH_STAT(ts.rx_dropped,pyo_rx_dropped)
    CONVERT_CHECK(pyo_rx_dropped)
    CONVERT_SWITCH_STAT(ts.tx_dropped,pyo_tx_dropped)
    CONVERT_CHECK(pyo_tx_dropped)
    
    CONVERT_SWITCH_STAT(ts.rx_errors,pyo_rx_errors)
    CONVERT_CHECK(pyo_rx_errors)
    CONVERT_SWITCH_STAT(ts.tx_errors,pyo_tx_errors)
    CONVERT_CHECK(pyo_tx_errors)

    CONVERT_SWITCH_STAT(ts.rx_frame_err,pyo_rx_frame_err)
    CONVERT_CHECK(pyo_rx_frame_err)
    CONVERT_SWITCH_STAT(ts.rx_over_err,pyo_rx_over_err)
    CONVERT_CHECK(pyo_rx_over_err)
    CONVERT_SWITCH_STAT(ts.rx_crc_err,pyo_rx_crc_err)
    CONVERT_CHECK(pyo_rx_crc_err)
    CONVERT_SWITCH_STAT(ts.collisions,pyo_collisions)
    CONVERT_CHECK(pyo_collisions)

    ret =  Py_BuildValue((char*)"{s:l, s:S, s:S, s:S, s:S, s:S, s:S, s:S, s:S, s:S, s:S, s:S, s:S}", 
            "port_no",    (int)ts.port_no, 
            "rx_packets",   pyo_rx_packets, 
            "tx_packets",   pyo_tx_packets, 
            "rx_bytes",     pyo_rx_bytes, 
            "tx_bytes",     pyo_tx_bytes, 
            "rx_dropped",   pyo_rx_dropped, 
            "tx_dropped",   pyo_tx_dropped, 
            "rx_errors",    pyo_rx_errors, 
            "tx_errors",    pyo_tx_errors, 
            "rx_frame_err", pyo_rx_frame_err, 
            "rx_over_err",  pyo_rx_over_err, 
            "rx_crc_err",   pyo_rx_crc_err, 
            "collisions",   pyo_collisions );

    // references are held by the dictionary, so 
    // we can decref...  Fall through

error:    
    Py_XDECREF(pyo_rx_packets);
    Py_XDECREF(pyo_tx_packets);
    Py_XDECREF(pyo_rx_bytes);
    Py_XDECREF(pyo_tx_bytes);
    Py_XDECREF(pyo_rx_dropped);
    Py_XDECREF(pyo_tx_dropped);
    Py_XDECREF(pyo_rx_errors);
    Py_XDECREF(pyo_tx_errors);
    Py_XDECREF(pyo_rx_frame_err);
    Py_XDECREF(pyo_rx_over_err);
    Py_XDECREF(pyo_rx_crc_err);
    Py_XDECREF(pyo_collisions);

    return ret; // will be NULL on error 
}

template <>
PyObject*
to_python(const ofp_flow_mod& m) 
{
    PyObject* dict = PyDict_New();
    if (!dict) {
        return 0;
    }
    pyglue_setdict_string(dict, "match", to_python(m.match));
    pyglue_setdict_string(dict, "command", to_python(ntohs(m.command)));
    pyglue_setdict_string(dict, "idle_timeout",
                          to_python(ntohs(m.idle_timeout)));
    pyglue_setdict_string(dict, "hard_timeout",
                          to_python(ntohs(m.hard_timeout)));
    pyglue_setdict_string(dict, "buffer_id", to_python(ntohl(m.buffer_id)));
    pyglue_setdict_string(dict, "priority", to_python(ntohs(m.priority)));
    pyglue_setdict_string(dict, "cookie", to_python(ntohll(m.cookie)));
    /* XXX actions */
    return dict;
}

template <>
PyObject*
to_python(const ofp_flow_stats& fs)
{
    PyObject* dict = PyDict_New();
    if (!dict) {
        return 0;
    }
    pyglue_setdict_string(dict, "table_id", to_python(fs.table_id));
    pyglue_setdict_string(dict, "match", to_python(fs.match));
    pyglue_setdict_string(dict, "cookie", to_python(ntohll(fs.cookie)));
    pyglue_setdict_string(dict, "duration_sec", to_python(ntohl(fs.duration_sec)));
    pyglue_setdict_string(dict, "duration_nsec", to_python(ntohl(fs.duration_nsec)));
    pyglue_setdict_string(dict, "priority", to_python(ntohs(fs.priority)));
    pyglue_setdict_string(dict, "idle_timeout",
                          to_python(ntohs(fs.idle_timeout)));
    pyglue_setdict_string(dict, "hard_timeout",
                          to_python(ntohs(fs.hard_timeout)));
    pyglue_setdict_string(dict, "packet_count",
                          to_python(ntohll(fs.packet_count)));
    pyglue_setdict_string(dict, "byte_count",
                          to_python(ntohll(fs.byte_count)));
    /* Actions are not included, since ofp_flow_stats only includes them as a
     * trailing variable-length array and it would be too risky to assume that
     * that data was available.  Use Flow_stats instead, which does the right
     * thing for C++. */
    return dict;
}

template <>
PyObject*
to_python(const Flow_stats& fs)
{
    PyObject* dict = to_python(static_cast<const ofp_flow_stats&>(fs));
    /* XXX actions */
    return dict;
}

template <>
PyObject*
to_python(const ofp_match& m)
{
    PyObject* dict = PyDict_New();
    if (!dict) {
        return 0;
    }

    /* XXX the literal strings below should have macro names, e.g. DL_VLAN,
     * used in common with pyapi.py. */

    uint32_t wildcards = ntohl(m.wildcards);
    if (!(wildcards & OFPFW_IN_PORT)) {
        pyglue_setdict_string(dict, "in_port", to_python(ntohs(m.in_port)));
    }
    if (!(wildcards & OFPFW_DL_SRC)) {
        pyglue_setdict_string(dict, "dl_src", to_python(ethernetaddr(m.dl_src)));
    }
    if (!(wildcards & OFPFW_DL_DST)) {
        pyglue_setdict_string(dict, "dl_dst", to_python(ethernetaddr(m.dl_dst)));
    }
    if (!(wildcards & OFPFW_DL_VLAN)) {
        pyglue_setdict_string(dict, "dl_vlan", to_python(ntohs(m.dl_vlan)));
    }
    if (!(wildcards & OFPFW_DL_VLAN_PCP)) {
        pyglue_setdict_string(dict, "dl_vlan_pcp", to_python(ntohs(m.dl_vlan_pcp)));
    }
    if (!(wildcards & OFPFW_DL_TYPE)) {
        pyglue_setdict_string(dict, "dl_type", to_python(ntohs(m.dl_type)));
    }
    int nw_src_n_wild = (wildcards & OFPFW_NW_SRC_MASK) >> OFPFW_NW_SRC_SHIFT;
    if (nw_src_n_wild < 32) {
        pyglue_setdict_string(dict, "nw_src", to_python(ntohl(m.nw_src)));
        pyglue_setdict_string(dict, "nw_src_n_wild", to_python(nw_src_n_wild));
    }
    int nw_dst_n_wild = (wildcards & OFPFW_NW_DST_MASK) >> OFPFW_NW_DST_SHIFT;
    if (nw_dst_n_wild < 32) {
        pyglue_setdict_string(dict, "nw_dst", to_python(ntohl(m.nw_dst)));
        pyglue_setdict_string(dict, "nw_dst_n_wild", to_python(nw_dst_n_wild));
    }
    if (!(wildcards & OFPFW_NW_TOS)) {
        pyglue_setdict_string(dict, "nw_tos", to_python(m.nw_tos));
    }
    if (!(wildcards & OFPFW_NW_PROTO)) {
        pyglue_setdict_string(dict, "nw_proto", to_python(m.nw_proto));
    }
    if (!(wildcards & OFPFW_TP_SRC)) {
        pyglue_setdict_string(dict, "tp_src", to_python(ntohs(m.tp_src)));
    }
    if (!(wildcards & OFPFW_TP_DST)) {
        pyglue_setdict_string(dict, "tp_dst", to_python(ntohs(m.tp_dst)));
    }
    return dict;
}

template <>
PyObject*
to_python(const std::vector<Table_stats>& p)
{
    PyObject *list = PyList_New(0);
    for (int i=0; i<p.size(); ++i) {
        PyObject *ts = to_python(p[i]);
        if (PyList_Append(list, ts) < 0) {
            Py_XDECREF(list);
            Py_XDECREF(ts);
            return NULL;
        }
        Py_XDECREF(ts);
    }
    return list;
}

template <>
PyObject*
to_python(const std::vector<Port_stats>& p)
{
    PyObject *list = PyList_New(0);
    for (int i=0; i<p.size(); ++i) {
        PyObject *ts = to_python(p[i]);
        if (PyList_Append(list, ts) < 0) {
            Py_XDECREF(list);
            Py_XDECREF(ts);
            return NULL;
        }
        Py_XDECREF(ts);
    }
    return list;
}

template <>
PyObject*
to_python(const std::vector<Port>& p)
{
    PyObject *list = PyList_New(0);
    for (int i=0; i<p.size(); ++i) {
        PyObject *port = to_python(p[i]);
        if (PyList_Append(list, port) < 0) {
            Py_XDECREF(list);
            Py_XDECREF(port);
            return NULL;
        }
        Py_XDECREF(port);
    }
    return list;
}

template <>
PyObject*
to_python(const ethernetaddr& addr) 
{
    return to_python(addr.hb_long());
}

template <>
PyObject*
to_python(const ipaddr& addr) 
{
    return to_python(ntohl(addr.addr));
}

template <>
PyObject*
to_python(const datapathid& id) 
{
    return to_python(id.as_host());
}

template <>
PyObject*
to_python(const Flow& flow)
{
    static PyObject *m = PyImport_ImportModule("nox.lib.netinet.netinet");
    if (!m) {
        throw std::runtime_error("Could not retrieve NOX Python "
                                 "Netinet module" +
                                 vigil::applications::pretty_print_python_exception());
    }

    Flow *f = new Flow(flow);
    static swig_type_info *s =
        SWIG_TypeQuery("_p_Flow");
    if (!s) {
        throw std::runtime_error("Could not find Flow SWIG type_info");
    }
    return SWIG_Python_NewPointerObj(f, s, SWIG_POINTER_OWN | 0);

//     PyObject* dict = PyDict_New();
//     if (!dict) {
//         return 0;
//     }

//     pyglue_setdict_string(dict, "in_port", to_python(ntohs(flow.in_port)));
//     pyglue_setdict_string(dict, "dl_vlan", to_python(ntohs(flow.dl_vlan)));
//     pyglue_setdict_string(dict, "dl_vlan_pcp", to_python(ntohs(flow.dl_vlan_pcp)));
//     pyglue_setdict_string(dict, "dl_src", to_python(flow.dl_src));
//     pyglue_setdict_string(dict, "dl_dst", to_python(flow.dl_dst));
//     pyglue_setdict_string(dict, "dl_type", to_python(ntohs(flow.dl_type)));
//     pyglue_setdict_string(dict, "nw_src", to_python(ntohl(flow.nw_src)));
//     pyglue_setdict_string(dict, "nw_dst", to_python(ntohl(flow.nw_dst)));
//     pyglue_setdict_string(dict, "nw_proto", to_python(flow.nw_proto));
//     pyglue_setdict_string(dict, "tp_src", to_python(ntohs(flow.tp_src)));
//     pyglue_setdict_string(dict, "tp_dst", to_python(ntohs(flow.tp_dst)));

//     return dict;
}

template <>
void
call_python_function(boost::intrusive_ptr<PyObject> callable,
                     boost::intrusive_ptr<PyObject> args)
{
    Co_critical_section in_critical_section;

    PyObject *pyret = PyObject_CallObject(callable.get(), args.get());
    Py_XDECREF(pyret);

    if (!pyret) {
        const string exc = 
            vigil::applications::pretty_print_python_exception();
        vlog().
            log(vlog().get_module_val("pyrt"), Vlog::LEVEL_ERR, 
                "unable to call a Python function/method:\n%s", exc.c_str());
    }
}

} // namespace vigil

#endif /* TWISTED_ENABLED */
