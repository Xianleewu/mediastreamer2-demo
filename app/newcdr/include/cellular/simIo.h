#ifndef SIM_IO_H
#define SIM_IO_H

#define COMMAND_READ_BINARY 0xb0
#define COMMAND_UPDATE_BINARY 0xd6
#define COMMAND_READ_RECORD 0xb2
#define COMMAND_UPDATE_RECORD 0xdc
#define COMMAND_SEEK 0xa2
#define COMMAND_GET_RESPONSE 0xc0

#define GET_RESPONSE_EF_SIZE_BYTES 15
#define GET_RESPONSE_EF_IMG_SIZE_BYTES 10

// GSM SIM file ids from TS 51.011
#define EF_ADN 0x6F3A
#define EF_FDN 0x6F3B
#define EF_GID1 0x6F3E
#define EF_SDN 0x6F49
#define EF_EXT1 0x6F4A
#define EF_EXT2 0x6F4B
#define EF_EXT3 0x6F4C
#define EF_EXT6 0x6fc8  // Ext record for EF[MBDN]
#define EF_MWIS 0x6FCA
#define EF_MBDN 0x6fc7
#define EF_PNN 0x6fc5
#define EF_OPL 0x6fc6
#define EF_SPN 0x6F46
#define EF_SMS 0x6F3C
#define EF_ICCID 0x2fe2
#define EF_AD 0x6FAD
#define EF_MBI 0x6fc9
#define EF_MSISDN 0x6f40
#define EF_SPDI 0x6fcd
#define EF_SST 0x6f38
#define EF_CFIS 0x6FCB
#define EF_IMG 0x4f20

// USIM SIM file ids from TS 131.102
#define EF_PBR 0x4F30
#define EF_LI 0x6F05

// GSM SIM file ids from CPHS (phase 2, version 4.2) CPHS4_2.WW6
#define EF_MAILBOX_CPHS 0x6F17
#define EF_VOICE_MAIL_INDICATOR_CPHS 0x6F11
#define EF_CFF_CPHS 0x6F13
#define EF_SPN_CPHS 0x6f14
#define EF_SPN_SHORT_CPHS 0x6f18
#define EF_INFO_CPHS 0x6f16
#define EF_CSP_CPHS 0x6f15

// CDMA RUIM file ids from 3GPP2 C.S0023-0
#define EF_CST 0x6f32
#define EF_RUIM_SPN 0x6F41

// ETSI TS.102.221
#define EF_PL 0x2F05
// 3GPP2 C.S0065
#define EF_CSIM_LI 0x6F3A
#define EF_CSIM_SPN 0x6F41
#define EF_CSIM_MDN 0x6F44
#define EF_CSIM_IMSIM 0x6F22
#define EF_CSIM_CDMAHOME 0x6F28
#define EF_CSIM_EPRL 0x6F5A
#define EF_CSIM_MIPUPP 0x6F4D

//ISIM access
#define EF_IMPU 0x6f04
#define EF_IMPI 0x6f02
#define EF_DOMAIN 0x6f03
#define EF_IST 0x6f07
#define EF_PCSCF 0x6f09
#define EF_PSI 0x6fe5

#define MF_SIM_DF_TELECOM "3F007F10"
#define MF_SIM_DF_GSM "3F007F20"
#define MF_SIM "3F00"
#define MF_SIM_DF_TELECOM_DF_PHONEBOOK "3F007F105F3A"
#define MF_SIM_DF_TELECOM_DF_GRAPHICS "3F007F105F50"
#define MF_SIM_DF_ADF "3F007FFF"
/*
MF_SIM = "3F00";
DF_TELECOM = "7F10";
DF_PHONEBOOK = "5F3A";
DF_GRAPHICS = "5F50";
DF_GSM = "7F20";
DF_CDMA = "7F25";

//UICC access
DF_ADF = "7FFF";
*/

typedef struct {
    int command;    /* one of the commands listed for TS 27.007 +CRSM*/
    int fileid;     /* EF id */
    char path[10];     /* "pathid" from TS 27.007 +CRSM command.
                       Path is in hex asciii format eg "7f205f70"
                       Path must always be provided.
                    */
    int p1;
    int p2;
    int p3;
    char data[128];     /* May be NULL*/
    char pin2[10];     /* May be NULL*/
    char aidPtr[10];   /* AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value. */
} SIM_IO;

typedef struct {
    int ef_id;
    char data[128];
}sim_data;

char *getSimEFPath(int efid);
char *getUsimEFPath(int efid);
char *getCommonIccEFPath(int efid);
#endif
