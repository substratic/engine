#ifndef __subst_log_h
#define __subst_log_h

void subst_log_file_set(const char *file_path);
void subst_log(const char *format, ...);
void subst_log_mem(void *ptr, const char *format, ...);

#endif
