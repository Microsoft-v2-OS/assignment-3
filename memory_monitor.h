#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H


typedef struct {
    unsigned long long vm_size_kb; 
    unsigned long long vm_rss_kb;  
} MemoryUsage;

typedef struct {
    long long vm_size_change_kb;  
    long long vm_rss_change_kb;   
} MemoryDelta;


int read_memory_usage(MemoryUsage *usage);

/* Print a labeled memory snapshot to stdout. Accepts a NULL label. */
void print_memory_usage(const char *label, const MemoryUsage *usage);


int calculate_memory_delta(const MemoryUsage *before,
                           const MemoryUsage *after,
                           MemoryDelta *delta);

void print_memory_delta(const char *label, const MemoryDelta *delta);

#endif /* MEMORY_MONITOR_H */
