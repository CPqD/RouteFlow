rowtemplate = "<tr class=\"bg\{style}\">"
rowtemplate += "<td>{vm_id}</td>"
rowtemplate += "<td>{vm_port}</td>"
rowtemplate += "<td>{ct_id}</td>"
rowtemplate += "<td>{dp_id}</td>"
rowtemplate += "<td>{dp_port}</td>"
rowtemplate += "<td>{vs_id}</td>"
rowtemplate += "<td>{vs_port}</td>"
rowtemplate += "</tr>"

function process_entry(i, msg) {
    msg["id"] = i;
    msg["style"] = i % 2;
}

function rftable_init() {
    var table = document.getElementById("rftable");
    $.ajax({
        url: "/rftable",
        dataType: 'jsonp',
        success: function (data) {
            data.sort(function(a, b) { return compare_objects(a, b, "vm_id"); });

            for (var i in data) {
                process_entry(i, data[i])
                table.innerHTML += apply_template(rowtemplate, data[i]);
            }
        }
    });
}

function rftable_stop() {
}
