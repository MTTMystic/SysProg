/**
 * Parallel Make Lab
 * CS 241 - Fall 2019
 */
 

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include <stdio.h>

static graph * dep_graph;


/**Status numbers meanings
 * 0: Rule unsatisfied
 * 1: Rule satisfied
 * 2: Rule dropped due to cyclical dependency
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
    }

    rec_stack[t_idx] = 0;
    return 0;
}

int detectCycle(char * target) {
    vector * targets = graph_vertices(dep_graph);
    size_t n_targets = vector_size(targets);

    printf("n targets is: %zu\n", n_targets);

    char visited[n_targets];
    char rec_stack[n_targets];

    for (size_t idx = 0; idx < n_targets; idx++) {
        visited[idx] = 0;
        rec_stack[idx] = 0;
    }

    int t_idx = vector_idx_of(targets, target);
    printf("idx of this target is: %d\n", t_idx);
    return cycleHelper(target, t_idx, targets, visited, rec_stack);
}

void satisfy_rule(rule_t * rule) {
    //figure out if this rule depends on any other rules
    vector * depends = graph_neighbors(dep_graph, rule->target);
    //check for a cycle in the graph involving this rule
    if (detectCycle(rule->target)) {
        vector_destroy(depends);
        rule->state = 2;
        return;
    }
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!

    //parse makefile to get dependency graph
    dep_graph = parser_parse_makefile(makefile, targets);

    vector * goal_rules = graph_neighbors(dep_graph, "");
    for (size_t r_idx = 0; r_idx < vector_size(goal_rules); r_idx++) {
        char * target = vector_get(goal_rules, r_idx);
        rule_t * rule = (rule_t *) graph_get_vertex_value(dep_graph, target);
        satisfy_rule(rule);
        //announce if a goal rule is dropped because of cyclical dependencies
        if (rule->state == 2) {
            print_cycle_failure(target);
        }

    }   

    return 0;
}
