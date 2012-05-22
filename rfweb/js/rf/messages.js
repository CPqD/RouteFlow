RFCLIENT_RFSERVER = 0
RFSERVER_RFPROXY = 1

types = {
    0: {
        "name": "VMRegisterRequest",
        "show": true,
        "channel": RFCLIENT_RFSERVER,
    },
    
    1: {
        "name": "VMRegisterResponse",
        "show": true,
        "channel": RFCLIENT_RFSERVER,
    },
    
    2: {
        "name": "VMConfig",
        "show": true,
        "channel": RFCLIENT_RFSERVER,
    },
    
    3: {
        "name": "DatapathConfig",
        "show": false,
        "channel": RFSERVER_RFPROXY,
    },
    
    4: {
        "name": "RouteInfo",
        "show": false,
        "channel": RFCLIENT_RFSERVER,
    },
    
    5: {
        "name": "FlowMod",
        "show": false,
        "channel": RFSERVER_RFPROXY,
    },
    
    6: {
        "name": "DatapathJoin",
        "show": true,
        "channel": RFSERVER_RFPROXY,
    },
    
    7: {
        "name": "DatapathLeave",
        "show": true,
        "channel": RFSERVER_RFPROXY,
    },
    
    8: {
        "name": "VMMap",
        "show": false,
        "channel": RFSERVER_RFPROXY,
    },
}

rowtemplate = "<tr class=\"row bg\{style}\">"
rowtemplate += "<td class=\"expand_control\" id=\"msg_{id}_expand\" onclick=\"toggle('msg_{id}')\"><span class=\"ui-icon ui-icon-triangle-1-e\">Details</span></td>";
rowtemplate += "<td>{to}</td>"
rowtemplate += "<td>{status}</td>"
rowtemplate += "<td>{type}</td>"
rowtemplate += "<tr id=\"msg_{id}_content\" class=\"hidden_msg_content\">"
rowtemplate += "<td></td>"
rowtemplate += "<td class=\"message_content\" colspan=\"3\">"
rowtemplate += "{content}"
rowtemplate += "</td>"
rowtemplate += "</tr>"
rowtemplate += "</tr>"

cbtemplate = "<input type=\"checkbox\" name=\"types\" {checked} value=\"{value}\">{name}<br />"

function process_message(i, msgprefix, msg) {
    msg["id"] = msgprefix + i;
    msg["type"] = types[msg["type"]].name;
    
    var to = toHex(parseInt(msg["to"]), 64);
    if (to != "")
        msg["to"] = to;
        
    if (msg["read"])
        msg["status"] = "read"
    else
        msg["status"] = "unread"
        
    var content = "";
    for (var attr in msg["content"]) {
        if (attr.match(/_id$/))
            content += attr + ": " + toHex(parseInt(msg["content"][attr]), 64) + "<br />";
        else
            content += attr + ": " + msg["content"][attr] + "<br />";
    }
    msg["content"] = content;
    msg["style"] = i % 2;
}

function update_table(table, msgprefix) {
    var msgtypes = $('input[name=types]:checked').map(function() {
        return this.value;
    }).get();
    
    var request = table.replace("_", "<->");
    request += "?types=" + msgtypes;
    
    var table = $("#" + table);
    table.html("");
    
    $.ajax({
        url: "messages/" + request,
        dataType: 'json',
        success: function (data) {
            for (var i in data) {
                process_message(i, msgprefix, data[i]);
                table.html(table.html() + apply_template(rowtemplate, data[i]));
            }
        }
    });
}

function update() {
    update_table("rfclient_rfserver", "ss");
    update_table("rfserver_rfproxy", "sc");
}

function toggle(id) {
    var el = document.getElementById(id + "_content");
    var ex = document.getElementById(id + "_expand");
    if (el.getAttribute("class") == "hidden_msg_content") {
        el.setAttribute("class", "msg_content");
        ex.innerHTML = "<span class=\"ui-icon ui-icon-triangle-1-s\">Details</span>";
    }
    else {
        el.setAttribute("class", "hidden_msg_content");
        ex.innerHTML = "<span class=\"ui-icon ui-icon-triangle-1-e\">Details</span>";
    }
}

function start() {
    var rfclient_rfserver_filters = $("#rfclient_rfserver_filters");
    var rfserver_rfproxy_filters = $("#rfserver_rfproxy_filters");
    var filters;
    for (type in types) {
        if (types[type].channel == RFCLIENT_RFSERVER)
            filters = rfclient_rfserver_filters;
        else if (types[type].channel == RFSERVER_RFPROXY)
            filters = rfserver_rfproxy_filters;
        else
            continue;
        
        var checked = "";
        if (types[type].show)
            checked = "checked=\"checked\"";
        
        filters.html(filters.html() + apply_template(cbtemplate, {"value": type, "name": types[type].name, "checked": checked}));
    }
    
    $('input[name=types]').click(function() { update(); });
}

function messages_init() {
	start();
	update(); 
}

function messages_stop() {
}
