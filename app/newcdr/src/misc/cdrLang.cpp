
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>

#include "cdrLang.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "cdrLang.cpp"
#include "debug.h"

#define LANG_FILE_CN	"/res/lang/zh-CN.bin"
#define LANG_FILE_TW	"/res/lang/zh-TW.bin"
#define LANG_FILE_EN	"/res/lang/en.bin"
#define LANG_FILE_JPN	"/res/lang/jpn.bin"
#define LANG_FILE_KR	"/res/lang/korean.bin"
#define LANG_FILE_RS	"/res/lang/russian.bin"



using namespace android;

#define maxsize 35
static ssize_t  __getline(char **lineptr, ssize_t *n, FILE *stream)
{
	ssize_t count=0;
	int buf;

	if(*lineptr == NULL) {
		*n=maxsize;
		*lineptr = (char*)malloc(*n);
	}

	if(( buf = fgetc(stream) ) == EOF ) {
		return -1;
	}

	do
	{
		if(buf=='\n') {    
			count += 1;
			break;
		}

		count++;

		*(*lineptr+count-1) = buf;
		*(*lineptr+count) = '\0';

		if(*n <= count)
			*lineptr = (char*)realloc(*lineptr,count*2);
		buf = fgetc(stream);
	} while( buf != EOF);

	return count;
}


cdrLang::cdrLang()
	: lang(LANG_ERR)
	, mLogFont(NULL)
{

}


int cdrLang::initLabel(void)
{
	String8 dataFile;
	if(lang == LANG_ERR) {
		db_error("lang is no initialized\n");
		return -1;
	}

	if(lang == LANG_CN) 
		dataFile = LANG_FILE_CN;
	else if(lang == LANG_TW)
		dataFile = LANG_FILE_TW;
	else if(lang == LANG_EN)
		dataFile = LANG_FILE_EN;
	else if(lang == LANG_JPN)
		dataFile = LANG_FILE_JPN;
	else if(lang == LANG_KR)
		dataFile = LANG_FILE_KR;
	else if(lang == LANG_RS)
		dataFile = LANG_FILE_RS;

	else {
		db_error("invalid lang %d\n", lang);
		return -1;
	}

	if(loadLabelFromLangFile(dataFile) < 0) {
		db_error("load label from %s failed\n", dataFile.string());
		return -1;
	}

	UTF8_to_MBS();

	return 0;
}



int cdrLang::loadLabelFromLangFile(String8 langFile)
{
	FILE *fp;
	char *line = NULL;
	ssize_t len = 0;

	fp = fopen(langFile.string(), "r");
	if (fp == NULL) {
		db_error("open file %s failed: %s\n", langFile.string(), strerror(errno));
		return -1;
	}

	if(label.size() != 0) {
		db_msg("size is %zd\n", label.size());
		label.clear();
		db_msg("size is %zd\n", label.size());
	}
	while ((__getline(&line, &len, fp)) != -1) {
//		db_msg("Retrieved line of length %ld :\n", read);
//		db_msg("%s\n", line);
		label.add(String8(line));
	}

	free(line);
	db_msg("size is %zd\n", label.size());
	fclose(fp);

	return 0;
}

int cdrLang::UTF8_to_MBS(void)
{
	Vector <String8>labelConvert;
	unsigned int convertLen;
	unsigned int len;
	unsigned short usBuf[100];
	char desBuf[100];

	int i;

	if(mLogFont == NULL) {
		db_error("mLogFont is NULL\n");
		return -1;
	}

	for(unsigned int count = 0 ; count < label.size(); count++) {
		memset(usBuf, 0, sizeof(usBuf));
		memset(desBuf, 0, sizeof(desBuf));
		len = utf8_to_ucs2(usBuf, label.itemAt(count));
		WCS2MBSEx(mLogFont, (unsigned char*)desBuf, usBuf, len, 0, len*2, (int*)&convertLen);
		#if 0
		fprintf(stderr, "MBS:\n");
		for (i=0; i<convertLen*2; i++) {
			fprintf(stderr, "%x ", desBuf[i]);
		}
		fprintf(stderr, "\n");
		#endif
		labelConvert.add(String8(desBuf));
	}

	db_msg("size is %zd\n", label.size());
	label.clear();
	db_msg("size is %zd\n", label.size());
	label = labelConvert;
	db_msg("size is %zd\n", label.size());

	return 0;
}

const char* cdrLang::getLabel(unsigned int labelIndex)
{
	if(labelIndex < label.size())
		return label.itemAt(labelIndex).string();
	else {
		db_msg("invalide label Index: %d, size is %zd\n", labelIndex, label.size());
		return NULL;
	}
}

