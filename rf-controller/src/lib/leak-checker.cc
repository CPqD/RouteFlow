/* Copyright 2008, 2009 (C) Nicira, Inc.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */

/* These debug settings cause the leak checker to make NOX run 10 slower. */
#undef _GLIBCXX_CONCEPT_CHECKS
#undef _GLIBCXX_DEBUG
#undef _GLIBCXX_DEBUG_PEDANTIC

#include "leak-checker.hh"
#include <algorithm>
#include <boost/foreach.hpp>
#include <sstream>
#include <inttypes.h>
#include <list>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <tr1/unordered_map>
#include <vector>
#include "vlog.hh"

namespace vigil {

static Vlog_module log("leak-checker");

#if !defined(HAVE_MALLOC_HOOKS) || !defined(HAVE_TLS)
void
leak_checker_start(const char *)
{
    VLOG_WARN(log, "not enabling leak checker:"
#ifndef HAVE_MALLOC_HOOKS
              " the libc in use does not have the required hooks"
#endif
#if !defined(HAVE_MALLOC_HOOKS) && !defined(HAVE_TLS)
              " and "
#endif
#ifndef HAVE_TLS
              " the C++ implementation does not support thread-local storage"
#endif
        );
}

void
leak_checker_set_limit(off_t)
{
}

void
leak_checker_claim(const void *)
{
}

void
leak_checker_usage()
{
    printf("\nLeak checking options:\n"
           "  --check-leaks=FILE      (accepted but ignored in this build)\n"
           "  --leak-limit=BYTES      (accepted but ignored in this build)\n");
}
#else /* HAVE_MALLOC_HOOKS */
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>

typedef void *malloc_hook_type(size_t, const void *);
typedef void *realloc_hook_type(void *, size_t, const void *);
typedef void free_hook_type(void *, const void *);

struct hooks {
    malloc_hook_type *malloc_hook_func;
    realloc_hook_type *realloc_hook_func;
    free_hook_type *free_hook_func;
};

struct segment {
    uintptr_t vm_start;
    uintptr_t vm_end;
    uintptr_t vm_pgoff;
    bool read, write, exec, shared;
    std::string file_name;
};

static const int max_frames = 32;
struct block {
    uintptr_t address;
    size_t size;
    uintptr_t frames[max_frames];
    int n_frames;
    std::list<block*>::iterator queue_iterator;
    block();
};
typedef std::list<block*> block_list;
typedef std::tr1::unordered_map<uintptr_t, block> block_hash;

/* These need to be dynamically allocated or we segfault when free() is called
 * after they are destructed at program termination. */
static const int max_blocks = 2048;
static block_hash* block_table;
static block_list* block_queue;

block::block() : queue_iterator(block_queue->end())
{}

static malloc_hook_type hook_malloc;
static realloc_hook_type hook_realloc;
static free_hook_type hook_free;

static struct hooks libc_hooks;
static const struct hooks our_hooks = { hook_malloc, hook_realloc, hook_free };

static FILE *file;
static bool is_pipe;
static off_t limit = 1000ULL * 1000 * 1000;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static bool
operator<(const segment& a, const segment& b)
{
    return a.vm_start < b.vm_start;
}

static void
get_hooks(struct hooks *hooks)
{
    hooks->malloc_hook_func = __malloc_hook;
    hooks->realloc_hook_func = __realloc_hook;
    hooks->free_hook_func = __free_hook;
}

static void
set_hooks(const struct hooks *hooks)
{
    __malloc_hook = hooks->malloc_hook_func;
    __realloc_hook = hooks->realloc_hook_func;
    __free_hook = hooks->free_hook_func;
}

void
leak_checker_start(const char *file_name)
{
    if (!file) {
        if (file_name[0] == '|') {
            is_pipe = true;
            file = popen(file_name + 1, "w");
            if (!file) {
                VLOG_WARN(log, "failed to start \"%s\": %s",
                          file_name, strerror(errno));
                return;
            }
        } else {
            is_pipe = false;
            // if we're writing to a normal file, append the pid
            // to prevent overwriting the file on restart
            std::stringstream fnamestream; 
            fnamestream << file_name << "." << (long)getpid();
            file_name = fnamestream.str().c_str(); 
            file = fopen(file_name, "w");
            if (!file) {
                VLOG_WARN(log, "failed to create \"%s\": %s",
                          file_name, strerror(errno));
                return;
            }
        }
        VLOG_WARN(log, "enabled memory leak logging to \"%s\"", file_name);
        if (!block_table) {
            block_table = new block_hash;
            block_queue = new block_list;
        }
        get_hooks(&libc_hooks);
        set_hooks(&our_hooks);
    }
}

void
leak_checker_stop()
{
    if (file) {
        if (is_pipe) {
            pclose(file);
        } else {
            fclose(file);
        }
        file = NULL;
        set_hooks(&libc_hooks);
        VLOG_WARN(log, "disabled memory leak logging");
    }
}

void
leak_checker_set_limit(off_t limit_)
{
    VLOG_WARN(log, "Setting leak logging file size limit to %"PRIdMAX" bytes", 
        limit_);
    limit = limit_;
}

static const segment*
find_segment(const std::vector<segment>& segments, uintptr_t address)
{
    int l = 0;
    int r = (int) segments.size() - 1;
    while (l <= r) {
        int m = (l + r) / 2;
        const segment& s = segments[m];
        if (address < s.vm_start) {
            r = m - 1;
        } else if (address >= s.vm_end) {
            l = m + 1;
        } else {
            return &s;
        }
    }
    return NULL;
}

static std::vector<segment>
read_maps()
{
    static const char file_name[] = "/proc/self/maps";
    std::vector<segment> segments;
    char line[1024];
    int line_number;
    FILE *f;

    f = fopen(file_name, "r");
    if (f == NULL) {
        VLOG_WARN(log, "opening %s failed: %s", file_name, strerror(errno));
        return segments;
    }

    for (line_number = 1; fgets(line, sizeof line, f); line_number++) {
        char read_c, write_c, exec_c, shared_c;
        char binary[sizeof line];
        segment s;

        binary[0] = '\0';
        if (sscanf(line, "%"SCNxPTR"-%"SCNxPTR" %c%c%c%c "
                   "%"SCNxPTR" %*s %*s %s",
                   &s.vm_start, &s.vm_end,
                   &read_c, &write_c, &exec_c, &shared_c,
                   &s.vm_pgoff, binary) >= 7) {
            s.read = read_c == 'r';
            s.write = write_c == 'w';
            s.exec = exec_c == 'x';
            s.shared = exec_c == 's';
            s.file_name.assign(binary);
            segments.push_back(s);
        } else {
            VLOG_WARN(log, "%s:%d: parse error (%s)",
                      file_name, line_number, line);
        }
    }
    fclose(f);
    std::sort(segments.begin(), segments.end());
    return segments;
}

static uintptr_t
get_max_stack(uintptr_t low)
{
    std::vector<segment> segments = read_maps();
    const segment* s = find_segment(segments, low);
    if (s) {
        return s->vm_end;
    } else {
        VLOG_WARN(log, "no stack found for sp=%08"PRIxPTR, low);
        return -1;
    }
}

static void
get_stack_limits(uintptr_t *lowp, uintptr_t *highp)
{
    static __thread uintptr_t high;
    uintptr_t low = (uintptr_t) &low;
    if (!high) {
        high = get_max_stack(low);
    }
    *lowp = low;
    *highp = high;
}

static bool
in_stack(void *p)
{
    uintptr_t address = (uintptr_t) p;
    uintptr_t low, high;
    get_stack_limits(&low, &high);
    return address >= low && address < high;
}

static void
check_segment(uintptr_t address)
{
    /* These need to be dynamically allocated or we segfault when free() is
     * called after they are destructed at program termination. */
    static std::vector<segment>* segments;
    static std::vector<uintptr_t>* blacklist;
    if (!segments) {
        segments = new std::vector<segment>;
        blacklist = new std::vector<uintptr_t>;
    }
    if (!find_segment(*segments, address)) {
        *segments = read_maps();

        /* This page size doesn't have to be accurate.  It's really just the
         * granularity of the blacklist. */
        const uintptr_t page_size = 4096;

        if (!find_segment(*segments, address)) {
            const uintptr_t page_base = address & ~(page_size - 1);
//            VLOG_WARN(log, "0x%08"PRIxPTR" failed to appear in segments, "
//                      "blacklisting 0x%08"PRIxPTR, address, page_base);
            blacklist->push_back(page_base);
        }

        std::vector<segment> tmp;
        BOOST_FOREACH (uintptr_t page, *blacklist) {
            if (!find_segment(*segments, page)) {
                segment s;
                s.vm_start = page;
                s.vm_end = page + page_size;
                s.vm_pgoff = -1;
                s.read = s.write = s.exec = s.shared = false;
                s.file_name = "blacklisted";
                tmp.push_back(s);
            }
        }
        segments->insert(segments->end(), tmp.begin(), tmp.end());
        std::sort(segments->begin(), segments->end());

        BOOST_FOREACH (const segment& s, *segments) {
            fprintf(file, "segment: %08"PRIxPTR"-%08"PRIxPTR" %08"PRIxPTR" "
                    "%c%c%c%c %s\n",
                    s.vm_start, s.vm_end, s.vm_pgoff,
                    s.read ? 'r' : '-',
                    s.write ? 'w' : '-',
                    s.exec ? 'x' : '-',
                    s.shared ? 's' : 'p',
                    s.file_name.c_str());
        }
    }
}

static block
backtrace(uintptr_t address, size_t size = 0)
{
    block b;
    b.address = address;
    b.size = size;
    b.n_frames = 0;
    for (void** f = (void **) __builtin_frame_address(0);
         in_stack(f) && b.n_frames < max_frames;
         f = (void **) f[0]) {
        b.frames[b.n_frames++] = (uintptr_t) f[1];
    }
    return b;
}

static void PRINTF_FORMAT(2, 3)
log_callers(block& b, const char *format, ...)
{
    for (int i = 0; i < b.n_frames; i++) {
        check_segment(b.frames[i]);
    }

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    putc(':', file);
    for (int i = 0; i < b.n_frames; i++) {
        fprintf(file, " %"PRIxPTR, b.frames[i]);
    }
    putc('\n', file);
}

static void
reset_hooks()
{
    static int count;

    if (count++ >= 100 && limit && file && !is_pipe) {
        struct stat s;
        count = 0;
        if (fstat(fileno(file), &s) < 0) {
            VLOG_WARN(log, "cannot fstat leak checker log file: %s",
                      strerror(errno));
            return;
        }
        if (s.st_size > limit) {
            VLOG_WARN(log, "leak checker log file size exceeded limit");
            leak_checker_stop();
            return;
        }
    }
    if (file) {
        set_hooks(&our_hooks);
    }
}

static void
trim_blocks()
{
    block& b = *block_queue->front();
    log_callers(b, "malloc(%zu) -> %"PRIxPTR, b.size, b.address);
    block_queue->pop_front();
    if (!block_table->erase(b.address)) {
        abort();
    }
}

static bool
free_block(uintptr_t address)
{
    block_hash::iterator i = block_table->find(address);
    if (i != block_table->end()) {
        block& b = i->second;
        block_queue->erase(b.queue_iterator);
        block_table->erase(i);
        return true;
    } else {
        return false;
    }
}

static void
alloc_block(uintptr_t address, size_t size)
{
    free_block(address);

    block_hash::value_type v(address, backtrace(address, size));
    block& b = block_table->insert(v).first->second;
    b.queue_iterator = block_queue->insert(block_queue->end(), &b);
}

static void *
hook_malloc(size_t size, const void *)
{
    pthread_mutex_lock(&mutex);
    set_hooks(&libc_hooks);
    void* p = malloc(size);
    get_hooks(&libc_hooks);

    if (p) {
        if (block_table->size() >= max_blocks) {
            trim_blocks();
        }
        alloc_block((uintptr_t) p, size);
    }

    reset_hooks();
    pthread_mutex_unlock(&mutex);

    return p;
}

void
leak_checker_usage()
{
    printf("\nLeak checking options:\n"
           "  --check-leaks=FILE      log memory allocation calls to FILE\n"
           "  --leak-limit=SIZE       log to FILE at most SIZE bytes\n");
}

void
leak_checker_claim(const void *p)
{
    if (!file) {
        return;
    }

    if (p) {
        pthread_mutex_lock(&mutex);
        set_hooks(&libc_hooks);
        block b = backtrace((uintptr_t) p);
        log_callers(b, "claim(%p)", p);
        reset_hooks();
        pthread_mutex_unlock(&mutex);
    }
}

static void
hook_free(void *p, const void *)
{
    if (!p) {
        return;
    }

    pthread_mutex_lock(&mutex);
    set_hooks(&libc_hooks);
    free(p);
    get_hooks(&libc_hooks);

    uintptr_t address = (uintptr_t) p;
    if (!free_block(address)) {
        block b = backtrace(address);
        log_callers(b, "free(%"PRIxPTR")", b.address);
    }

    reset_hooks();
    pthread_mutex_unlock(&mutex);
}

static void *
hook_realloc(void *p, size_t size, const void *)
{
    pthread_mutex_lock(&mutex);
    set_hooks(&libc_hooks);
    void* q = realloc(p, size);
    get_hooks(&libc_hooks);

    if (p != q) {
        free_block((uintptr_t) p);
        alloc_block((uintptr_t) q, size);
    }

    reset_hooks();
    pthread_mutex_unlock(&mutex);

    return q;
}
#endif /* HAVE_MALLOC_HOOKS */

} // namespace vigil
