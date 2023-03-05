/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include "exec_parser.h"

static so_exec_t *exec;
static struct sigaction old_action;


int f;

int getpagesize();


static void segv_handler(int signum, siginfo_t *info, void *context)
{
	int i, found_seg = 0, page, nr_pages, page_size = getpagesize(), page_offset, rc;
	int *seg_data;
	void *page_vaddr, *mmap_mem;
	uintptr_t addr_seg;
	so_seg_t segment;

	if (signum != SIGSEGV) {
		old_action.sa_sigaction(signum, info, context);
		return;
	}

	for (i = 0; i < exec->segments_no; i++) {
		segment = exec->segments[i];
		addr_seg = (uintptr_t)info->si_addr - segment.vaddr;
		nr_pages = segment.mem_size / page_size;
		page = addr_seg / page_size;

		if (addr_seg >= 0 && addr_seg <= segment.mem_size) {
			found_seg = 1;
			if (!exec->segments[i].data) {
				exec->segments[i].data = calloc(nr_pages, sizeof(int));

			}

			seg_data = (int*)exec->segments[i].data;
			if (seg_data[page] == 0) {
				page_offset = segment.offset + page * page_size;
				page_vaddr = (void *)segment.vaddr + page * page_size;

				mmap_mem = mmap((void*)page_vaddr, page_size, PERM_W,  MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);

				if (mmap_mem == MAP_FAILED) {
					perror("MAP FAILED\n");
					exit(EXIT_FAILURE);
				}

				if (segment.file_size > page * page_size && segment.file_size >= (page + 1) * page_size) {
					mmap_mem = mmap((void*)page_vaddr, page_size, PERM_W,  MAP_PRIVATE | MAP_FIXED, f, page_offset);

					if (mmap_mem == MAP_FAILED) {
						perror("MAP FAILED\n");
						exit(EXIT_FAILURE);
					}

				} else if (segment.file_size > page * page_size && segment.file_size < (page + 1) * page_size){
					rc = pread(f, mmap_mem, segment.file_size - page * page_size, page_offset);
					memset((void*)segment.vaddr + segment.file_size, 0, (page + 1) * page_size - segment.file_size);
					
					if (rc == -1) {
						perror("PREAD FAILED\n");
						exit(EXIT_FAILURE);
					}
				}

				rc = mprotect((void*)page_vaddr, page_size, segment.perm);
				if(rc == -1) {
					perror("MPROTECT FAILED\n");
					exit(EXIT_FAILURE);
				}

				seg_data[page] = 1;

			} else {
				old_action.sa_sigaction(signum, info, context);
				return;
			}
		}
	}

	if (found_seg == 0) {
		old_action.sa_sigaction(signum, info, context);
		return;
	}
}


int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, &old_action);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	f = open(path, O_RDONLY);
	if (f < 0) {
		perror("FILE ERROR\n");
		return -1;
	}
	so_start_exec(exec, argv);
	for (int i = 0; i < exec->segments_no; i++) {
		free(exec->segments[i].data);
	}
	close(f);
	return 0;
}
