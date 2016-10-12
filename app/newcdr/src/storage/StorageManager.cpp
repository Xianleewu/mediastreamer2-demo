#include "StorageManager.h"
#undef LOG_TAG
#define LOG_TAG "StorageManager"
#include "debug.h"

#define BEGIN_TRANSACTION "begin transaction"
#define COMMIT_TRANSACTION "commit transaction"

static StorageManager* gInstance;
static Mutex gInstanceMutex;
static Mutex mDBInitLock;

static DBC_TYPE gDBType[] = {
	DB_TEXT,
	DB_UINT32, 
	DB_TEXT,
	DB_TEXT,
};

static struct sql_tb gSqlTable[] = {
	{"file", "VARCHAR(128)"},
	{"time", "INTEGER"},
	{"type", "VARCHAR(32)"},		/* "video" "pic" */
	{"info", "VARCHAR(32)"},		/* "normal"  */
	{"dir", "VARCHAR(32)"},		        /* "video" "photo"  */
};

static file_threshold_t video_f = 
        {"video",TYPE_VIDEO,0};

static file_threshold_t video_r = 
        {"video_r",TYPE_VIDEO,0};

static file_threshold_t video_f_related = 
        {"video_f_rlt",TYPE_VIDEO,20};

static file_threshold_t photo_f = 
        {"photo",TYPE_PIC,10};

/*每种文件删除时，要把对应的缩略图文件同时删除*/
static file_threshold_t fileThreshods[] = {
        video_f, /*前camera1080p循环录制*/
        video_r,/*后camera循环录制*/
        video_f_related,/*前camera关联视频*/
        photo_f,/*前camera 1080p抓拍*/
        
};
void StorageManager::init()
{
	mStorageMonitor = new StorageMonitor;
	mStorageMonitor->init();
	mStorageMonitor->setMonitorListener(this);
	if(isMount(MOUNT_POINT)) {
		mMounted = true;
		checkDirs();
	}
	else
		mMounted = false;
	dbInit();
	db_info("%s is %s\n", MOUNT_PATH, (mMounted == true) ? "mounted" : "not mount");
}

void StorageManager::exit()
{
	delete mStorageMonitor;
	mStorageMonitor = NULL;
}

int StorageManager::shareVol()
{
	if (mStorageMonitor) {
		return mStorageMonitor->mountToPC();
	}
	return -1;
}
int StorageManager::unShareVol()
{
	if (mStorageMonitor) {
		return mStorageMonitor->unmountFromPC();
	}
	return -1;
}
StorageManager::StorageManager()
	: mStorageMonitor(NULL)
	, mDBC(NULL)
#ifdef EVENTMANAGER_READY
	, mListener(NULL)
#endif
{
	init();
}

StorageManager::~StorageManager()
{
	exit();
}
StorageManager* StorageManager::getInstance(void)
{
	db_msg("getInstance\n");
	Mutex::Autolock _l(gInstanceMutex);
	if(gInstance != NULL) {
		return gInstance;
	}
	gInstance = new StorageManager();
	return gInstance;
}

void StorageManager::notify(int what, int status, char *msg)
{
	switch(status){
	case STORAGE_STATUS_UNMOUNTING:
        case STORAGE_STATUS_REMOVE:
		{
			db_msg("~~unmounting");
			//dbUpdate();
			mMounted = false;
			if(mListener)
				mListener->notify(EVENT_MOUNT, 0);

			if(mUpdateT != NULL)
			{
				mUpdateT->stopThread();
				mUpdateT.clear();
				mUpdateT = NULL;
			}
			break;
		}		
	case STORAGE_STATUS_MOUNTED:
		{
			db_msg("~~mounted");
			mMounted = true;
			checkDirs();
			//dbUpdate();
			mUpdateT = new DbUpdateThread(this);
			status_t err = mUpdateT->start();
			if (err != OK) {
					mUpdateT.clear();
			}
                        //dbReset();
			if(mListener)
				mListener->notify(EVENT_MOUNT, 1);
			break;
		}
	}
}
int StorageManager::isInsert(const char *checkPath)
{
	FILE *fp;
	db_msg("------------checkPath = %s \n", checkPath);
	if (!(fp = fopen(checkPath, "r"))) {
		db_msg(" isInsert fopen fail, checkPath = %s\n", checkPath);
		return 0;
	}
	if(fp){
		fclose(fp);
	}
	return 1;
}

int StorageManager::isMount(const char *checkPath)
{
	const char *path = "/proc/mounts";
	FILE *fp;
	char line[255];
	const char *delim = " \t";
	ALOGV(" isMount checkPath=%s \n",checkPath);
	if (!(fp = fopen(path, "r"))) {
		ALOGV(" isMount fopen fail,path=%s\n",path);
		return 0;
	}
	memset(line,'\0',sizeof(line));
	while(fgets(line, sizeof(line), fp))
	{
		char *save_ptr = NULL;
		char *p        = NULL;
		//ALOGV(" isMount line=%s \n",line);
		if (line[0] != '/' || strncmp("/dev/block/vold",line,strlen("/dev/block/vold")) != 0)
		{
			memset(line,'\0',sizeof(line));
			continue;
		}       
		if ((p = strtok_r(line, delim, &save_ptr)) != NULL) {		    			
			if ((p = strtok_r(NULL, delim, &save_ptr)) != NULL){
				ALOGV(" isMount p=%s \n",p);
				if(strncmp(checkPath,p,strlen(checkPath) ) == 0){
					fclose(fp);
					ALOGV(" isMount return 1\n");
					return 1;				
				}				
			}					
		}		
	}//end while(fgets(line, sizeof(line), fp))	
	if(fp){
		fclose(fp);
	}
	return 0;
}

bool StorageManager::isInsert(void)
{
	mInsert = mStorageMonitor->isInsert();
	db_msg("------------mInsert = %d \n", mInsert);
	return mInsert;
}
bool StorageManager::isMount(void)
{
	//mMounted = mStorageMonitor->isMount();
	db_msg("------------mMounted = %d \n", mMounted);
	return mMounted;
}

String8 StorageManager::lockFile(const char *path)
{
	String8 file(path);
	// /mnt/external_sd/video/time_A.mp4 -> /mnt/external_sd/video/time_A_SOS.mp4
	file = file.getBasePath();
	file += FLAG_SOS;
	file += EXT_VIDEO_MP4;
	db_msg("lockFile:%s --> %s", path, file.string());
	reNameFile(path, file.string());
	return file;
}

int StorageManager::reNameFile(const char *oldpath, const char *newpath)
{
	int ret;

	ret = rename(oldpath,newpath);
	if(ret != 0){
		db_error("reNameFile fail oldpath=%s, newpath = %s,ret=%d\n",oldpath,newpath,ret);
	}

	return ret;
}

int needDeleteFiles()
{
	unsigned long long as = availSize(MOUNT_PATH);
	unsigned long long ts = totalSize(MOUNT_PATH);
	db_msg("[Size]:%lluMB/%lluMB, reserved:%lluMB", as, ts, RESERVED_SIZE);

	if (as <= RESERVED_SIZE) {
		db_msg("!Disk FULL");
		return RET_DISK_FULL;
	}
	
	if (as <= LOOP_COVERAGE_SIZE) {
		db_msg("!Loop Coverage,%lluMB,LOOP_COVERAGE_SIZE:%lluMB",as,LOOP_COVERAGE_SIZE);
		return RET_NEED_LOOP_COVERAGE;
	}
	
	return 0;
}

int needDeleteFiles(file_threshold_t* threashold_file )
{
	unsigned long long as = availSize(MOUNT_PATH);
	unsigned long long ts = totalSize(MOUNT_PATH);
        unsigned long long dirs = 0;
        char path[128]= {0};
	db_msg("[Size]:%lluMB/%lluMB, reserved:%lluMB", as, ts, RESERVED_SIZE);
        if(threashold_file->percentum == 0){

        
	    if (as <= RESERVED_SIZE) {
		db_msg("!Disk FULL");
		return RET_DISK_FULL;
	    }
	
	    if (as <= LOOP_COVERAGE_SIZE) {
		db_msg("!Loop Coverage,%lluMB,LOOP_COVERAGE_SIZE:%lluMB",as,LOOP_COVERAGE_SIZE);
		return RET_NEED_LOOP_COVERAGE;
	    }
        }else{
            sprintf(path,"%s%s/",MOUNT_PATH,threashold_file->dir);
            dirs = totalDirSize(path);
            if (as <= RESERVED_SIZE) {
		db_msg("!Disk FULL");
		return RET_DISK_FULL;
	    }
	
	    if( dirs >= (ts*threashold_file->percentum/100)) {
		db_msg("!Loop Coverage,totalSize");
		return RET_NEED_LOOP_COVERAGE;
	    }
            
        }
	
	return 0;
}


int StorageManager::deleteFile(const char *file)
{
	int ret = 0;
	String8 path(file);
	if (FILE_EXSIST(file)) {
		if (remove(file) != 0) {
			ret = -1;
			db_msg("remove file failed: %s", file);
		}else {
			db_msg("removed file: %s", file);
		}
	}else {
		db_msg("file not existed:%s", file);
	}
	
	/* delete thumbnail */
	if (path.getPathExtension() == EXT_VIDEO_MP4 ||
		path.getPathExtension() == EXT_PIC_JPG) {
		String8 stime;
		stime = path.getBasePath().getPathLeaf();
		path = (path.getPathDir())+"/"+THUMBNAIL_DIR+stime+EXT_PIC_BMP;// /mnt/external_sd/video/time_A.mp4 ->/mnt/external_sd/video->/mnt/external_sd/video/.thumb/time_A.bmp
		db_msg("delete pic :%s", path.string());
		return deleteFile(path.string());
	}
	return ret;
}
int StorageManager::deleteFilesUntilSize(const char *type, const char *info,const char *path,unsigned long long sizeMbytes)
{

	Mutex::Autolock _l(mDBLock);

	void *ptr;

	int count = 0;
	int ret = 0;
	String8 filter;
	String8 filter_type("WHERE TYPE ='");
	String8 tmp;
	String8 time;
	char absPath[128] = {0};

	sprintf(absPath,"%s%s/",MOUNT_PATH,path);
	db_debug("%s delete files in /mnt/external_sd/%s,type(%s)",__func__, path,type);
	if (strcmp(type, TYPE_PIC) == 0) {
		filter_type += type;
                if(path != NULL){
		    filter_type += "' AND dir ='";
		    filter_type += path;
                }
		filter_type += "'";
	} else {
		filter_type += type;
                if(path != NULL){
		    filter_type += "' AND dir ='";
		    filter_type += path;
                }
		filter_type += "' AND info ='";
		filter_type += info;
		filter_type += "'";
	}
	db_debug("%s %d, %s\n", __FUNCTION__, __LINE__, filter_type.string());
	mDBC->setFilter(filter_type);

	count = mDBC->getRecordCnt();
	db_debug("%s recordcnt:%d\n", __FUNCTION__, count);
	if (count <= 0) {
		/* check datebase */
		system("ls -l /data");
		system("df /data");
		return RET_IO_NO_RECORD;
	}

	mDBC->bufPrepare();
	do {
		if ((count--) == 0) {
			return RET_IO_NO_RECORD;
		}
		system("ls -l /mnt/external_sd/video");
		filter = filter_type;
		filter += " ORDER BY time ASC";
		mDBC->setFilter(filter);
		tmp = "file";
		ptr = mDBC->getFilterFirstElement(tmp, 0);
		if (!ptr) {
			db_debug("fail to read db");
			return RET_IO_DB_READ;
		}
		ret = deleteFile((const char*)ptr);
		if (ret < 0) {
			db_debug("fail to delete file %s", (const char *)ptr);
			return RET_IO_DEL_FILE;
		}
		db_debug("%s has been deleted\n", (char *)ptr);
		tmp = String8::format("DELETE FROM %s WHERE file='%s'", mDBC->getTableName().string(), (char *)ptr);
		db_debug("execute sql %s", tmp.string());
		mDBC->setSQL(tmp);
		mDBC->executeSQL();

		unsigned long long dirSize  = totalDirSize(absPath);
		db_debug("(%s) dirSize(%lluMB),most space(%lluMB)",absPath,dirSize,sizeMbytes);
		if (dirSize < (sizeMbytes)) {
                        db_debug(" dir has more space, break");
			break;
		}
                if (dirSize == 0) {
                        db_debug(" dir size is 0, break");
			break;
		}
	} while(1);
	mDBC->bufRelease();

	return RET_IO_OK;
}
int StorageManager::deleteFiles(const char *type, const char *info,const char *path)
{

	Mutex::Autolock _l(mDBLock);

	void *ptr;

	int count = 0;
	int ret = 0;
	String8 filter;
	String8 filter_type("WHERE TYPE ='");
	String8 tmp;
	String8 time;

	db_debug("%s delete files in /mnt/external_sd/%s,type(%s)",__func__, path,type);
	if (strcmp(type, TYPE_PIC) == 0) {
		filter_type += type;
                if(path != NULL){
                    if(strcmp(path,video_f.dir) == 0 || strcmp(path,video_r.dir) == 0){
			    filter_type += "' AND (dir ='";
			    filter_type += video_f.dir;
			    filter_type += "' OR dir ='";
			    filter_type += video_r.dir;
			    filter_type += "')";

                    }else{
			    filter_type += "' AND dir ='";
			    filter_type += path;
			    filter_type += "'";
                    }
                }
	} else {
		filter_type += type;
                if(path != NULL){
                    if(strcmp(path,video_f.dir) == 0 || strcmp(path,video_r.dir) == 0){
			    filter_type += "' AND (dir ='";
			    filter_type += video_f.dir;
			    filter_type += "' OR dir ='";
			    filter_type += video_r.dir;
			    filter_type += "')";



                    }else{
			    filter_type += "' AND dir ='";
			    filter_type += path;
			    filter_type += "'";
                    }
                }
		filter_type += " AND info ='";
		filter_type += info;
		filter_type += "'";
	}
	db_debug("%s %d, %s\n", __FUNCTION__, __LINE__, filter_type.string());
	mDBC->setFilter(filter_type);

	count = mDBC->getRecordCnt();
	db_debug("%s recordcnt:%d\n", __FUNCTION__, count);
	if (count <= 0) {
		/* check datebase */
		system("ls -l /data");
		system("df /data");
		return RET_IO_NO_RECORD;
	}

	mDBC->bufPrepare();
	do {
		if ((count--) == 0) {
			db_debug("return RET_IO_NO_RECORD");
			return RET_IO_NO_RECORD;
		}
		system("ls -l /mnt/external_sd/video");
		filter = filter_type;
		filter += " ORDER BY time ASC";
		mDBC->setFilter(filter);
		tmp = "file";
		ptr = mDBC->getFilterFirstElement(tmp, 0);
		if (!ptr) {
			db_debug("fail to read db");
			return RET_IO_DB_READ;
		}
		ret = deleteFile((const char*)ptr);
		if (ret < 0) {
			db_debug("fail to delete file %s", (const char *)ptr);
			return RET_IO_DEL_FILE;
		}
		db_debug("%s has been deleted\n", (char *)ptr);
		tmp = String8::format("DELETE FROM %s WHERE file='%s'", mDBC->getTableName().string(), (char *)ptr);
		db_debug("execute sql %s", tmp.string());
		mDBC->setSQL(tmp);
		mDBC->executeSQL();
		unsigned long long as  = availSize(MOUNT_PATH);
		db_debug("as size %lld", as);
		if (as == 0) {
			db_debug("as = 0 ,break!");
			break;
		}
		if (as > RESERVED_SIZE) {
			db_debug("as more than reserved size, return!");
			break;
		}
	} while(1);
	mDBC->bufRelease();

	return RET_IO_OK;
}

int StorageManager::deleteFilesByThread()
{
	int ret = 0;
	while(1)
	{
		if(thresholds.isEmpty() || mDbUpdated == false){
			usleep(1*1000);
			continue;
		}
		filelock.lock();
		file_threshold_t *threashold_file = thresholds.itemAt(0);
		thresholds.removeAt(0);
		filelock.unlock();
		if((ret = needDeleteFiles(threashold_file))) {
			deleteFiles(threashold_file);
		}
	}

	return ret;
}
int StorageManager::deleteFiles(file_threshold_t *threashold_file)
{
	int ret = 0;
        char *path = threashold_file->dir;
        if(threashold_file->percentum == 0){
            ret = deleteFiles(TYPE_VIDEO, INFO_NORMAL,path);
	    if (ret != RET_IO_OK) {
		    ret = deleteFiles(TYPE_VIDEO, INFO_LOCK,path);
	    }
            ret = deleteFiles(TYPE_PIC, INFO_NORMAL,path);

        }else{
            unsigned long long ts = totalSize(MOUNT_PATH);
            unsigned long long dirNeedSize = ts*threashold_file->percentum/100;
            ret = deleteFilesUntilSize(TYPE_VIDEO, INFO_NORMAL,path,dirNeedSize);
            if (ret != RET_IO_OK) {
		    ret = deleteFilesUntilSize(TYPE_VIDEO, INFO_LOCK,path,dirNeedSize);
	    }
            ret = deleteFilesUntilSize(TYPE_PIC, INFO_NORMAL,path,dirNeedSize);
            
        }
	if(needDeleteFiles() != 0){
		ret = deleteFiles(TYPE_VIDEO, INFO_NORMAL,video_f.dir);
	        if (ret != RET_IO_OK) {
		    ret = deleteFiles(TYPE_VIDEO, INFO_LOCK,video_f.dir);
	        }
                ret = deleteFiles(TYPE_PIC, INFO_NORMAL,video_f.dir);


         }
        
	return ret;
}

file_threshold_t* findThreshold(int cam_type, fileType_t type)
{

        file_threshold_t *threashold_file = &video_f;
        if(type == VIDEO_TYPE_NORMAL || type == VIDEO_TYPE_IMPACT ){
            if(cam_type == CAM_UVC){
                threashold_file = &video_r;
            }

        }else if(type == PHOTO_TYPE_NORMAL){
            threashold_file = &photo_f;

        }else if(type == VIDEO_TYPE_RELATED){
            threashold_file = &video_f_related;

        }

        return threashold_file;

}
int StorageManager::generateFile(time_t &now, String8 &file, int cam_type, fileType_t type)
{
	
	int fd;
	int ret;
	struct tm *tm=NULL;
	char buf[128]= {0};
	char path_temp[128]= {0};
	const char *camType = FILE_FRONT;
	const char *suffix = FLAG_NONE;
	const char *fileType = EXT_VIDEO_MP4;
	const char *path = CON_2STRING(MOUNT_PATH, VIDEO_DIR);
        file_threshold_t *threshlod_file = findThreshold(cam_type,type);
        sprintf(path_temp,"%s%s/",MOUNT_PATH,threshlod_file->dir);
        path = path_temp;

	if(isMount()== false)
		return RET_NOT_MOUNT;
	db_msg("%s %d", __FUNCTION__, __LINE__);
	//if((ret = needDeleteFiles(threshlod_file))) {
		//if((type == PHOTO_TYPE_NORMAL) && (ret == RET_DISK_FULL))
		//	return RET_DISK_FULL;
		//else {
		//	ret = deleteFiles(threshlod_file);

		//	if(ret != RET_IO_OK && ret != RET_IO_NO_RECORD) {
		//		db_error("deleteFiles ret: RET_IO:%d\n", ret);
		//		return ret;
		//	}
		//}
			filelock.lock();
			while(mFileMgrT == NULL)
			{
				mFileMgrT = new FileMgrThread(this);
				status_t err = mFileMgrT->start();
				if (err != OK) {
						mFileMgrT.clear();
						mFileMgrT = NULL;
						db_msg("---FileMgrThread init error!!!!!");
						//return err;
				}
			}
			thresholds.push_back(threshlod_file);
			filelock.unlock();
			
	//}

	//db_msg("%s %d", __FUNCTION__, __LINE__);
	now = getDateTime(&tm);
	//db_msg("%s %d", __FUNCTION__, __LINE__);
	if (type == VIDEO_TYPE_IMPACT) {
		suffix = FLAG_SOS;
//		path = CON_2STRING(MOUNT_PATH, LOCK_DIR);
	}
	if (type == PHOTO_TYPE_NORMAL) {
		fileType = EXT_PIC_JPG;
		//path = CON_2STRING(MOUNT_PATH, PIC_DIR);
	}
	if (cam_type == CAM_UVC) {
		camType = FILE_BACK;
	}
	//db_msg("%s %d", __FUNCTION__, __LINE__);
	sprintf(buf, "%s%04d%02d%02d_%02d%02d%02d%s%s%s",path,tm->tm_year + 1900, tm->tm_mon + 1,
		tm->tm_mday, tm->tm_hour,tm->tm_min, tm->tm_sec, camType, suffix, fileType);
	//db_msg("%s %d", __FUNCTION__, __LINE__);
	file = buf;
	fd = open(buf, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		db_msg("failed to open file '%s'!!\n", buf);
		return RET_IO_OPEN_FILE;
	}
	db_debug("generate file %s", file.string());
	return fd;
}

int StorageManager::generateFile2(time_t now, String8 &file, int cam_type, fileType_t type)
{

    int fd;
    struct tm *tm=NULL;
    char buf[128]= {0};
    char path_temp[128]= {0};
    const char *camType = FILE_FRONT;
    const char *suffix = FLAG_NONE;
    const char *fileType = EXT_VIDEO_MP4;
    const char *path = CON_2STRING(MOUNT_PATH, VIDEO_DIR);

    file_threshold_t *threshlod_file = findThreshold(cam_type,type);
    sprintf(path_temp,"%s%s/%s",MOUNT_PATH,threshlod_file->dir,THUMBNAIL_DIR);
    path = path_temp;

    if(isMount()== false)
        return RET_NOT_MOUNT;
    db_msg("%s %d", __FUNCTION__, __LINE__);

    filelock.lock();
    while(mFileMgrT == NULL)
    {
        mFileMgrT = new FileMgrThread(this);
        status_t err = mFileMgrT->start();
        if (err != OK) {
            mFileMgrT.clear();
            mFileMgrT = NULL;
            db_msg("---FileMgrThread init error!!!!!");
        }
    }
    thresholds.push_back(threshlod_file);
    filelock.unlock();

    tm = localtime(&now);

    if (type == VIDEO_TYPE_IMPACT) {
        suffix = FLAG_SOS;
    }
    if (type == PHOTO_TYPE_NORMAL) {
        fileType = EXT_PIC_JPG;
    }
    if (cam_type == CAM_UVC) {
        camType = FILE_BACK;
    }
    sprintf(buf, "%s%04d%02d%02d_%02d%02d%02d%s%s%s",path,tm->tm_year + 1900, tm->tm_mon + 1,
            tm->tm_mday, tm->tm_hour,tm->tm_min, tm->tm_sec, camType, suffix, fileType);
    file = buf;
    fd = open(buf, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        db_msg("failed to open file '%s'!!\n", buf);
        return RET_IO_OPEN_FILE;
    }
    db_debug("generate file %s", file.string());
    return fd;
}

int StorageManager::doMount(const char *fsPath, const char *mountPoint,
		bool ro, bool remount, bool executable,
		int ownerUid, int ownerGid, int permMask, bool createLost) {
	int rc;
	unsigned long flags;
	char mountData[255];
	flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC;
	flags |= (executable ? 0 : MS_NOEXEC);
	flags |= (ro ? MS_RDONLY : 0);
	flags |= (remount ? MS_REMOUNT : 0);

	sprintf(mountData,"utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
			ownerUid, ownerGid, permMask, permMask); 
	rc = mount(fsPath, mountPoint, "vfat", flags, mountData);
	if (rc && errno == EROFS) {
		db_error("%s mount fail try again", fsPath);
		rc = mount(fsPath, mountPoint, "vfat", flags, mountData);
		db_error(" mount rc=%d", rc);
	}

	if (rc == 0 && createLost) {
		char *lost_path;
		asprintf(&lost_path, "%s/LOST.DIR", mountPoint);
		if (access(lost_path, F_OK)) {
			if (mkdir(lost_path, 0755)) {
				db_error("Unable to create LOST.DIR (%s)", strerror(errno));
			}
		}
		free(lost_path);
	}
	return rc;
}

int StorageManager::format(CommonListener *listener)
{

#if 0
	mFT = new FormatThread(this, listener);
	status_t err = mFT->start();
	if (err != OK) {
		mFT.clear();
	}
#else
	mForceSkipDb = true;
	while(!mDbUpdated){
        
		usleep(100);
		ALOGD("mDbUpdating usleep 100ms!!");
	}
	mForceSkipDb = false;
	format();
	//dbReset();
//	listener->finish(0, 0);	
#endif
	return 0;
}

int StorageManager::format()
{
	int ret = -1;
/*	db_msg("format start\n");
	if(isMount()== true) {
		if((ret = umount(MOUNT_PATH)) != 0)
		{
			usleep(500000);
			ALOGV("format umount ret=%d,try again\n",ret);
			Process::killProcessesWithOpenFiles(MOUNT_PATH, 2);
			if((ret = umount(MOUNT_PATH)) != 0){
				ALOGV("format umount again fail ret=%d\n",ret);
				return -1;
			}		
		}	
	}
    
	ret = system("newfs_msdos -F 32 -O android -b 65536 -c 128 /dev/block/vold/179:0");
	if(ret!= 0)
	{
		db_debug("***system format erro ret =%d\n",ret);
		return -1;
	}
	ret = doMount("/dev/block/vold/179:0", MOUNT_PATH, false, false, true, 0, 0, 0, true);
	if(ret != 0){
		ALOGV("__doMount erro ret=%d,error=%s\n",ret,strerror(errno));
		return -1;
	}
	db_msg("format end ret=%d\n",ret);
	*/
	if (mStorageMonitor) {
		return mStorageMonitor->formatSDcard();
    }
	checkDirs();
	dbReset();
	return ret;
}

long long totalSize(const char *path)
{
	struct statfs diskinfo;
	long long totalSize = 0;
	if(statfs(path, &diskinfo) != -1){
		totalSize = diskinfo.f_blocks * diskinfo.f_bsize;
		return totalSize >> 20; //MB
	}    	
	return 0;
}

long long availSize(const char *path)
{
	struct statfs diskinfo;
	long long availSize = 0;
	if(statfs(path, &diskinfo) != -1){
		availSize = diskinfo.f_bfree * diskinfo.f_bsize;
		return availSize >> 20; //MB
	}    	
	return 0;
}

long long totalDirSize(const char *dir)
{
        DIR *dp;
        struct dirent *entry;
        struct stat statbuf;
        long long totalSize = 0;
        char* chCurPath;

        if ((dp = opendir(dir)) == NULL)
        {
            fprintf(stderr, "Cannot open dir: %s\n", dir);
            exit(0);
        }
        chCurPath = getcwd(NULL, 0);
        //db_msg("current work path: %s\n", chCurPath);

        chdir(dir);
        //db_msg("current work path changed to: %s\n", dir);
        while ((entry = readdir(dp)) != NULL)
        {
            lstat(entry -> d_name, &statbuf);
            if (S_ISDIR(statbuf.st_mode))
            {
                if (strcmp(".", entry -> d_name) == 0 ||
                    strcmp("..", entry -> d_name) == 0)
                {
                    continue;
                }

                totalSize += totalDirSize(entry -> d_name);
            }
            else
            {
                totalSize += (statbuf.st_size >> 20);
            }
        }
        db_msg("dir %s total size %d MB\n", dir, totalSize);
        chdir(chCurPath);
        //db_msg("exit,current work path return to: %s\n", chCurPath);
        free(chCurPath);
        closedir(dp);
        return totalSize;
}
int getFiles(char *dir, char *filetype, Vector<String8> &files)
{
	db_msg("%s %d", __FUNCTION__, __LINE__);
	DIR *p_dir;
	String8 file(dir);
	struct stat stat;
	struct dirent *p_dirent;
	p_dir = opendir(dir);
	if (p_dir == NULL) {
		db_error("fail to opendir %s", dir);
		return -1;
	}
	while( (p_dirent = readdir(p_dir)) ) {
		if (strcmp(p_dirent->d_name, ".") == 0 ||
				strcmp(p_dirent->d_name, "..") == 0) {
			continue;
		}
		file = dir;
		file += p_dirent->d_name;
		if(lstat(file.string(), &stat) == -1) {
			db_error("fail to get file stat %s", file.string());
			continue;
		}
		if (!S_ISREG(stat.st_mode)) {
			db_error("%s is not a regular file", file.string());
			continue;
		}
		String8 ext = file.getPathExtension();
		if(ext.isEmpty()) {
			continue;
		}
		if(strcasecmp(ext.string(), filetype) == 0) {
			files.add(file);
		} else {
			//db_debug("file is not type:%s", filetype);
		}
	}
	closedir(p_dir);
	//db_msg("%s %d", __FUNCTION__, __LINE__);
	return 0;
}

void StorageManager::dbReset()
{


	db_debug("%s %d %p\n", __FUNCTION__, __LINE__, mDBC);
	mDBC->deleteTable(String8(CDR_TABLE));

	mDbUpdated = false;
	dbInit();
	mDbUpdated = true;
}

void StorageManager::dbInit()
{
	db_debug("%s %d %p\n", __FUNCTION__, __LINE__, mDBC);
	Mutex::Autolock _l(mDBInitLock);
	if (mDBC == NULL) {
		mDBC = new DBCtrl();
		db_debug("%s %d %p\n", __FUNCTION__, __LINE__, mDBC);
	}
	mDBC->createTable(String8(CDR_TABLE), gSqlTable, ARRYSIZE(gSqlTable));	
	mDBC->setColumnType(gDBType, 0);
	dbUpdate();
}

time_t parase_time(char *ptr1)
{
	char *ptr;
	char chr;
	int value;
	struct tm my_tm;
	time_t my_time_t;
	/*
	   ptr = strchr(filename, '.');
	   if(ptr == NULL) {
	   printf("not found the delim\n");
	   return 0;
	   }
	   db_msg("filename is %s\n", filename);
	   ptr1 = strndup(filename, ptr - filename - 1);
	   */
	memset(&my_tm, 0, sizeof(my_tm));
	if (strlen(ptr1) >= strlen("20141201_121212")) {
	/************ year ************/
	ptr = ptr1;
	chr = ptr[4];
	ptr[4] = '\0';
	value = atoi(ptr);
	my_tm.tm_year = value - 1900;
	ptr[4] = chr;

	/************ month ************/
	ptr = ptr + 4;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_mon = value - 1;
	ptr[2] = chr;

	/************ day ************/
	ptr = ptr + 2;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_mday = value;

	/************ hour ************/
	ptr += 3;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_hour = value;
	ptr[2] = chr;

	/************ minite ************/
	ptr += 2;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_min = value;
	ptr[2] = chr;

	/************ second ************/
	ptr += 2;
	chr = ptr[2]; 
	ptr[2] = '\0';
	value = atoi(ptr);
	my_tm.tm_sec = value;
	} else {
		my_tm.tm_year = 2014;
		my_tm.tm_mon  = 12;
		my_tm.tm_mday = 1;
		my_tm.tm_hour = 1;
		my_tm.tm_min  = 1;
		my_tm.tm_sec  = 1;
	}
#if 0
	db_msg("year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d\n", 
			my_tm.tm_year, 
			my_tm.tm_mon, 
			my_tm.tm_mday, 
			my_tm.tm_hour, 
			my_tm.tm_min, 
			my_tm.tm_sec);
#endif
	my_time_t = mktime(&my_tm);
	//db_msg("time_t is %lu, time str is %s\n", my_time_t, ctime(&my_time_t));
	return my_time_t;
}

int StorageManager::fileSize(String8 path)
{
	struct stat	buf; 
	stat(path.string(),&buf);
	if (buf.st_size == 0) {
		db_msg("%s size is 0", path.string());
	}
	return buf.st_size;
}

int StorageManager::dbUpdate(char *type)
{
	Vector<String8> files_db, files;
	String8 file, file_db, tmp, stime;
	//	Vector<String8>::iterator result;
	Elem elem;
	time_t time;
	int size, size_db, i, ret=0;
	const char *ext = NULL;
        int cnt = ARRYSIZE(fileThreshods);
        int k = 0;
        char buf[128] = {0};

	if (0 == strcmp(type, TYPE_VIDEO)) {
		ext = EXT_VIDEO_MP4;
	} else if (0 == strcmp(type, TYPE_PIC)) {
		ext = EXT_PIC_JPG;
	}
	
        for (k=0; k<cnt; k++) {
                if(!mMounted || mForceSkipDb){
                    return 0;
                }
                if(0 != strcmp(type, fileThreshods[k].type)){
                    continue;
                }
                memset(buf,0,128);
                sprintf(buf,"%s%s/",MOUNT_PATH,fileThreshods[k].dir);
                files.clear();
		getFiles(buf, (char*)ext, files);

        

		/*dbGetFiles(type, files_db);
		size_db = files_db.size();
		for(i=0; i<size_db; i++) {
			file_db = files_db.itemAt(i);
			if (!FILE_EXSIST(file_db.string())) {	//File is in the database but does not exist
				db_msg("has not found %s, delete from database", file_db.string());
				dbDelFile(file_db.string());	//delete file record in the database			
				ret = deleteFile(file_db.string());
				if (ret < 0) {
					db_debug("fail to delete file %s", (const char *)file_db.string());
					return RET_IO_DEL_FILE;
				}
			} else {
				db_msg("has found %s in the tfcard", file_db.string());
			}
		}*/
		size = files.size();
		mDBC->setSQL(String8(BEGIN_TRANSACTION));
		mDBC->sqlite3Exec();
		//ALOGD("begin transaction++");
		for(i=0; i<size; i++)
		{
			//isValidVideoFile(files.itemAt(i));
			if(!mMounted ||  mForceSkipDb){
			    return 0;
			}
			file = files.itemAt(i);
			if (!dbIsFileExist(file)) {	//file exsit but there is no record in the database
				if (fileSize(file) == 0) {	//file size is 0
					//deleteFile(file.string());
					continue;
				}
				time = parase_time((char*)file.getBasePath().getPathLeaf().string());
				elem.file = (char *)file.string();
				elem.time = (long unsigned int)time;
				elem.type = type;
				elem.info = (char*)INFO_NORMAL;
				elem.dir = fileThreshods[k].dir;
				if (0 == strcmp(type, TYPE_VIDEO)) {
					if(strstr(file.string(), FLAG_SOS) != NULL) {
						db_msg("file type is %s\n", FLAG_SOS);
						elem.info = (char*)INFO_LOCK;
					}
				}
				dbAddFileByTransaction(&elem);	//new file insert to the database
			}
			//db_msg("file %s exists in the database", files.itemAt(i).string());
		}
		//ALOGD("commit transaction");
		mDBC->setSQL(String8(COMMIT_TRANSACTION));
		mDBC->sqlite3Exec();

        }
	return 0;
}

int StorageManager::dbUpdate()
{
	db_msg("------dbUpate");
	if(isMount()== false) {
		db_error("not mount\n");
		return -1;
	}
	ALOGD("-----%s ++",__func__);
	dbUpdate((char*)TYPE_VIDEO);
	dbUpdate((char*)TYPE_PIC);
	ALOGD("-----%s --",__func__);
	return 0;
}

void StorageManager::dbExit()
{
	if (mDBC) {
		delete mDBC;
		mDBC = NULL;
	}
}

/* create dirs
 * /mnt/external_sd/video
 * /mnt/external_sd/photo
 * /mnt/external_sd/video/.thumb
 * /mnt/external_sd/photo/.thumb
 */

int StorageManager::checkDirs()
{
	int retval = 0;
	int cnt = ARRYSIZE(fileThreshods);
	int i = 0;
	char buf[128] = {0};
	/*if(access(CON_2STRING(MOUNT_PATH, VIDEO_DIR), F_OK) != 0) {
		if(mkdir(CON_2STRING(MOUNT_PATH, VIDEO_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_2STRING(MOUNT_PATH, PIC_DIR), F_OK) != 0) {

		if(mkdir(CON_2STRING(MOUNT_PATH, PIC_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR), F_OK) != 0) {

		if(mkdir(CON_3STRING(MOUNT_PATH, VIDEO_DIR, THUMBNAIL_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}
	if(access(CON_3STRING(MOUNT_PATH, PIC_DIR, THUMBNAIL_DIR), F_OK) != 0) {

		if(mkdir(CON_3STRING(MOUNT_PATH, PIC_DIR, THUMBNAIL_DIR), 0777) == -1) {
			db_msg("mkdir failed: %s\n", strerror(errno));
			retval = -1;
		}
	}*/
        for (i=0; i<cnt; i++) {
            memset(buf,0,128);
            sprintf(buf,"%s%s",MOUNT_PATH,fileThreshods[i].dir);
	    if(access(buf, F_OK) != 0) {
                if(mkdir(buf, 0777) == -1) {
	    		db_msg("mkdir (%s) failed: %s\n",buf, strerror(errno));
	    		retval = -1;
	    	}
            }
            memset(buf,0,128);
            sprintf(buf,"%s%s/%s",MOUNT_PATH,fileThreshods[i].dir,THUMBNAIL_DIR);
	    if(access(buf, F_OK) != 0) {
                if(mkdir(buf, 0777) == -1) {
	    		db_msg("mkdir (%s) failed: %s\n",buf, strerror(errno));
	    		retval = -1;
	    	}
            }

        }
	return retval;
}

void addSingleQuotes(String8 &str)
{
	String8 tmp("'");
	tmp += str;
	tmp += ("'");
	str = tmp;
}

bool StorageManager::dbIsFileExist(String8 file)
{
	String8 filter("where file=");
	int cnt = 0;
	addSingleQuotes(file);
	filter += file;
	Mutex::Autolock _l(mDBLock);
	mDBC->setFilter(filter);
	cnt = mDBC->getRecordCnt();
	return (cnt>0)?true:false;
}

/* Count item, such as file, time, type, info etc.*/
int StorageManager::dbCount(char *item)
{
	Mutex::Autolock _l(mDBLock);
	String8 str("where type=");
	String8 item_str(item);
	addSingleQuotes(item_str);
	mDBC->setFilter(str+item_str);
	return mDBC->getRecordCnt();
}

void StorageManager::dbGetFiles(char *item, Vector<String8>&file)
{
	int total = dbCount(item);
	int i = 0;
	char *ptr = NULL;
	Mutex::Autolock _l(mDBLock);
	String8 str("where type=");
	String8 item_str(item);
	addSingleQuotes(item_str);
	mDBC->setFilter(str+item_str);
	mDBC->bufPrepare();
	while(i < total) {
		ptr = (char *)mDBC->getElement(String8("file"), 0, i);
		if (ptr) {
			file.add(String8(ptr));
		}
		i++;
	}
	mDBC->bufRelease();
}

void StorageManager::dbGetFile(char* item, String8 &file)
{
	char *ptr = NULL;
	Mutex::Autolock _l(mDBLock);
	String8 str("where type=");
	String8 item_str(item);
	addSingleQuotes(item_str);
	mDBC->setFilter(str+item_str);
	mDBC->bufPrepare();
	ptr = (char *)mDBC->getElement(String8("file"), 0, 0);
	if (ptr) {
		file = String8(ptr);
	}
	mDBC->bufRelease();
}
int StorageManager::dbGetSortByTimeRecord(dbRecord_t &record, int index)
{
	char *ptr = NULL;
	int total, retval;

	retval = dbCount((char*)TYPE_VIDEO);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	total = retval;

	retval = dbCount((char*)TYPE_PIC);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	total += retval;

	Mutex::Autolock _l(mDBLock);
	if(index >= total) {
		db_error("invalid index: %d, total is %d\n", index, total);
		return -1;
	}
	mDBC->setFilter(String8("ORDER BY time DESC") );
	mDBC->bufPrepare();
	ptr = (char *)mDBC->getElement(String8("file"), 0, index);
	if (ptr) {
		record.file = String8(ptr);
	}
	ptr = (char *)mDBC->getElement(String8("type"), 0, index);
	if (ptr) {
		record.type = String8(ptr);
	}
	ptr = (char *)mDBC->getElement(String8("info"), 0, index);
	if (ptr) {
		record.info = String8(ptr);
	}
	ptr = (char *)mDBC->getElement(String8("dir"), 0, index);
	if (ptr) {
		record.dir = String8(ptr);
	}
        mDBC->bufRelease();

	return 0;
}
void StorageManager::dbAddFile(Elem *elem)
{
	char str[256];
	int ret;
	sprintf(str, "INSERT INTO %s VALUES('%s',%lu,'%s','%s','%s')",mDBC->getTableName().string(),
			elem->file,elem->time,elem->type,elem->info,elem->dir);
	//db_msg("sql is %s\n", str);
	Mutex::Autolock _l(mDBLock);
	mDBC->setSQL(String8(str));
	ret = mDBC->executeSQL();
	if (ret != SQLITE_OK) {
		db_msg("error to add file:%d\n", ret);
	}
}

void StorageManager::dbAddFileByTransaction(Elem *elem)
{
	char str[256];
	int ret;
	sprintf(str, "INSERT INTO %s VALUES('%s',%lu,'%s','%s','%s')",mDBC->getTableName().string(),
			elem->file,elem->time,elem->type,elem->info,elem->dir);
	//db_msg("sql is %s\n", str);
	mDBC->setSQL(String8(str));
	ret = mDBC->sqlite3Exec();
	if (ret != SQLITE_OK) {
		db_msg("error to add file:%d\n", ret);
	}
}
void StorageManager::dbDelFile(char const *file)
{
	Mutex::Autolock _l(mDBLock);
	char str[128];
	sprintf(str, "DELETE from %s WHERE file='%s'",mDBC->getTableName().string(), file);
	mDBC->setSQL(String8(str));
	mDBC->executeSQL();
}

void StorageManager::dbUpdateFile(const char* oldname, const char *newname)
{
	Mutex::Autolock _l(mDBLock);
	const String8 sql(String8::format("UPDATE %s SET %s = '%s' WHERE %s = '%s' ",
		mDBC->getTableName().string(), gSqlTable[0].item, oldname, gSqlTable[0].item, newname));
	mDBC->setSQL(sql);
	mDBC->executeSQL();
}

int StorageManager::saveVideoFrameToBmpFile(VideoFrame* videoFrame, const char* bmpFile)
{
	unsigned int width, height, size;
	unsigned char* data;
	MYBITMAP myBitmap;
	RGB pal[256] = {{0,0,0,0}};
	int Bpp;	/* Bytes per pixel */
	int depth;
	data = (unsigned char*)videoFrame + sizeof(VideoFrame);
	width = videoFrame->mWidth;
	height = videoFrame->mHeight;
	size = videoFrame->mSize;
	Bpp = size / (width * height);
	depth = Bpp * 8;
	//db_msg("Bpp is %d\n", Bpp);
	//db_msg("depth is %d\n", depth);
	myBitmap.flags  = MYBMP_TYPE_NORMAL;
	myBitmap.frames = 1;
	myBitmap.depth  = depth;
	myBitmap.w      = width;
	myBitmap.h      = height;
	myBitmap.pitch  = width * Bpp;
	myBitmap.size   = size;
	myBitmap.bits   = data;
	return SaveMyBitmapToFile(&myBitmap, pal, bmpFile);
}

int StorageManager::savePicture(void* data, int size, int fd)
{
	int bytes_write = 0;
	int bytes_to_write = size;
	void *ptr = data;

	//db_msg("savePciture, size is %d\n", size);

	while((bytes_write = write(fd, ptr, bytes_to_write)) )
	{
		if((bytes_write == -1) && (errno != EINTR)) {
			db_msg("wirte error: %s\n", strerror(errno));
			break;
		} else if(bytes_write == bytes_to_write) {
			break;
		} else if( bytes_write > 0 ) {
			ptr = (void *)((char *)ptr + bytes_write);
			bytes_to_write -= bytes_write;
		}
	}
	if(bytes_write == -1) {
		db_error("save picture failed: %s\n", strerror(errno));
		return -1;
	}

	fsync(fd);
	close(fd);

	return 0;
}

bool StorageManager::isDiskFull(void)
{
	if(needDeleteFiles() == RET_DISK_FULL)
		return true;
	return false;
}

void StorageManager::setEventListener(EventListener* pListener)
{
	mListener = pListener;
}

