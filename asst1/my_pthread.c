// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"

/* Globals */
static sched * scheduler = NULL;
static sigset_t set;
int timesSwitched = 0; // TODO temp
int fuckFlag = 0; // TODO remove this (its for sigset_t)

/* Queue Functions */

queue * queue_init() {
    queue * q = (queue *) malloc(sizeof(queue));
    q->head = q->rear = NULL;
    q->size = 0;

    return q;
}

void queue_enqueue(void * element, queue * q) {
    node * n = (node *) malloc(sizeof(node));
    n->data = element;
    
    if(isEmpty(q)) {
        // 1 element means rear and head are the same
        q->head = n;
        q->head->next = NULL;
        q->head->prev = NULL;

        q->rear = n;
    } else {
        n->next = NULL;
        n->prev = q->rear;
        q->rear->next = n;
        q->rear = n;
    }

    q->size += 1;
}

void * queue_dequeue(queue * q) {
    if (isEmpty(q)) {
        return NULL;
    } else {
        void * data = q->head->data;

        if (q->head->next == NULL) { // only 1
            q->head = NULL;
        } else { // 2+
            q->head = q->head->next;
            q->head->prev = NULL;
        }

        q->size -= 1;
        
        return data;
    }
}

int isEmpty(queue * q) {
    return q->size == 0;
}

/* Alarm-related functions */

static void setAlarm() {
    if (scheduler == NULL) {
        printf("Error: scheduler not initialized");
        return;
    } else if(scheduler->timerSet == 1) {
        printf("Error: timer already set");
        return;
    }

    scheduler->timerSet = 1;
    ualarm(scheduler->interval, scheduler->interval);
}

tcb * tcb_init() {
    tcb * t = (tcb * ) malloc(sizeof(tcb));
    t->id = (int) t; // set id = to address for utility
    getcontext(&(t->context));
    t->state = 0;
    return t;
}

void alrm_handler(int signo) {
//void alrm_handler(int signo, siginfo_t* siginfo, void* context) {
    printf("SWITCH! - %d\n", timesSwitched++);
    //tcb * t_old = tcb_init();
    //memcpy((void*)&t_old->context, context, sizeof(ucontext_t));
    //t_old->context.uc_stack.ss_sp = malloc(t_old->context.uc_stack.ss_size);
    //memcpy((void*)t_old->context.uc_stack.ss_sp, ((ucontext_t*) context)->uc_stack.ss_sp, t_old->context.uc_stack.ss_size);
    /*if (!scheduler->mainThreadCreated) { 
        scheduler->mainThreadCreated = 1;
        scheduler->curr = ((tcb*) queue_dequeue(scheduler->s_queue));
        setcontext(&scheduler->curr->context);
        //t_old->context = *((ucontext_t *) context);
        //queue_enqueue((void *) t_old, scheduler->s_queue);
    }*/
    //else {
    tcb *old = scheduler->curr;
    scheduler->curr = ((tcb*) queue_dequeue(scheduler->s_queue));
    queue_enqueue((void*) old, scheduler->s_queue);
    swapcontext(&old->context, &scheduler->curr->context);
        //scheduler->curr->context = t_old->context;
        //queue_enqueue((void *) scheduler->curr, scheduler->s_queue);
    //}
    


    // see if curr is still running or over?
    // if (still running) { re-queue }
    //
    // then dequeue next tcb, set sched->curr, and setcontext on it
    //tcb * t = (tcb *) queue_dequeue(scheduler->s_queue);
    //scheduler->curr = t;
    //printf("Thing to be switched into, id: %d\n", t->id);
    //setcontext(&(t->context));
}

/* Scheduling functions */

void sched_init() { // initializes global scheduler variable
    if (scheduler != NULL) {
        printf("Error: scheduler already created!");
    } else {
        scheduler = (sched *) malloc(sizeof(sched));
        scheduler->timerSet = 0;
        scheduler->interval = 200;
        scheduler->s_queue = queue_init();
        scheduler->mainThreadCreated = 0;
        
        return;
    }
}

/* create a new thread */
int my_pthread_create(void *(*function)(void*), void * arg) {
    if (fuckFlag) {
        pthread_sigmask(SIG_BLOCK, &set, NULL);
    }

    tcb * t = tcb_init(); // thread to be created
    tcb * curr;

    // create new thread
    getcontext(&(t->context));
    t->context.uc_stack.ss_sp = malloc(MEM);
    t->context.uc_stack.ss_size = MEM;
    t->context.uc_stack.ss_flags = 0;
    t->context.uc_link = 0;
    // TODO uc_link
    makecontext(&(t->context), function, 0);
    // reset the new thread's signal blocker
    sigemptyset(&(t->context.uc_sigmask));

    // should only be executed on first thread create
    if (scheduler == NULL) { 
        sched_init();
        setAlarm();
        signal(SIGALRM, alrm_handler);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        sigemptyset(&set); // TODO put these in their proper place
        sigaddset(&set, SIGALRM);
        fuckFlag = 1;
        curr = tcb_init();

        /*tcb * t2 = tcb_init(); */
        /*t2->context.uc_stack.ss_sp = malloc(MEM);*/
        /*t2->context.uc_stack.ss_size = MEM;*/
        /*queue_enqueue((void *) t2, scheduler->s_queue);*/
        /*getcontext(&(t2->context));*/
    } else {
        curr = scheduler->curr;
    }
    
    t->context.uc_link = &curr->context;
    // enqueue the thread that called create
    // set curr equal to new thread
    queue_enqueue((void *) curr, scheduler->s_queue);
    scheduler->curr = t;
    
    swapcontext(curr->context, t->context);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	return t->id;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	return 0;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};
