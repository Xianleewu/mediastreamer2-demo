/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SCREENRECORD_SCREENRECORD_H
#define SCREENRECORD_SCREENRECORD_H

namespace android{

#define kVersionMajor 1
#define kVersionMinor 2

struct ScreenFrame {
  int    width;         // in number of pixels
  int    height;        // in number of pixels
  uint32_t fourcc;        // compression
  uint32_t pixel_width;   // width of a pixel, default is 1
  uint32_t pixel_height;  // height of a pixel, default is 1
  int64_t  elapsed_time;  // elapsed time since the creation of the frame
                        // source (that is, the camera), in nanoseconds.
  int64_t  time_stamp;    // timestamp of when the frame was captured, in unix
                        // time with nanosecond units.
  uint32_t data_size;     // number of bytes of the frame data

  int    rotation;      // rotation in degrees of the frame (0, 90, 180, 270)

  void*  data;          // pointer to the frame data. This object allocates the
};

class ScreenRecordCallback {
public:
        virtual void OnFrame(struct ScreenFrame &frame) = 0;
        virtual ~ScreenRecordCallback(){}
};
class MsgCallback {
public:
        virtual void OnMsg(unsigned char *buf, int length) = 0;
        virtual ~MsgCallback(){}
};

bool RegisterMsgCallback(MsgCallback *callback);
int RemoteCall(unsigned char *buf, int length);
bool StartScreenRecordThread(int width, int height, ScreenRecordCallback *callback);
bool StopScreenRecordThread();
bool IsScreenRecordThreadRunning();
bool JoinRecordThread();
bool QueryScreenFormat(int *width, int *height, int *mFps, int *fourcc);
}
#endif /*SCREENRECORD_SCREENRECORD_H*/
