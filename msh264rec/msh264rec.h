#ifndef MSH264REC_H
#define MSH264REC_H

#include "mediastreamer2/msfilter.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32
#	include <io.h>
#	ifndef R_OK
#		define R_OK 0x2
#	endif
#	ifndef W_OK
#		define W_OK 0x6
#	endif
#   ifndef F_OK
#       define F_OK 0x0
#   endif

#	ifndef S_IRUSR
#	define S_IRUSR S_IREAD
#	endif

#	ifndef S_IWUSR
#	define S_IWUSR S_IWRITE
#	endif

#	define open _open
#	define read _read
#	define write _write
#	define close _close
#	define access _access
#	define lseek _lseek
#else /*WIN32*/

#	ifndef O_BINARY
#	define O_BINARY 0
#	endif

#endif /*!WIN32*/

extern MSFilterDesc ms_h264_rec_desc;

#define MS_H264_REC_OPEN	MS_FILTER_METHOD(MS_H264_REC_ID,0,const char)
#define MS_H264_REC_START	MS_FILTER_METHOD_NO_ARG(MS_H264_REC_ID,1)
#define MS_H264_REC_STOP	MS_FILTER_METHOD_NO_ARG(MS_H264_REC_ID,2)
#define MS_H264_REC_CLOSE	MS_FILTER_METHOD_NO_ARG(MS_H264_REC_ID,3)


#endif // LIBMSH264REC_H
