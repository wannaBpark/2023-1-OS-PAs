/**********************************************************************
 * Copyright (c) 2020-2023
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"

 /**
  * Ready queue of the system
  */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process* current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable* ptbr;

/**
 * TLB of the system.
 */
extern struct tlb_entry tlb[1UL << (PTES_PER_PAGE_SHIFT * 2)];


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * lookup_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Translate @vpn of the current process through TLB. DO NOT make your own
 *   data structure for TLB, but should use the defined @tlb data structure
 *   to translate. If the requested VPN exists in the TLB and it has the same
 *   rw flag, return true with @pfn is set to its PFN. Otherwise, return false.
 *   The framework calls this function when needed, so do not call
 *   this function manually.
 *
 * RETURN
 *   Return true if the translation is cached in the TLB.
 *   Return false otherwise
 */
bool lookup_tlb(unsigned int vpn, unsigned int rw, unsigned int* p_pfn)
{
	struct tlb_entry* p_tlb = &tlb[0];

	while (p_tlb->valid) {
		//printf("Searching for VPN : %d, RW: %d\n and p_tlb->vpn : %d and rw : %d",
		//	       vpn,rw,p_tlb->vpn, p_tlb->rw);	
		if (p_tlb->vpn == vpn && p_tlb->rw >= rw) {
			*p_pfn = p_tlb->pfn;
			return true;
		}
		++p_tlb;
	}

	return false;
}


/**
 * insert_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Insert the mapping from @vpn to @pfn for @rw into the TLB. The framework will
 *   call this function when required, so no need to call this function manually.
 *   Note that if there exists an entry for @vpn already, just update it accordingly
 *   rather than removing it or creating a new entry.
 *   Also, in the current simulator, TLB is big enough to cache all the entries of
 *   the current page table, so don't worry about TLB entry eviction. ;-)
 */
void insert_tlb(unsigned int vpn, unsigned int rw, unsigned int pfn)
{
	struct tlb_entry* p_tlb = &tlb[0];
	struct tlb_entry _tlb = { true, rw, vpn, pfn };

	while (p_tlb->valid) {
		if (p_tlb->vpn == vpn && p_tlb->rw == rw) {
			p_tlb->pfn = pfn;
			return;
		}
		++p_tlb;
	}

	*p_tlb = _tlb;
	(++p_tlb)->valid = false;

	printf("current size : %d\n", p_tlb - &tlb[0]);
}


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with ACCESS_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with ACCESS_READ should not be accessible with
 *   ACCESS_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
	struct pte_directory* p_pd;
	struct pte* p_pte;
	struct pte _pte = { true, rw, 0, rw };
	int pd_idx = vpn / NR_PTES_PER_PAGE;
	int pte_idx = vpn % NR_PTES_PER_PAGE;
	size_t i, pfnum;
	bool isFree = false;

	for (i = 0; i < NR_PAGEFRAMES; ++i) {
		isFree = (mapcounts[i] == 0) ? true : false;
		if (isFree)
			break;
	}

	// isFree is false if there's no free page frame
	// Return -1 if all page frames are allocated.
	if (isFree == false)
		return -1;

	pfnum = i;
	_pte.pfn = pfnum;

	++mapcounts[pfnum];

	p_pd = current->pagetable.outer_ptes[pd_idx];
	if (p_pd == NULL) {
		p_pd = (struct pte_directory*)malloc(sizeof(struct pte) * NR_PTES_PER_PAGE);
		current->pagetable.outer_ptes[pd_idx] = p_pd;
	}
	p_pte = &p_pd->ptes[pte_idx];
	*p_pte = _pte;
	//p_pte->rw = rw;
	return pfnum;
}


/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, rw, pfn) is set @false or 0.
 *   Also, consider the case when a page is shared by two processes,
 *   and one process is about to free the page. Also, think about TLB as well ;-)
 */
void free_page(unsigned int vpn)
{
	struct pte_directory* p_pd;
	struct pte* p_pte;
	struct pte _pte = { false, false,false,false };
	int pd_idx = vpn / NR_PTES_PER_PAGE;
	int pte_idx = vpn % NR_PTES_PER_PAGE;
	size_t i, pfnum;

	p_pd = current->pagetable.outer_ptes[pd_idx];
	p_pte = &p_pd->ptes[pte_idx];


	pfnum = p_pte->pfn;
	--mapcounts[pfnum];

	*p_pte = _pte;
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	size_t ret = false;
	int pd_idx = vpn / NR_PTES_PER_PAGE;
	int pte_idx = vpn % NR_PTES_PER_PAGE;
	struct pte* p_pte = &(current->pagetable.outer_ptes[pd_idx]->ptes[pte_idx]);

	//printf("Entered handler page fault\n");
	//printf("ACCESS : %d Private : %d\n", p_pte->rw, p_pte->private);
	if (rw == ACCESS_WRITE && p_pte->private != ACCESS_READ) {
		int pfnum = p_pte->pfn;
		int* p_mapcnt = &mapcounts[pfnum];
		

		if (*p_mapcnt == 1) {
			p_pte->rw = 0x03;
			p_pte->private = ACCESS_WRITE; // Now it's only owned by one (mapcnt == 1)
			return ret = true;		
		} else if (*p_mapcnt >= 2) {
			--*p_mapcnt; // reduce ref count
			//printf("write new memory vpn : %d rw: %d\n", vpn, rw);
			ret = alloc_page(vpn, rw);
			//printf("inserted vpn : %d access : %d pfnum : %d\n", vpn, p_pte->rw, p_pte->pfn);
		        p_pte->rw = 0x03;
			return ret == -1 ? false : true;	
		}
	}
	return false;
}


/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table.
 *   To implement the copy-on-write feature, you should manipulate the 
 * 
 * 
 * 
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for
 *   storing some useful information :-)
 */
void switch_process(unsigned int pid)
{
	struct process* pos = NULL;
	struct process* nxt = NULL;
	struct tlb_entry* p_tlb = &tlb[0];
	if (!list_empty(&processes)) {
		list_for_each_entry_safe(nxt, pos, &processes, list) {
			if (nxt->pid == pid) {
				break;
			}
		}
	}

	if (nxt != NULL && nxt->pid == pid) {
		goto CHANGECUR;
	}

	// If there is no proccess with @pid

	nxt = (struct process*)malloc(sizeof(struct process));
	
	//memcpy(&nxt->pagetable, &current->pagetable, sizeof(struct pagetable));
	INIT_LIST_HEAD(&nxt->list);	

	for (int i = 0; i < NR_PTES_PER_PAGE; ++i) {
		struct pte_directory* p_pd = current->pagetable.outer_ptes[i];
		struct pte_directory* p_nxtpd;

		// Unavailable pd -> no need to memcpy
		if (!p_pd) continue;

		p_nxtpd = (struct pte_directory*)malloc(sizeof(struct pte) * NR_PTES_PER_PAGE);
		
		for (int j = 0; j < NR_PTES_PER_PAGE; ++j) {
			struct pte* p_pte = &p_pd->ptes[j];
			//struct pte* p_npte = &p_nxtpd->ptes[j];

			if (!p_pte->valid) continue;

			++mapcounts[p_pte->pfn];
			//p_pte->valid = false;
			if (p_pte->rw == ACCESS_WRITE) {
				p_pte->private = p_pte->rw; // save the original RW value
			}
			p_pte->rw = ACCESS_READ;
			memcpy(&p_nxtpd->ptes[j], p_pte, sizeof(struct pte));
			//p_nxtpd->ptes[j].rw = ACCESS_READ;
			//p_nxtpd->ptes[j].private = false;
			//printf("pfn complete : %d %d\n", p_pte->pfn, p_nxtpd->ptes[j].pfn);
		}
		//memcpy(p_nxtpd, p_pd, sizeof(struct pte) * NR_PTES_PER_PAGE);
		nxt->pagetable.outer_ptes[i] = p_nxtpd;
	}
CHANGECUR:

	list_del_init(&nxt->list);
	list_add_tail(&current->list, &processes);
	ptbr = &nxt->pagetable;
	nxt->pid = pid;
	current = nxt;

	while (p_tlb->valid) {
		p_tlb++->valid = false;
	}

	return;
}
