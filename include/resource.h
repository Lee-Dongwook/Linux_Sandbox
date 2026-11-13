#ifndef RESOURCE_H
#define RESOURCE_H

int resource_apply_cpu_limit(unsigned int seconds);
int resource_apply_memory_limit(unsigned long megabytes);
int resource_apply_file_size_limit(unsigned long megabytes);
int resource_apply_nofile_limit(unsigned int max_files);
int resource_disable_core_dump(void);
int resource_apply_no_new_privs(void);

#endif
