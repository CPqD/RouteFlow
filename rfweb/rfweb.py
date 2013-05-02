#!/usr/bin/env python
#-*- coding:utf-8 -*-

# WARNING: this web application is just a toy and should not be used for 
# production purposes.

# TODO: make a more serious and configurable web application. It could even be
# based on this one.

from cgi import parse_qs, escape
import json
import urlparse
from wsgiref.util import shift_path_info
import pymongo
import bson.json_util
import os
import os.path

import sys
sys.path.append("../")

import rflib.defs as defs
import rflib.ipc.MongoIPC as MongoIPC
from rflib.ipc.RFProtocolFactory import RFProtocolFactory

 
def usage():
    print('Usage: %s <channel>\n' % sys.argv[0])
    print('channel: "rfclient" or "rfproxy"')
    sys.exit(1)

PLAIN = 0
HTML = 1
JSON = 2
CSS = 3
JS = 4
GIF = 5
PNG = 6
JPEG = 7

CONTENT_TYPES = {
PLAIN: "text/plain",
HTML: "text/html",
JSON: "application/json",
CSS: "text/css",
JS: "text/javascript",
GIF: "image/gif",
PNG: "image/png",
JPEG: "image/jpeg",
}

exts = {
".js": JS,
".html": HTML,
".css": CSS,
".png": PNG,
".gif": GIF,
".jpg": JPEG,
".jpeg": JPEG,
".json": JSON,
}

db_conn = None

# Convert a possible ID string to hex.
# This differs from defs.format_id because it only tries to convert if the value
# can be represented as an integer
def format_id(value):
    try:
        value = int(value)
        return defs.format_id(value)
    except ValueError, TypeError:
        return value
    
# TODO: make this function generate pretty output for any given message.
# An alternative would be changing the auto-generated IPC message code to 
# generate prettier messages itself, but it's a longer task.
factory = RFProtocolFactory()
def prettify_message(envelope):
    result = {}

    result[MongoIPC.FROM_FIELD] = format_id(envelope[MongoIPC.FROM_FIELD])    
    result[MongoIPC.TO_FIELD] = format_id(envelope[MongoIPC.TO_FIELD])
    result[MongoIPC.READ_FIELD] = envelope[MongoIPC.READ_FIELD]
    result[MongoIPC.TYPE_FIELD] = envelope[MongoIPC.TYPE_FIELD]
    
    msg = MongoIPC.take_from_envelope(envelope, factory)
    result[MongoIPC.CONTENT_FIELD] = str(msg)

    return result
    
def messages(env, conn):
    channel = shift_path_info(env)
    if channel != None and channel != "":
        # TODO: escape parameters
        messages = []
        try:
            table = getattr(conn.db, channel);
        except:
            return (404, "Invalid channel", JSON)

        request = parse_qs(env["QUERY_STRING"])
        limit = 50
        query = {}
        if "limit" in request:
            limit = request["limit"][0]
            
        if "types" in request:
            types = request["types"][0]
            if types == "":
                return (200, json.dumps({}, default=bson.json_util.default), JSON)
            query["$or"] = []
            for type_ in types.split(","):
                try:
                    value = int(type_)
                except:
                    continue
                query["$or"].append({"type": value})

        for envelope in table.find(query, limit=limit, sort=[("$natural", pymongo.DESCENDING)]):
            messages.append(prettify_message(envelope))

        return (200, json.dumps(messages, default=bson.json_util.default), JSON)


    else:
        return (404, "Channel not specified", JSON)


def switch(env, conn):
    id_ = shift_path_info(env)
    if id_ != None and id_ != "":
        switch = conn.db.rfstats.find_one(id_)
        return (200, json.dumps(switch["data"], default=bson.json_util.default), JSON)
    else:
        return (404, "Switch not specified", JSON)

def topology(env, conn):
    elements_list = []
    elements = conn.db.rfstats.find()
    for element in elements:
        element["id"] = element["_id"]
        del element["_id"]
        del element["data"]
        elements_list.append(element)
    return (200, json.dumps(elements_list, default=bson.json_util.default), JSON)
    
def pretiffy_rftable_entry(entry):
    result = {}
        
    # TODO: these field names should be defined somewhere else
    result["vm_id"] = format_id(entry["vm_id"])
    result["vm_port"] = int(entry["vm_port"])
    result["dp_id"] = format_id(entry["dp_id"])
    result["dp_port"] = int(entry["dp_port"])
    result["vs_id"] = format_id(entry["vs_id"])
    result["vs_port"] = int(entry["vs_port"])
    result["ct_id"] = format_id(entry["ct_id"])
    
    return result
    
def rftable(env, conn):
    entries = []
    for doc in conn.db.rftable.find():
        entries.append(pretiffy_rftable_entry(doc))
    return (200, json.dumps(entries, default=bson.json_util.default), JSON)


def application(env, start_response):
    path = shift_path_info(env)
    request = parse_qs(env["QUERY_STRING"])
    try:
        callback = request["callback"][0]
    except KeyError:
        callback = None
    
    global db_conn
    if db_conn is None:
        try:
            # TODO: use defs.py
            db_conn = pymongo.Connection("localhost", 27017)
        except:
            db_conn = None
    
    status = 404
    rbody = ""
    ctype = PLAIN

    if (path == "rftable"):
        status, rbody, ctype = rftable(env, db_conn)
    elif (path == "topology"):
        status, rbody, ctype = topology(env, db_conn)        
    elif (path == "switch"):
        status, rbody, ctype = switch(env, db_conn)
    elif (path == "messages"):
        status, rbody, ctype = messages(env, db_conn)
    else:
        path = os.path.join(os.getcwd(), path + env["PATH_INFO"])
        if os.path.exists(path) and os.path.isfile(path):
            f = open(path, "r");
            status = 200
            rbody = f.read()
            ctype = exts[os.path.splitext(path)[1]]
            f.close()
        else:
            status = 200
            rbody = "JSON requests:\n" \
                    "GET /rftable: RouteFlow table\n" \
                    "GET /topology: network topology\n" \
                    "GET /switch/[id]: stats and flows for switch [id]\n" \
                    "GET /messages/[channel]: messages in channel [channel]\n" \
                    "\n" \
                    "Pages:\n" \
                    "GET /index.html: main page\n"

    if ctype == JSON and callback is not None:
        rbody = "{0}({1})".format(callback, rbody)

    if (status == 404):
        start_response("404 Not Found", [("Content-Type", CONTENT_TYPES[ctype])])
    elif (status == 200):
        start_response("200 OK", [("Content-Type", CONTENT_TYPES[ctype]),
                                  ("Content-Length", str(len(rbody)))])

    return [rbody]
