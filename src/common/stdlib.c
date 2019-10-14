#include <common/stdlib.h>
#include <stdint.h>

void memcpy(void* src, void* dst, int size){
	uint32_t* source = (uint32_t*) src;
	uint32_t* destination = (uint32_t*) dst;

	while (size--) {
		*destination++ = *source++;
	}


}

void bzero(void* dst, int size){
	uint32_t* destination = (uint32_t*) dst;
	while (size--) {
		*destination++ = 0;
	}
}

char itoa(int val){
	
}