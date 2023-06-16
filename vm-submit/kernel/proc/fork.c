/******************************************************************************/
/* Important Spring 2023 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{       
        // NOT_YET_IMPLEMENTED("VM: do_fork");
        KASSERT(regs != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(curproc != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(curproc->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        proc_t * p = proc_create("child");        

        vmmap_destroy(p->p_vmmap);
        p->p_vmmap = vmmap_clone(curproc->p_vmmap);
        p->p_vmmap->vmm_proc = p;

        vmarea_t *vma;
        list_iterate_begin(&(p->p_vmmap->vmm_list), vma, vmarea_t, vma_plink) {

            vmarea_t *tmp = vmmap_lookup(curproc->p_vmmap, vma->vma_start);

            if ((vma->vma_flags & MAP_TYPE) == MAP_PRIVATE) {
                
                mmobj_t *shadow_obj2= shadow_create();
                shadow_obj2->mmo_shadowed = tmp->vma_obj;
                shadow_obj2->mmo_un.mmo_bottom_obj = tmp->vma_obj->mmo_un.mmo_bottom_obj;

                mmobj_t *shadow_obj1= shadow_create();
                shadow_obj1->mmo_shadowed = tmp->vma_obj;
                shadow_obj1->mmo_un.mmo_bottom_obj = tmp->vma_obj->mmo_un.mmo_bottom_obj;

                tmp->vma_obj->mmo_ops->ref(tmp->vma_obj);

                vma->vma_obj = shadow_obj1;
                tmp->vma_obj = shadow_obj2;

            } else {
                vma->vma_obj = tmp->vma_obj;
                tmp->vma_obj->mmo_ops->ref(tmp->vma_obj);
            }


            list_insert_tail(mmobj_bottom_vmas(tmp->vma_obj), &vma->vma_olink);


        } list_iterate_end();


        kthread_t *t = kthread_clone(curthr);
        t->kt_proc = p;

        KASSERT(p->p_state == PROC_RUNNING);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(p->p_pagedir != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");
        KASSERT(t->kt_kstack != NULL);
        dbg(DBG_PRINT, "(GRADING3A 7.a)\n");

        list_insert_tail(&p->p_threads, &t->kt_plink);

        regs->r_eax = 0;
        t->kt_ctx.c_esp = fork_setup_stack(regs, t->kt_kstack);
        t->kt_ctx.c_eip = (uint32_t) userland_entry;
        t->kt_ctx.c_pdptr = p->p_pagedir;
        t->kt_ctx.c_kstack = (uintptr_t) t->kt_kstack;
                
        for (int i = 0; i < NFILES; i++) {
            if (curproc->p_files[i]) {
                p->p_files[i] = curproc->p_files[i];
                fref(p->p_files[i]);
            }

        }

        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all();
        p->p_brk = curproc->p_brk;
        p->p_start_brk = curproc->p_start_brk;
        sched_make_runnable(t);
        
        dbg(DBG_PRINT, "(GRADING3B)\n");
        return p->p_pid;
}
