/**
 * Savvy Scheduler
 * CS 241 - Fall 2019
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "print_functions.h"

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
    int job_num; //number associated with job
    int priority; //job's current priority
    double runtime;
    double ex_time;

    double robin_time;

    double arrival_time; //when job arrived for scheduling
    double start_time;//when process really started
    double end_time; //process end time
    
} job_info;


int n_jobs;
double wait_time;
double turnaround_time;
double res_time;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    // TODO: Implement me!
    job * job_a = (job *) a;
    job * job_b = (job *) b;

    job_info * ja = (job_info *) job_a->metadata;
    job_info * jb = (job_info *) job_b->metadata;

    if (ja->arrival_time < jb->arrival_time) {
        return -1;
    } else if (ja->arrival_time > jb->arrival_time) {
        return 1;
    } else {
        return 0;
    }
} 

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    // TODO: Implement me!
    job * job_a = (job *) a;
    job * job_b = (job *) b;

    job_info * ja = (job_info *) job_a->metadata;
    job_info * jb = (job_info *) job_b->metadata;

    if (ja->priority < jb->priority) {
        return -1;
    } else if (ja->priority > jb->priority) {
        return 1;
    } else {
        return break_tie(a, b);
    }

}

int comparer_psrtf(const void *a, const void *b) {
    job * job_a = (job *) a;
    job * job_b = (job *) b;

    job_info * ja = (job_info *) job_a->metadata;
    job_info * jb = (job_info *) job_b->metadata;

    double a_rt = ja->runtime - ja->ex_time;
    double b_rt = jb->runtime - jb->ex_time;
    //remaining time = runtime - extime
    if (a_rt < b_rt) {
        return -1;
    } else if (a_rt > b_rt) {
        return 1;
    } else {
        return break_tie(a, b);
    }
}

int comparer_rr(const void *a, const void *b) {
    // TODO: Implement me!
    job * job_a = (job *) a;
    job * job_b = (job *) b;

    job_info * ja = (job_info *) job_a->metadata;
    job_info * jb = (job_info *) job_b->metadata;

    if (ja->robin_time < jb->robin_time){
        return -1;
    } else if (ja->robin_time > jb->robin_time) {
        return 1;
    } else {
        return break_tie(a, b);
    }
    
}

int comparer_sjf(const void *a, const void *b) {
    job * job_a = (job *) a;
    job * job_b = (job *) b;

    job_info * ja = (job_info *) job_a->metadata;
    job_info * jb = (job_info *) job_b->metadata;

    if (ja->runtime < jb->runtime) {
        return -1;
    } else if (ja->runtime > jb->runtime) {
        return 1;
    } else {
        return break_tie(a, b);
    }
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO: Implement me!
    job_info * meta = (job_info *) malloc(sizeof(job_info));
    
    meta->runtime = sched_data->running_time;
    meta->priority = sched_data->priority;
    meta->job_num = job_number;
    meta->arrival_time = time;
    meta->start_time = 0;
    meta->end_time = 0;
    meta->robin_time = 0;

    newjob->metadata = meta;
    
    n_jobs++;
    priqueue_offer(&pqueue, newjob);
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    bool pre_scheme = pqueue_scheme == PPRI || pqueue_scheme == PSRTF || pqueue_scheme == RR;
    
    if (job_evicted) {
        job_info * j = (job_info *) job_evicted->metadata;
        j->ex_time = (time - j->start_time);
        j->robin_time = time;
        if (!pre_scheme) {
            return job_evicted;
        }
        priqueue_offer(&pqueue, job_evicted);
    }


    if (priqueue_size(&pqueue) == 0) {
        return NULL;
    }

    job * next_job = priqueue_poll(&pqueue);
    job_info * nj = (job_info *) next_job->metadata;

    nj->robin_time = time;
    if (nj->start_time == 0) {
        nj->start_time = time;
    }
    return next_job;
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO: Implement me!
    job_info * j = (job_info *)(job_done->metadata);
    j->end_time = time;

    wait_time += ((j->end_time - j->arrival_time) - j->runtime);
    turnaround_time += j->end_time - j->arrival_time;
    res_time += j->start_time - j->arrival_time;

    free(j);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return wait_time / n_jobs;
}

double scheduler_average_turnaround_time() {
    return turnaround_time / n_jobs;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return res_time / n_jobs;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
