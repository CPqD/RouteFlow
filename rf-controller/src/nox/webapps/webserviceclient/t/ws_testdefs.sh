source "${testdefs_sh}"

database="./nox/webapps/webserviceclient/t/unit_test_db"

function PassWSCmd() {
    local stdout="$1"
    local stderr="$2"
    local log="$3"
    local req_method="$4"
    shift 4
    local user="${NCLI_USER:-admin}"
    local passwd="${NCLI_PASSWD:-admin}"
    rm -f "$stdout" "$stderr" "$log"
    if ! python ./nox/webapps/webserviceclient/t/ws_test.py -P 8888 --no-ssl \
          -u "$user" -p "$passwd" -r $req_method \
          --debug --debug-logfile="$log" "$@" >"$stdout" 2>"$stderr" ; then
        nox_test_fail "cmd exit code == 0"
        tail_file 30 "$log"
    fi
}

function FailWSCmd() {
    local stdout="$1"
    local stderr="$2"
    local log="$3"
    local req_method="$4"
    shift 4
    local user="${NCLI_USER:-admin}"
    local passwd="${NCLI_PASSWD:-admin}"
    rm -f "$stdout" "$stderr" "$log"
    if ! python ./nox/webapps/webserviceclient/t/ws_test.py -P 8888 --no-ssl \
          -u "$user" -p "$passwd" -r $req_method  \
          --debug --debug-logfile="$log" "$@" >"$stdout" 2>"$stderr" ; then
        nox_test_fail "cmd exit code != 0"
        tail_file 30 "$log"
    fi
}


function get_value_for_key() {
    local file="$1"
    local key="$2"
    if [ -e "$file" ]; then
        value=$(grep "^$key" $file | sed -n  "s/.*= \(.*\)/\1/p")
    fi
    
    if [ "$value" == "" ]; then
        nox_test_fail "$key not found"
    fi
    echo "$value"
}

