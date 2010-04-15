#include "mgutil.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char* human_readable_number(uint64_t number)
{
  char number_buf[1024];
  if (number >= 1000 * 1000 * 1000) {
    double prefix = ((double) number) / (1000 * 1000 * 1000);
    snprintf(number_buf,1024,"%.2fG",prefix);
  } else if (number >= 1000 * 1000) {
    double prefix = ((double) number) / (1000 * 1000);
    snprintf(number_buf,1024,"%.2fM",prefix);
  } else if (number >= 1000) {
    double prefix = ((double) number) / 1000;
    snprintf(number_buf,1024,"%.2fK",prefix);
  } else {
    snprintf(number_buf,1024,"%u",(unsigned int) number);
  }
  return strdup(number_buf);
}

char* human_readable_filesize(uint64_t file_size)
{
  char number_buf[1024];
  if (file_size > 1024 * 1024 * 1024) {
    double prefix = ((double) file_size) / (1024 * 1024 * 1024);
    snprintf(number_buf,1024,"%.2fG",prefix);
  } else if (file_size > 1024 * 1024) {
    double prefix = ((double) file_size) / (1024 * 1024);
    snprintf(number_buf,1024,"%.2fM",prefix);
  } else if (file_size > 1024) {
    double prefix = ((double) file_size) / 1024;
    snprintf(number_buf,1024,"%.2fK",prefix);
  } else {
    snprintf(number_buf,1024,"%u",(unsigned int) file_size);
  }
  return strdup(number_buf);
}

uint64_t number_from_string(char* str)
{
  uint64_t final_number = 0;
  unsigned int number;
  char unit;
  sscanf(str,"%d%c", &number, &unit);
  char lower_unit = tolower(unit);
  if (lower_unit == 'k') {
    final_number = number * 1000;
  } else if (lower_unit == 'm') {
    final_number = number * 1000 * 1000;
  } else if (lower_unit == 'b') {
    final_number = ((uint64_t)number) * 1000LL * 1000LL * 1000LL;
  } else
    final_number = number;
  
  return final_number;
}

uint64_t size_from_string(char* str)
{
  uint64_t final_size = 0;
  unsigned int size;
  char unit;
  sscanf(str,"%d%c", &size, &unit);
  char lower_unit = tolower(unit);
  if (lower_unit == 'k') {
    final_size = size * 1024;
  } else if (lower_unit == 'm') {
    final_size = size * 1024 * 1024;
  } else if (lower_unit == 'g') {
    final_size = ((uint64_t)size) * 1024LL * 1024LL * 1024LL;
  } else
    final_size = size;
  
  return final_size;
}
