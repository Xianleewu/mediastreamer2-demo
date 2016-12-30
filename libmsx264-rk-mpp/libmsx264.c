/*
mediastreamer2 mpp264 plugin
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/rfc3984.h"

#include <mpp/mpp_buffer.h>
#include <mpp/mpp_frame.h>
#include <mpp/mpp_meta.h>
#include <mpp/mpp_packet.h>
#include <mpp/mpp_task.h>
#include <mpp/rk_mpi.h>
#include <mpp/rk_mpi_cmd.h>
#include <mpp/rk_type.h>
#include <mpp/vpu_api.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _MSC_VER
#include <stdint.h>
#endif

#ifndef VERSION
#define VERSION "1.4.1"
#endif

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))
#define MPP_VSWAP(a, b)         { a ^= b; b ^= a; a ^= b; }

#define RC_MARGIN 10000 /*bits per sec*/
#define H264_HDR_LEN 3

typedef struct _RkMpiEncoder {
    MppCtx						mpiCtx;
    MppApi						*mpi;
    MppCodingType				encType;
    MppEncConfig				encCfg;
} RkMpiEncoder;

/* the goal of this small object is to tell when to send I frames at startup:
at 2 and 4 seconds*/
typedef struct VideoStarter{
    uint64_t next_time;
    int i_frame_count;
}VideoStarter;

typedef struct _EncData {
    RkMpiEncoder *encoder;
    MppBufferGroup frm_grp;
    MppBufferGroup pkt_grp;
    MppPacket packet;
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppEncSeiMode sei_mode;

    uint32_t hor_stride;
    uint32_t ver_stride;
    MppFrameFormat fmt;

    char *extradata;
    int extradata_size;
    int mode;
    uint64_t framenum;
    Rfc3984Context *packer;
    int keyframe_int;
    VideoStarter starter;
    MSVideoConfiguration vconf;
    bool_t generate_keyframe;
}EncData;

MPP_RET rk_encoder_create(RkMpiEncoder **context, MppCodingType encType) {
    MPP_RET ret = MPP_OK;

    RkMpiEncoder *pCtx = (RkMpiEncoder *)malloc(sizeof(RkMpiEncoder));
    memset((void*)pCtx, 0, sizeof(RkMpiEncoder));
    pCtx->encType = encType;

    ret = mpp_create(&pCtx->mpiCtx, &pCtx->mpi);

    if (MPP_OK != ret) {
        ms_error("mpp_create failed\n");
        return ret;
    }

    ret = mpp_init(pCtx->mpiCtx, MPP_CTX_ENC, encType);
    if (MPP_OK != ret) {
        ms_error("mpp_init failed\n");
        return ret;
    }

    *context = pCtx;
    return MPP_OK;
}

MPP_RET rk_encoder_destory(RkMpiEncoder *context) {
    if (context->mpiCtx) {
        mpp_destroy(context->mpiCtx);
        context->mpiCtx = NULL;
    }
    return MPP_OK;
}

MPP_RET rk_encoder_set_seimode(RkMpiEncoder *context, MppEncSeiMode *seiMode) {
    MPP_RET ret = MPP_OK;

    ret = context->mpi->control(context->mpiCtx, MPP_ENC_SET_SEI_CFG, seiMode);
    if (MPP_OK != ret) {
        ms_error("mpi control enc set sei failed\n");
        return ret;
    }

    return MPP_OK;
}

MPP_RET rk_encoder_set_cfg(RkMpiEncoder *context, MppEncConfig *cfg) {
    MPP_RET ret = MPP_OK;

    ret = context->mpi->control(context->mpiCtx, MPP_ENC_SET_CFG, cfg);
    if (MPP_OK != ret) {
        ms_error("mpi control enc set config info failed\n");
        return ret;
    }

    return MPP_OK;
}

//pps and sps info
MPP_RET rk_encoder_get_extrainfo(RkMpiEncoder *context, MppPacket *packet) {
    MPP_RET ret = MPP_OK;

    ret = context->mpi->control(context->mpiCtx, MPP_ENC_GET_EXTRA_INFO, packet);
    if (MPP_OK != ret) {
        ms_error("mpi control enc get extra info failed\n");
        return ret;
    }

    return MPP_OK;
}


MPP_RET rk_encode_frame(RkMpiEncoder *context, MppBuffer pkt_buf_in, MppBuffer frm_buf_in, MppPacket *pkt_out) {
    MPP_RET ret = MPP_OK;
    MppFrame  frame = NULL;
    MppPacket packet = NULL;
    MppTask task = NULL;

    ret = mpp_frame_init(&frame);
    if (MPP_OK != ret) {
        ms_error("mpp_frame_init failed\n");
        return ret;
    }

    mpp_frame_set_width(frame, context->encCfg.width);
    mpp_frame_set_height(frame, context->encCfg.height);
    mpp_frame_set_hor_stride(frame, MPP_ALIGN(context->encCfg.width, 16));
    mpp_frame_set_ver_stride(frame, MPP_ALIGN(context->encCfg.height, 16));
    mpp_frame_set_buffer(frame, frm_buf_in);
    mpp_packet_init_with_buffer(&packet, pkt_buf_in);

    do {
        ret = context->mpi->dequeue(context->mpiCtx, MPP_PORT_INPUT, &task);
        if (ret) {
            ms_error("mpp task input dequeue failed\n");
            return ret;
        }
        if (task == NULL) {
            ms_message("mpi dequeue from MPP_PORT_INPUT fail, task equal with NULL!");
            usleep(3000);
        } else {
            break;
        }
    } while (1);


    mpp_task_meta_set_frame (task, KEY_INPUT_FRAME,  frame);
    mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, packet);

    ret = context->mpi->enqueue(context->mpiCtx, MPP_PORT_INPUT, task);
    if (ret) {
        ms_error("mpp task input enqueue failed\n");
        return ret;
    }

    do {
        ret = context->mpi->dequeue(context->mpiCtx, MPP_PORT_OUTPUT, &task);
        if (ret) {
            ms_error("mpp task output dequeue failed\n");
            return ret;
        }

        if (task) {
            MppFrame packet_out = NULL;

            mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &packet_out);

            ret = context->mpi->enqueue(context->mpiCtx, MPP_PORT_OUTPUT, task);
            if (ret) {
                ms_error("mpp task output enqueue failed\n");
                return ret;
            }
            break;
        }
    } while (1);

    if (packet && (packet != pkt_buf_in)) {
        *pkt_out = packet;
    }

    if (frame) {
        mpp_frame_deinit(&frame);
    }

    return MPP_OK;
}

static void video_starter_init(VideoStarter *vs){
    vs->next_time=0;
    vs->i_frame_count=0;
}

static void video_starter_first_frame(VideoStarter *vs, uint64_t curtime){
    vs->next_time=curtime+2000;
}

static bool_t video_starter_need_i_frame(VideoStarter *vs, uint64_t curtime){
    if (vs->next_time==0) return FALSE;
    if (curtime>=vs->next_time){
        vs->i_frame_count++;
        if (vs->i_frame_count==1){
            vs->next_time+=2000;
        }else{
            vs->next_time=0;
        }
        return TRUE;
    }
    return FALSE;
}

static void enc_init(MSFilter *f){
    EncData *d=ms_new(EncData,1);

    memset(d, 0, sizeof(EncData));
    d->encoder = NULL;
    d->frm_grp = NULL;
    d->pkt_grp = NULL;

    d->packet = NULL;
    d->frm_buf = NULL;
    d->pkt_buf = NULL;
    //d->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;

    d->sei_mode = MPP_ENC_SEI_MODE_ONE_SEQ;
    d->fmt = MPP_FMT_YUV420P;

    /* some default paramters */
    d->vconf.bitrate_limit = 0;
    d->vconf.fps = 30.0;
    d->vconf.mincpu = 1;
    d->vconf.required_bitrate = 38400;
    d->vconf.vsize = (MSVideoSize) {640, 360};

    d->hor_stride = MPP_ALIGN(d->vconf.vsize.width, 16);
    d->ver_stride = MPP_ALIGN(d->vconf.vsize.height, 16);

    d->keyframe_int=10; /*10 seconds */
    d->mode=1;
    d->framenum=0;
    d->generate_keyframe=FALSE;
    d->packer=NULL;

    f->data=d;
}

static void enc_uninit(MSFilter *f){
    EncData *d=(EncData*)f->data;
    ms_free(d);
}

static void apply_bitrate(MSFilter *f){
    EncData *d=(EncData*)f->data;
    float bitrate;

    bitrate=(float)d->vconf.required_bitrate*0.92;
    if (bitrate>RC_MARGIN)
        bitrate-=RC_MARGIN;

    d->vconf.bitrate_limit = bitrate;
}

static void enc_preprocess(MSFilter *f){
    EncData *d=(EncData*)f->data;

    size_t frame_size = d->hor_stride * d->ver_stride * 3 / 2;
    size_t packet_size = d->vconf.vsize.width * d->vconf.vsize.height;
    size_t mdinfo_size  = (((d->hor_stride + 255) & (~255)) / 16) * (d->ver_stride / 16) * 4;
    int ret = -1;

    /* create encoder demo */
    ret = mpp_buffer_group_get_internal(&d->frm_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        ms_error("failed to get buffer group for input frame ret %d\n", ret);
        //goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&d->pkt_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        ms_error("failed to get buffer group for output packet ret %d\n", ret);
        //goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(d->frm_grp, &d->frm_buf, frame_size);
    if (ret) {
        ms_error("failed to get buffer for input frame ret %d\n", ret);
        //goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(d->pkt_grp, &d->pkt_buf, packet_size);
    if (ret) {
        ms_error("failed to get buffer for input frame ret %d\n", ret);
        //goto MPP_TEST_OUT;
    }

    rk_encoder_create(&d->encoder, MPP_VIDEO_CodingAVC);


    /* set encoder paramters */
    d->encoder->encCfg.size        = sizeof(d->encoder->encCfg);
    d->encoder->encCfg.width       = d->vconf.vsize.width;
    d->encoder->encCfg.height      = d->vconf.vsize.height;
    d->encoder->encCfg.hor_stride  = d->hor_stride;
    d->encoder->encCfg.ver_stride  = d->ver_stride;
    d->encoder->encCfg.format      = d->fmt;
    d->encoder->encCfg.rc_mode     = 0;
    d->encoder->encCfg.skip_cnt    = 0;
    d->encoder->encCfg.fps_in      = 30;
    d->encoder->encCfg.fps_out     = 30;
    d->encoder->encCfg.bps         = 8 * 1024 * 200;
    d->encoder->encCfg.qp          = 24;
    d->encoder->encCfg.gop         = 60;

    d->encoder->encCfg.profile     = 100;
    d->encoder->encCfg.level       = 22;
    d->encoder->encCfg.cabac_en    = 1;

    rk_encoder_set_seimode(d->encoder, &d->sei_mode);
    rk_encoder_set_cfg(d->encoder, &d->encoder->encCfg);

    ms_message("set encoder w:%d h:%d sw:%d sh%d", d->vconf.vsize.width, d->vconf.vsize.height
                , d->encoder->encCfg.hor_stride, d->encoder->encCfg.ver_stride);

    rk_encoder_get_extrainfo(d->encoder, &d->packet);

    if(d->packet) {
        void *ptr = mpp_packet_get_pos(d->packet);
        size_t len = mpp_packet_get_length(d->packet);

        d->extradata= malloc(len);
        if(d->extradata == NULL) {
            ms_error("extradata malloc failed");
            rk_encoder_destory(d->encoder);

            return;
        }

        d->extradata_size = len;
        memcpy(d->extradata, ptr, len);

        d->packet = NULL;
    }

    d->packer=rfc3984_new();
    rfc3984_set_mode(d->packer,d->mode);
    rfc3984_enable_stap_a(d->packer,FALSE);

    apply_bitrate(f);

    d->framenum=0;

    video_starter_init(&d->starter);
}

static void h264_nals_to_msgb(char *nals, int nals_len, MSQueue * nalq) {
    unsigned int i;
    unsigned int slice_len = 0;
    char *nal_last = NULL;
    char *nal_next = NULL;
    mblk_t *m = NULL;

    if(nals_len <= H264_HDR_LEN) {
        ms_error("nals is null");
        return;
    }

    nal_last = nals + H264_HDR_LEN;

    /*int bytes;*/
    for (i=H264_HDR_LEN;i<nals_len - H264_HDR_LEN;++i){
        if((nals[i] == 0x00) && (nals[i+1] == 0x00) && (nals[i] == 0x01)) {
            nal_next = nals + i + H264_HDR_LEN;
            slice_len = nal_next - nal_last - H264_HDR_LEN;

            m=allocb(slice_len + 10,0);
            memcpy(m->b_wptr, nal_last, slice_len);
            if (nal_last[0] == 0x67) {
                ms_message("A SPS is being sent.");
            }else if (nal_last[0] == 0x68) {
                ms_message("A PPS is being sent.");
            }

            nal_last = nal_next;
            m->b_wptr+=slice_len;
            ms_queue_put(nalq,m);
        }
    }

    slice_len = nals_len - (nal_last - nals);

    if(slice_len) {
        m=allocb(slice_len + 10,0);
        memcpy(m->b_wptr, nal_last, slice_len);
        if (nal_last[0] == 0x67) {
            ms_message("A SPS is being sent.");
        }else if (nal_last[0] == 0x68) {
            ms_message("A PPS is being sent.");
        }

        m->b_wptr+=slice_len;
        ms_queue_put(nalq,m);
    }
}

static void enc_process(MSFilter *f){
    EncData *d=(EncData*)f->data;
    uint32_t ts=f->ticker->time*90LL;
    mblk_t *im;
    MSPicture pic;
    MSQueue nalus;

    if (d->encoder==NULL){
        ms_queue_flush(f->inputs[0]);
        return;
    }

    ms_queue_init(&nalus);
    while((im=ms_queue_get(f->inputs[0]))!=NULL){
        if (ms_yuv_buf_init_from_mblk(&pic,im)==0){
            int num_nals=0;
            void *ptr = NULL;
            size_t len = 0;
            void *buf = mpp_buffer_get_ptr(d->frm_buf);
            int y_size = pic.w * pic.h;
            int uv_size = pic.w * pic.h / 4;

            MppPacket packNal = NULL;

            //copy yuv 420p data
            memcpy(buf, pic.planes[0], y_size);
            memcpy(buf + y_size, pic.planes[1], uv_size);
            memcpy(buf + y_size + uv_size, pic.planes[2], uv_size);

            /*send I frame 2 seconds and 4 seconds after the beginning */
            if (video_starter_need_i_frame(&d->starter,f->ticker->time))
                d->generate_keyframe=TRUE;

            if (d->generate_keyframe){
                ms_message("generate key frame here!");
                d->generate_keyframe=FALSE;
                h264_nals_to_msgb(d->extradata, d->extradata_size, &nalus);
            }else {
                //generate normal frame here
            }

            rk_encode_frame(d->encoder, d->pkt_buf, d->frm_buf, &packNal);

            if(!packNal){
                continue;
            }

            ptr	= mpp_packet_get_pos(packNal);
            len	= mpp_packet_get_length(packNal);

            if (ptr && len){
                h264_nals_to_msgb(ptr, len, &nalus);
                rfc3984_pack(d->packer,&nalus,f->outputs[0],ts);
                if (d->framenum==0)
                    video_starter_first_frame(&d->starter,f->ticker->time);
                d->framenum++;
            } else {
                ms_error("mpp_encoder_encode() error.");
            }
            mpp_packet_deinit(&packNal);
        }
        freemsg(im);
    }
}

static void enc_postprocess(MSFilter *f){
    EncData *d=(EncData*)f->data;
    rfc3984_destroy(d->packer);
    d->packer=NULL;
    if (d->encoder != NULL){
        rk_encoder_destory(d->encoder);
        d->encoder=NULL;
    }
}

static int enc_get_br(MSFilter *f, void*arg){
    EncData *d=(EncData*)f->data;
    *(int*)arg=d->vconf.required_bitrate;
    return 0;
}

static int enc_set_configuration(MSFilter *f, void *arg) {
    EncData *d = (EncData *)f->data;
    const MSVideoConfiguration *vconf = (const MSVideoConfiguration *)arg;
    if (vconf != &d->vconf) memcpy(&d->vconf, vconf, sizeof(MSVideoConfiguration));

    if (d->vconf.required_bitrate > d->vconf.bitrate_limit)
        d->vconf.required_bitrate = d->vconf.bitrate_limit;
    if (d->encoder) {
        ms_filter_lock(f);
        apply_bitrate(f);
        //if (x264_encoder_reconfig(d->enc, &d->params) != 0) {
        //    ms_error("x264_encoder_reconfig() failed.");
        //}
        ms_filter_unlock(f);
        return 0;
    }

    ms_message("Video configuration set: bitrate=%dbits/s, fps=%f, vsize=%dx%d", d->vconf.required_bitrate, d->vconf.fps, d->vconf.vsize.width, d->vconf.vsize.height);
    return 0;
}

static int enc_set_br(MSFilter *f, void *arg) {
    EncData *d = (EncData *)f->data;
    int br = *(int *)arg;
    if (d->encoder != NULL) {
        /* Encoding is already ongoing, do not change video size, only bitrate. */
        d->vconf.required_bitrate = br;
    } else {

    }
    return 0;
}

static int enc_set_fps(MSFilter *f, void *arg){
    EncData *d=(EncData*)f->data;
    d->vconf.fps=*(float*)arg;
    enc_set_configuration(f, &d->vconf);
    return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
    EncData *d=(EncData*)f->data;
    *(float*)arg=d->vconf.fps;
    return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
    EncData *d=(EncData*)f->data;
    *(MSVideoSize*)arg=d->vconf.vsize;
    return 0;
}

static int enc_set_vsize(MSFilter *f, void *arg){
    EncData *d = (EncData *)f->data;
    MSVideoSize *vs = (MSVideoSize *)arg;
    d->vconf.vsize = *vs;
    d->hor_stride = MPP_ALIGN(d->vconf.vsize.width, 16);
    d->ver_stride = MPP_ALIGN(d->vconf.vsize.height, 16);
    enc_set_configuration(f, &d->vconf);
    return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
    EncData *d=(EncData*)f->data;
    const char *fmtp=(const char *)arg;
    char value[12];
    if (fmtp_get_value(fmtp,"packetization-mode",value,sizeof(value))){
        d->mode=atoi(value);
        ms_message("packetization-mode set to %i",d->mode);
    }
    return 0;
}

static int enc_req_vfu(MSFilter *f, void *arg){
    EncData *d=(EncData*)f->data;
    d->generate_keyframe=TRUE;
    return 0;
}

static int enc_get_configuration_list(MSFilter *f, void *data) {
    EncData *d = (EncData *)f->data;

    return 0;
}


static MSFilterMethod enc_methods[] = {
    { MS_FILTER_SET_FPS,                       enc_set_fps                },
    { MS_FILTER_SET_BITRATE,                   enc_set_br                 },
    { MS_FILTER_GET_BITRATE,                   enc_get_br                 },
    { MS_FILTER_GET_FPS,                       enc_get_fps                },
    { MS_FILTER_GET_VIDEO_SIZE,                enc_get_vsize              },
    { MS_FILTER_SET_VIDEO_SIZE,                enc_set_vsize              },
    { MS_FILTER_ADD_FMTP,                      enc_add_fmtp               },
    { MS_FILTER_REQ_VFU,                       enc_req_vfu                },
    { MS_VIDEO_ENCODER_REQ_VFU,                enc_req_vfu                },
    { MS_VIDEO_ENCODER_GET_CONFIGURATION_LIST, enc_get_configuration_list },
    { MS_VIDEO_ENCODER_SET_CONFIGURATION,      enc_set_configuration      },
    { 0,                                       NULL                       }
};

#ifndef _MSC_VER

static MSFilterDesc x264_enc_desc={
    .id=MS_FILTER_PLUGIN_ID,
    .name="MSMPP264Enc",
    .text="A H264 encoder based on mpp framework",
    .category=MS_FILTER_ENCODER,
    .enc_fmt="H264",
    .ninputs=1,
    .noutputs=1,
    .init=enc_init,
    .preprocess=enc_preprocess,
    .process=enc_process,
    .postprocess=enc_postprocess,
    .uninit=enc_uninit,
    .methods=enc_methods
};

#else

static MSFilterDesc x264_enc_desc={
    MS_FILTER_PLUGIN_ID,
    "MSMPP264Enc",
    "A H264 encoder based on mpp framework",
    MS_FILTER_ENCODER,
    "H264",
    1,
    1,
    enc_init,
    enc_preprocess,
    enc_process,
    enc_postprocess,
    enc_uninit,
    enc_methods
};

#endif

MS2_PUBLIC void libmsx264_init(void){
    ms_filter_register(&x264_enc_desc);
    ms_message("msmpp264-" VERSION " plugin registered.");
}

