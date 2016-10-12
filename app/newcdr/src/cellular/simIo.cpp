#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <utils/Log.h>

#include <cellular/simIo.h>

#define LOG_TAG "CELLULAR"


char *getSimEFPath(int efid) {
// TODO(): DF_GSM can be 7F20 or 7F21 to handle backward compatibility.
// Implement this after discussion with OEMs.
    switch(efid) {
        case EF_SMS:
             return MF_SIM_DF_TELECOM;//MF_SIM + DF_TELECOM;
        case EF_EXT6:
        case EF_MWIS:
        case EF_MBI:
        case EF_SPN:
        case EF_AD:
        case EF_MBDN:
        case EF_PNN:
        case EF_SPDI:
        case EF_SST:
        case EF_CFIS:
        case EF_GID1:
	     return MF_SIM_DF_GSM;//MF_SIM + DF_GSM;
        case EF_MAILBOX_CPHS:
        case EF_VOICE_MAIL_INDICATOR_CPHS:
	case EF_CFF_CPHS:
	case EF_SPN_CPHS:
	case EF_SPN_SHORT_CPHS:
        case EF_INFO_CPHS:
        case EF_CSP_CPHS:
	     return MF_SIM_DF_GSM;//MF_SIM + DF_GSM;
    }
    return getCommonIccEFPath(efid);
}

char *getUsimEFPath(int efid) {
    switch(efid) {
        case EF_SMS:
        case EF_EXT6:
        case EF_MWIS:
        case EF_MBI:
        case EF_SPN:
        case EF_AD:
        case EF_MBDN:
        case EF_PNN:
        case EF_OPL:
        case EF_SPDI:
        case EF_SST:
        case EF_CFIS:
        case EF_MAILBOX_CPHS:
        case EF_VOICE_MAIL_INDICATOR_CPHS:
        case EF_CFF_CPHS:
        case EF_SPN_CPHS:
        case EF_SPN_SHORT_CPHS:
        case EF_FDN:
        case EF_MSISDN:
        case EF_EXT2:
        case EF_INFO_CPHS:
        case EF_CSP_CPHS:
        case EF_GID1:
        case EF_LI:
             return MF_SIM_DF_ADF;//MF_SIM + DF_ADF;
        case EF_PBR:
        // we only support global phonebook.
             return MF_SIM_DF_TELECOM_DF_PHONEBOOK;//MF_SIM + DF_TELECOM + DF_PHONEBOOK;
    }
    return getCommonIccEFPath(efid);
}

char *getCommonIccEFPath(int efid) {
     switch(efid) {
         case EF_ADN:
         case EF_FDN:
         case EF_MSISDN:
         case EF_SDN:
         case EF_EXT1:
         case EF_EXT2:
         case EF_EXT3:
         case EF_PSI:
              return MF_SIM_DF_TELECOM;//MF_SIM + DF_TELECOM;
         case EF_ICCID:
         case EF_PL:
              return MF_SIM;
         case EF_PBR:
         // we only support global phonebook.
              return MF_SIM_DF_TELECOM_DF_PHONEBOOK;//MF_SIM + DF_TELECOM + DF_PHONEBOOK;
         case EF_IMG:
              return MF_SIM_DF_TELECOM_DF_GRAPHICS;//MF_SIM + DF_TELECOM + DF_GRAPHICS;
    }
         return MF_SIM_DF_TELECOM_DF_PHONEBOOK;
}
