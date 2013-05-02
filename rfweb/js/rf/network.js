ICON_SIZE = 128;
LABEL_SIZE = 12;
SPACING = 12;

SWITCH_ICON = new Image();
SWITCH_ICON.src = 'images/icons/switch.png';

RFSERVER_ICON = new Image();
RFSERVER_ICON.src = 'images/icons/server_a.png';

RFCONTROLLER_ICON = new Image();
RFCONTROLLER_ICON.src = 'images/icons/server_b.png';

// Browser compatibility stuff
var labelType, useGradients, nativeTextSupport, animate;

var rgraph = undefined; // Network graph
var previous_pos = {}; // Keep the positions when updating
var selected_node = undefined; // Selected node id
var interval_id; // Update timer

// From: http://thejit.org/static/v20/Jit/Examples/RGraph/example1.js
(function() {
  var ua = navigator.userAgent,
      iStuff = ua.match(/iPhone/i) || ua.match(/iPad/i),
      typeOfCanvas = typeof HTMLCanvasElement,
      nativeCanvasSupport = (typeOfCanvas == 'object' || typeOfCanvas == 'function'),
      textSupport = nativeCanvasSupport
        && (typeof document.createElement('canvas').getContext('2d').fillText == 'function');
  //I'm setting this based on the fact that ExCanvas provides text support for IE
  //and that as of today iPhone/iPad current text support is lame
  labelType = (!nativeCanvasSupport || (textSupport && !iStuff))? 'Native' : 'HTML';
  nativeTextSupport = labelType == 'Native';
  useGradients = nativeCanvasSupport;
  animate = !(iStuff || !nativeCanvasSupport);
})();
//

$jit.RGraph.Plot.NodeTypes.implement({
    'switch': {
        'render': function(node, canvas) {
            var pos = node.pos.getc(true);
            canvas.getCtx().drawImage(SWITCH_ICON,
                                      0, ICON_SIZE/2.5,             // sx, sy
                                      ICON_SIZE, ICON_SIZE/5,       // sWidth, sHeight
                                      pos.x - ICON_SIZE/2, pos.y - (ICON_SIZE/5)/2, // dx, dy
                                      ICON_SIZE, ICON_SIZE/5);      // dWidth, dHeight
        },

        'contains': function (node, pos) {
            return this.nodeHelper.rectangle.contains(node.pos.getc(true), pos, ICON_SIZE, ICON_SIZE);
        },
    },

    'rfserver': {
        'render': function(node, canvas) {
            var pos = node.pos.getc(true);
            canvas.getCtx().drawImage(RFSERVER_ICON, pos.x - ICON_SIZE/2, pos.y - ICON_SIZE/2, ICON_SIZE, ICON_SIZE);
        },

        'contains': function (node, pos) {
            return this.nodeHelper.rectangle.contains(node.pos.getc(true), pos, ICON_SIZE, ICON_SIZE);
        }
    },

    'rfproxy': {
        'render': function(node, canvas) {
            var pos = node.pos.getc(true);
            canvas.getCtx().drawImage(RFCONTROLLER_ICON, pos.x - ICON_SIZE/2, pos.y - ICON_SIZE/2, ICON_SIZE, ICON_SIZE);
        },

        'contains': function (node, pos) {
            return this.nodeHelper.rectangle.contains(node.pos.getc(true), pos, ICON_SIZE, ICON_SIZE);
        }
    },
});

function show_info(node, data) {
	var info = "";
	var table = "";

    // RFServer
    if (node.data.$type == "rfserver") {
        info = "<div class=\"tips\"><p>The RouteFlow server manages all the associations between the virtual and physical environments.</p><p>It has full control over the FIB and RIB, and services can be built on top of it.</p></div>";
        table = "";
    }
    // RFProxy
    else if (node.data.$type == "rfproxy") {
        info = "<div class=\"tips\"><p>The RouteFlow proxy is the controller application that configures basic flows in the switches and redirects routing traffic to the RouteFlow clients.</p><p>It also provides an interface so that the RouteFlow server can manage the flows.</p></div></div>";
        table = "";
    }
    // Switches
    else if (node.data.$type == "switch") {
        // RFVS
        if (is_rfvs(node.id))
            info = "<div class=\"tips\"><p>A RouteFlow virtual switch (RFVS) connects RouteFlow clients.</p></div>";
    	var list = [];

	    // TODO: use a loop to automate this task
	    // ofp_desc_stats
	    list = [];
	    if (data.desc != undefined) {
		    info = info + "<div class='subsection'><div class='subsection_title'>Description</div><ul class=\"switchdesc\"><li>";
		    list.push("Manufacturer: " + data.desc.mfr_desc);
		    list.push("Hardware descrpition: " + data.desc.hw_desc);
		    list.push("Software description: " + data.desc.sw_desc);
		    list.push("Serial number: " + data.desc.serial_num);
		    list.push("Datapath description: " + data.desc.dp_desc);
		    info = info + list.join("</li><li>") + "</li></ul></div>";
	    }

	    // ofp_aggregate_stats
	    list = [];
	    if (data.aggregate != undefined) {
		    info = info + "<div class='subsection'><div class='subsection_title'>Aggregated statistics</div><ul class=\"switchdesc\"><li>";
		    list.push("Packet count: " + data.aggregate.packet_count);
		    list.push("Byte count: " + data.aggregate.byte_count);
		    list.push("Flow count: " + data.aggregate.flow_count);
		    info = info + list.join("</li><li>") + "</li></ul></div>";
	    }

        // TODO?: ofp_table_stats

	    // Build the bottom flow table
	    table = "<table>" +
	        "<tr class='header'>" +
		        "<td>Match</td>" +
		        "<td>Actions</td>" +
		        "<td>Priority</td>" +
		        "<td>Packet/byte count</td>" +
	        "</tr>";
        var rowtemplate = "<tr class=\"bg\{style}\">"
        rowtemplate += "<td>{match}</td>"
        rowtemplate += "<td>{actions}</td>"
        rowtemplate += "<td>{priority}</td>"
        rowtemplate += "<td>{packet_byte_count}</td>"
        rowtemplate += "</tr>"
        
        var rows = [];
        for (var i in data.flows) {
            flow = data.flows[i];
            var values = {
                "match": prettify_match(flow.match),
                "actions": prettify_actions(flow.actions),
                "priority": flow.priority,
                "packet_byte_count": flow.packet_count + "/" + flow.byte_count,
            }
            rows.push(values);
        }
        
        rows.sort(function(a, b) { return compare_objects(a, b, "priority"); });
        rows.reverse();
        for (var i in rows) {
            rows[i].style = i % 2;
            table += apply_template(rowtemplate, rows[i]);
        }
	    table += "</table>";
    }

    // Close everything and display data
    info = "<div class='section'><div class='section_title'>" + node.name + "</div>" + info + "</div>"
    $('#switchinfo').html(info);
	$('#flows').html(table);
}

function prettify_match(match) {
    var string = "";
    for (var m in match) {
        if (m != "wildcards")
            string += m + ": " + flow.match[m] + ", "
    }
    if (string == "")
        string = "All";
    return rstrip(string, ", ");
}

function prettify_actions(actions) {
    function get_params_list(params) {
        var params_list = [];
        var elements = params.split(", ");
        for (var i in elements)
            params_list.push(elements[i].split(": "));
        return params_list;
    }

    var string = "";
    var re = /(\w*)\[([^\]]*)\]/;
    for (var i in actions) {
        var result = actions[i].match(re);
        if (result == null || result == undefined || result.length < 3) {
            string += actions[i] + ", ";
            continue;
        }

        var action = result[1];
        var params = result[2];

        // ofp_action_output
        if (action == "ofp_action_output") {
            action = "OUTPUT";
            var params_list = get_params_list(params);
            params = ""
            for (var i in params_list) {
                var param = params_list[i][0];
                var value = params_list[i][1];
                // port
                if (param == "port") {
                    if (parseInt(value) == 0xfffd)
                        port = "CONTROLLER";
                    params += "port: " + value + ", ";
                }
            }
        }

        // ofp_action_dl_addr
        else if (action == "ofp_action_dl_addr") {
            action = "";
            var params_list = get_params_list(params);
            params = ""
            for (var i in params_list) {
                var param = params_list[i][0];
                var value = params_list[i][1];
                // type
                if (param == "type") {
                    type = parseInt(value);
                    if (type == 4)
                        action = "SET_DL_SRC";
                    else if (type == 5)
                        action = "SET_DL_DST";
                }
                // dl_addr
                else if (param == "dl_addr") {
                    params += param + ": " + value + ", ";
                }
            }
        }

        // append action
        string += action.toUpperCase() + "(" + rstrip(params, ", ") + ")" + ", ";
    }
    if (string == "")
        string = "DROP";
    return rstrip(string, ", ");
}

function network_start_updating() {
    interval_id = setInterval("network_update()", 5000);
}

function network_stop_updating() {
    clearInterval(interval_id);
}

function build() {
    rgraph = new $jit.RGraph({
      'injectInto': 'infovis',
        Node: {
            'overridable': true,
            'height': ICON_SIZE + SPACING + LABEL_SIZE,
        },

        Edge: {
            'overridable': true,
            'color': '#000000'
        },

        Navigation: {
          enable: true,
          panning: "avoid nodes",
          zooming: 100,
        },

        // If you change style here, change .label in network.css too.
        Label: {
            overridable: false,
            type: labelType,
            style: "bold",
            size: 12,
            family: 'sans-serif',
            textAlign: 'center',
            color: '#000'
        },

        interpolation: 'polar',
        transition: $jit.Trans.linear,
        duration: 250,
        fps: 30,
        levelDistance: 250,

        Events: {
            enable: true,
            type: 'Native',

            onDragMove: function(node, eventInfo, e) {
                network_stop_updating();
                var pos = eventInfo.getPos();
                node.pos.setc(pos.x, pos.y);
                rgraph.plot();
            },

            onDragEnd: function(node, eventInfo, e) {
                network_start_updating();
            },

            onClick: function(node, eventInfo, e) {
                if (node) {
                    selected_node = node.id;
                    node_update(node.id);
                }
            },

            onRightClick: function(node, eventInfo, e) {
                if (node) {
                    network_stop_updating();
                    previous_pos = {};
                    rgraph.onClick(node.id, { hideLabels: false });
                    network_start_updating();
                }
            },
        },

        // HTML labels for compatibility
        onCreateLabel: function(domElement, node){
            domElement.innerHTML = node.name;
            domElement.setAttribute("class", "label");
        },

        onPlaceLabel: function(domElement, node) {
            var style = domElement.style;
            var left = parseInt(style.left);
            var top = parseInt(style.top);
            var w = domElement.offsetWidth;
            style.left = (left - w / 2) + 'px';
            style.top = top + (ICON_SIZE/2) + 'px';
        },
    });
}

function node_update(id) {
    $.getJSON("/switch/" + selected_node,
        function (data) {
            if (data == null || data == undefined)
                return;
            show_info(rgraph.graph.getNode(selected_node), data);
        });
}

function network_update() {
    $.getJSON("/topology",
        function (data) {
            if (data == null || data == undefined)
                return;

            // Pre-process data
            nodes = [];
            var center = 0;
            for (var i in data) {
                var node = data[i]
                node["name"] = node["id"];
                // Mark rfproxy as the central node (used when creating the graph)
                if (node["name"] == "rfproxy")
                    center = i;

                if (node["type"] == "rfserver")
                    node.name = "RFServer"
                else if (node["type"] == "rfproxy")
                    node.name = "RFProxy"
                else if (is_rfvs(node.id))
                    node.name = "RouteFlow virtual switch"

                node["data"] = {};
                if (node["type"] == "switch")
                    node["data"]["$height"] = ICON_SIZE/5 + SPACING + LABEL_SIZE;
                node["data"]["$type"] = node["type"];
                delete node["type"];
                node["adjacencies"] = node["links"];
                delete node["links"];
                nodes.push(node);
            }

            // If the graph hasn't been built yet, build it
            if (rgraph == undefined) {
                build();
                rgraph.loadJSON(nodes, center);
                rgraph.plot();
                rgraph.refresh();
                rgraph.canvas.scale(0.7, 0.7);
                return;
            }

            // Save old node positions
            rgraph.graph.eachNode(function(node) {
                previous_pos[node.id] = node.getPos();
            });

            // Update
            rgraph.loadJSON(nodes);
            rgraph.refresh();

            // Restore old positions
            rgraph.graph.eachNode(function(node) {
                if (node.id in previous_pos) {
                    node.setPos(previous_pos[node.id]);
                }
            });
            rgraph.plot();

            if (selected_node != undefined) {
                node_update(selected_node);
            }
    });
}

function network_init() {
    network_update();
    network_start_updating();
}

function network_stop() {
	network_stop_updating();
}
