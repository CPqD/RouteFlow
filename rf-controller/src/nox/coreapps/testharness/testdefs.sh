# Convenient definitions for use in tests
#
# Copyright (C) 2009 Nicira Networks, Inc.

# --- Configuration variables providing defaults tests can override
NOX_TEST_RUNTIME=5

# --- Important "constants"
NOX_ASSERT_PASS="+assert_pass:"
NOX_ASSERT_FAIL="+assert_fail:"

# --- Diable job control to eliminate annoying "killed" messages
set +m

# --- Utility functions
function nox_test_init() {
    printf "+runtime: %d\n" "$NOX_TEST_RUNTIME"
}

function nox_test_verbose() {
    local value="${1:-True}"
    printf "+verbose: %s\n" "$value"
}

function nox_test_debug() {
    local value="${1:-True}"
    printf "+debug: %s\n" "$value"
}

function nox_test_output() {
    printf "+output: %s\n" "$1"
}

function nox_test_output_stdin() {
    local banner="$1"
    nox_test_output "==== start: $banner ===="
    xargs -d'\n' printf "+output: %s\n"
    nox_test_output "====   end: $banner ===="
}

function nox_test_msg() {
    printf "+print_msg: %s\n" "$*"
}

function nox_test_pass() {
    printf "%s %s\n" "$NOX_ASSERT_PASS" "$1"
}

function nox_test_fail() {
    printf "%s %s\n" "$NOX_ASSERT_FAIL" "$1"
}

function is_running() {
    local pid="$1"
    local command_name="$2"

    local process_on_pid=$(ps -p "$pid" -o "comm=")
    printf "%s\n" "$process_on_pid" | grep -i "$command_name\$" >/dev/null
}

function nox_test_outfile() {
    printf "%s/%s\n" "$abs_testdir" "$1"
}

function nox_pidfile() {
    nox_test_outfile "nox.pid"
}

function nox_pid() {
    local pidfile=$(nox_pidfile "$1")
    [ -f "$pidfile" ] && cat "$pidfile"
}

function nox_logfile() {
    nox_test_outfile "nox.output"
}

function nox_initfile() {
    nox_test_outfile "nox.boostrap_complete"
}

function nox_is_running() {
    local pid=$(nox_pid)
    [ -n "$pid" ] && is_running $pid "nox_core"
}

function show_file() {
    local file="$1"
    local filename=$(basename "$file")
    local banner="${2-$filename}"

    cat "$file" | nox_test_output_stdin "$banner"
}

function tail_file() {
    local nlines="$1"
    local file="$2"
    local filename=$(basename "$file")
    local banner="${3-$filename}"

    tail -n $nlines "$file" | nox_test_output_stdin "tail of $banner"
}

function wait_for_nox_startup() {
    while [ ! -e "$(nox_initfile)" ]; do
        sleep 1
    done
    sleep 1   # Ensure bootstrap complete events occur for all components
}

function start_nox() {
    local pidfile=$(nox_pidfile)
    local logfile=$(nox_logfile)
    local initfile=$(nox_initfile)
    ./nox_core -v initindicator=file=$initfile "$@" >"$logfile" 2>&1 &
    printf "%s\n" "$!" >"$pidfile"
    wait_for_nox_startup
    if ! nox_is_running; then
        nox_test_fail "NOX started successfully"
        tail_file 100 "$logfile"
        return 1
    else
        nox_test_pass "NOX started successfully"
    fi
    return 0
}

function stop_nox() {
    if nox_is_running; then
        # It would be great to have a "clean" way to stop nox
        local pid=$(nox_pid)
        [ -n "$pid" ] && kill -9 $pid >/dev/null 2>/dev/null
        nox_test_pass "NOX still running at end of test"
    else
        tail_file 100 "$logfile"
        nox_test_fail "NOX still running at end of test"
    fi
}

function nox_test_assert_file_empty() {
    local file="$1"
    local filename=$(basename "$file")

    if [ $(wc -c <"$1") -ne 0 ]; then
        nox_test_fail "'$filename' is empty"
        show_file "$file"
    fi
    nox_test_pass "'$filename' is empty"
}

function nox_test_assert_file_matches() {
    local file="$1"
    local filename=$(basename "$file")
    shift
    # remaining arguments are options to pass to diff

    cat > "$file.cmp"
    if ! diff -u "$@" "$file" "$file.cmp" >"$file.cmp.diff"; then
        nox_test_fail "'$filename' has correct contents"
        show_file "$file.cmp.diff"
    fi
    nox_test_pass "'$filename' has correct contents"
}

function nox_test_assert_file_contains() {
    local file="$1"
    local filename=$(basename "$file")
    shift

    failed=n
#    for re in "$@"; do
    while read -r re; do
        if ! grep -q "$re" "$file"; then
            nox_test_fail "$filename Failed to matche regexp: "$re""
            failed=y
        else
            nox_test_pass "$filename matches regexp: "$re""
        fi
    done
    if [ "$failed" = "y" ]; then
        show_file "$file"
    fi
}
