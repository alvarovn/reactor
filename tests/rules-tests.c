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
#include <src/reactor.h>
#include <stdio.h>

// TODO Clean this mess won't hurt

START_TEST(test_parser_prop_rule){
    struct r_rule *rule;
    struct rr_obj *fromobj,
                    *toobj,
                    *eventsobj,
                    *actionobj;
    struct r_action *action;
    struct prop_action *paction;
    
    char *line = "STATE_A STATE_B event1 PROP localhost   :   655";
    rule = r_rule_parse(line);
    
    fail_unless((fromobj = get_rule_obj(rule->objs, RULE_FROM)) != NULL);
    fail_unless(strcmp((char *)fromobj->data, "STATE_A") == 0);
    
    fail_unless((toobj = get_rule_obj(rule->objs, RULE_TO)) != NULL);
    fail_unless(strcmp((char *)toobj->data, "STATE_B") == 0);
    
    fail_unless((eventsobj = get_rule_obj(rule->objs, RULE_EVENTS)) != NULL);
    fail_unless(eventsobj->down != NULL);
    
    
    fail_unless(get_rule_obj(eventsobj->down, 0) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 0))->data, "event1") == 0);
    
    fail_unless((get_rule_obj(eventsobj->down, 0))->next == NULL);
    fail_unless((get_rule_obj(eventsobj->down, 0))->down == NULL);
    
    fail_unless((actionobj = get_rule_obj(rule->objs, RULE_RACTION)) != NULL);
    fail_unless((action = (struct r_action *) actionobj->data) != NULL);
    
    fail_unless(actionobj->next == NULL);
    fail_unless(actionobj->down == NULL);
    
    fail_unless(action->atype == PROP);
    paction = (struct prop_action *)action->action;
    fail_unless(strcmp(paction->addr, "localhost") == 0,
        "\"localhost\" address expected instead of \"%s\"", paction->addr
    );
    fail_unless(paction->port == 655,
        "\"655\" port expected instead of \"%s\"", paction->port
    );
    fail_unless(strcmp(paction->enids->data, "event1") == 0);
    
    action_free(actionobj->data);
    actionobj->data = NULL;
    rule_objs_free(actionobj);
    actionobj = NULL;
    eventsobj->next = NULL;
    // TODO This is not completely freeing the actions
    rules_free(rule);
    
    line = "STATE_A STATE_B event1&event2 PROP localhost";
    rule = r_rule_parse(line);
    
    fail_unless((fromobj = get_rule_obj(rule->objs, RULE_FROM)) != NULL);
    fail_unless(strcmp((char *)fromobj->data, "STATE_A") == 0);
    
    fail_unless((toobj = get_rule_obj(rule->objs, RULE_TO)) != NULL);
    fail_unless(strcmp((char *)toobj->data, "STATE_B") == 0);
    
    fail_unless((eventsobj = get_rule_obj(rule->objs, RULE_EVENTS)) != NULL);
    fail_unless(eventsobj->down != NULL);
    
    fail_unless(get_rule_obj(eventsobj->down, 0) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 0))->data, "event1") == 0);
    
    fail_unless(get_rule_obj(eventsobj->down->next, 0) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down->next, 0))->data, "event2") == 0);
    
    fail_unless((get_rule_obj(eventsobj->down, 0))->next->next == NULL);
    fail_unless((get_rule_obj(eventsobj->down, 0))->next->down == NULL);
    
    fail_unless((actionobj = get_rule_obj(rule->objs, RULE_RACTION)) != NULL);
    fail_unless((action = (struct r_action *) actionobj->data) != NULL);
    
    fail_unless(actionobj->next == NULL);
    fail_unless(actionobj->down == NULL);
    
    fail_unless(action->atype == PROP);
    paction = (struct prop_action *)action->action;
    fail_unless(strcmp(paction->addr, "localhost") == 0);
    fail_unless(paction->port == 6500);
    fail_unless(strcmp(paction->enids->data, "event2") == 0,
        "'event2 expected instead of '%s'", paction->enids->next->data);
    fail_unless(strcmp(paction->enids->next->data, "event1") == 0,
        "'event2 expected instead of '%s'", paction->enids->next->data
    );
    
    action_free(actionobj->data);
    actionobj->data = NULL;
    rule_objs_free(actionobj);
    actionobj = NULL;
    eventsobj->next = NULL;
    rules_free(rule);
    
    line = "STATE_A STATE_B event1 PROP";
    rule = r_rule_parse(line);
    
    fail_unless(rule != NULL);
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    
    rules_free(rule);
    line = "STATE_A STATE_B event1 PROP localhost:34pp";
    mark_point ();
    rule = r_rule_parse(line);
    
    fail_unless(rule != NULL);
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    
    rules_free(rule);
}
END_TEST

START_TEST(test_parser_none_rule){
    struct r_rule *rule;
    struct rr_obj *fromobj,
                    *toobj,
                    *eventsobj,
                    *actionobj;
    struct r_action *action;
    
    char *line = "STATE_A STATE_B event1";
    rule = r_rule_parse(line);
    
    fail_unless(rule != NULL);
    
    fail_unless(rule->errors == NULL);
    
    fail_unless((fromobj = get_rule_obj(rule->objs, RULE_FROM)) != NULL);
    fail_unless(strcmp((char *)fromobj->data, "STATE_A") == 0);
    
    fail_unless((toobj = get_rule_obj(rule->objs, RULE_TO)) != NULL);
    fail_unless(strcmp((char *)toobj->data, "STATE_B") == 0);
    
    fail_unless((eventsobj = get_rule_obj(rule->objs, RULE_EVENTS)) != NULL);
    fail_unless(eventsobj->down != NULL);
    
    fail_unless(get_rule_obj(eventsobj->down, 0) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 0))->data, "event1") == 0);
    
    fail_unless((get_rule_obj(eventsobj->down, 0))->next == NULL);
    fail_unless((get_rule_obj(eventsobj->down, 0))->down == NULL);
    
    fail_unless((actionobj = get_rule_obj(rule->objs, RULE_RACTION)) != NULL);
    fail_unless((action = (struct r_action *) actionobj->data) != NULL);
    
    fail_unless(actionobj->next == NULL);
    fail_unless(actionobj->down == NULL);
    
    fail_unless(action->atype == NONE);
    
    action_free(actionobj->data);
    actionobj->data = NULL;
    rule_objs_free(actionobj);
    actionobj = NULL;
    eventsobj->next = NULL;
    rules_free(rule);
    line = "STATE_A STATE_B event1 NONE";
    rule = r_rule_parse(line);
    
    fail_unless(rule != NULL);
    
    fail_unless(rule->errors == NULL);
    
    fail_unless((fromobj = get_rule_obj(rule->objs, RULE_FROM)) != NULL);
    fail_unless(strcmp((char *)fromobj->data, "STATE_A") == 0);
    
    fail_unless((toobj = get_rule_obj(rule->objs, RULE_TO)) != NULL);
    fail_unless(strcmp((char *)toobj->data, "STATE_B") == 0);
    
    fail_unless((eventsobj = get_rule_obj(rule->objs, RULE_EVENTS)) != NULL);
    fail_unless(eventsobj->down != NULL);
    
    fail_unless(get_rule_obj(eventsobj->down, 0) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 0))->data, "event1") == 0);
    
    fail_unless((get_rule_obj(eventsobj->down, 0))->next == NULL);
    fail_unless((get_rule_obj(eventsobj->down, 0))->down == NULL);
    
    fail_unless((actionobj = get_rule_obj(rule->objs, RULE_RACTION)) != NULL);
    fail_unless((action = (struct r_action *) actionobj->data) != NULL);
    
    fail_unless(actionobj->next == NULL);
    fail_unless(actionobj->down == NULL);
    
    fail_unless(action->atype == NONE);
    
    action_free(actionobj->data);
    actionobj->data = NULL;
    rule_objs_free(actionobj);
    actionobj = NULL;
    eventsobj->next = NULL;
    rules_free(rule);
    
    line = "STATE_A STATE_B event1 NONE this shouldn't be here";
    rule = r_rule_parse(line);
    
    fail_unless(rule != NULL);
    
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    
    rules_free(rule);
}
END_TEST

START_TEST(test_parser_cmd_rule){
    struct r_rule *rule;
    struct rr_obj *fromobj,
                    *toobj,
                    *eventsobj,
                    *actionobj;
    struct r_action *action;
    struct cmd_action *cmdaction;
    char *line;
    line = "STATE_A STATE_B event1& event2 &event3& event4 CMD echo \"test\" >> /tmp/test";
    rule = r_rule_parse(line);
    fail_unless(rule != NULL);
    
    fail_unless(rule->errors == NULL);
    
    fail_unless((fromobj = get_rule_obj(rule->objs, RULE_FROM)) != NULL);
    fail_unless(strcmp((char *)fromobj->data, "STATE_A") == 0);
    
    fail_unless((toobj = get_rule_obj(rule->objs, RULE_TO)) != NULL);
    fail_unless(strcmp((char *)toobj->data, "STATE_B") == 0);
    
    fail_unless((eventsobj = get_rule_obj(rule->objs, RULE_EVENTS)) != NULL);
    fail_unless(eventsobj->down != NULL);
    
    fail_unless(get_rule_obj(eventsobj->down, 0) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 0))->data, "event1") == 0);
    
    fail_unless(get_rule_obj(eventsobj->down, 1) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 1))->data, "event2") == 0);
    
    fail_unless(get_rule_obj(eventsobj->down, 2) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 2))->data, "event3") == 0);
    
    fail_unless(get_rule_obj(eventsobj->down, 3) != NULL);
    fail_unless(strcmp((char *)(get_rule_obj(eventsobj->down, 3))->data, "event4") == 0);
    
    fail_unless((actionobj = get_rule_obj(rule->objs, RULE_RACTION)) != NULL);
    fail_unless((action = (struct r_action *) actionobj->data) != NULL);
    
    fail_unless(actionobj->next == NULL);
    fail_unless(actionobj->down == NULL);
    
    fail_unless(action->atype == CMD);
    cmdaction = (struct cmd_action *)action->action;
    fail_unless(strcmp(cmdaction->cmd, "echo \"test\" >> /tmp/test") == 0);
    
    action_free(actionobj->data);
    actionobj->data = NULL;
    rule_objs_free(actionobj);
    actionobj = NULL;
    eventsobj->next = NULL;
    rules_free(rule);
    
    line = "STATE_A STATE_B event1& event2 &event3& event4 CMD";
    rule = r_rule_parse(line);
    
    fail_unless(rule != NULL);
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
}
END_TEST

START_TEST(test_tokenizer_subexp_end){
    struct r_rule *rule;
    struct rr_expr rexpr;
    struct rr_expr subrexpr;
    char *line;
    
    rexpr.exprnum = 0;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &subrexpr;
    
    subrexpr.exprnum = 1;
    subrexpr.tokensep = '|';
    subrexpr.end = ' ';
    subrexpr.trim = true;
    subrexpr.next = NULL;
    subrexpr.subexpr = NULL;
    
    line = "A B | C";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors == NULL);
    fail_unless(rule->objs != NULL);
    
    fail_unless(strcmp(rule->objs->data, "A") == 0);
    fail_unless(rule->objs->next != NULL);
    fail_unless(rule->objs->down == NULL);
    
    fail_unless(rule->objs->next->data == NULL);
    fail_unless(rule->objs->next->next == NULL);
    fail_unless(rule->objs->next->down != NULL);
    
    fail_unless(strcmp(rule->objs->next->down->data, "B") == 0);
    fail_unless(rule->objs->next->down->next != NULL);
    fail_unless(rule->objs->next->down->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->down->next->data, "C") == 0);
    fail_unless(rule->objs->next->down->next->next == NULL);
    fail_unless(rule->objs->next->down->next->down == NULL);
    
    rules_free(rule);
    
    line = "A B";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors == NULL);
    fail_unless(rule->objs != NULL);
    
    fail_unless(strcmp(rule->objs->data, "A") == 0);
    fail_unless(rule->objs->next != NULL);
    fail_unless(rule->objs->down == NULL);
    
    fail_unless(rule->objs->next->data == NULL);
    fail_unless(rule->objs->next->next == NULL);
    fail_unless(rule->objs->next->down != NULL);
    
    fail_unless(strcmp(rule->objs->next->down->data, "B") == 0);
    fail_unless(rule->objs->next->down->next == NULL);
    fail_unless(rule->objs->next->down->down == NULL);
    
    rules_free(rule);
}
END_TEST

START_TEST(test_tokenizer_sep_end){
    struct r_rule *rule;
    struct rr_expr rexpr;
    struct rr_expr subrexpr;
    char *line;
    
    rexpr.exprnum = 0;
    rexpr.tokensep = '&';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &subrexpr;
    
    subrexpr.exprnum = 2;
    subrexpr.tokensep = '|';
    subrexpr.end = '&';
    subrexpr.trim = true;
    subrexpr.next = NULL;
    subrexpr.subexpr = NULL;
    
    line = "A & B & C | D | & E";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    
    rule_errors_free(rule->errors);
    
    line = "A & B & C | D &";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    
    rule_errors_free(rule->errors);
    
    line = "A & B & ";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    
    rule_errors_free(rule->errors);
}
END_TEST

START_TEST(test_tokenizer_tokens_lack){
    struct r_rule *rule;
    struct rr_expr rexpr;
    struct rr_expr subrexpr;
    struct rr_expr subrexpr2;
    char *line;
    
    rexpr.exprnum = 0;
    rexpr.tokensep = '|';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &subrexpr;
    
    subrexpr.exprnum = 2;
    subrexpr.tokensep = '&';
    subrexpr.end = '|';
    subrexpr.trim = true;
    subrexpr.next = &subrexpr;
    subrexpr.subexpr = NULL;
    
    subrexpr2.exprnum = 3;
    subrexpr2.tokensep = '\0';
    subrexpr2.end = '\0';
    subrexpr2.trim = true;
    subrexpr2.next = NULL;
    subrexpr2.subexpr = NULL;
    
    line = "A | B";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors == NULL);
    fail_unless(rule->objs != NULL);
    
    fail_unless(strcmp(rule->objs->data, "A") == 0);
    fail_unless(rule->objs->down == NULL);
    fail_unless(rule->objs->next != NULL);
    
    fail_unless(strcmp(rule->objs->next->data, "B") == 0);
    fail_unless(rule->objs->next->down == NULL);
    fail_unless(rule->objs->next->next == NULL);
    
    rules_free(rule);
        
    line = "A | B | C";
       
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors == NULL);
    fail_unless(rule->objs != NULL);
    
    fail_unless(strcmp(rule->objs->data, "A") == 0);
    fail_unless(rule->objs->down == NULL);
    fail_unless(rule->objs->next != NULL);
    
    fail_unless(strcmp(rule->objs->next->data, "B") == 0);
    fail_unless(rule->objs->next->down == NULL);
    fail_unless(rule->objs->next->next != NULL);
    
    fail_unless(rule->objs->next->next->data == NULL);
    fail_unless(rule->objs->next->next->next == NULL);
    fail_unless(rule->objs->next->next->down != NULL);
    
    fail_unless(strcmp(rule->objs->next->next->down->data, "C") == 0);
    fail_unless(rule->objs->next->next->down->next == NULL);
    fail_unless(rule->objs->next->next->down->down == NULL);
    
    rules_free(rule);
}
END_TEST

START_TEST(test_tokenizer_empty_subexpr){
    struct r_rule *rule;
    struct rr_expr rexpr;
    struct rr_expr subrexpr;
    char *line;
    
    rexpr.exprnum = 0;
    rexpr.tokensep = '|';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &subrexpr;
    
    subrexpr.exprnum = 2;
    subrexpr.tokensep = '&';
    subrexpr.end = '|';
    subrexpr.trim = true;
    subrexpr.next = NULL;
    subrexpr.subexpr = NULL;
    
    line = "A | B | | C";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors != NULL);
    fail_unless(rule->objs == NULL);
    rule_errors_free(rule->errors);
}
END_TEST

START_TEST(test_tokenizer_comments){
    struct r_rule *rule;
    struct rr_expr rexpr;
    char *line;
    
    line = "A | B # This is a very simple rule";
    
    rexpr.exprnum = 0;
    rexpr.tokensep = '|';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = NULL;
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->objs != NULL);
    fail_unless(rule->errors == NULL);
    
    fail_unless(strcmp(rule->objs->data, "A") == 0);
    fail_unless(rule->objs->next != NULL);
    fail_unless(rule->objs->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->data, "B") == 0);
    fail_unless(rule->objs->next->next == NULL);
    fail_unless(rule->objs->next->down == NULL);
    
    rules_free(rule);
    
    line = "# This is comment line";
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->objs == NULL);
    fail_unless(rule->errors == NULL);
    rules_free(rule);
}
END_TEST

START_TEST(test_tokenizer_notrim){
    struct r_rule *rule;
    struct rr_expr rexpr;
    struct rr_expr iexpr;
    char *line;
    
    line = "A  |B| C|z y x| D";
    
    rexpr.exprnum = 0;
    rexpr.tokensep = '|';
    rexpr.end = '\0';
    rexpr.trim = false;
    rexpr.next = NULL;
    rexpr.subexpr = &iexpr;
    
    iexpr.exprnum = 3;
    iexpr.tokensep = ' ';
    iexpr.end = '|';
    iexpr.trim = false;
    iexpr.next = NULL;
    iexpr.subexpr = NULL;
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->objs != NULL);
    fail_unless(rule->errors == NULL);
    
    fail_unless(strcmp(rule->objs->data, "A  ") == 0);
    fail_unless(rule->objs->next != NULL);
    fail_unless(rule->objs->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->data, "B") == 0);
    fail_unless(rule->objs->next->next != NULL);
    fail_unless(rule->objs->next->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->next->data, " C") == 0);
    fail_unless(rule->objs->next->next->next != NULL);
    fail_unless(rule->objs->next->next->down == NULL);
    
    fail_unless(rule->objs->next->next->next->data == NULL);
    fail_unless(rule->objs->next->next->next->next != NULL);
    fail_unless(rule->objs->next->next->next->down != NULL);
    
    fail_unless(strcmp(rule->objs->next->next->next->down->data, "z") == 0);
    fail_unless(rule->objs->next->next->next->down->next != NULL);
    fail_unless(rule->objs->next->next->next->down->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->next->next->down->next->data, "y") == 0);
    fail_unless(rule->objs->next->next->next->down->next->next != NULL);
    fail_unless(rule->objs->next->next->next->down->next->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->next->next->down->next->data, "x") == 0);
    fail_unless(rule->objs->next->next->next->down->next->next == NULL);
    fail_unless(rule->objs->next->next->next->down->next->down == NULL);
    
    fail_unless(strcmp(rule->objs->next->next->next->next->data, " D") == 0);
    fail_unless(rule->objs->next->next->next->next->next == NULL);
    fail_unless(rule->objs->next->next->next->next->down == NULL);
    rules_free(rule);

}
END_TEST

START_TEST(test_tokenizer_empty_rule){
    struct r_rule *rule;
    struct rr_expr rexpr;
    char *line;
    
    line = " ";
    
    rexpr.exprnum = 0;
    rexpr.tokensep = '&';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = NULL;
    
    rule = tokenize_rule(line, &rexpr);
    
    fail_unless(rule->errors == NULL);
    fail_unless(rule->objs == NULL);
}
END_TEST

START_TEST(test_tokenizer_empty_token){
    // Return the correct error and everything else clean
    struct r_rule *rrule;
    struct rr_expr rexpr;
    struct rr_expr evexpr;
    struct rr_expr actexpr;
    char *expectedmsg;
    char *line;
    line = "A B e1 & & e5 PROP localhost";
    
    rexpr.exprnum = 0;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &evexpr;
    
    evexpr.exprnum = 2;
    evexpr.tokensep = '&';
    evexpr.end = ' ';
    evexpr.trim = true;
    evexpr.next = &actexpr;
    evexpr.subexpr = NULL;
    
    actexpr.exprnum = 4;
    actexpr.tokensep = '\0';
    actexpr.end = '\0';
    actexpr.trim = true;
    actexpr.next = NULL;
    actexpr.subexpr = NULL;
    
    rrule = tokenize_rule(line, &rexpr);
    
    fail_unless(rrule->errors != NULL);
    fail_unless(rrule->objs == NULL);
    rule_errors_free(rrule->errors);
    
    rrule->line = "A B &e1 & e5 PROP localhost";
    
    rrule = tokenize_rule(line, &rexpr);
    
    fail_unless(rrule->errors != NULL);
    fail_unless(rrule->objs == NULL);
    rule_errors_free(rrule->errors);
}
END_TEST

START_TEST(test_tokenizer_reactor_rule_short){
    struct r_rule *rrule;
    struct rr_expr rexpr;
    struct rr_expr evexpr;
    struct rr_expr actexpr;
    char *expectedmsg;
    char *line;
    
    expectedmsg = "'%s' token content was expected instead of '%s'";
    line = "A B e1 PROP localhost";
    
    rexpr.exprnum = 0;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &evexpr;
    
    evexpr.exprnum = 2;
    evexpr.tokensep = '&';
    evexpr.end = ' ';
    evexpr.trim = true;
    evexpr.next = &actexpr;
    evexpr.subexpr = NULL;
    
    actexpr.exprnum = 4;
    actexpr.tokensep = '\0';
    actexpr.end = '\0';
    actexpr.trim = true;
    actexpr.next = NULL;
    actexpr.subexpr = NULL;
    
    rrule = tokenize_rule(line, &rexpr);
    
    fail_unless(rrule->errors == NULL);
    
    fail_unless(rrule->objs != NULL);
    
    fail_unless(strcmp(rrule->objs->data, "A") == 0,
        expectedmsg, "A", rrule->objs->data
    );
    fail_unless(rrule->objs->down == NULL);
    fail_unless(rrule->objs->next != NULL);
    
    fail_unless(strcmp(rrule->objs->next->data, "B") == 0,
        expectedmsg, "B", rrule->objs->next->data
    );
    fail_unless(rrule->objs->next->down == NULL);
    fail_unless(rrule->objs->next->next != NULL);
    
    fail_unless(rrule->objs->next->next->data == NULL);
    fail_unless(rrule->objs->next->next->down != NULL);
    fail_unless(rrule->objs->next->next->next != NULL);

    fail_unless(strcmp(rrule->objs->next->next->down->data, "e1") == 0,
        expectedmsg, "e1", rrule
    );
    fail_unless(rrule->objs->next->next->down->down == NULL);
    fail_unless(rrule->objs->next->next->down->next == NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->next->data, "PROP") == 0,
        expectedmsg, "PROP", rrule->objs->next->next->next->data
    );
    fail_unless(rrule->objs->next->next->next->down == NULL);
    fail_unless(rrule->objs->next->next->next->next != NULL);

    fail_unless(rrule->objs->next->next->next->next->data == NULL);
    fail_unless(rrule->objs->next->next->next->next->down != NULL);
    fail_unless(rrule->objs->next->next->next->next->next == NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->next->next->down->data, "localhost") == 0,
        expectedmsg, "localhost", rrule->objs->next->next->next->data
    );
    fail_unless(rrule->objs->next->next->next->next->down->down == NULL);
    fail_unless(rrule->objs->next->next->next->next->down->next == NULL);
    rules_free(rrule);
}
END_TEST

START_TEST(test_tokenizer_reactor_rule_long){
    struct r_rule *rrule;
    struct rr_expr rexpr;
    struct rr_expr evexpr;
    struct rr_expr actexpr;
    char *line;
    char *expectedmsg;
    
    expectedmsg = "'%s' token content was expected instead of '%s'";
    line = "A  B e1 &e2 & e3&e4& e5  CMD echo \"A->B\" >> /tmp/test";
    
    rexpr.exprnum = 0;
    rexpr.tokensep = ' ';
    rexpr.end = '\0';
    rexpr.trim = true;
    rexpr.next = NULL;
    rexpr.subexpr = &evexpr;
    
    evexpr.exprnum = 2;
    evexpr.tokensep = '&';
    evexpr.end = ' ';
    evexpr.trim = true;
    evexpr.next = &actexpr;
    evexpr.subexpr = NULL;
    
    actexpr.exprnum = 4;
    actexpr.tokensep = '\0';
    actexpr.end = '\0';
    actexpr.trim = true;
    actexpr.next = NULL;
    actexpr.subexpr = NULL;
    
    rrule = tokenize_rule(line, &rexpr);

    fail_unless(rrule->errors == NULL);
    
    fail_unless(rrule->objs != NULL);
    
    fail_unless(strcmp(rrule->objs->data, "A") == 0,
        expectedmsg, "A", rrule->objs->data
    );
    fail_unless(rrule->objs->down == NULL);
    fail_unless(rrule->objs->next != NULL);
    
    fail_unless(strcmp(rrule->objs->next->data, "B") == 0,
        expectedmsg, "B", rrule->objs->next->data
    );
    fail_unless(rrule->objs->next->down == NULL);
    fail_unless(rrule->objs->next->next != NULL);
    
    fail_unless(rrule->objs->next->next->data == NULL);
    fail_unless(rrule->objs->next->next->down != NULL);
    fail_unless(rrule->objs->next->next->next != NULL);

    fail_unless(strcmp(rrule->objs->next->next->down->data, "e1") == 0,
        expectedmsg, "e1", rrule
    );
    fail_unless(rrule->objs->next->next->down->down == NULL);
    fail_unless(rrule->objs->next->next->down->next != NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->down->next->data, "e2") == 0,
        expectedmsg, "e2", rrule
    );
    fail_unless(rrule->objs->next->next->down->next->down == NULL);
    fail_unless(rrule->objs->next->next->down->next->next != NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->down->next->next->data, "e3") == 0,
        expectedmsg, "e3", rrule
    );
    fail_unless(rrule->objs->next->next->down->next->next->down == NULL);
    fail_unless(rrule->objs->next->next->down->next->next->next != NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->down->next->next->next->data, "e4") == 0,
        expectedmsg, "e4", rrule
    );
    fail_unless(rrule->objs->next->next->down->next->next->next->down == NULL);
    fail_unless(rrule->objs->next->next->down->next->next->next->next != NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->down->next->next->next->next->data, "e5") == 0,
        expectedmsg, "e5", rrule
    );
    fail_unless(rrule->objs->next->next->down->next->next->next->next->down == NULL);
    fail_unless(rrule->objs->next->next->down->next->next->next->next->next == NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->next->data, "CMD") == 0,
        expectedmsg, "CMD", rrule->objs->next->next->next->data
    );
    fail_unless(rrule->objs->next->next->next->down == NULL);
    fail_unless(rrule->objs->next->next->next->next != NULL);

    fail_unless(rrule->objs->next->next->next->next->data == NULL);
    fail_unless(rrule->objs->next->next->next->next->down != NULL);
    fail_unless(rrule->objs->next->next->next->next->next == NULL);
    
    fail_unless(strcmp(rrule->objs->next->next->next->next->down->data, "echo \"A->B\" >> /tmp/test") == 0,
        expectedmsg, "echo \"A->B\" >> /tmp/test", rrule->objs->next->next->next->data
    );
    fail_unless(rrule->objs->next->next->next->next->down->down == NULL);
    fail_unless(rrule->objs->next->next->next->next->down->next == NULL);
    rules_free(rrule);
}
END_TEST

Suite* make_rules_suite(void){
    int msgsize;
    int msgtype;
    Suite *s = suite_create("Rules");

    /* Core test case */

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_tokenizer_subexp_end);
    tcase_add_test(tc_core, test_tokenizer_sep_end);
    tcase_add_test(tc_core, test_tokenizer_reactor_rule_long);
    tcase_add_test(tc_core, test_tokenizer_reactor_rule_short);
    tcase_add_test(tc_core, test_tokenizer_empty_token);
    tcase_add_test(tc_core, test_tokenizer_comments);
    tcase_add_test(tc_core, test_tokenizer_tokens_lack);
    tcase_add_test(tc_core, test_tokenizer_empty_subexpr);
    tcase_add_test(tc_core, test_tokenizer_empty_rule);
    tcase_add_test(tc_core, test_parser_cmd_rule);
    tcase_add_test(tc_core, test_parser_none_rule);
    tcase_add_test(tc_core, test_parser_prop_rule);
    suite_add_tcase(s, tc_core);
    return s;
}