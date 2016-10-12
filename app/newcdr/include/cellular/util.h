#ifndef UTIL_H
#define UTIL_H

typedef void* T_SYS_SEM;

/* Semaphore functions. */
T_SYS_SEM sys_sem_new(unsigned int init_val);
void      sys_sem_signal(T_SYS_SEM sem);
void      sys_sem_wait(T_SYS_SEM sem);
void      sys_sem_free(T_SYS_SEM sem);
int       sys_sem_wait_timeout(T_SYS_SEM sem, unsigned int timeout);

#endif /* UTIL_H */

