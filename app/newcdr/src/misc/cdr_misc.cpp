/*************************************************************************
  > File Name: src/cdr_misc.c
  > Description: 
  > Author: CaoDan
  > Mail: caodan2519@gmail.com 
  > Created Time: 2014年04月14日 星期一 19:45:55
 ************************************************************************/

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>


#include "misc.h"
#undef LOG_TAG 
#define LOG_TAG "cdr_misc.cpp"
#include "debug.h"

void LineEx (HDC hdc, int x1, int y1, int x2, int y2)
{
	MoveTo(hdc, x1, y1);
	LineTo(hdc, x2, y2);
}

const char* removeSuffix(const char *file, char *buf)
{
	const char *ptr;
	ptr = strrchr(file, '.');
	if(ptr == NULL)
		return NULL;
	strncpy(buf, file, (ptr - file));
	buf[ ptr - file] = '\0';
	return buf;
}
const char* getBaseName(const char *file, char *buf)
{
	const char *ptr;	
	if(file == NULL || buf == NULL)
		return NULL;
	ptr = strrchr(file, '/');
	if(ptr == NULL) {
		strcpy(buf, file);
	} else {
		strcpy(buf, ptr + 1);
	}
	return buf;
}
const char* getDir(const char *file, char *buf)
{
        const char *ptr;
        if(file == NULL || buf == NULL)
                return NULL;
        ptr = strrchr(file, '/');
        if(ptr == NULL) {
                strcpy(buf,file);
        } else {
                memcpy(buf, file,(strlen(file)-strlen(ptr)));
        }

        return buf;
}
int copyFile(const char* from, const char* to)
{
	db_msg("copy file\n");
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 8192
#endif
	int from_fd,to_fd;
	int bytes_read,bytes_write;
	char buffer[BUFFER_SIZE + 1];
	char *ptr;
	int retval = 0;

	if(access(from, F_OK | R_OK) != 0) {
		db_error("%s\n", strerror(errno));
		return -1;
	}

	if((from_fd=open(from, O_RDONLY)) == -1 )
	{
		db_error("Open %s Error:%s\n", from, strerror(errno));
		return -1;
	}

	if((to_fd = open(to, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
	{
		db_error("Open %s Error:%s\n", to, strerror(errno));
		close(from_fd);
		return -1;
	}

	while( (bytes_read = read(from_fd, buffer, BUFFER_SIZE)) )
	{
		if((bytes_read == -1) && ( errno != EINTR)) {
			db_msg("read error: %s\n", strerror(errno));
			retval = -1;
			break;
		}

		else if(bytes_read > 0)
		{
			ptr = buffer;
			while( (bytes_write = write(to_fd, ptr, bytes_read)) )
			{
				if((bytes_write == -1) && (errno != EINTR)) {
					db_msg("wirte error: %s\n", strerror(errno));
					retval = -1;
					break;
				}
				else if(bytes_write == bytes_read) {
					break;
				}
				else if( bytes_write > 0 )
				{
					ptr += bytes_write;
					bytes_read -= bytes_write;
				}
			}
			if(bytes_write == -1) {
				retval = -1;
				break;
			}
		}
	}
	db_msg("copy %s to %s\n", from, to);
	fsync(to_fd);
	close(from_fd);
	close(to_fd);
	return retval;
}

#define DISP_DEV	"/dev/disp"
#define DISP_CMD_LAYER_CK_OFF	0x52

void ck_off(int hlay)
{
	int ret;
	int fd;
	unsigned long args[4] = {0};
	args[0] = 0;
	args[1] = hlay;

	fd = open(DISP_DEV, O_RDWR);
	if(fd < 0) {
		db_error("open %s failed\n", DISP_DEV);
		return;
	}
	ret = ioctl(fd, DISP_CMD_LAYER_CK_OFF, args);
	if(ret < 0) {
		db_error("ck off failed\n");
	}
	close(fd);
}
#define DISP_CMD_LAYER_ALPHA_ON			0x4c
#define DISP_CMD_LAYER_ALPHA_OFF		0x4d
#define DISP_CMD_LAYER_SET_ALPHA_VALUE	0x4f
int set_ui_alpha(unsigned char alpha)
{
	int fd;	
	int hlay = 100;
	unsigned int args[4] = {0};
	if( (fd = open(DISP_DEV, O_RDWR)) == -1) {
		db_error("open %s failed: %s", DISP_DEV, strerror(errno));
		return -1;
	}
	args[0] = 0;
	args[1] = hlay;
	if(alpha < 255) {
		if(ioctl(fd, DISP_CMD_LAYER_ALPHA_ON, args) == -1) {
			db_error("set alpha failed: %s\n", strerror(errno));
			close(fd);
			return -1;
		}
		args[2] = alpha;
		if(ioctl(fd, DISP_CMD_LAYER_SET_ALPHA_VALUE, args) == -1) {
			db_error("set alpha failed: %s\n", strerror(errno));
			close(fd);
			return -1;
		}
	} else if(alpha == 255){
		args[2] = 255;
		if(ioctl(fd, DISP_CMD_LAYER_ALPHA_OFF, args) == -1) {
			db_error("set alpha failed: %s\n", strerror(errno));
			close(fd);
			return -1;
		}
	}
	close(fd);
	return 0;
}
#define USE_KEY_SOUND
#ifdef USE_KEY_SOUND
static audio_format_t audio_format;
static char *keySoundDataBuffer = NULL;
static int keySoundReadNum = 0;
static int chunkFormat;
void playKeySound()
{
	static AudioTrack *audio = NULL;
	if(keySoundDataBuffer == NULL)return;
	if(audio != NULL)delete audio;
	MemoryHeapBase *mMemHeap = new MemoryHeapBase(keySoundReadNum, 0, "AudioTrack Heap Base");	
	MemoryBase *mMemBase = new MemoryBase(mMemHeap, 0, keySoundReadNum);
	memcpy(mMemHeap->getBase(),keySoundDataBuffer,keySoundReadNum);
	audio = new AudioTrack( AUDIO_STREAM_MUSIC,
                            chunkFormat,
                            audio_format,
                            AUDIO_CHANNEL_OUT_STEREO,
                            mMemBase,
                            AUDIO_OUTPUT_FLAG_PRIMARY,
                            NULL,
                            NULL,
                             0,
                             0);

	audio->start();
}
int initKeySound(const char *path)
{
	struct riff_wave_header riff_wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;
    int more_chunks = 1;

	int file_pos_cur,file_pos_end;
	int buffer_cnt;

	FILE *keySoundFile;

   keySoundFile = fopen(path, "rb");
    if (!keySoundFile) {
        ALOGE("Unable to open file '%s'\n", path);
        return -1;
    }
	
	fread(&riff_wave_header, sizeof(riff_wave_header), 1, keySoundFile);
    if ((riff_wave_header.riff_id != ID_RIFF) ||
        (riff_wave_header.wave_id != ID_WAVE)) {
        ALOGE("Error: '%s' is not a riff/wave file\n", path);
        fclose(keySoundFile);	
        return -1;
    }

    do {
        fread(&chunk_header, sizeof(chunk_header), 1, keySoundFile);

        switch (chunk_header.id) {
        case ID_FMT:
            fread(&chunk_fmt, sizeof(chunk_fmt), 1, keySoundFile);
            /* If the format header is larger, skip the rest */
            if (chunk_header.sz > sizeof(chunk_fmt))
                fseek(keySoundFile, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            break;
        default:
            /* Unknown chunk, skip bytes */
            fseek(keySoundFile, chunk_header.sz, SEEK_CUR);
        }
    } while (more_chunks);
	
 
	if (chunk_fmt.bits_per_sample == 32)
	{
		audio_format = AUDIO_FORMAT_PCM_32_BIT;
	}
    else if (chunk_fmt.bits_per_sample == 16)
    {
		audio_format = AUDIO_FORMAT_PCM_16_BIT;
    }
	chunkFormat = chunk_fmt.sample_rate;	
	file_pos_cur=ftell(keySoundFile);		
	fseek(keySoundFile, 0, SEEK_END);
	file_pos_end=ftell(keySoundFile);
	
	buffer_cnt=file_pos_end-file_pos_cur;
	
    keySoundDataBuffer = (char*)malloc(buffer_cnt);
    if (!keySoundDataBuffer) {
        ALOGE("Unable to allocate %d bytes\n", buffer_cnt);
		fclose(keySoundFile);
        return -1;
    }
	fseek(keySoundFile, file_pos_cur, SEEK_SET);	
	keySoundReadNum = fread(keySoundDataBuffer, 1, buffer_cnt, keySoundFile);
	if(keySoundReadNum<=0){
        ALOGE("Unable to allocate %d bytes\n", buffer_cnt);
		fclose(keySoundFile);
        return -1;
    }	
	fclose(keySoundFile);
	return 0;
}
#endif
