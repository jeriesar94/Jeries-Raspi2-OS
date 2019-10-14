#include <kernel/list.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_HEAP_SIZE (1024*1024) // bytes-> KB -> KB -> MB



typedef enum {
	NONE = 0x00000000,
	CORE = 0x54410001,
	MEM = 0x54410002,
	VIDEOTEXT = 0x54410003,
	RAMDISK = 0x54410004,
	INITRD2 = 0x54410005,
	SERIAL = 0x54410006,
	REVISION = 0x54410007,
	VIDEOLFB = 0x54410008,
	CMDLINE = 0x54410009
} atag_tag_t;

typedef struct {
	uint32_t size;
	uint32_t start;
} mem_t;

typedef struct atag {
	uint32_t tag_size;
	atag_tag_t tag;
	union {
		mem_t mem;
	}
} atag_t;

typedef struct {
	uint8_t allocated : 1;
	uint8_t kernel_page : 1;
	uint8_t kernel_heap_page : 1;
	uint32_t reserved : 29;
} page_flags_t;

typedef struct page {
	uint32_t vmem_addr;
	page_flags_t flags;
	DEFINE_LINK(page);

} page_t;

//Memory heap segment
typedef struct heap_segment {
	struct heap_segment * next;
	struct heap_segment * prev;
	uint32_t is_allocated;
	uint32_t segment_size;  // Includes this header
} heap_segment_t;



uint32_t get_mem_size(atag_t*);
void mem_initalization(atag_t*);
void* allocate_page(void);
void free_page(void*);
void* kmalloc(uint32_t);
void kfree(void*);
static void heap_initialization(uint32_t);