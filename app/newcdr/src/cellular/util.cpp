#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <utils/Log.h>

#include <cellular/util.h>

#define LOG_TAG "CELLULAR"

typedef signed long Int32;
typedef unsigned long UInt32;

/**
 * This function allocates memory
 *
 * @param UInt32 size    size to allocate
 * @return void *           pointer to memory or NULL if alloc failed
 */
void *memutil_alloc(UInt32 size)
{
    /* system call to malloc is only done here */
    void *p_mem = malloc(size);
		
    return(p_mem);
}

/**
 * This function frees memory
 *
 * @param void *p_mem
 * @return Int32             1
 */
Int32 memutil_free(void *p_mem)
{
    if( p_mem )
    {
        /* system call to free is only done here */
        free(p_mem);
        p_mem = NULL;
    }
    return 1;
}

/**
 * This function creates a new semaphore.
 *
 * @param init_val       The initial value for the semaphore.
 * @return               A pointer to the new semaphore structure.
 *                       Function 'hangs' if not successful.
 */
/* Semaphore functions. */
T_SYS_SEM sys_sem_new(
    unsigned int init_val)
{
    sem_t *p_sem = (sem_t *)memutil_alloc(sizeof(sem_t));
    if(NULL == p_sem)
    {
        return NULL;
    }
    if(-1 == sem_init(p_sem, 0, init_val))
    {
        memutil_free(p_sem);
        return NULL;
    }
    return p_sem;
}


/**
 * This function frees a semaphore.
 *
 * @param p_sem     A pointer to the semaphore to free.
 * @return          None
 *                  Function 'hangs' if not successful.
 */
void sys_sem_free(
    T_SYS_SEM p_sem)
{
    sem_destroy((sem_t *)p_sem);
    memutil_free(p_sem);
}


/**
 * This function signals a semaphore.
 *
 * @param p_sem     A pointer to the semaphore to signal.
 * @return          None
 *                  Function 'hangs' if not successful.
 */
void sys_sem_signal(
    T_SYS_SEM p_sem)
{
    sem_post((sem_t *)p_sem);
}


/**
 * This function waits for a semaphore.
 *
 * @param p_sem     A pointer to the semaphore on which to wait.
 * @return          None
 *                  Function 'hangs' if not successful.
 */
void sys_sem_wait(
    T_SYS_SEM p_sem)
{
    int ret = -1;

    /* restart sem_wait if interrupted by system call */
    do
    {
        ret = sem_wait((sem_t *)p_sem);
        if(0 != ret)
        {
            if(EINTR != errno)
            {
                ALOGE("%s: sem_wait returned errno(%d)\n",
                    __FUNCTION__, errno);
            }
        }
    } while((0 != ret) && (EINTR == errno));

}

/**
 * This function waits for a semaphore, but only until timeout
 *
 * @param p_sem     A pointer to the semaphore on which to wait.
 * @param timeout   duration to wait (in secs)
 * @return          value returned from sem_timedwait 
 *                  Function 'hangs' if not successful.
 */
int sys_sem_wait_timeout(
    T_SYS_SEM p_sem,
    unsigned int timeout)
{
    int ret = -1;
    struct timespec to = {0,0};

//    RIL_LOGFN("%s++ \n", __FUNCTION__);
#if !defined(HOST_TEST_SETUP)
    clock_gettime(CLOCK_REALTIME, &to);
#endif
    to.tv_sec += (time_t) timeout;

    ALOGD("%s: timeout in %u sec(s)\n", __FUNCTION__, timeout);

    /* wait for semaphore to succeed or timeout */
    /* restart if interrupted by system calls */
    do
    {
        ret = sem_timedwait((sem_t *)p_sem, &to);
        if(0 != ret)
        {
            if(EINTR != errno)
            {
                ALOGE("%s: sem_wait returned errno(%d)\n",
                    __FUNCTION__, errno);
            }
        }
    } while((0 != ret) && (EINTR == errno));

    return ret;
}

