/**
 * Deadlock Demolition
 * CS 241 - Fall 2019
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>
#include <stdio.h>
//global ra graph for deadlock prevention
static graph * ra_graph;
//mutex to prevent concurrent modifications to ra graph
static pthread_mutex_t ts_mut;

struct drm_t {
    //plain-jane mutex lock
    pthread_mutex_t mut;
    //tracks which thread currently holds the lock
    pthread_t * holder;
};

int vector_index_of(vector * v, void * elem) {
    for (size_t idx = 0; idx < vector_size(v); idx++) {
        if (vector_get(v, idx) == elem) {
            return idx;
        }
    }

    return -1;
}

bool cycleHelper(void * vertex, int idx, vector * vertices, bool * visited, bool * recStack) {
    visited[idx] = true;
    recStack[idx] = true;

    vector * neighbors = graph_neighbors(ra_graph, vertex);

    for (size_t n_idx = 0; n_idx < vector_size(neighbors); n_idx++) {
        void * neighbor = vector_get(neighbors, n_idx);
        int neighbor_vidx = vector_index_of(vertices, neighbor);
        bool alreadyVisited = visited[neighbor_vidx] == true;
        bool rec_result = cycleHelper(neighbor, neighbor_vidx, vertices, visited, recStack);

        if (!alreadyVisited && rec_result) {
            vector_destroy(neighbors);
            return true;
        } else if (recStack[neighbor_vidx] == true) {
            vector_destroy(neighbors);
            return true;
        }
    }

    recStack[idx] = false;
    vector_destroy(neighbors);
    return false;
}

bool detectCycle() {
    vector * vertices = graph_vertices(ra_graph);
    size_t n_vertices = vector_size(vertices);

    bool visited[n_vertices];
    bool rec_stack[n_vertices];

    for (size_t i = 0; i < n_vertices; i++) {
        visited[i] = false;
        rec_stack[i] = false;
    }

    for (size_t idx = 0; idx < n_vertices; idx++) {
        void * current_vtx = vector_get(vertices, idx);
        if (cycleHelper(current_vtx, idx, vertices, visited, rec_stack)) {
            vector_destroy(vertices);
            return true;
        }
    }

    vector_destroy(vertices);
    return false;
}

drm_t *drm_init() {
    drm_t * new_drm = (drm_t *) malloc(sizeof(drm_t));
    //lazy initialization
    if (!ra_graph) {
        ra_graph = shallow_graph_create();
    }

    graph_add_vertex(ra_graph, (void *) new_drm);
    pthread_mutex_init(&ts_mut, NULL);
    pthread_mutex_init(&new_drm->mut, NULL);
    new_drm->holder = NULL; //no thread currently holds the lock
    return new_drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    bool thread_in_rag = graph_contains_vertex(ra_graph, (void *) thread_id);

    pthread_mutex_lock(&ts_mut); //lock this mutex to prevent concurrent graph changes
    bool thread_can_unlock = thread_in_rag && graph_adjacent(ra_graph, (void *) drm, (void *) thread_id);
    pthread_mutex_unlock(&ts_mut); //unlock graph change mutex

    //check that the given thread_id exists in the graph
    if (!thread_can_unlock) {     
        return 0; //return failure
    }


    pthread_mutex_lock(&ts_mut);
    //remove edge from drm to thread
    graph_remove_edge(ra_graph, (void *) drm, (void *) thread_id);
    pthread_mutex_unlock(&ts_mut);

    drm->holder = NULL; //indicate that no thread currently holds this lock

    pthread_mutex_unlock(&drm->mut); //unlock mutex

    return 1;   //return success

}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    //check that the given thread does not already own the lock
    bool thread_in_rag = graph_contains_vertex(ra_graph, (void *) thread_id);
    
    pthread_mutex_lock(&ts_mut); //prevent concurrent graph changes
    bool thread_owns_lock = thread_in_rag && graph_adjacent(ra_graph, (void *) drm, (void *) thread_id);
    pthread_mutex_unlock(&ts_mut); //prevent concurrent graph changes

    if (thread_owns_lock) {
        return 0; //return lock failure
    }

    if (!thread_in_rag) {
        //add thread id to graph
        graph_add_vertex(ra_graph, (void *) thread_id);
    }

    pthread_mutex_lock(&ts_mut);

    //add a request edge 
    graph_add_edge(ra_graph, (void *) thread_id, (void *) drm);

    //check for a cycle after adding the request edge
    bool deadlock_created = detectCycle();

    if (deadlock_created) {
        //if adding the request edge caused deadlock, remove it and return lock failure
        graph_remove_edge(ra_graph, (void *) thread_id, (void *) drm);
        pthread_mutex_unlock(&ts_mut); //unlock graph change mutex
        return 0; //lock failure
    }

    pthread_mutex_unlock(&ts_mut);

    //if no deadlock will happen, check that the lock is not already 'owned'
    pthread_mutex_lock(&drm->mut);
    
    pthread_mutex_lock(&ts_mut);
    graph_remove_edge(ra_graph, (void *) thread_id, (void *) drm);
    graph_add_edge(ra_graph, (void *) drm, (void *) thread_id);
    drm->holder = thread_id;
    pthread_mutex_unlock(&ts_mut);

    /* Your code here */
    return 1;
}

void drm_destroy(drm_t *drm) {
    graph_remove_vertex(ra_graph, (void *) drm);
    pthread_mutex_destroy(&ts_mut);
    pthread_mutex_destroy(&drm->mut);
    free(drm);
    return;
}
