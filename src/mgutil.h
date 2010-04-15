#ifndef __MGUTIL_H__
#define __MGUTIL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

char* human_readable_number(uint64_t number);
char* human_readable_filesize(uint64_t file_size);
uint64_t number_from_string(char* string);
uint64_t size_from_string(char* string);

#ifdef __cplusplus
}
#endif

#endif
