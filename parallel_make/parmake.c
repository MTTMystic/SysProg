/**
 * Parallel Make Lab
 * CS 241 - Fall 2019
 */
 

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>


static graph * dep_graph;

/**Status numbers meanings
 * 0: Rule unsatisfied
 * 1: Rule satisfied
 * 2: Rule dropped
 * 3: Rule failed
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

        vector * neighbors = graph_neighbors(dep_graph, target);

        for (size_t n_idx = 0; n_idx < vector_size(neighbors); n_idx++) {
            char * neighbor = (char *) vector_get(neighbors, n_idx);
            int n_vidx = vector_idx_of(targets, neighbor);
            char already_visited = visited[n_vidx];
            
            if (!already_visited && cycleHelper(neighbor, n_vidx, targets, visited, rec_stack)) {
                vector_destroy(neighbors);
                return 1;
            } else if (rec_stack[n_vidx]) {
                vector_destroy(neighbors);
                return 1;
            }
            
        }

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

void satisfy_rule(rule_t * rule) {
    
    /*Rule should only run again when:
        1) It is the name of a file on disk.
        2) It only depends on other files on disk.
        3) One of its dependencies has been updated more recently than it has.
    */
    if (is_fod(rule) && depends_on_fod(rule)) {
        int run_again = newer_file_change(rule);
        rule->state = !run_again;    //mark rule as unsatisfied if necessary
    }

    if (rule->state == 1 || rule->state == 3) {
        return;     //don't satisfy rules already ran and/or failed
    }


    //recursively satisfy this rule's dependencies
    vector * deps = graph_neighbors(dep_graph, rule->target);
    for(size_t d_idx = 0; d_idx < vector_size(deps); d_idx++) {
        char * dep_target = vector_get(deps, d_idx);
        rule_t * dep_rule = (rule_t *) graph_get_vertex_value(dep_graph, dep_target);
        
        satisfy_rule(dep_rule);

        //do not satisfy dep rules that have already failed
        if (dep_rule->state == 3) {
            rule->state = 3;
        }  
    }

    vector_destroy(deps);

    size_t c_idx = 0;
    char * command;

    while (rule->state != 3 && c_idx < vector_size(rule->commands)) {
        command = vector_get(rule->commands, c_idx);
        int command_result = system(command);
        if (command_result != 0) {
            rule->state = 3;
            return;
        }

        c_idx++;
    }

    rule->state = 1;
    return;    
}

void satisfy_goal_rules() {
    vector * goal_rules = graph_neighbors(dep_graph, ""); //get goal rules
    for (size_t g_idx = 0; g_idx < vector_size(goal_rules); g_idx++) {
        char * target = (char *) vector_get(goal_rules, g_idx);
        rule_t * rule = graph_get_vertex_value(dep_graph, target);
        int hasCycle = detectCycle(target);
        if (hasCycle) {
            rule->state = 2; //skip goal rules in cycle (or whose dependents are)
            print_cycle_failure(target);
            continue;
        }
        satisfy_rule(rule);
    }

    vector_destroy(goal_rules);
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!

    //parse makefile to get dependency graph
    dep_graph = parser_parse_makefile(makefile, targets);

    satisfy_goal_rules();

    graph_destroy(dep_graph);
    return 0;
}
