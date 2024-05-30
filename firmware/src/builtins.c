#include <stddef.h>

typedef void (*init_function)(void);

void __libc_init_array(void) {
	extern init_function __init_array_start, __init_array_end;

	init_function *array_start = &__init_array_start;
	init_function *array_end = &__init_array_end;

	for(init_function *cnt = array_start; cnt != array_end; cnt++) {
		(*cnt)();
      	} 
}


 void *memset(void *data, int value, size_t size) {
         
         unsigned char *data_ = data;
         for(size_t i=0; i<size;i++) {
                 data_[i] = value;
         }

         return data;
 }


 void *memcpy(void *dest, const void *src, size_t size) {

         unsigned char *dest_ = dest;
         const unsigned char *src_ = src;

         for(size_t i = 0; i< size;i++) {
                 dest_[i] = src_[i];
         }

         return dest;

}
