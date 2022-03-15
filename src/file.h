#ifndef __subst_file_h
#define __subst_file_h

FILE *subst_file_open(const char *file_name, const char *mode_string);
FILE *subst_file_from_string(const char *file_contents);
char *subst_file_read_all(const char *file_path);

#endif
