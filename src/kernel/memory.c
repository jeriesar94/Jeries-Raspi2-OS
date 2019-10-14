#include <kernel/memory.h>
#include <common/stdlib.h>
#include <stddef.h>

extern uint8_t __end;
static uint32_t num_pages;

DEFINE_LIST(page);
IMPLEMENT_LIST(page);

static page_t* array_of_pages;
page_list_t list_of_free_pages;

static heap_segment_t* heap_seg_list_head;

uint32_t get_mem_size(atag_t* tag){
	/*
		This code snippet would return a value from the ATAGS.
		This is the part that we will use if running from a real machine.
	*/
	while(tag->tag != NONE){
		if(tag->tag == MEM){
			return tag->mem.size;
		}
		tag = ((uint32_t*)tag) + tag->tag_size;
	}
	
	/*
		This snippet would return a hardcoded value
		This is for the case this was started on a VM
	*/
	
	// Return A hardcoded value (128 MB)
	// return 1024*1024*128;
}

void mem_initalization(atag_t* atags) {

	// Declare mem init vars
	uint32_t all_mem_size;
	uint32_t page_array_len;
	uint32_t num_kernel_pages;
	uint32_t end_of_pages_array_addr;
	uint32_t index;

	// Find out the total number of pages present in memory
	all_mem_size = get_mem_size(atags);
	num_pages = all_mem_size / PAGE_SIZE;

	// Find the length of the array of mem pages
	page_array_len = sizeof(page_t)*num_pages;

	// Set address of array of pages to the address at the end of the kernel image
	array_of_pages = (page_t*)& __end;

	// We zero out all pages and initialize the list of free pages
	bzero(array_of_pages, page_array_len);
	INITIALIZE_LIST(list_of_free_pages);

	// obtain number of kernel pages, which is the same as finding the maximum possible number of pages up until the __end address
	num_kernel_pages = ((uint32_t)&__end) / PAGE_SIZE;

	// Iterate over kernel pages and mark them as reserved for kernel (set flags)
	for (index = 0; index < num_kernel_pages; index++) {
		array_of_pages[index].vmem_addr = index * PAGE_SIZE;
		array_of_pages[index].flags.allocated = 1;
		array_of_pages[index].flags.kernel_page = 1;
		array_of_pages[index].flags.kernel_heap_page = 0;
	}

	// Find the number of heap pages
	uint32_t heap_pages_num = (KERNEL_HEAP_SIZE / PAGE_SIZE);

	// Reserve Heap pages
	for (; index < num_kernel_pages + heap_pages_num; index++) {
		array_of_pages[index].vmem_addr = index * PAGE_SIZE;
		array_of_pages[index].flags.allocated = 1;
		array_of_pages[index].flags.kernel_page = 0;
		array_of_pages[index].flags.kernel_heap_page = 1;
	
	}

	// Mark the rest of the pages as free, and append all free pages to free pages linked list
	for (; index < num_pages; index++) {
		array_of_pages[index].flags.allocated = 0;
		append_page_list(&list_of_free_pages, &array_of_pages[index]);
	}

	// We then initialize the heap
	end_of_pages_array_addr = (uint32_t)&__end + page_array_len;
	heap_initialization(end_of_pages_array_addr);



}

void* allocate_page(void) {
	page_t* page;
	void* page_mem_addr;

	// Check if the size of free pages is zero or not, return 0 if there are no free pages
	if (size_page_list(&list_of_free_pages) == 0) {
		return 0;
	}

	page = pop_page_list(&list_of_free_pages);
	// Allocate the page, and reserve it to kernel
	page->flags.allocated = 1;
	page->flags.kernel_page = 1;

	// Obtain memory address as a physical address relative to page size and array of pages 
	page_mem_addr = (void*)((page - array_of_pages) * PAGE_SIZE);

	// Zero out the page you have obtained with PAGE_SIZE amount of zeros, to fill the whole size of page
	bzero(page_mem_addr, PAGE_SIZE);

	// Return the relative address of reserverd memory page
	return page_mem_addr;


}

void free_page(void* page_ptr) {
	page_t* page;

	// Obtain the page metadata from physical address
	page = array_of_pages + (((uint32_t)page_ptr) / PAGE_SIZE);

	// Remark the page as free and append it to free pages list
	page->flags.allocated = 0;
	page->flags.kernel_page = 0;
	append_page_list(&list_of_free_pages, page);
}


// Create a function that allocates a kernel memory block (random byte size instead of 4k)

void* kmalloc(uint32_t alloc_size) {
	heap_segment_t *curr_seg, *best_seg = NULL;
	int size, max_size = 0x7fffffff;

	// Total size of allocation is equal to the requested allocation size in addition to the size of the header requested.
	alloc_size += sizeof(heap_segment_t);

	// We need to align the allocation size with the 16 byte limit, the reason for 6 byte is as follows:
	// - We know that the reserved area is going to be the header plus the data
	// - In order to provide for an adequate size we assume the size to be twice of header size
	// - Header size is equal to its elements:
	//		1. unint32_t is_allocated variable = 32 bit
	//		2. unint32_t segment_size variable = 32 bit
	//	* The sum of these variables is 64 bit. 
	// We have two of these headers which means 2 * 64 bit = 128 bit.
	// 16 bytes = 16 * 8  bits = 128 bits. Hence the size of two headers.

	// Here we do find the modulus of the allocation size with 16 bytes
	uint32_t alloc_size_residue = alloc_size % 16;

	// We utilize the value of residue to add the needed amount to the alloc_size for it to fit to 16 bytes; else it is already aligned with current size
	alloc_size += alloc_size_residue ? 16 - alloc_size_residue : 0;

	// Loop over segments, and if you find a segment that is not allocated and fits the allocation size, then set it as the best suitable segment.
	// Keep looping until you find the segment with smallest size that fits the required allocation size and is empty
	for (curr_seg = heap_seg_list_head; curr_seg != NULL; curr_seg = curr_seg->next) {
		size = curr_seg->segment_size - alloc_size;
		if (!curr_seg->is_allocated && size < max_size && size >= 0) {
			max_size = size;
			best_seg = curr_seg;
		}
	}

	// If no suitable segment was found, then best segment would be NULL, and we return it.
	if (best_seg == NULL) {
		return best_seg;
	}
	
	// We check to see if the segment chosen is larger than twice the size of the header
	// If that is so, we split it in half
	if (max_size > (int)(2 * sizeof(heap_segment_t))) {
		// As we do have to have 2 split segments, we zero out as follows:
		// We do get the address of the best segment , and we add the requested allocation size to it
		// Now, we have the head of the allocation area, so we zero it out with the size of one heap segment type.
		bzero(((void*)best_seg) + alloc_size, sizeof(heap_segment_t));
		curr_seg = best_seg->next;
		best_seg->next = ((void*)best_seg) + alloc_size;
		best_seg->next->next = curr_seg;
		best_seg->next->prev = best_seg;
		best_seg->next->segment_size = best_seg->segment_size - alloc_size;
		best_seg->segment_size = alloc_size;
	}

	best_seg->is_allocated = 1;

	return best_seg + 1;
}

void kfree(void* ptr_to_mem) {

	// Obtain location of segment
	heap_segment_t* segment = (heap_segment_t*)ptr_to_mem - sizeof(heap_segment_t);
	segment->is_allocated = 0;

	// Connect segments to the left, to make one large block
	while (segment->prev != NULL && !segment->prev->is_allocated) {
		segment->prev->next = segment->next;
		segment->prev->segment_size += segment->segment_size;
		segment = segment->prev;
	
	}

	// Connect segments to the right, to make one large block
	while (segment->next != NULL && !segment->next->is_allocated) {
		segment->segment_size += segment->next->segment_size;
		segment->next = segment->next->next;
	
	}
}

// Function to initialize heap
static void heap_initialization(uint32_t heap_head_addr) {
	heap_seg_list_head = (heap_segment_t*)heap_head_addr;
	bzero(heap_seg_list_head, sizeof(heap_segment_t));
	heap_seg_list_head->segment_size = KERNEL_HEAP_SIZE;
}