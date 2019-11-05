/**
 * Parallel Make Lab
 * CS 241 - Fall 2019
 */
 

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <pthread.h>

static graph * dep_graph;
static set * rules;
static pthread_mutex_t rules_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static  pthread_cond_t  rule_cv = PTHREAD_COND_INITIALIZER;

static pthread_t * workers;

/**Status numbers meanings
 * -1: Rule failed
 * 0: Rule unsatisfied
 * 1: Rule satisfied
 * 3: Rule ready to run
 * 4: Rule running
 * 
 * */
//cycle detection code based on algorithm found at geeksforgeeks and adapted for this purpose

int vector_idx_of(vector * v, char * elem) {
    for (size_t idx = 0; idx < vector_size(v); idx++) {
        char * c_v = (char *) vector_get(v, idx);
        if (!strcmp(c_v, elem)) {
            return idx;
        }       
    }
    return -1;
}

int cycleHelper(char * target, int t_idx, vector * targets, char visited[], char rec_stack[]){
    if (!visited[t_idx]) {
        
        visited[t_idx] = 1;
        rec_stack[t_idx] = 1;

        rule_t * rule = (rule_t *) graph_get_vertex_value(dep_graph, target);
        vector * neighbors = graph_neighbors(dep_graph, target);

        for (size_t n_idx = 0; n_idx < vector_size(neighbors); n_idx++) {
            char * neighbor = (char *) vector_get(neighbors, n_idx);
            int n_vidx = vector_idx_of(targets, neighbor);
            char already_visited = visited[n_vidx];
            //rule_t * n_rule = (rule_t *) graph_get_vertex_value(dep_graph, neighbor);

            if (!already_visited && cycleHelper(neighbor, n_vidx, targets, visited, rec_stack)) {
                rule->state = -1;
                vector_destroy(neighbors);
                return 1;
            } else if (rec_stack[n_vidx]) {
                rule->state = -1;
                vector_destroy(neighbors);
                return 1;
            }
            
        }

        rule->state = 0;
        vector_destroy(neighbors);
    }

    
    rec_stack[t_idx] = 0;
    return 0;
}

int detectCycle(char * target) {
    vector * targets = graph_vertices(dep_graph);
    size_t n_targets = vector_size(targets);
    char visited[n_targets];
    char rec_stack[n_targets];

    for (size_t idx = 0; idx < n_targets; idx++) {
        visited[idx] = 0;
        rec_stack[idx] = 0;
    }

    int t_idx = vector_idx_of(targets, target);
    int hasCycle = cycleHelper(target, t_idx, targets, visited, rec_stack);
    
    vector_destroy(targets);
    return hasCycle;
}

int is_fod(rule_t * rule) {
    return access(rule->target, R_OK) != -1;
}

int depends_on_fod(rule_t * rule) {
    vector * deps = graph_neighbors(dep_graph, rule->target);
    for (size_t d_idx = 0; d_idx < vector_size(deps); d_idx++) {
        char * dep_target = vector_get(deps, d_idx);
        rule_t * dep_rule = (rule_t *) graph_get_vertex_value(dep_graph, dep_target);
        if (!is_fod(dep_rule)) {
            return 0;
        }
    }

    vector_destroy(deps);
    return 1;
}

int newer_file_change(rule_t * rule) {
    vector * deps = graph_neighbors(dep_graph, rule->target);
    struct stat rule_stat;
    struct stat dep_stat;

    stat(rule->target, &rule_stat);

    struct timespec rule_mtim = rule_stat.st_mtim;

    char dep_newer = 0;
    for (size_t dep_idx = 0; dep_idx < vector_size(deps); dep_idx++) {
        char * target = vector_get(deps, dep_idx);
        rule_t * dep_rule = (rule_t *) graph_get_vertex_value(dep_graph, target);

        stat(dep_rule->target, &dep_stat);

        struct timespec dep_mtim = dep_stat.st_mtim;

        dep_newer = (rule_mtim.tv_sec < dep_mtim.tv_sec) || (rule_mtim.tv_nsec < dep_mtim.tv_nsec);

        if (dep_newer) {
            vector_destroy(deps);
            return 1;
        }
    
    }

    vector_destroy(deps);
    return 0;
}


void parse_targets_helper(char * target) {
    rule_t * rule = (rule_t *) graph_get_vertex_value(dep_graph, target);

    //if (rule->state == -1) {
        //return; 
    //}

    set_add(rules, rule);
    vector * neighbors = graph_neighbors(dep_graph, target);
    for (size_t n_idx = 0; n_idx < vector_size(neighbors); n_idx++) {
        char * neighbor = (char *) vector_get(neighbors, n_idx);
        rule_t * n_rule = (rule_t *) graph_get_vertex_value(dep_graph, neighbor);
        set_add(rules, n_rule);
        parse_targets_helper(neighbor);
    }

    vector_destroy(neighbors);
}

void parse_targets() {
    vector * goal_rules = graph_neighbors(dep_graph, "");
    for (size_t idx = 0; idx < vector_size(goal_rules); idx++) {
        char * target = (char *) vector_get(goal_rules, idx);
        parse_targets_helper(target);
    }

    vector_destroy(goal_rules); 
}


int mark_cyclic(char * target) {
    int isCyclic = detectCycle(target);
    if (isCyclic) {
        rule_t * rule = graph_get_vertex_value(dep_graph, target);
        rule->state = -1;
        return 1;
    } else {
        return 0;
    }
}

void drop_cyclic_goals() {
    vector * goals = graph_neighbors(dep_graph, "");
    for (size_t idx = 0; idx < vector_size(goals); idx++) {
        char * target = vector_get(goals, idx);
        int cyclic = mark_cyclic(target);
        if (cyclic) {
            print_cycle_failure(target);
        }
    }
    vector_destroy(goals);
}

void clear_rules() {
    vector * rules_vec = set_elements(rules);
    for (size_t idx = 0; idx < vector_size(rules_vec); idx++) {
        rule_t * rule = (rule_t *) vector_get(rules_vec, idx);
        if (rule->state != 0 && rule->state != 3) {
            set_remove(rules, rule);
            //printf("Removed %s from rules.\n", rule->target);
        }
    }
}

void check_rule(rule_t * rule) {
    pthread_mutex_lock(&lock);
    if (rule->state != 0) {
        return;
    }

    //printf("This is a call to graph_neighbors for %s.\n", rule->target);
    vector * deps = graph_neighbors(dep_graph, rule->target);
    //printf("Yep! This call to graph_neighbors for %s worked.\n", rule->target);
    int failed_dep = 0;
    int deps_satisfied = 1;
    for (size_t d_idx = 0; d_idx < vector_size(deps); d_idx++) {
        char * dep = (char *) vector_get(deps, d_idx);
        rule_t * d_rule = (rule_t *) graph_get_vertex_value(dep_graph, dep);

        if (d_rule->state == -1) {
            failed_dep = 1;
            break;
        }

        if (d_rule->state != 1) {
            deps_satisfied = 0;
            break;
        }
    }

    if (failed_dep) {
        rule->state = -1;
    } else if (deps_satisfied) {
        rule->state = 3;
    } else {
        rule->state = 0;
    }

    vector_destroy(deps);
}

rule_t * find_rule() {
    vector * rules_vec = set_elements(rules);
    for (size_t idx = 0; idx < vector_size(rules_vec); idx++) {
        rule_t * rule = (rule_t *) vector_get(rules_vec, idx);
        check_rule(rule);
        if (rule->state == 3) {
            vector_destroy(rules_vec);
            return rule;
        }
    }

    vector_destroy(rules_vec);
    return NULL;
}

void execute_rule(rule_t * rule) {
    rule->state = 4;

    if (is_fod(rule) && depends_on_fod(rule)) {
        if (!newer_file_change(rule)) {
            rule->state = 0;
            return;
        }
    }

    for (size_t c_idx = 0; c_idx < vector_size(rule->commands); c_idx++) {
        char * command = (char *) vector_get(rule->commands, c_idx);
        int ret_val = system(command);
        if (ret_val != 0) {
            rule->state = -1;
            return;
        }
    }

    rule->state = 1;
}

void worker_exit() {
   int remaining = set_cardinality(rules);

   if (remaining == 0) {
       pthread_cond_broadcast(&rule_cv);
       pthread_mutex_unlock(&rules_lock);
       pthread_exit(0);
   } 
}

void worker_loop() {
    while(1) {
        pthread_mutex_lock(&rules_lock);
        rule_t * exec_rule = NULL;
        
        while (1) {
            clear_rules();
            worker_exit();
            size_t idx = 0;
            vector * rules_vec = set_elements(rules);
            exec_rule = find_rule();

            vector_destroy(rules_vec);
            if (exec_rule == NULL) {
                worker_exit();
                pthread_cond_wait(&rule_cv, &rules_lock);
            } else {
                break;
            }
        }

        pthread_mutex_unlock(&rules_lock);
        execute_rule(exec_rule);
        pthread_cond_broadcast(&rule_cv);
    }
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!

    //parse makefile to get dependency graph
    dep_graph = parser_parse_makefile(makefile, targets);

    rules = shallow_set_create();
    workers = calloc(num_threads, sizeof(pthread_t));

    drop_cyclic_goals();
    parse_targets();
    
    for (size_t idx = 0; idx < num_threads; idx++) {
        pthread_create(&workers[idx], NULL, (void *) worker_loop, NULL);
    }

    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(workers[i], NULL);
    }

    graph_destroy(dep_graph);
    set_destroy(rules);
    pthread_mutex_destroy(&rules_lock);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&rule_cv);

    return 0;
}
