/*
mediastreamer2 x264 plugin
Copyright (C) 2006-2010 Belledonne Communications SARL (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msfilerec.h"
#include "mediastreamer2/mscommon.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _MSC_VER
#include <stdint.h>
#endif

#ifndef VERSION
#define VERSION "1.4.1"
#endif

#include "msh264rec.h"

#define HEADER_LEN 4

static int rec_close(MSFilter *f, void *arg);
static char header[] = {0x00,0x00,0x00,0x01};

typedef struct RecState{
    int fd;
    MSRecorderState state;
} RecState;

static void rec_init(MSFilter *f){
    RecState *s=ms_new0(RecState,1);
    s->fd=-1;
    s->state=MSRecorderClosed;
    f->data=s;
}

static void rec_process(MSFilter *f){
    RecState *s=(RecState*)f->data;
    mblk_t *m;
    int err;
    while((m=ms_queue_get(f->inputs[0]))!=NULL)
    {
        mblk_t *it=m;
        ms_mutex_lock(&f->lock);
        if (s->state==MSRecorderRunning)
        {
            while(it!=NULL){
                int len=(int)(it->b_wptr-it->b_rptr);
                write(s->fd,header,HEADER_LEN);
                if ((err=write(s->fd,it->b_rptr,len))!=len)
                {
                    if (err<0)
                        ms_warning("MSFileRec: fail to write %i bytes: %s",len,strerror(errno));
                }
                it=it->b_cont;
            }
        }
        ms_mutex_unlock(&f->lock);
        freemsg(m);
    }
}

static int rec_open(MSFilter *f, void *arg){
    RecState *s=(RecState*)f->data;
    const char *filename=(const char*)arg;
    int flags;

    if (s->fd!=-1)
    {
        rec_close(f,NULL);
    }

    flags=O_WRONLY|O_CREAT|O_TRUNC|O_BINARY;

    s->fd=open(filename,flags, S_IRUSR|S_IWUSR);
    if (s->fd==-1)
    {
        ms_warning("Cannot open %s: %s",filename,strerror(errno));
        return -1;
    }

    ms_message("MSH264Rec: recording into %s",filename);
    ms_mutex_lock(&f->lock);
    s->state=MSRecorderPaused;
    ms_mutex_unlock(&f->lock);
    return 0;
}

static int rec_start(MSFilter *f, void *arg){
    RecState *s=(RecState*)f->data;
    if (s->state!=MSRecorderPaused){
        ms_error("MSH264Rec: cannot start, state=%i",s->state);
        return -1;
    }
    ms_mutex_lock(&f->lock);
    s->state=MSRecorderRunning;
    ms_mutex_unlock(&f->lock);
    return 0;
}

static int rec_stop(MSFilter *f, void *arg){
    RecState *s=(RecState*)f->data;
    ms_mutex_lock(&f->lock);
    s->state=MSRecorderPaused;
    ms_mutex_unlock(&f->lock);
    return 0;
}

static int rec_close(MSFilter *f, void *arg){
    RecState *s=(RecState*)f->data;
    ms_mutex_lock(&f->lock);
    s->state=MSRecorderClosed;
    if (s->fd!=-1){
        close(s->fd);
        s->fd=-1;
    }
    ms_mutex_unlock(&f->lock);
    return 0;
}

static int rec_get_state(MSFilter *f, void *arg){
    RecState *s=(RecState*)f->data;
    *(MSRecorderState*)arg=s->state;
    return 0;
}

static void rec_uninit(MSFilter *f){
    RecState *s=(RecState*)f->data;
    if (s->fd!=-1) rec_close(f,NULL);
    ms_free(s);
}

static MSFilterMethod rec_methods[]={
    {	MS_H264_REC_OPEN	,	rec_open	},
    {	MS_H264_REC_START	,	rec_start	},
    {	MS_H264_REC_STOP	,	rec_stop	},
    {	MS_H264_REC_CLOSE	,	rec_close	},
    {	MS_RECORDER_OPEN	,	rec_open	},
    {	MS_RECORDER_START	,	rec_start	},
    {	MS_RECORDER_PAUSE	,	rec_stop	},
    {	MS_RECORDER_CLOSE	,	rec_close	},
    {	MS_RECORDER_GET_STATE	,	rec_get_state	},
    {	0			,	NULL		}
};

#ifndef _MSC_VER

MSFilterDesc ms_h264_rec_desc={
    .id=MS_H264_REC_ID,
    .name="MSH264Rec",
    .text="h264 file recorder",
    .category=MS_FILTER_OTHER,
    .ninputs=1,
    .noutputs=0,
    .init=rec_init,
    .process=rec_process,
    .uninit=rec_uninit,
    .methods=rec_methods
};

#else

MSFilterDesc ms_h264_rec_desc={
    MS_H264_REC_ID,
    "MSH264Rec",
    "h264 file recorder",
    MS_FILTER_OTHER,
    NULL,
    1,
    0,
    rec_init,
    NULL,
    rec_process,
    NULL,
    rec_uninit,
    rec_methods
};

#endif

MS2_PUBLIC void libmsh264rec_init(MSFactory *factory){
    ms_factory_register_filter(factory, &ms_h264_rec_desc);
    ms_message("msh264rec-" VERSION " plugin registered.");
}
