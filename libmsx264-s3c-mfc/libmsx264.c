/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * lib.c
 * Copyright (C) 2015 wuqiang <coderwuqiang@163.com>
 *
 * libmsx264 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libmsx264 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/rfc3984.h"

#include "SsbSipH264Encode.h"
#include "FrameExtractor.h"
#include "H264Frames.h"
#include "LogMsg.h"
#include "MfcDriver.h"

#include "stdlib.h"
#include "fcntl.h"
#include "sys/file.h"
#include "string.h"

#ifdef _MSC_VER
#include <stdint.h>
#endif

#ifndef VERSION
#define VERSION "1.4.1"
#endif

//#define H264_DEBUG

#ifdef H264_DEBUG
FILE *fp;
#endif

#define RC_MARGIN 51200000 /*bits per sec*/
#define SPECIAL_HIGHRES_BUILD_CRF 28
#define INPUT_BUFFER_SIZE       (204800)
static unsigned char delimiter_h264[4] = { 0x00, 0x00, 0x00, 0x01 };

#define MS_X264_CONF(required_bitrate, bitrate_limit, resolution, fps_pc, cpus_pc, fps_mobile, cpus_mobile) \
{ required_bitrate, bitrate_limit, { MS_VIDEO_SIZE_ ## resolution ## _W, MS_VIDEO_SIZE_ ## resolution ## _H },fps_mobile, cpus_mobile, NULL }

static const MSVideoConfiguration H264_conf_list[] = {
    MS_X264_CONF(2048000, 1000000, UXGA, 25, 4, 12, 2), /*1200p*/

    MS_X264_CONF(1024000, 5000000, SXGA_MINUS, 25, 4, 12, 2),

    MS_X264_CONF(1024000, 5000000, 720P, 25, 4, 12, 2),

    MS_X264_CONF( 750000, 2048000, XGA, 20, 4, 12, 2),

    MS_X264_CONF( 500000, 1024000, SVGA, 20, 2, 12, 2),

    MS_X264_CONF( 256000, 800000, VGA, 15, 2, 12, 2), /*480p*/

    MS_X264_CONF( 128000, 512000, CIF, 15, 1, 12, 2),

    MS_X264_CONF( 100000, 1024000, VGA, 15, 1, 10, 1), /*240p*/

    MS_X264_CONF( 100000, 380000, QVGA, 15, 1, 20, 1), /*240p*/

    MS_X264_CONF( 128000, 170000, QCIF, 10, 1, 10, 1),
    MS_X264_CONF( 64000, 128000, QCIF, 10, 1, 7, 1),
    MS_X264_CONF( 0, 64000, QCIF, 10, 1, 5, 1) };

typedef struct _h264_nals {
    unsigned char *encoded_buf;
    long nal_size;
} h264_nals;

typedef struct _H264_param {
    int video_size;
    int video_width;
    int video_height;
    int video_bitrate;
    int video_fps;
} H264_param;

typedef struct _EncData {
    void *enc;
    FRAMEX_CTX *pFrameExCtx;
    FRAMEX_STRM_PTR file_strm;
    H264_param params;
    int mode;
    uint32_t framenum;
    Rfc3984Context *packer;
    int keyframe_int;
    const MSVideoConfiguration *vconf_list;
    MSVideoConfiguration vconf;
    bool_t generate_keyframe;
} EncData;

void *mfc_encoder_init(int width, int height, int frame_rate, int bitrate,
                       int gop_num) {
    void *handle;

    handle = SsbSipH264EncodeInit(width, height, frame_rate, bitrate, gop_num);
    if (handle == NULL) {
        LOG_MSG(LOG_ERROR, "Test_Encoder", "SsbSipH264EncodeInit Failed\n");
        return NULL;
    }

    SsbSipH264EncodeExe(handle);

    return handle;
}

static void enc_init(MSFilter *f) {
#ifdef H264_DEBUG
    fp = fopen("/mnt/Test.h264", "wr");
#endif
    EncData *d = ms_new(EncData,1);
    d->enc = NULL;
    d->pFrameExCtx = NULL;
    d->keyframe_int = 15;   // 10 seconds
    d->mode = 1; 		    // 0: single nal; 1: fu-a
    d->framenum = 0;
    d->generate_keyframe = FALSE;
    d->packer = NULL;
    d->vconf_list = H264_conf_list;
    d->vconf = ms_video_find_best_configuration_for_bitrate(d->vconf_list,
                                                            10240000, ms_get_cpu_count());
    f->data = d;
}

static void enc_uninit(MSFilter *f) {
    EncData *d = (EncData*) f->data;
    ms_free(d);
}

static void apply_bitrate(MSFilter *f) {
    EncData *d = (EncData*) f->data;
    H264_param *params = &d->params;
    float bitrate;

    bitrate = (float) d->vconf.required_bitrate * 0.92;
    if (bitrate > RC_MARGIN)
        bitrate -= RC_MARGIN;
    params->video_bitrate = bitrate;
}

static void enc_preprocess(MSFilter *f) {
    EncData *d = (EncData*) f->data;
    H264_param *params = &d->params;

    d->packer = rfc3984_new();
    rfc3984_set_mode(d->packer, d->mode);
    rfc3984_enable_stap_a(d->packer, FALSE);

    params->video_width = d->vconf.vsize.width;
    params->video_height = d->vconf.vsize.height;
    params->video_size = d->vconf.vsize.width * d->vconf.vsize.height * 3 / 2;
    params->video_fps = (int) d->vconf.fps;

    apply_bitrate(f);

    ms_message("Video configuration set: bitrate=%dbits/s, fps=%d, vsize=%dx%d",
               params->video_bitrate, params->video_fps, params->video_width,
               params->video_height);

    d->enc = mfc_encoder_init(params->video_width, params->video_height,
                              params->video_fps, params->video_bitrate, d->keyframe_int);
    d->pFrameExCtx = FrameExtractorInit(FRAMEX_IN_TYPE_MEM, delimiter_h264,
                                        sizeof(delimiter_h264), 1);

    if ((d->enc == NULL) || (d->pFrameExCtx == NULL)) {
        ms_error("Fail to create H264 encoder.");
    } else {
        ms_message("Created an H264 encoder.");
    }
    d->framenum = 0;
}

static void h264nals_to_msgb(EncData *d, h264_nals nal, MSQueue * nalus) {
    mblk_t *m;
    unsigned int   nFrameLength;
    unsigned char *pStrmBuf = NULL;
    unsigned char *oldPoint = NULL;
    /* Set Extractor input memory range*/
    d->file_strm.p_start = d->file_strm.p_cur = nal.encoded_buf;
    d->file_strm.p_end = (nal.encoded_buf + nal.nal_size);
    FrameExtractorFirst(d->pFrameExCtx, &d->file_strm);
    /* Save old point in case Memory leak */
    oldPoint = pStrmBuf = (unsigned char*) malloc(INPUT_BUFFER_SIZE);
    if (pStrmBuf == NULL)
        return;
    /* First Frame need separate out SPS and PPS */
    if (d->framenum == 0) {
        unsigned int sliceLen;
        unsigned int position;
        unsigned int lastPosition;
        unsigned char *curPoint = NULL;

        nFrameLength = ExtractConfigStreamH264(d->pFrameExCtx, &d->file_strm,pStrmBuf, INPUT_BUFFER_SIZE, NULL);
        curPoint = pStrmBuf;
        for (position = 4; position < nFrameLength - 4; position++) {
            if ((pStrmBuf[position] == 0x00) && (pStrmBuf[position + 1] == 0x00)
                    && (pStrmBuf[position + 2] == 0x00) && (pStrmBuf[position + 3] == 0x01)) {
                lastPosition = position;
                sliceLen = lastPosition - sliceLen;
                m = allocb(sliceLen + 10, 0);
                memcpy(m->b_wptr, curPoint + 4, sliceLen - 4);
#ifdef H264_DEBUG
                fwrite(curPoint, sliceLen, 1, fp);
                fwrite("ENDOF", sizeof("ENDOF"), 1, fp);
#endif
                curPoint += sliceLen;
                m->b_wptr += (sliceLen - 4);
                ms_queue_put(nalus, m);
            }
        }

        sliceLen = nFrameLength - lastPosition;

        m = allocb(sliceLen + 10, 0);
        memcpy(m->b_wptr, curPoint + 4, sliceLen - 4);
#ifdef H264_DEBUG
        fwrite(curPoint, sliceLen, 1, fp);
        fwrite("ENDOF", sizeof("ENDOF"), 1, fp);
#endif
        m->b_wptr += (sliceLen - 4);
        ms_queue_put(nalus, m);
    }
    else {
        while (TRUE) {
            nFrameLength = 0;

            memset(pStrmBuf, 0, INPUT_BUFFER_SIZE);
            nFrameLength = NextFrameH264(d->pFrameExCtx, &d->file_strm,pStrmBuf,INPUT_BUFFER_SIZE, NULL);
            if (nFrameLength < 4)
                break;
#ifdef H264_DEBUG
            fwrite(pStrmBuf, nFrameLength, 1, fp);
            fwrite("ENDOF", sizeof("ENDOF"), 1, fp);
#endif
            m = allocb(nFrameLength + 10, 0);
            /* Remove head 0x00,0x00,0x00,0x01 */
            memcpy(m->b_wptr, pStrmBuf + 4, nFrameLength - 4);
            m->b_wptr += (nFrameLength - 4);
            if (pStrmBuf[4] == 0x67) {
                ms_message("A SPS is being sent.");
            } else if (pStrmBuf[4] == 0x68) {
                ms_message("A PPS is being sent.");
            }
            ms_queue_put(nalus, m);
        }
    }
    free(oldPoint);
}

void *mfc_encoder_exe(void *handle, unsigned char *yuv_buf, int frame_size,
                      int first_frame, long *size) {
    unsigned char *p_inbuf, *p_outbuf;
    int hdr_size;

    p_inbuf = (unsigned char*) SsbSipH264EncodeGetInBuf(handle, 0);

    memcpy(p_inbuf, yuv_buf, frame_size);

    SsbSipH264EncodeExe(handle);
    if (first_frame) {
        SsbSipH264EncodeGetConfig(handle, H264_ENC_GETCONF_HEADER_SIZE,
                                  &hdr_size);
    }

    p_outbuf = (unsigned char*) SsbSipH264EncodeGetOutBuf(handle, size);

    return p_outbuf;
}

static void enc_process(MSFilter *f) {
    EncData *d = (EncData*) f->data;
    uint32_t ts = f->ticker->time * 90LL;
    unsigned char yuv420[d->params.video_size];
    mblk_t *im;
    MSPicture pic;
    MSQueue nalus;
    h264_nals onal;

    if (d->enc == NULL) {
        ms_queue_flush(f->inputs[0]);
        return;
    }

    ms_queue_init(&nalus);

    while ((im = ms_queue_get(f->inputs[0])) != NULL) {
        if (ms_yuv_buf_init_from_mblk(&pic, im) == 0) {

            memset(&onal, 0, sizeof(onal));
            memcpy(yuv420, pic.planes[0],
                    d->params.video_width * d->params.video_height);
            memcpy(yuv420 + (d->params.video_width * d->params.video_height),
                   pic.planes[1],
                    d->params.video_width * d->params.video_height / 4);
            memcpy(
                        yuv420
                        + (d->params.video_width * d->params.video_height
                           * 5 / 4), pic.planes[2],
                    d->params.video_width * d->params.video_height / 4);
            //fwrite(yuv420, d->params.video_size, 1, fp);

            onal.encoded_buf = (unsigned char*) mfc_encoder_exe(d->enc, yuv420,
                                                                d->params.video_size, d->framenum == 0 ? 1 : 0,
                                                                &onal.nal_size);

            if (onal.encoded_buf != NULL) {
                //fwrite(onal.encoded_buf, onal.nal_size, 1, fp);
                h264nals_to_msgb(d, onal, &nalus);
                rfc3984_pack(d->packer, &nalus, f->outputs[0], ts);
                d->framenum++;
            } else {
                ms_error("encoder return a null data");
            }
        }

        freemsg(im);
    }
}

static void enc_postprocess(MSFilter *f) {
    EncData *d = (EncData*) f->data;
    rfc3984_destroy(d->packer);
#ifdef H264_DEBUG
    fclose(fp);
#endif
    d->packer = NULL;
    if (d->enc != NULL) {
        SsbSipH264EncodeDeInit(d->enc);
        d->enc = NULL;
    }
}

static int enc_get_br(MSFilter *f, void*arg) {
    EncData *d = (EncData*) f->data;
    *(int*) arg = d->vconf.required_bitrate;
    return 0;
}

static int enc_set_configuration(MSFilter *f, void *arg) {
    EncData *d = (EncData *) f->data;
    const MSVideoConfiguration *vconf = (const MSVideoConfiguration *) arg;
    if (vconf != &d->vconf)
        memcpy(&d->vconf, vconf, sizeof(MSVideoConfiguration));

    if (d->vconf.required_bitrate > d->vconf.bitrate_limit)
        d->vconf.required_bitrate = d->vconf.bitrate_limit;
    if (d->enc) {
        ms_filter_lock(f);
        apply_bitrate(f);
        //if (x264_encoder_reconfig(d->enc, &d->params) != 0)
        ms_filter_unlock(f);
        return 0;
    }

    ms_message("Video configuration set: bitrate=%dbits/s, fps=%f, vsize=%dx%d",
               d->vconf.required_bitrate, d->vconf.fps, d->vconf.vsize.width,
               d->vconf.vsize.height);
    return 0;
}

static int enc_set_br(MSFilter *f, void *arg) {
    EncData *d = (EncData *) f->data;
    int br = *(int *) arg;
    if (d->enc != NULL) {
        /* Encoding is already ongoing, do not change video size, only bitrate. */
        d->vconf.required_bitrate = br;
        enc_set_configuration(f, &d->vconf);
    } else {
        MSVideoConfiguration best_vconf =
                ms_video_find_best_configuration_for_bitrate(d->vconf_list, br,
                                                             ms_get_cpu_count());
        enc_set_configuration(f, &best_vconf);
    }
    return 0;
}

static int enc_set_fps(MSFilter *f, void *arg) {
    EncData *d = (EncData*) f->data;
    d->vconf.fps = *(float*) arg;
    enc_set_configuration(f, &d->vconf);
    return 0;
}

static int enc_get_fps(MSFilter *f, void *arg) {
    EncData *d = (EncData*) f->data;
    *(float*) arg = d->vconf.fps;
    return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg) {
    EncData *d = (EncData*) f->data;
    *(MSVideoSize*) arg = d->vconf.vsize;
    return 0;
}

static int enc_set_vsize(MSFilter *f, void *arg) {
    MSVideoConfiguration best_vconf;
    EncData *d = (EncData *) f->data;
    MSVideoSize *vs = (MSVideoSize *) arg;
    best_vconf = ms_video_find_best_configuration_for_size(d->vconf_list, *vs,ms_get_cpu_count());
    d->vconf.vsize = *vs;
    d->vconf.fps = best_vconf.fps;
    d->vconf.bitrate_limit = best_vconf.bitrate_limit;
    enc_set_configuration(f, &d->vconf);
    return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg) {
    EncData *d = (EncData*) f->data;
    const char *fmtp = (const char *) arg;
    char value[12];
    if (fmtp_get_value(fmtp, "packetization-mode", value, sizeof(value))) {
        d->mode = atoi(value);
        ms_message("packetization-mode set to %i", d->mode);
    }
    return 0;
}

static int enc_req_vfu(MSFilter *f, void *arg) {
    EncData *d = (EncData*) f->data;
    d->generate_keyframe = TRUE;
    return 0;
}

static int enc_get_configuration_list(MSFilter *f, void *data) {
    EncData *d = (EncData *) f->data;
    const MSVideoConfiguration **vconf_list =
            (const MSVideoConfiguration **) data;
    *vconf_list = d->vconf_list;
    return 0;
}

static MSFilterMethod enc_methods[] = { { MS_FILTER_SET_FPS, enc_set_fps }, {
                                            MS_FILTER_SET_BITRATE, enc_set_br }, { MS_FILTER_GET_BITRATE, enc_get_br }, {
                                            MS_FILTER_GET_FPS, enc_get_fps }, { MS_FILTER_GET_VIDEO_SIZE, enc_get_vsize }, {
                                            MS_FILTER_SET_VIDEO_SIZE, enc_set_vsize }, {
                                            MS_FILTER_ADD_FMTP, enc_add_fmtp }, { MS_FILTER_REQ_VFU, enc_req_vfu }, {
                                            MS_VIDEO_ENCODER_REQ_VFU, enc_req_vfu }, {
                                            MS_VIDEO_ENCODER_GET_CONFIGURATION_LIST, enc_get_configuration_list }, {
                                            MS_VIDEO_ENCODER_SET_CONFIGURATION, enc_set_configuration }, { 0, NULL } };

#ifndef _MSC_VER

static MSFilterDesc x264_enc_desc = {
    .id = MS_FILTER_PLUGIN_ID,
    .name =
    "MSX264Enc",
    .text = "A H264 encoder based on s3c6410 MFC project",
    .category = MS_FILTER_ENCODER,
    .enc_fmt = "H264",
    .ninputs = 1,
    .noutputs = 1,
    .init = enc_init,
    .preprocess = enc_preprocess,
    .process = enc_process,
    .postprocess = enc_postprocess,
    .uninit =enc_uninit,
    .methods = enc_methods };

#else

static MSFilterDesc x264_enc_desc= {
    MS_FILTER_PLUGIN_ID,
    "MSX264Enc",
    "A H264 encoder based on s3c6410 MFC project",
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

MS2_PUBLIC void libmsx264_init(void) {
    ms_filter_register(&x264_enc_desc);
    ms_message("msx264-" VERSION " plugin registered.");
}

