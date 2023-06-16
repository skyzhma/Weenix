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

#include "globals.h"
#include "errno.h"
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{

        if (fd >= 0 && fd < NFILES && !curproc->p_files[fd]) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }

        if (fd >= NFILES) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }

        if (!(flags & MAP_PRIVATE) && !(flags & MAP_SHARED)) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }
        
        if ((flags & MAP_FIXED) && !addr) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }

        if ((int)len <= 0) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }

        if (addr && !PAGE_ALIGNED(addr)) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }

        vnode_t *v = NULL;
        if (fd >= 0) {
            
            file_t *f = curproc->p_files[fd];
            v = f->f_vnode;
            
            if (off > v->vn_len) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -1;
            }

            if ((flags & MAP_SHARED) && (prot & PROT_WRITE) && !(f->f_mode & FMODE_WRITE)) {
                dbg(DBG_PRINT, "(GRADING3D)\n");
                return -1;
            }
        } else if (!(flags & MAP_ANON)) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -1;
        }

        vmarea_t *vma;

        int t = vmmap_map(curproc->p_vmmap, v, ADDR_TO_PN(addr), (len - 1) / PAGE_SIZE + 1, prot, flags, off, VMMAP_DIR_HILO, &vma);
        if (t < 0) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return t;
        }
        *ret = (void *) PN_TO_ADDR(vma->vma_start);
        pt_unmap_range(curproc->p_pagedir, (uintptr_t) PN_TO_ADDR(vma->vma_start),
               (uintptr_t) PN_TO_ADDR(vma->vma_start)
               + (uintptr_t) PAGE_ALIGN_UP(len));
    
        tlb_flush_range((uintptr_t) PN_TO_ADDR(vma->vma_start),(uint32_t) PAGE_ALIGN_UP(len) / PAGE_SIZE);
        
        KASSERT(NULL != curproc->p_pagedir);
        dbg(DBG_PRINT, "(GRADING3A 2.a)\n");
        dbg(DBG_PRINT, "(GRADING3B)\n");
        return t;

}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{

        if (len == 0 || len == (size_t)-1 || !addr || !PAGE_ALIGNED(addr) || (uint32_t)addr >= USER_MEM_HIGH) {
            dbg(DBG_PRINT, "(GRADING3D)\n");
            return -EINVAL;
        }
        int t = vmmap_remove(curproc->p_vmmap, ADDR_TO_PN(addr), (len - 1) / PAGE_SIZE + 1);        
        tlb_flush_all();
        dbg(DBG_PRINT, "(GRADING3B)\n");
        return t;

}

