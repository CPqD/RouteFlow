rowtemplate = "<tr class=\"bg\{style}\">"
rowtemplate += "<td>{vm_id}</td>"
rowtemplate += "<td>{vm_port}</td>"
rowtemplate += "<td>{vs_id}</td>"
rowtemplate += "<td>{vs_port}</td>"
rowtemplate += "<td>{dp_id}</td>"
rowtemplate += "<td>{dp_port}</td>"
rowtemplate += "</tr>"

function process_entry(i, msg) {
    msg["id"] = i;
    msg["vm_id"] = toHex(parseInt(msg["vm_id"]), 64)
    msg["vs_id"] = toHex(parseInt(msg["vs_id"]), 64)
    msg["dp_id"] = toHex(parseInt(msg["dp_id"]), 64)
    msg["style"] = i % 2;
}

function rftable_init() {
    var table = document.getElementById("rftable");
    $.ajax({
        url: "/rftable",
        dataType: 'jsonp',
        success: function (data) {
            data.sort(function(a, b) { return a["vm_id"] < b["vm_id"]; });
            for (var i in data) {
                process_entry(i, data[i])
                table.innerHTML += apply_template(rowtemplate, data[i]);
            }
        }
    });
}

function rftable_stop() {
}
