/*************************************************************************
  > File Name: debug.h
  > Description: 
		for debug options
  > Author: CaoDan
  > Mail: 464114320@qq.com 
  > Created Time: 2014年03月28日 星期五 11:43:51
 ************************************************************************/

#ifndef __DEBUG_H__
#define __DEBUG_H__

/* 
 * if USE_ANDROID_DEBUG is set to 1, then use the android log mechanism
 * if USE_ANDROID_DEBUG is set to 0, then ues the fprint instead
 * */
#define USE_ANDROID_DEBUG	1

/* if DB_LOG_LEVEL set to 1, then all the debug info is included */
#define DB_LOG_LEVEL	1
#undef	DB_MSG
#undef	DB_DEBUG
#undef	DB_INFO
#undef	DB_WARN
#undef	DB_ERROR

#if DB_LOG_LEVEL	== 5
#define DB_ERROR
#elif DB_LOG_LEVEL	== 4
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL	== 3
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 2
#define DB_DEBUG
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 1
#define DB_MSG
#define DB_DEBUG
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#endif

#if DB_LOG_LEVEL > 0
#undef LOG_NDEBUG
#undef NDEBUG
#else
#define LOG_NDEBUG
#define NDEBUG
#endif

#if USE_ANDROID_DEBUG
#include <cutils/log.h>
static char db_buf[100];
#endif

#if USE_ANDROID_DEBUG

#ifdef DB_ERROR
#define db_error(...)	\
	do { \
		sprintf(db_buf, "%s line[%04d] %s()", LOG_TAG, __LINE__, __FUNCTION__); \
		ALOG(LOG_ERROR, db_buf, ##__VA_ARGS__); \
	} while(0)
#else
#define db_error(...)
#endif

#ifdef DB_WARN
#define db_warn(...) \
	do { \
		sprintf(db_buf, "%s line[%04d] %s()", LOG_TAG, __LINE__, __FUNCTION__); \
		ALOG(LOG_WARN, db_buf, ##__VA_ARGS__); \
	} while(0)
#else
#define db_warm(...)
#endif

#ifdef DB_INFO
#define db_info(...)	\
	do { \
		sprintf(db_buf, "%s line[%04d] %s()", LOG_TAG, __LINE__, __FUNCTION__); \
		ALOG(LOG_INFO, db_buf, ##__VA_ARGS__);\
	} while(0)
#else
#define db_info(...)
#endif

#ifdef DB_DEBUG
#define db_debug(...)	\
	do { \
		sprintf(db_buf, "%s line[%04d] %s()", LOG_TAG, __LINE__, __FUNCTION__); \
		ALOG(LOG_DEBUG, db_buf, ##__VA_ARGS__); \
	} while(0)
#else
#define db_debug(...)
#endif

#ifdef DB_MSG
#define db_msg(...)	\
	do { \
		sprintf(db_buf, "%s line[%04d] %s()", LOG_TAG, __LINE__, __FUNCTION__); \
		ALOG(LOG_VERBOSE, db_buf, ##__VA_ARGS__);\
	} while(0)
#else
#define db_msg(...)
#endif

#else

#ifdef DB_ERROR
#define db_error(fmt, ...) \
	do { fprintf(stderr, LOG_TAG "(error): line[%04d] %s() ", __LINE__, __FUNCTION__); fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_error(fmt, ...)
#endif /* DB_ERROR */

#ifdef DB_WARN
#define db_warn(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(warn): line[%04d] %s()", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_warn(fmt, ...)
#endif /* DB_WARN */

#ifdef DB_INFO
#define db_info(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(info): line[%04d] %s()", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_info(fmt, ...)
#endif /* DB_INFO */

#ifdef DB_DEBUG
#define db_debug(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(debug): "); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_debug(fmt, ...)
#endif /* DB_DEBUG */

#ifdef DB_MSG
#define db_msg(fmt, ...) \
	do { fprintf(stdout, LOG_TAG "(msg): line:[%04d] %s() ", __LINE__, __FUNCTION__); fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)
#else
#define db_msg(fmt, ...)
#endif /* DB_MSG */

#endif


#endif		/* __DEBUG_H__ */
