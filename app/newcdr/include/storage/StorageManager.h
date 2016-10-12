#ifndef _STORAGE_MANAGER_H
#define _STORAGE_MANAGER_H

#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <fcntl.h>

#include <utils/String8.h>
#include <StorageMonitor.h>
#include <DBCtrl.h>
#include <private/media/VideoFrame.h>

#include <minigui/common.h>
#include <minigui/gdi.h>

#include "Process.h"
#include "misc.h"
#include "cdr_res.h"
#include "camera.h"

#include "Rtc.h"
#include "EventManager.h"

using namespace android;

long long totalSize(const char *path);
long long availSize(const char *path);
long long totalDirSize(const char *path);

#define MOUNT_POINT "/mnt/external_sd"
#define MOUNT_PATH MOUNT_POINT"/"
#define SZ_1M ((unsigned long long)(1<<20))
#define SZ_1G ((unsigned long long)(1<<30))
//#define RESERVED_SIZE (SZ_1M * 300)
#define RESERVED_SIZE ((unsigned long long)((SZ_1M*1000)/SZ_1M))
#define LOOP_COVERAGE_SIZE (RESERVED_SIZE + ((SZ_1M*300)/SZ_1M))

#define FILE_SZ_1M ((uint32_t)(1*1024*1024))

#define CSI_PREALLOC_SIZE ((uint32_t)(FILE_SZ_1M * 100))
#define UVC_PREALLOC_SIZE ((uint32_t)(FILE_SZ_1M * 200))
#define AWMD_PREALLOC_SIZE ((uint32_t)(FILE_SZ_1M * 100))


#define CDR_TABLE "CdrFile"

#define FILE_EXSIST(PATH)	(!access(PATH, F_OK))

#define FLAG_NONE ""
#define FLAG_SOS "_SOS"

#define FILE_FRONT "A"
#define FILE_BACK "B"

#define EXT_VIDEO_MP4 ".mp4"
#define EXT_PIC_JPG ".jpg"
#define EXT_PIC_BMP ".bmp"

enum {
	RET_IO_OK = 0,
	RET_IO_NO_RECORD = -150,
	RET_IO_DB_READ = -151,
	RET_IO_DEL_FILE = -152,
	RET_IO_OPEN_FILE = -153,
	RET_NOT_MOUNT = -154,
	RET_DISK_FULL = -155,
	RET_NEED_LOOP_COVERAGE = -156
};

typedef struct elem
{
	char *file;
	long unsigned int time;
	char *type;
	char *info;
	char *dir;
}Elem;

typedef struct {
	String8 file;
	int time;
	String8 type;
	String8 info;
	String8 dir;
}dbRecord_t;

typedef struct file_threshold
{
        char dir[32];
        char type[32];
        int  percentum;
}file_threshold_t;

class CommonListener
{
public:
	CommonListener(){};
	virtual ~CommonListener(){}
	virtual void finish(int what, int extra) = 0;
};


class StorageManager : public StorageMonitorListener
{
private:
	void init();
	StorageManager();
	~StorageManager();
	StorageManager& operator=(const StorageManager& o);
public:
	static StorageManager* getInstance();
	void exit();
	void notify(int what, int status, char *msg);
	bool isInsert();
	bool isMount();
	int doMount(const char *fsPath, const char *mountPoint,
	             bool ro, bool remount, bool executable,
	             int ownerUid, int ownerGid, int permMask, bool createLost);
	int format(CommonListener *listener);
	int reNameFile(const char *oldpath, const char *newpath);
	void dbInit();
	void dbExit();
	void dbReset();
	int dbCount(char *item);
	void dbGetFiles(char *item, Vector<String8>&file);
	void dbGetFile(char *item, String8 &file );
	int dbGetSortByTimeRecord(dbRecord_t &record, int index);
	void dbAddFile(Elem *elem);
	void dbAddFileByTransaction(Elem *elem);
	void dbDelFile(char const *file);
	int generateFile(time_t &now, String8 &file, int cam_type, fileType_t type);
	int generateFile2(time_t now, String8 &file, int cam_type, fileType_t type);
	int saveVideoFrameToBmpFile(VideoFrame* videoFrame, const char* bmpFile);
	int savePicture(void* data, int size, int fd);
	int checkDirs();
	int dbUpdate();
	int dbUpdate(char *type);
	int deleteFile(const char *file);
	bool dbIsFileExist(String8 file);
	int shareVol();
	int unShareVol();
	bool isDiskFull();
	int fileSize(String8 path);
	void dbUpdateFile(const char* oldname, const char *newname);
	String8 lockFile(const char *path);
	class FormatThread : public Thread
	{
	public:
		virtual bool threadLoop() {
			mSM->format();
			mSM->dbReset();
			mListener->finish(0, 0);
			return false;
		}
		status_t start() {
			return run("FormatThread", PRIORITY_BACKGROUND);
		}
		FormatThread(StorageManager *sm, CommonListener *listener) {mSM = sm; mListener = listener;};
	private:
		StorageManager *mSM;
		CommonListener *mListener;
	};
        class DbUpdateThread : public Thread
	{
	public:
		virtual bool threadLoop() {
			mSM->dbReset();
			return false;
		}
		status_t start() {
			return run("DbUpdateThread", PRIORITY_BACKGROUND);
		}
                void stopThread()
                {
                        requestExitAndWait();
                }

		DbUpdateThread(StorageManager *sm) {mSM = sm;};
	private:
		StorageManager *mSM;
	};
        class FileMgrThread : public Thread
	{
	public:
		virtual bool threadLoop() {
			mSM->deleteFilesByThread();
			return false;
		}
		status_t start() {
			return run("FileMgrThread", PRIORITY_BACKGROUND);
		}
                void stopThread()
                {
                        requestExitAndWait();
                }

		FileMgrThread(StorageManager *sm) {mSM = sm;};
	private:
		StorageManager *mSM;
	};

	void setEventListener(EventListener *pListener);
private:
	int isInsert(const char *checkPath);
	int isMount(const char *checkPath);
	int deleteFiles(const char *type, const char *info,const char *path);
	int deleteFilesUntilSize(const char *type, const char *info,const char *path,unsigned long long sizeMbytes);
	int deleteFiles(file_threshold_t *threashold_file);
	int deleteFilesByThread();
	int format();
	StorageMonitor *mStorageMonitor;
	DBCtrl *mDBC;
	Mutex mDBLock;
	sp <FormatThread> mFT;
	sp <DbUpdateThread> mUpdateT;
	sp <FileMgrThread> mFileMgrT;
	Vector<file_threshold_t *> thresholds;
	Mutex filelock;
	bool mInsert;
	bool mMounted;
	bool mDbUpdated;
	bool mForceSkipDb;
	EventListener *mListener;
};
#endif
