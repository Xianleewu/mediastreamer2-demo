/*   ----------------------------------------------------------------------
Copyright (C) 2014-2016 Fuzhou Rockchip Electronics Co., Ltd

     Sec Class: Rockchip Confidential

V1.0 Dayao Ji <jdy@rock-chips.com>
---------------------------------------------------------------------- */


#ifndef __FWK_GL_API_849390303__
#define __FWK_GL_API_849390303__
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#include "fwk_gl_def.h"

#define LOG_TAG "fwk_msg"


#if 1
#define fwk_debug(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(msg): line:[%04d] %s() \n", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)



#define FWK_CHECK( _X_ )	do{ fwk_debug(#_X_ "\r\n");\
		if( (_X_)  != 0)\
		{\
			fwk_debug( "call %s failed ", #_X_);\
		}\
		fwk_debug("Call %s success!", #_X_);\
	}while(0)
#endif




extern int fwk_create_queue(int mod_id);
extern void fwk_remove_queue(void);


extern int fwk_send_message(FWK_MSG *msg);
extern int fwk_send_message_ext(int sour, int dest,int msg_id, void *data, int size);
extern FWK_MSG * fwk_get_message(int queque, int msgflg, int *ret);
extern int fwk_free_message(FWK_MSG **msg);





extern void *FWK_Malloc(int size, const char * file, int line);
extern void FWK_Free(const char *file, int line, void * mem);

extern void *_FWK_MEMCPY (void * _DEST_PTR, int _DEST_LEN, void *  _SRC_PTR, int _SRC_LEN, int _SIZE, const char *file, int line);
extern void *_FWK_MEMSET(void *_DEST_PTR, int _VAL, int _SIZE, const char *file, int line);
extern char * _FWK_STRNCPY (char * _DEST_PTR, int  _DEST_LEN, const char *_SRC_PTR, int _SIZE, const char *file, int line);
extern char *_FWK_STRCAT(char *_DEST_PTR, int  _DEST_LEN, const char *_SRC_PTR, const char *file, int line);
extern int _FWK_STRLEN (char *_STR_PTR, const char *file, int line);
extern int _FWK_SPRINTF (char * _DEST_PTR, int  _DEST_LEN, const char *file, int line, const char *_FMT, ...);



#define FWK_MALLOC(size)		FWK_Malloc((size), __FILE__, __LINE__)
#define FWK_FREE(_PMEM)		do{\
							FWK_Free(__FILE__, __LINE__, (_PMEM));  \
							if((_PMEM)) (_PMEM) = NULL;\
							}while(0)
#define FWK_MEMCPY(_DEST_PTR, _DEST_LEN, _SRC_PTR, _SRC_LEN, _SIZE)  _FWK_MEMCPY((_DEST_PTR), (_DEST_LEN), (_SRC_PTR), (_SRC_LEN), (_SIZE), __FILE__, __LINE__)
#define FWK_MEMSET(_DEST_PTR, _VAL, _SIZE) _FWK_MEMSET((_DEST_PTR), (_VAL), (_SIZE), __FILE__, __LINE__)
#define FWK_STRNCPY(_DEST_PTR, _DEST_LEN, _SRC_PTR, _SIZE) _FWK_STRNCPY((_DEST_PTR), (_DEST_LEN), (_SRC_PTR), (_SIZE), __FILE__, __LINE__)
#define FWK_STRCAT(_DEST_PTR, _DEST_LEN, _SRC_PTR) _FWK_STRCAT((_DEST_PTR), (_DEST_LEN), (_SRC_PTR), __FILE__, __LINE__)
#define FWK_STRLEN(_STR_PTR) _FWK_STRLEN((_STR_PTR), __FILE__, __LINE__)
#define FWK_SPRINTF(_DEST_PTR, _DEST_LEN, _FMT, ...) _FWK_SPRINTF((_DEST_PTR),(_DEST_LEN), __FILE__,__LINE__,_FMT, ##__VA_ARGS__)





extern int fwk_view_init(void);
extern int fwk_controller_init(void);



#endif

