#ifndef _CAMERA_H
#define _CAMERA_H

#define CAM_CNT 2
enum{
	CAM_CSI = 0,
	CAM_UVC,
	CAM_TVD
};

typedef enum {
	ImageQualityQQVGA = 0, /*160---120*/
	ImageQualityQVGA,	   /*320---240*/
	ImageQualityVGA,	   /*640---480*/
	ImageQuality720P,	   /*1280--720*/
	ImageQuality1080P,	   /*1920-1080*/
	/*unsupported below*/
	ImageQuality5M,	       /*2560-1920*/
	ImageQuality8M,	       /*3264-2448*/
}ImageQuality_t;


#define FRONT_CAM_FRAMERATE 27
#define BACK_CAM_FRAMERATE 20

#define CAM_CSI_BITRATE (10*1024*1024)
#define CAM_UVC_BITRATE (4*1024*1024)
/*
const char WHITE_BALANCE_WARM_FLUORESCENT[] = "warm-fluorescent";
const char WHITE_BALANCE_TWILIGHT[] = "twilight";
const char WHITE_BALANCE_SHADE[] = "shade";
*/
#endif
