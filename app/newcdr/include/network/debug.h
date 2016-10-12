#ifndef __DEBUG_H
#define __DEBUG_H

#define DBG	true
#define TAG	"[NETWORK]"
#define LOGE(...) do { __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__); \
		} while(0)
#define LOGD(...) do { if (DBG) \
			__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__); \
		} while(0)

#endif
