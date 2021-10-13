#include <common/lock.h>
#include <common/kprint.h>
#include <arch/machine/smp.h>
#include <common/macro.h>
#include <mm/kmalloc.h>
#include <object/thread.h>
#include <sched/context.h>
#include <sched/sched.h>

#define TEST_NUM        512

#if CHCORE_PLAT == hikey970
#define THREAD_NUM      128
#else
#define THREAD_NUM      1024
#endif

volatile int sched_start_flag = 0;
volatile int sched_finish_flag = 0;


struct thread *rr_sched_choose_thread(void);
int rr_sched_enqueue(struct thread *thread);
void rr_sched(void);
extern struct list_head rr_ready_queue[PLAT_CPU_NUM];

void tst_rr(void);

int pbrr_sched_enqueue(struct thread *thread);
int pbrr_sched_dequeue(struct thread *thread);
struct thread *pbrr_sched_choose_thread(void);
int pbrr_sched(void);
void tst_pbrr(void);

static int is_valid_prev_thread(struct thread *thread)
{
	if ((thread == NULL) || (thread == THREAD_ITSELF))
		return 0;
	return 1;
}

/* Test scheduler */
void tst_sched(void)
{
        if (cur_sched_ops == &rr)
                tst_rr();
        else if (cur_sched_ops == &pbrr)
                tst_pbrr();
        /* No other policy supported */
	if (smp_get_cpu_id() == 0) {
                kinfo("[TEST] sched succ!\n");
        }
}

void tst_rr(void)
{
        int cpuid = 0;
        struct thread *thread = 0;

        cpuid = smp_get_cpu_id();
        /* check each queue shoule be empty */
        BUG_ON(!list_empty(&rr_ready_queue[cpuid]));
        /* should return idle thread */
        thread = rr_sched_choose_thread();
        BUG_ON(thread->thread_ctx->type != TYPE_IDLE);

	/* ============ Start Barrier ============ */
	lock(&big_kernel_lock);
	sched_start_flag++;
	unlock(&big_kernel_lock);
	while (sched_start_flag != PLAT_CPU_NUM) ;
	/* ============ Start Barrier ============ */
        
        int i = 0, j = 0, k = 0, prio = 0;
        struct thread_ctx *thread_ctx = NULL;

        /* Init threads */
        for(i = 0; i < THREAD_NUM; i++) {
                for (j = 0 ; j < PLAT_CPU_NUM; j++) {
                        prio = (i + j) % MAX_PRIO;
                        thread = kmalloc(sizeof(struct thread));
                        BUG_ON(!(thread->thread_ctx = create_thread_ctx()));
                        init_thread_ctx(thread, 0, 0, prio, TYPE_TESTS, j);
                        for (k = 0; k < REG_NUM; k++)
                                thread->thread_ctx->ec.reg[k] = prio;
                        BUG_ON(rr_sched_enqueue(thread));
                }
        }

        for (j = 0; j < TEST_NUM; j++) {
                /* Each core try to get those threads */
                for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                        /* get thread and dequeue from ready queue */
                        do {
                                /* do it again if choose idle thread */
                                rr_sched();
                                BUG_ON(!current_thread);
                                BUG_ON(!current_thread->thread_ctx);
                                current_thread->thread_ctx->sc->budget = 0;

                                if (is_valid_prev_thread(current_thread->prev_thread))
					current_thread->prev_thread->thread_ctx->kernel_stack_state = KS_FREE;

                        } while (current_thread->thread_ctx->type == TYPE_IDLE);
                        BUG_ON(!current_thread->thread_ctx->sc);
                        /* Current thread set affinitiy */
                        current_thread->thread_ctx->affinity = (i + cpuid) % PLAT_CPU_NUM;
                        thread_ctx = (struct thread_ctx *)switch_context();
                        for (k = 0; k < REG_NUM; k++)
                                BUG_ON(thread_ctx->ec.reg[k] != thread_ctx->prio);
                }
        }

        for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                /* get thread and dequeue from ready queue */
                do {
                        /* do it again if choose idle thread */
                        rr_sched();
                        BUG_ON(!current_thread);
                        BUG_ON(!current_thread->thread_ctx);
                        current_thread->thread_ctx->sc->budget = 0;

                        if (is_valid_prev_thread(current_thread->prev_thread))
					current_thread->prev_thread->thread_ctx->kernel_stack_state = KS_FREE;

                } while (current_thread->thread_ctx->type == TYPE_IDLE);
                BUG_ON(!current_thread->thread_ctx->sc);
                destroy_thread_ctx(current_thread);
                kfree(current_thread);
                current_thread = NULL;
        }

        /* ============ Finish Barrier ============ */
	lock(&big_kernel_lock);
	sched_finish_flag++;
	unlock(&big_kernel_lock);
	while (sched_finish_flag != PLAT_CPU_NUM) ;
	/* ============ Finish Barrier ============ */

        /* check each queue shoule be empty */
        BUG_ON(!list_empty(&rr_ready_queue[cpuid]));
}

void tst_pbrr(void)
{
        int cpuid = 0;
        struct thread *thread = 0;

        cpuid = smp_get_cpu_id();

	/* ============ Start Barrier ============ */
	lock(&big_kernel_lock);
	sched_start_flag++;
	unlock(&big_kernel_lock);
	while (sched_start_flag != PLAT_CPU_NUM) ;
	/* ============ Start Barrier ============ */
        
        int i = 0, j = 0, k = 0, prio = 0;
        struct thread_ctx *thread_ctx = NULL;

        /* Init threads */
        for(i = 0; i < THREAD_NUM; i++) {
                for (j = 0 ; j < PLAT_CPU_NUM; j++) {
                        prio = (i + j) % MAX_PRIO;
                        thread = kmalloc(sizeof(struct thread));
                        BUG_ON(!(thread->thread_ctx = create_thread_ctx()));
                        init_thread_ctx(thread, 0, 0, prio, TYPE_TESTS, j);
                        for (k = 0; k < REG_NUM; k++)
                                thread->thread_ctx->ec.reg[k] = prio;
                        BUG_ON(pbrr_sched_enqueue(thread));
                }
        }

        for (j = 0; j < TEST_NUM; j++) {
                /* Each core try to get those threads */
                for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                        /* get thread and dequeue from ready queue */
                        do {
                                /* do it again if choose idle thread */
                                pbrr_sched();
                                BUG_ON(!current_thread);
                                BUG_ON(!current_thread->thread_ctx);
                                current_thread->thread_ctx->sc->budget = 0;

				if (current_thread->prev_thread)
					current_thread->prev_thread->thread_ctx->kernel_stack_state = KS_FREE;


                                //if (current_thread->thread_ctx->prev_ctx)
                                 //       current_thread->thread_ctx->prev_ctx->kernel_stack_state = KS_FREE;

                        } while (current_thread->thread_ctx->type == TYPE_IDLE);
                        BUG_ON(!current_thread->thread_ctx->sc);
                        /* Current thread set affinitiy */
                        current_thread->thread_ctx->affinity = (i + cpuid) % PLAT_CPU_NUM;
                        thread_ctx = (struct thread_ctx *)switch_context();
                        for (k = 0; k < REG_NUM; k++)
                                BUG_ON(thread_ctx->ec.reg[k] != thread_ctx->prio);
                }
        }

        for (i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                /* get thread and dequeue from ready queue */
                do {
                        /* do it again if choose idle thread */
                        pbrr_sched();
                        BUG_ON(!current_thread);
                        BUG_ON(!current_thread->thread_ctx);
                        current_thread->thread_ctx->sc->budget = 0;

			if (current_thread->prev_thread)
					current_thread->prev_thread->thread_ctx->kernel_stack_state = KS_FREE;

                        //if (current_thread->thread_ctx->prev_ctx)
                         //       current_thread->thread_ctx->prev_ctx->kernel_stack_state = KS_FREE;

                } while (current_thread->thread_ctx->type == TYPE_IDLE);
                BUG_ON(!current_thread->thread_ctx->sc);
                destroy_thread_ctx(current_thread);
                kfree(current_thread);
                current_thread = NULL;
        }

        /* ============ Finish Barrier ============ */
	lock(&big_kernel_lock);
	sched_finish_flag++;
	unlock(&big_kernel_lock);
	while (sched_finish_flag != PLAT_CPU_NUM) ;
	/* ============ Finish Barrier ============ */

        /* check each queue shoule be empty */
}
