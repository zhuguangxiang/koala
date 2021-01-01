/*===-- task.c - Koala Coroutine Library --------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements koala coroutine.                                      *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "task.h"
#include "common.h"
#include "mm.h"
#include "task_event.h"
#include "task_timer.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <valgrind/valgrind.h>

#ifdef __cplusplus
extern "C" {
#endif

/* linked locked deque node */
typedef struct lldq_node {
    struct lldq_node *next;
} lldq_node_t;

/* linked locked deque */
typedef struct lldq_deque {
    uint32_t count;
    lldq_node_t *head;
    lldq_node_t *tail;
    lldq_node_t dummy;
    uint64_t steal_count;
    pthread_mutex_t lock;
} lldq_deque_t;

/* task state */
typedef enum {
    TASK_STATE_RUNNING = 1,
    TASK_STATE_READY = 2,
    TASK_STATE_SUSPEND = 3,
    TASK_STATE_DONE = 4,
} task_state_t;

/* task context */
typedef struct task_context {
    void *stkbase;
    int stksize;
    ucontext_t uctx;
} task_context_t;

/* task */
typedef struct task {
    lldq_node_t dq_node;
    task_context_t context;
    task_entry_t entry;
    void *arg;
    task_timer_t timer;
    task_state_t volatile state;
    uint64_t id;
    void *volatile result;
    void *data;
} task_t;

/* task processor per thread */
typedef struct task_proc {
    int id;
    task_t *volatile current;
    task_t idle_task;
    lldq_deque_t ready_deque;
    pthread_t pid;
    uint64_t yield_count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} task_proc_t;

static lldq_deque_t global_deque;
static lldq_deque_t suspend_deque;
static lldq_deque_t done_deque;
static int num_procs;
static task_proc_t *procs;
static _Atomic uint64_t task_idgen = 0;
static int task_shutdown = 0;
static __thread task_proc_t *current;
static int shutdown = 0;
static pthread_t monitor;

/* task routine */
static void task_go_routine(void *arg)
{
    task_t *task = arg;
    void *result = task->entry(task->arg);
    task->result = result;
    task->state = TASK_STATE_DONE;
    task_yield();
    /* !! NEVER go here !! */
    abort();
}

static int context_init(task_context_t *ctx, int stksize, void *arg)
{
    void *stk = mm_alloc(stksize);
    ctx->stkbase = stk;
    ctx->stksize = stksize;
    // VALGRIND_STACK_REGISTER(stk, stk + stksize);

    ucontext_t *uctx = &ctx->uctx;
    getcontext(uctx);
    uctx->uc_link = NULL;
    uctx->uc_stack.ss_sp = stk;
    uctx->uc_stack.ss_size = stksize;
    uctx->uc_stack.ss_flags = 0;
    sigemptyset(&uctx->uc_sigmask);
    makecontext(uctx, (void (*)())task_go_routine, 1, arg);
    return 0;
}

static inline void context_fini(task_context_t *ctx)
{
    // VALGRIND_STACK_DEREGISTER(ctx->stkbase);
    mm_free(ctx->stkbase);
}

static inline int context_save(task_context_t *ctx)
{
    return getcontext(&ctx->uctx);
}

static inline int context_load(task_context_t *ctx)
{
    return setcontext(&ctx->uctx);
}

static inline int context_switch(task_context_t *from, task_context_t *to)
{
    return swapcontext(&from->uctx, &to->uctx);
}

static int lldq_init(lldq_deque_t *deque)
{
    deque->dummy.next = NULL;
    deque->tail = &deque->dummy;
    deque->head = deque->tail;
    deque->count = 0;
    pthread_mutex_init(&deque->lock, NULL);
    return 0;
}

static lldq_node_t *lldq_pop_head(lldq_deque_t *deque)
{
    pthread_mutex_lock(&deque->lock);
    lldq_node_t *head = deque->head;
    lldq_node_t *first = head->next;
    if (first) {
        head->next = first->next;
        first->next = NULL;
        deque->count--;
    }
    if (first == deque->tail) { deque->tail = deque->head; }
    pthread_mutex_unlock(&deque->lock);
    return first;
}

static void lldq_push_tail(lldq_deque_t *deque, lldq_node_t *node)
{
    pthread_mutex_lock(&deque->lock);
    lldq_node_t *tail = deque->tail;
    tail->next = node;
    deque->tail = node;
    node->next = NULL;
    deque->count++;
    pthread_mutex_unlock(&deque->lock);
}

static inline void schedule(task_t *task)
{
    lldq_push_tail(&current->ready_deque, &task->dq_node);
}

static task_t *next_task(void)
{
    return (task_t *)lldq_pop_head(&current->ready_deque);
}

static void steal_tasks(void)
{
    int i = current->id;
    int end = num_procs;
    int steal_index = i;
    int max_count = procs[i].ready_deque.count;
    int other_count;

    /* if current proc has task, no need steal tasks. */
    if (max_count > 0) return;

    for (int j = 0; j < end; j++) {
        if (j == i) continue;
        other_count = procs[j].ready_deque.count;
        if (max_count < other_count) {
            max_count = other_count;
            steal_index = j;
        }
    }

    if (steal_index != i) {
        task_proc_t *from = procs + steal_index;
        task_proc_t *to = current;
        lldq_deque_t *steal_dq = &from->ready_deque;
        task_t *task;
        int num_steal = max_count >> 1;
        while (num_steal-- > 0) {
            task = (task_t *)lldq_pop_head(steal_dq);
            if (task) {
                printf("[proc-%u]steal task-%lu from proc-%d\n", to->id,
                    task->id, from->id);
                lldq_push_tail(&to->ready_deque, &task->dq_node);
            }
        }
    }
}

static int get_tasks_from_global(void)
{
    int count = 0;
    task_t *task = (task_t *)lldq_pop_head(&global_deque);
    while (task) {
        schedule(task);
        task = (task_t *)lldq_pop_head(&global_deque);
        ++count;
    }
    return count;
}

static inline void load_balance(void)
{
    if (!get_tasks_from_global()) steal_tasks();
}

static inline task_t *current_task(void)
{
    return current->current;
}

int current_pid(void)
{
    return current->id;
}

uint64_t current_tid(void)
{
    return current_task()->id;
}

/* idle task per processor */
static inline void init_idle_task(task_t *task)
{
    task->state = TASK_STATE_RUNNING;
    task->id = ++task_idgen;
    printf("[proc-%u]task-%lu is idle\n", current->id, task->id);
    context_save(&task->context);
}

/* destroy task */
static void task_destroy(task_t *task)
{
    printf("task-%lu destroyed\n", task->id);
    assert(task != &current->idle_task);
    context_fini(&task->context);
    mm_free(task);
}

static inline void task_done(task_t *task)
{
    lldq_push_tail(&done_deque, &task->dq_node);
}

/* switch to new task */
static void task_switch_to(task_t *to)
{
    int done = 0;
    task_t *from = current_task();
    if (from->state == TASK_STATE_RUNNING) {
        from->state = TASK_STATE_READY;
        if (from != &current->idle_task) { schedule(from); }
        printf(
            "[proc-%u]task-%lu from running -> ready\n", current->id, from->id);
    }
    else if (from->state == TASK_STATE_DONE) {
        printf("[proc-%u]task-%lu: done\n", current->id, from->id);
        task_done(from);
        done = 1;
    }
    else if (from->state == TASK_STATE_SUSPEND) {
        printf("[proc-%u]task-%lu: suspended\n", current->id, from->id);
    }
    else {
        assert(0);
    }

    assert(to->state == TASK_STATE_READY);
    to->state = TASK_STATE_RUNNING;
    current->current = to;

    if (!done) {
        printf("[proc-%u]SWITCH to task-%lu\n", current->id, to->id);
        context_switch(&from->context, &to->context);
    }
    else {
        printf("[proc-%u]LOAD task-%lu\n", current->id, to->id);
        context_load(&to->context);
    }
}

static void init_proc(int id)
{
    current = procs + id;
    task_proc_t *proc = current;
    proc->id = id;
    printf("[proc-%u]running\n", proc->id);
    lldq_init(&proc->ready_deque);
    init_idle_task(&proc->idle_task);
    proc->current = &proc->idle_task;
    pthread_mutex_init(&proc->lock, NULL);
    pthread_cond_init(&proc->cond, NULL);
}

/*
 * pthread routine per processor(idle task)
 * get next task from current processor and run it
 */
static void *proc_go_routine(void *arg)
{
    init_proc((intptr_t)arg);

    task_t *task;
    while (!shutdown) {
        load_balance();
        task = next_task();
        if (task) {
            printf("[proc-%u]switch to task-%lu\n", current->id, task->id);
            task_switch_to(task);
        }
        else {
            printf("[proc-%u]No more tasks\n", current->id);
            sleep(1);
        }
    }
    return NULL;
}

/* monitor thread */
static void *monitor_thread(void *arg)
{
    /* initialize timers */
    init_timer();

    /* initialize events */
    init_event();

    while (!shutdown) { event_poll(); }

    return NULL;
}

void init_procs(int nproc)
{
    int ncpu = get_nprocs();
    if (nproc <= 0) { nproc = ncpu; }
    num_procs = nproc;
    procs = mm_alloc(sizeof(task_proc_t) * nproc);

    /* initialize queues */
    lldq_init(&global_deque);
    lldq_init(&suspend_deque);
    lldq_init(&done_deque);

    /* initialize processor 0 */
    init_proc(0);
    current->pid = pthread_self();
    
    /* initialize processor 1 ..< nproc */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    /* pthread_attr_setstacksize(); */

    for (int i = 1; i < nproc; i++) {
        pthread_create(
            &procs[i].pid, &attr, proc_go_routine, (void *)(intptr_t)i);
    }

    /* initialize moniter thread */
    pthread_create(&monitor, &attr, monitor_thread, NULL);

    /* destroy pthread attribute */
    pthread_attr_destroy(&attr);
}

task_t *task_create(task_entry_t entry, void *arg, void *tls)
{
    task_t *task = mm_alloc(sizeof(task_t));
    if (!task) {
        errno = ENOMEM;
        return NULL;
    }

    task->entry = entry;
    task->arg = arg;
    task->state = TASK_STATE_READY;
    task->id = ++task_idgen;
    task->data = tls;
    context_init(&task->context, 4096, task);

    schedule(task);
    printf("[proc-%u]task-%lu: created\n", current->id, task->id);
    return task;
}

void task_set_tls(void *tls)
{
    current_task()->data = tls;
}

void *task_tls(void)
{
    return current_task()->data;
}

void task_yield(void)
{
    load_balance();

    task_t *tsk = next_task();
    if (tsk)
        task_switch_to(tsk);
    else {
        printf("[proc-%u]No more tasks\n", current->id);
        if (current_task() != &current->idle_task) {
            // printf("[proc-%u]switch to idle task\n", current->id);
            task_switch_to(&current->idle_task);
        }
        else {
            printf("[proc-%u]idle task\n", current->id);
        }
    }
}

void task_resume(task_t *task)
{
    assert(task->state == TASK_STATE_SUSPEND);
    task->state = TASK_STATE_READY;
    lldq_push_tail(&global_deque, &task->dq_node);
}

void task_sleep(int timeout)
{
    task_t *task = current_task();
    assert(task != &current->idle_task);
    if (timeout) {
        task->state = TASK_STATE_SUSPEND;
        if (timeout > 0) {
            timer_start(&task->timer, timeout, (timer_func_t)task_resume, task);
        }
    }
    task_yield();
}

#ifdef __cplusplus
}
#endif
