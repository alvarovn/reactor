/*
    This file is part of reactor.
    
    Copyright (C) 2011  Álvaro Villalba Navarro <vn.alvaro@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

// #include <unistd.h>
#include <stdbool.h>
// #include <string.h>
// #include <arpa/inet.h>

#include "tests.h"
#include "reactor.h"
#include "rules.c"
#include "action.c"
#include "remote.c"
#include "libreactor.h"
#include <stdio.h>

START_TEST(test_tokenizer_comments){
    struct rule rule;
    struct rr_expr rexpr;
    
    rule.expr = &rexpr;
    rule.line = "A | B # This is a very simple rule";
    
    rexpr.exprnum = 1;
    rexpr.tokensep = '|';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.innerexpr = NULL;
    
    tokenize_rule(&rule);
    
    fail_unless(rule.tokens != NULL);
    fail_unless(rule.errors == NULL);
    
    fail_unless(strcmp(rule.tokens->data, "A") == 0);
    fail_unless(rule.tokens->next != NULL);
    fail_unless(rule.tokens->down == NULL);
    
    fail_unless(strcmp(rule.tokens->next->data, "B") == 0);
    fail_unless(rule.tokens->next->next == NULL);
    fail_unless(rule.tokens->next->down == NULL);
    
    tokens_free(rule.tokens);
    
    rule.line = "# This is comment line";
    
    tokenize_rule(&rule);
    
    fail_unless(rule.tokens == NULL);
    fail_unless(rule.errors == NULL);
    tokens_free(rule.tokens);
}
END_TEST

START_TEST(test_tokenizer_notrim){
    struct rule rule;
    struct rr_expr rexpr;
    struct rr_expr iexpr;
    
    rule.expr = &rexpr;
    rule.line = "A  |B| C|z y x| D";
    
    rexpr.exprnum = 1;
    rexpr.tokensep = '|';
    rexpr.end = '\0';
    rexpr.trim = false;
    rexpr.next = NULL;
    rexpr.innerexpr = &iexpr;
    
    iexpr.exprnum = 4;
    iexpr.tokensep = ' ';
    iexpr.end = '|';
    iexpr.trim = false;
    iexpr.next = NULL;
    iexpr.innerexpr = NULL;
    
    tokenize_rule(&rule);
    
    fail_unless(rule.tokens != NULL);
    fail_unless(rule.errors == NULL);
    
    fail_unless(strcmp(rule.tokens->data, "A  ") == 0);
    fail_unless(rule.tokens->next != NULL);
    fail_unless(rule.tokens->down == NULL);
    
    fail_unless(strcmp(rule.tokens->next->data, "B") == 0);
    fail_unless(rule.tokens->next->next != NULL);
    fail_unless(rule.tokens->next->down == NULL);
    
    fail_unless(strcmp(rule.tokens->next->next->data, " C") == 0);
    fail_unless(rule.tokens->next->next->next != NULL);
    fail_unless(rule.tokens->next->next->down == NULL);
    
    fail_unless(rule.tokens->next->next->next->data == NULL);
    fail_unless(rule.tokens->next->next->next->next != NULL);
    fail_unless(rule.tokens->next->next->next->down != NULL);
    
    fail_unless(strcmp(rule.tokens->next->next->next->down->data, "z") == 0);
    fail_unless(rule.tokens->next->next->next->down->next != NULL);
    fail_unless(rule.tokens->next->next->next->down->down == NULL);
    
    fail_unless(strcmp(rule.tokens->next->next->next->down->next->data, "y") == 0);
    fail_unless(rule.tokens->next->next->next->down->next->next != NULL);
    fail_unless(rule.tokens->next->next->next->down->next->down == NULL);
    
    fail_unless(strcmp(rule.tokens->next->next->next->down->next->data, "x") == 0);
    fail_unless(rule.tokens->next->next->next->down->next->next == NULL);
    fail_unless(rule.tokens->next->next->next->down->next->down == NULL);
    
    fail_unless(strcmp(rule.tokens->next->next->next->next->data, " D") == 0);
    fail_unless(rule.tokens->next->next->next->next->next == NULL);
    fail_unless(rule.tokens->next->next->next->next->down == NULL);
    tokens_free(rule.tokens);

}
END_TEST

START_TEST(test_tokenizer_empty_rule){
    struct rule rule;
    struct rr_expr rexpr;
    
    rule.expr = &rexpr;
    rule.line = " ";
    
    rexpr.exprnum = 1;
    rexpr.tokensep = '&';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.innerexpr = NULL;
    
    tokenize_rule(&rule);
    
    fail_unless(rule.errors == NULL);
    fail_unless(rule.tokens == NULL);
}
END_TEST

START_TEST(test_tokenizer_reactor_wrong_rules){
    // Return the correct error and everything else clean
    struct rule rrule;
    struct rr_expr rexpr;
    struct rr_expr evexpr;
    struct rr_expr actexpr;
    char *expectedmsg;
    rrule.line = "A B e1 & & e5 PROP localhost";
    rrule.expr = &rexpr;
    
    rexpr.exprnum = 1;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.innerexpr = &evexpr;
    
    evexpr.exprnum = 3;
    evexpr.tokensep = '&';
    evexpr.end = ' ';
    evexpr.trim = true;
    evexpr.next = &actexpr;
    evexpr.innerexpr = NULL;
    
    actexpr.exprnum = 5;
    actexpr.tokensep = '\0';
    actexpr.end = '\0';
    actexpr.trim = true;
    actexpr.next = NULL;
    actexpr.innerexpr = NULL;
    
    tokenize_rule(&rrule);
    
    fail_unless(rrule.errors != NULL);
    fail_unless(rrule.tokens == NULL);
    errors_free(rrule.errors);
    
    rrule.line = "A B &e1 & e5 PROP localhost";
    
    tokenize_rule(&rrule);
    
    fail_unless(rrule.errors != NULL);
    fail_unless(rrule.tokens == NULL);
    errors_free(rrule.errors);
}
END_TEST

START_TEST(test_tokenizer_reactor_rule_short){
    struct rule rrule;
    struct rr_expr rexpr;
    struct rr_expr evexpr;
    struct rr_expr actexpr;
    char *expectedmsg;
    
    expectedmsg = "'%s' token content was expected instead of '%s'";
    rrule.line = "A B e1 PROP localhost";
    rrule.expr = &rexpr;
    
    rexpr.exprnum = 1;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.innerexpr = &evexpr;
    
    evexpr.exprnum = 3;
    evexpr.tokensep = '&';
    evexpr.end = ' ';
    evexpr.trim = true;
    evexpr.next = &actexpr;
    evexpr.innerexpr = NULL;
    
    actexpr.exprnum = 5;
    actexpr.tokensep = '\0';
    actexpr.end = '\0';
    actexpr.trim = true;
    actexpr.next = NULL;
    actexpr.innerexpr = NULL;
    
    tokenize_rule(&rrule);
    
    fail_unless(rrule.errors == NULL);
    
    fail_unless(rrule.tokens != NULL);
    
    fail_unless(strcmp(rrule.tokens->data, "A") == 0,
        expectedmsg, "A", rrule.tokens->data
    );
    fail_unless(rrule.tokens->down == NULL);
    fail_unless(rrule.tokens->next != NULL);
    
    fail_unless(strcmp(rrule.tokens->next->data, "B") == 0,
        expectedmsg, "B", rrule.tokens->next->data
    );
    fail_unless(rrule.tokens->next->down == NULL);
    fail_unless(rrule.tokens->next->next != NULL);
    
    fail_unless(rrule.tokens->next->next->data == NULL);
    fail_unless(rrule.tokens->next->next->down != NULL);
    fail_unless(rrule.tokens->next->next->next != NULL);

    fail_unless(strcmp(rrule.tokens->next->next->down->data, "e1") == 0,
        expectedmsg, "e1", rrule
    );
    fail_unless(rrule.tokens->next->next->down->down == NULL);
    fail_unless(rrule.tokens->next->next->down->next == NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->next->data, "PROP") == 0,
        expectedmsg, "PROP", rrule.tokens->next->next->next->data
    );
    fail_unless(rrule.tokens->next->next->next->down == NULL);
    fail_unless(rrule.tokens->next->next->next->next != NULL);

    fail_unless(rrule.tokens->next->next->next->next->data == NULL);
    fail_unless(rrule.tokens->next->next->next->next->down != NULL);
    fail_unless(rrule.tokens->next->next->next->next->next == NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->next->next->down->data, "localhost") == 0,
        expectedmsg, "localhost", rrule.tokens->next->next->next->data
    );
    fail_unless(rrule.tokens->next->next->next->next->down->down == NULL);
    fail_unless(rrule.tokens->next->next->next->next->down->next == NULL);
    tokens_free(rrule.tokens);
}
END_TEST

START_TEST(test_tokenizer_reactor_rule_long){
    struct rule rrule;
    struct rr_expr rexpr;
    struct rr_expr evexpr;
    struct rr_expr actexpr;
    char *expectedmsg;
    
    expectedmsg = "'%s' token content was expected instead of '%s'";
    rrule.line = "A  B e1 &e2 & e3&e4& e5  CMD echo \"A->B\" >> /tmp/test";
    rrule.expr = &rexpr;
    
    rexpr.exprnum = 1;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.innerexpr = &evexpr;
    
    evexpr.exprnum = 3;
    evexpr.tokensep = '&';
    evexpr.end = ' ';
    evexpr.trim = true;
    evexpr.next = &actexpr;
    evexpr.innerexpr = NULL;
    
    actexpr.exprnum = 5;
    actexpr.tokensep = '\0';
    actexpr.end = '\0';
    actexpr.trim = true;
    actexpr.next = NULL;
    actexpr.innerexpr = NULL;
    
    tokenize_rule(&rrule);

    fail_unless(rrule.errors == NULL);
    
    fail_unless(rrule.tokens != NULL);
    
    fail_unless(strcmp(rrule.tokens->data, "A") == 0,
        expectedmsg, "A", rrule.tokens->data
    );
    fail_unless(rrule.tokens->down == NULL);
    fail_unless(rrule.tokens->next != NULL);
    
    fail_unless(strcmp(rrule.tokens->next->data, "B") == 0,
        expectedmsg, "B", rrule.tokens->next->data
    );
    fail_unless(rrule.tokens->next->down == NULL);
    fail_unless(rrule.tokens->next->next != NULL);
    
    fail_unless(rrule.tokens->next->next->data == NULL);
    fail_unless(rrule.tokens->next->next->down != NULL);
    fail_unless(rrule.tokens->next->next->next != NULL);

    fail_unless(strcmp(rrule.tokens->next->next->down->data, "e1") == 0,
        expectedmsg, "e1", rrule
    );
    fail_unless(rrule.tokens->next->next->down->down == NULL);
    fail_unless(rrule.tokens->next->next->down->next != NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->down->next->data, "e2") == 0,
        expectedmsg, "e2", rrule
    );
    fail_unless(rrule.tokens->next->next->down->next->down == NULL);
    fail_unless(rrule.tokens->next->next->down->next->next != NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->down->next->next->data, "e3") == 0,
        expectedmsg, "e3", rrule
    );
    fail_unless(rrule.tokens->next->next->down->next->next->down == NULL);
    fail_unless(rrule.tokens->next->next->down->next->next->next != NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->down->next->next->next->data, "e4") == 0,
        expectedmsg, "e4", rrule
    );
    fail_unless(rrule.tokens->next->next->down->next->next->next->down == NULL);
    fail_unless(rrule.tokens->next->next->down->next->next->next->next != NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->down->next->next->next->next->data, "e5") == 0,
        expectedmsg, "e5", rrule
    );
    fail_unless(rrule.tokens->next->next->down->next->next->next->next->down == NULL);
    fail_unless(rrule.tokens->next->next->down->next->next->next->next->next == NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->next->data, "CMD") == 0,
        expectedmsg, "CMD", rrule.tokens->next->next->next->data
    );
    fail_unless(rrule.tokens->next->next->next->down == NULL);
    fail_unless(rrule.tokens->next->next->next->next != NULL);

    fail_unless(rrule.tokens->next->next->next->next->data == NULL);
    fail_unless(rrule.tokens->next->next->next->next->down != NULL);
    fail_unless(rrule.tokens->next->next->next->next->next == NULL);
    
    fail_unless(strcmp(rrule.tokens->next->next->next->next->down->data, "echo \"A->B\" >> /tmp/test") == 0,
        expectedmsg, "echo \"A->B\" >> /tmp/test", rrule.tokens->next->next->next->data
    );
    fail_unless(rrule.tokens->next->next->next->next->down->down == NULL);
    fail_unless(rrule.tokens->next->next->next->next->down->next == NULL);
    tokens_free(rrule.tokens);
}
END_TEST

Suite* make_rules_suite(void){
    int msgsize;
    int msgtype;
    Suite *s = suite_create("Rules");

    /* Core test case */

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_tokenizer_reactor_rule_long);
    tcase_add_test(tc_core, test_tokenizer_reactor_rule_short);
    tcase_add_test(tc_core, test_tokenizer_reactor_wrong_rules);
    tcase_add_test(tc_core, test_tokenizer_empty_rule);
    tcase_add_test(tc_core, test_tokenizer_comments);
    
    suite_add_tcase(s, tc_core);
    return s;
}