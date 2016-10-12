/*************************************************************************
    > File Name: cdr_res.h
    > Description: 
		定义需要用到的资源
    > Author: CaoDan
    > Mail: caodan2519@gmail.com 
    > Created Time: 2014年04月18日 星期五 11:21:13
 ************************************************************************/

#ifndef __CDR_RES_H__
#define __CDR_RES_H__

/***************** file define *******************/
#define CON_2STRING(a, b)		a "" b
#define CON_3STRING(a, b, c)	a "" b "" c

#define TYPE_VIDEO "video"
#define TYPE_PIC "photo"

#define INFO_NORMAL "normal"
#define INFO_LOCK	"lock"

#define VIDEO_DIR TYPE_VIDEO"/"
#define PIC_DIR TYPE_PIC"/"
#define THUMBNAIL_DIR ".thumb/"

/* -------------------- hints index --------------------------------- */
#define HINT_INDEX_SAVING_SCREEN		0

#endif	/* __CDR_RES_H__ */
