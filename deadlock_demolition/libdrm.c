/**
 * Deadlock Demolition
 * CS 241 - Fall 2019
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>
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

bool cycleHelper(void * vertex, vector * visited, vector * recStack) {
    vector_push_back(visited, vertex);
    vector_push_back(recStack, vertex);

    int rec_idx = vector_index_of(recStack, vertex);

    vector * neighbors = graph_neighbors(ra_graph, vertex);

    for (size_t n_idx = 0; n_idx < vector_size(neighbors); n_idx++) {
        void * neighbor = vector_get(neighbors, n_idx);
        bool alreadyVisited = vector_index_of(visited, neighbor) != -1;
        bool rec_result = cycleHelper(neighbor, visited, recStack);

        if (!alreadyVisited && rec_result) {
            return true;
        } else if (vector_index_of(recStack, neighbor) != -1) {
            return true;
        }
    }

    vector_erase(recStack, rec_idx);
    return false;
}

bool detectCycle() {
    vector * visited = shallow_vector_create();
    vector * rec_stack = shallow_vector_create();

    vector * vertices = graph_vertices(ra_graph);

    for (size_t idx = 0; idx < vector_size(vertices); idx++) {
        void * current_vtx = vector_get(vertices, idx);
        if (cycleHelper(current_vtx, visited, rec_stack)) {
            return true;
        }
    }

    vector_destroy(visited);
    vector_destroy(rec_stack);

    return false;
}

drm_t *drm_init() {
    drm_t * new_drm = (drm_t *) malloc(sizeof(drm_t));
    //lazy initialization
    if (!ra_graph) {
        ra_graph = shallow_graph_create();
    }

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
