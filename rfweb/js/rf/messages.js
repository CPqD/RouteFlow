RFCLIENT_RFSERVER = 0
RFSERVER_RFPROXY = 1

channels = [
    {"id": RFCLIENT_RFSERVER,
     "name": "rfclient<->rfserver",
     "html_id": "rfclient_rfserver",
     "messages": {
        0: {"name": "PortRegister", "show": true},
        1: {"name": "PortConfig", "show": true},
        6: {"name": "RouteMod", "show": false},        
    }},
    
    {"id": RFSERVER_RFPROXY,
     "name": "rfserver<->rfproxy",
     "html_id": "rfserver_rfproxy",
     "messages": {
        2: {"name": "DatapathPortRegister", "show": true},
        3: {"name": "DatapathDown", "show": true},
        4: {"name": "VirtualPlaneMap", "show": true},
        5: {"name": "DataPlaneMap", "show": true},
        6: {"name": "RouteMod", "show": false},       
    }}, 
];

rowtemplate = "<tr class=\"row bg\{style}\">"
rowtemplate += "<td class=\"expand_control\" id=\"msg_{id}_expand\" onclick=\"toggle('msg_{id}')\"><span class=\"ui-icon ui-icon-triangle-1-e\">Details</span></td>";
rowtemplate += "<td>{from}</td>"
rowtemplate += "<td>{to}</td>"
rowtemplate += "<td>{type}</td>"
rowtemplate += "<td>{status}</td>"
rowtemplate += "<tr id=\"msg_{id}_content\" class=\"hidden_msg_content\">"
rowtemplate += "<td></td>"
rowtemplate += "<td class=\"message_content_area\" colspan=\"4\">"
rowtemplate += "{content}"
rowtemplate += "</td>"
rowtemplate += "</tr>"
rowtemplate += "</tr>"

cbtemplate = "<input type=\"checkbox\" name=\"{type}\" {checked} value=\"{value}\">{name}<br />"

function process_message(i, channel, msg) {
    msg.id = channel.html_id + "_msg_" + i;
    
    msg.type = channel.messages[msg.type].name;

    msg.status = "read";
    if (!msg.read)
        msg.status = "unread";
    msg.content = "<pre class='message_content'>" + msg.content + "</pre>";

    msg.style = i % 2;
}

function update_table(channel) {
    var table = channel.html_id;
    var msgprefix = table + "_type";
    
    var table = $("#" + table);
    table.html("");
    
    var msgtypes = $('input[name=' + msgprefix + ']:checked').map(function() {
        return this.value;
    }).get();
    if (msgtypes != "")
        var request = channel.name + "?types=" + msgtypes;
    else
        return;
        
    $.ajax({
        url: "messages/" + request,
        dataType: 'json',
        success: function (data) {
            for (var i in data) {
                process_message(i, channel, data[i]);
                table.html(table.html() + apply_template(rowtemplate, data[i]));
            }
        }
    });
}

function update() {
    for (var i in channels)
        update_table(channels[i]);
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
    for (var i in channels) {
        var channel = channels[i];
        var element = $("#" + channel.html_id + "_filters");
        var input_name = channel.html_id + "_type";
        
        for (var msg_id in channel.messages) {
            var message = channel.messages[msg_id];
            var checked = "";
            if (message.show)
                checked = "checked=\"checked\"";
            
            element.html(element.html() + apply_template(cbtemplate,
                                                         {"value": msg_id,
                                                         "name": message.name,
                                                         "type": input_name,
                                                         "checked": checked}));
        }

        $('input[name=' + input_name + ']').click(function() { update(); });
    }
}

function messages_init() {
	start();
	update();
}

function messages_stop() {
}
