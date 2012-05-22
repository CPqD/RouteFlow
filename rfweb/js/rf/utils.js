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

function apply_template(template, data) {
    var code = template;
    for (var attr in data) {
        code = code.replace(new RegExp("{" + attr + "}", "g"), data[attr]);
    }
    return code;
}
