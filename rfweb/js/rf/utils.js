RFVS_PREFIX = "0x72667673";
function is_rfvs(dp_id) {
    return dp_id.substr(0, RFVS_PREFIX.length) == RFVS_PREFIX;
}

function toHex(n, len) {
    if (isNaN(n))
        return "";
    n = n.toString(16);
    // Divide by four because a hex char holds 4 bits
    while (n.length < len / 4) {
        n = "0" + n;
    }
    return "0x" + n.toUpperCase();
}

function replace_all(string, old, new_) {
    return string.replace(new RegExp(old, "g"), new_);
}

function rstrip(string, terminal) {
    return string.replace(new RegExp(terminal + "$"), "");
}

function apply_template(template, data) {
    var code = template;
    for (var attr in data) {
        code = code.replace(new RegExp("{" + attr + "}", "g"), data[attr]);
    }
    return code;
}

function compare_objects(a, b, key) {
    return a[key] < b[key] ? -1 :
           a[key] > b[key] ?  1 : 0; 
}
