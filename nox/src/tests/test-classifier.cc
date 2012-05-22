/* Copyright 2008 (C) Nicira, Inc.
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
#undef _GLIBCXX_CONCEPT_CHECKS

#include <string>
#include <sstream>
#include <fstream>
#include <ctype.h>
#include <sys/time.h>

#include "test-classifier.hh"
#include "expr.hh"

#include <map>

#define EXIT_ASSERT(RET) if(!RET) exit(EXIT_FAILURE)

using namespace std;
using namespace vigil;

typedef std::list<pair<uint32_t,Rule<Packet_expr, void*> > > Rule_list;

void add_rmv_test(Classifier_t<Packet_expr, void *>& test, Rule_list& rules);
void check_lookup(Classifier_t<Packet_expr, void *>& test, Rule_list& rules);
bool read_rules(const char *filename, Rule_list& exprs);
bool set_field(Packet_expr& expr, std::string type, std::string strvalue);
void timed_test(Classifier_t<Packet_expr, void *>& test, Rule_list& rules);


int
main(int argc, char *argv[])
{
    Classifier_t<Packet_expr, void *> test;
    Rule_list rules, packets, to_delete;

    if (argc < 4) {
        printf("Usage: %s <policy file> <packets file> <rule file with expressions to delete from tree>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!read_rules(argv[1], rules)) {
        printf("policy read failed\n");
        exit(EXIT_FAILURE);
    }

    if (!read_rules(argv[2], packets)) {
        printf("packets read failed\n");
        exit(EXIT_FAILURE);
    }

    if (!read_rules(argv[3], to_delete)) {
        printf("to_delete read failed\n");
        exit(EXIT_FAILURE);
    }

//    printf("Running tests on empty tree\n");
    test.unbuild();
    test.build();
    add_rmv_test(test, rules);
//    test.print();

//    printf("Building tree...\n");
    test.build();
    add_rmv_test(test, rules);
//    test.print();

//    printf("timing test\n");
//    timed_test(test, packets);

//    printf("Unbuilding tree...\n");
    test.unbuild();
    add_rmv_test(test, rules);
//    test.print();

//    printf("Deleting first half of rules and building...\n");
    int i = rules.size() / 2;
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter) {
        EXIT_ASSERT(test.check_delete_rule(iter->first));
        if (--i == 0)
            break;
    }
    test.build();
//    test.print();

//    printf("Adding remaining rules and building...\n");
    i = rules.size() / 2;
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter) {
        EXIT_ASSERT((iter->first = test.check_add_rule(iter->second.priority,
                                                  iter->second.expr,
                                                  iter->second.action)) != 0);
        if (--i == 0)
            break;
    }
    test.build();
    add_rmv_test(test, rules);
//    test.print();

    if (to_delete.size() > 0) {
//        printf("Deleting exprs and cleaning...\n");
        for (Rule_list::iterator iter = to_delete.begin(); iter != to_delete.end(); ++iter)
            EXIT_ASSERT(test.check_delete_rules(&iter->second.expr));
        check_lookup(test, rules);
        check_lookup(test, to_delete);
        test.clean();
        check_lookup(test, rules);
        check_lookup(test, to_delete);
//        test.print();
    }

    return 0;
}

void
timed_test(Classifier_t<Packet_expr, void *>& test, Rule_list& rules)
{
    Classifier<Packet_expr, void *>& c = test.get_classifier();
    uint32_t n_iterations = 1000000 / rules.size();
    const Rule<Packet_expr, void *> *match;

    struct timeval before, after;

    EXIT_ASSERT(!gettimeofday(&before, NULL));
    if (rules.size() > 1) {
        for (uint32_t i = 0; i < n_iterations; i++) {
            for (Rule_list::iterator iter = rules.begin();
                 iter != rules.end(); ++iter)
            {
                Cnode_result<Packet_expr, void *, Packet_expr> result(&iter->second.expr);
                c.get_rules(result);
                match = result.next();
            }
        }
    } else {
        for (uint32_t i = 0; i < n_iterations; i++) {
            Cnode_result<Packet_expr, void *, Packet_expr> result(&rules.front().second.expr);
            c.get_rules(result);
            match = result.next();
        }
    }
    EXIT_ASSERT(!gettimeofday(&after, NULL));
//    double time = (after.tv_sec - before.tv_sec) * 1000 + ((double)(after.tv_usec - before.tv_usec))/1000;
//    printf("%u lookups in %fms\n", n_iterations * rules.size(), time);
}


void
add_rmv_test(Classifier_t<Packet_expr, void *>& test, Rule_list& rules)
{
    check_lookup(test, rules);

//    printf("   Deleting rules...\n");
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter)
        EXIT_ASSERT(test.check_delete_rule(iter->first));
    check_lookup(test, rules);

//    printf("   Adding rules to classifier...\n");
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter){
        EXIT_ASSERT((iter->first = test.check_add_rule(iter->second.priority,
                                                       iter->second.expr,
                                                       iter->second.action)) != 0);
    }
    check_lookup(test, rules);

//    printf("   Deleting some rules...\n");
    int i = 0;
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter) {
        if (i % 3 == 0)
            EXIT_ASSERT(test.check_delete_rule(iter->first));
    }
    check_lookup(test, rules);

//    printf("   Deleting all rules...\n");
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter)
        EXIT_ASSERT(test.check_delete_rule(iter->first));
    check_lookup(test, rules);

//    printf("   Re-adding rules to classifier...\n");
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter)
        EXIT_ASSERT((iter->first = test.check_add_rule(iter->second.priority,
                                                       iter->second.expr,
                                                       iter->second.action)) != 0);
    check_lookup(test, rules);
}

void
check_lookup(Classifier_t<Packet_expr, void *>& test, Rule_list& rules)
{
    for (Rule_list::iterator iter = rules.begin(); iter != rules.end(); ++iter)
        EXIT_ASSERT(test.check_lookup(&iter->second.expr));
}

bool
read_rules(const char *filename, Rule_list& rules)
{
    std::ifstream f(filename);
    std::string str;

    while (getline(f, str)) {
        uint32_t len = str.length();

        std::string::size_type prev = 0;
        std::string::size_type space;
        uint32_t ints[2];
        for (int i = 0; i < 2; i++) {
            space = str.find(' ', prev);
            std::stringstream ss(str.substr(prev, space - prev));
            ss >> ints[i];
            prev = space + 1;
            if (prev >= len)
                return false;
        }

        Packet_expr expr;
        std::string::size_type equal;
        while ((equal = str.find('=', prev)) != std::string::npos) {
            std::string::size_type comma = str.find(',', equal);
            if (comma == std::string::npos)
                comma = len;
            if (!set_field(expr, str.substr(prev, equal - prev),
                           str.substr(equal+1, comma - (equal+1))))
                return false;
            prev = comma + 1;
            if (prev >= len)
                break;
        }
        Rule<Packet_expr, void *> r(ints[0], ints[1], expr, NULL);
        rules.push_back(pair<uint32_t,Rule<Packet_expr, void *> >(ints[0], r));
    }
    return true;
}

uint8_t
hex_to_int(char ch)
{
    if (isdigit(ch))
        return ch - '0';
    return 10 + (tolower(ch) - 'a');
}

bool
set_field(Packet_expr& expr, std::string type, std::string strvalue)
{
    uint32_t value[2];

    value[1] = 0;

    if (type == "dlsrc" || type == "dldst") {
        uint8_t *byte_ptr = (uint8_t*)value;
        uint32_t len = strvalue.length();
        std::string::size_type pos = 0;
        for (uint32_t i = 0; i < 6; i++) {
            if ((pos + 2) > len)
                return false;
            byte_ptr[i] = hex_to_int(strvalue.at(pos++)) << 4;
            byte_ptr[i] += hex_to_int(strvalue.at(pos++));
            pos++;
        }
        if (type == "dlsrc")
            expr.set_field(Packet_expr::DL_SRC, value);
        else
            expr.set_field(Packet_expr::DL_DST, value);
    } else {
        std::stringstream ss(strvalue);
        ss >> value[0];

        if (type == "apsrc")
            expr.set_field(Packet_expr::AP_SRC, value);
        else if (type == "apdst")
            expr.set_field(Packet_expr::AP_DST, value);
        else if (type == "nwsrc")
            expr.set_field(Packet_expr::NW_SRC, value);
        else if (type == "nwdst")
            expr.set_field(Packet_expr::NW_DST, value);
        else if (type == "tpsrc")
            expr.set_field(Packet_expr::TP_SRC, value);
        else if (type == "tpdst")
            expr.set_field(Packet_expr::TP_DST, value);
        else if (type == "groupsrc")
            expr.set_field(Packet_expr::GROUP_SRC, value);
        else if (type == "groupdst")
            expr.set_field(Packet_expr::GROUP_DST, value);
        else if (type == "dlproto")
            expr.set_field(Packet_expr::DL_TYPE, value);
        else if (type == "nwproto")
            expr.set_field(Packet_expr::NW_PROTO, value);
        else
            return false;
    }
    return true;
}

