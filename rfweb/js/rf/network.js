ICON_SIZE = 128;
LABEL_SIZE = 12;
SPACING = 12;

SWITCH_ICON = new Image();
SWITCH_ICON.src = 'images/icons/switch.png';

RFSERVER_ICON = new Image();
RFSERVER_ICON.src = 'images/icons/server_a.png';

RFCONTROLLER_ICON = new Image();
RFCONTROLLER_ICON.src = 'images/icons/server_b.png';

var labelType, useGradients, nativeTextSupport, animate;
var rgraph = undefined;
var switches_info;
var tableMatches = {};
var previous_pos = {};
var selected_node = undefined;
var interval_id;

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
   
$jit.RGraph.Plot.NodeTypes.implement({
    'of-switch': {
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
    
    'rf-server': {
        'render': function(node, canvas) {
            var pos = node.pos.getc(true);
            canvas.getCtx().drawImage(RFSERVER_ICON, pos.x - ICON_SIZE/2, pos.y - ICON_SIZE/2, ICON_SIZE, ICON_SIZE);
        },
        
        'contains': function (node, pos) {
            return this.nodeHelper.rectangle.contains(node.pos.getc(true), pos, ICON_SIZE, ICON_SIZE);
        }
    },

    'controller': {
        'render': function(node, canvas) {
            var pos = node.pos.getc(true);
            canvas.getCtx().drawImage(RFCONTROLLER_ICON, pos.x - ICON_SIZE/2, pos.y - ICON_SIZE/2, ICON_SIZE, ICON_SIZE);
        },
        
        'contains': function (node, pos) {
            return this.nodeHelper.rectangle.contains(node.pos.getc(true), pos, ICON_SIZE, ICON_SIZE);
        }
    },
});

function show_info(node) {
    var contMatch = new Array();
    var contActions = new Array();
    var flowsActions = new Array();
	
	// Find the switch info
	var sw = undefined;
	for (var i in switches_info) {
	    if (switches_info[i]["id"] == node.id) {
	        sw = switches_info[i];
	        break;
	    }
	}
	if (sw == undefined)
	    return;

	// Build the right column statistics
	var html = "<div class='section'><div class='section_title'>" + node.name + "</div>";
	
	//ofp_desc_stats
	var switchData = sw["data"]['$ofp_desc_stats'];
	if(switchData) {
		html += "<div class='subsection'><div class='subsection_title'>Description</div><ul class=\"switchdesc\"><li>", list = [];
		list.push("Manufacturer: " + switchData[0]);
		list.push("Hardware descrpition: " + switchData[1]);
		list.push("Software description: " + switchData[2]);
		list.push("Serial number: " + switchData[3]);
		list.push("Datapath description: " + switchData[4]);
		html = html + list.join("</li><li>") + "</li></ul></div>";
	
	
	//ofp_aggregate_stats
	switchData = sw["data"]['$ofp_aggr_stats'];
	if(switchData) {
		html = html + "<div class='subsection'><div class='subsection_title'>Aggregated statistics</div><ul class=\"switchdesc\"><li>", list = [];
		list.push("Packet count: " + switchData[0]);
		list.push("Byte count: " + switchData[1]);
		list.push("Flow count: " + switchData[2]);
		html = html + list.join("</li><li>") + "</li></ul></div>";
	}
	
	//ofp_table_stats
	switchData = sw["data"]['$ofp_table_stats'];
	html = html + "<div class='subsection'><div class='subsection_title'>Table statistics</div><ul class=\"switchdesc\"><li>", list = [];
	if(switchData) {
		list.push("Table ID: " + switchData[0][0]);
		list.push("Name: " + switchData[0][1]);
		list.push("Active count: " + switchData[0][2]);
		list.push("Lookup count: " + switchData[0][3]);
		html = html + list.join("</li><li>") + "</li></ul></div>";
	}
	html += "</div>"
	// display information
	$('#switchinfo').html(html);
	
	// Build the bottom flow table
	var flows = sw["data"]['$flows'];		
	html = "<table>" +
	    "<tr class='header'>" +
		    "<td>#</td>" +
		    "<td>Matches</td>" +
		    "<td>Actions</td>" +
		    "<td>Packets</td>" +
		    "<td>Bytes</td>" +
	    "</tr>";
	var matches = "";

    var rowtemplate = "<tr class=\"bg\{style}\">"
    rowtemplate += "<td class=\"{status}\">{flow}</td>"
    rowtemplate += "<td>{matches}</td>"
    rowtemplate += "<td>{actions}</td>"
    rowtemplate += "<td>{packets}</td>"
    rowtemplate += "<td>{bytes}</td>"
    rowtemplate += "</tr>"
    
    var count = 0;
    var status = "";
	for(var i in flows) {
		var f = flows[i];
		
		var actions = f.ofp_actions[0];
		for (j in f.ofp_actions) {
			if(j!=0 && f.ofp_actions[j] != 'type' && f.ofp_actions[j] != 'len')
				actions = actions + f.ofp_actions[j];
		}
		if(tableMatches[node.id]!=undefined && tableMatches[node.id].search(f.ofp_match)!=-1) {
			//fluxo ja estah na flow table mas eh recente
			if(f.ofp_match in contMatch && contMatch[f.ofp_match] > 0) {
                --contMatch[f.ofp_match];
                status = "flow-kinda-new";
			// fluxo estah na flow table ha tempo
            } else {
				delete contMatch[f.ofp_match];
				// as acoes para este fluxo mudaram
				if(flowsActions && flowsActions[f.ofp_match] != undefined && flowsActions[f.ofp_match] != actions) {
						contActions[f.ofp_match] = 5;
					    status = "flow-action-changed";
				} else {
					if(contActions[f.ofp_match] && contActions[f.ofp_match] > 0) {
                        --contActions[f.ofp_match];
                    }
                    if(contActions[f.ofp_match] > 0) {
                        status = "flow-action-changed";
                    } else {
						delete contActions[f.ofp_match];
					}
				}
			}
		// fluxo acaba de entrar na flow table
		} else {
			contMatch[f.ofp_match] = 5;
			status = "flow-new";
		}
		flowsActions[f.ofp_match] = actions;
		matches = matches + f.ofp_match;

        var data = {
            "flow": f.flow,
            "matches": f.ofp_match,
            "actions": actions,
            "packets": f.packet_count,
            "bytes": f.byte_count,
            "status": status,
            "style": count % 2,
        }
        html += apply_template(rowtemplate, data);
		count = count + 1;
	}
	tableMatches[node.id] = matches;
	html = html + "</table>";
	
	$('#flows').html(html);
	}
}

function network_start_updating() {
    interval_id = setInterval("network_update()", 1000);
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
                    show_info(node);
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

function network_update() {
    $.getJSON("data/topology.json",
        function(data) {
            if (data == null || data == undefined)
                return;
                
            // Pre-process data
            for (node in data["nodes"]) {
                if (data["nodes"][node]["data"]["$type"] == "of-switch")
                    data["nodes"][node]["data"]["$height"] = ICON_SIZE/5 + SPACING + LABEL_SIZE;
            }
            
            // If the graph hasn't been built yet, build it
            if (rgraph == undefined) {
                build();
                rgraph.loadJSON(data["nodes"], 1);
                rgraph.refresh();
                rgraph.canvas.scale(0.7, 0.7);
                return;
            }

            // Save old node positions
            rgraph.graph.eachNode(function(node) {
                previous_pos[node.id] = node.getPos();
            });
            
            // Update
            rgraph.loadJSON(data["nodes"]);
            rgraph.refresh();
            
            // Restore old positions
            rgraph.graph.eachNode(function(node) {
                if (node.id in previous_pos) {
                    node.setPos(previous_pos[node.id]);
                }
            });
            rgraph.plot();
            
            if (selected_node != undefined) {
                show_info(rgraph.graph.getNode(selected_node));
            }
    });
    
    $.getJSON("data/switchstats.json",
        function(data) {
            switches_info = data["nodes"];
    });
}

function network_init() {
    network_start_updating();
    network_update();
}

function network_stop() {
	network_stop_updating();
}
