#ifdef BCMSDIO
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 BSQUARE Corporation.  All rights reserved.
//
// Module Name:
//
//    _SDCardRes.h   
//
// Abstract:
//
//    Header file defining Resource file strings
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _SDCARD_RES_DEFINED
#define _SDCARD_RES_DEFINED

    // resource file definitions
#define SD_COMPANY_NAME        "BSQUARE Corporation\0"
#define SD_LEGAL_COPYRIGHT     "© 2002 BSQUARE Corporation\0"
#define SD_PRODUCT_NAME        "SDIONow\0"
#define SD_PRODUCT_VERSION     "1.1\0"
#define SD_FILE_VERSION        "1.1\0"
#define SD_PRODUCT_VERSION_2   1,1,0,0
#define SD_FILE_VERSION_2      1,1,0,0

#define SD_BUS_VERSION_MAJOR   1
#define SD_BUS_VERSION_MINOR   1
#define SD_BUS_DRIVER_VERSION  ((SD_BUS_VERSION_MAJOR << 16) | SD_BUS_VERSION_MINOR)

   // include version information in resource
#define VS_VERSION_INFO     1

#ifndef LANG_NEUTRAL
// Primary language IDs.
#define LANG_NEUTRAL                     0x00
#define LANG_BULGARIAN                   0x02
#define LANG_CHINESE                     0x04
#define LANG_CROATIAN                    0x1a
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KOREAN                      0x12
#define LANG_NORWEGIAN                   0x14
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_ROMANIAN                    0x18
#define LANG_RUSSIAN                     0x19
#define LANG_SLOVAK                      0x1b
#define LANG_SLOVENIAN                   0x24
#define LANG_SPANISH                     0x0a
#define LANG_SWEDISH                     0x1d
#define LANG_TURKISH                     0x1f
#endif //!LANG_NEUTRAL

#ifndef SUBLANG_NEUTRAL
// Sublanguage IDs.
#define SUBLANG_NEUTRAL                  0x00
#define SUBLANG_DEFAULT                  0x01
#define SUBLANG_SYS_DEFAULT              0x02
#define SUBLANG_CHINESE_TRADITIONAL      0x01
#define SUBLANG_CHINESE_SIMPLIFIED       0x02
#define SUBLANG_CHINESE_HONGKONG         0x03
#define SUBLANG_CHINESE_SINGAPORE        0x04
#define SUBLANG_DUTCH                    0x01
#define SUBLANG_DUTCH_BELGIAN            0x02
#define SUBLANG_ENGLISH_US               0x01
#define SUBLANG_ENGLISH_UK               0x02
#define SUBLANG_ENGLISH_AUS              0x03
#define SUBLANG_ENGLISH_CAN              0x04
#define SUBLANG_ENGLISH_NZ               0x05
#define SUBLANG_ENGLISH_EIRE             0x06
#define SUBLANG_FRENCH                   0x01
#define SUBLANG_FRENCH_BELGIAN           0x02
#define SUBLANG_FRENCH_CANADIAN          0x03
#define SUBLANG_FRENCH_SWISS             0x04
#define SUBLANG_GERMAN                   0x01
#define SUBLANG_GERMAN_SWISS             0x02
#define SUBLANG_GERMAN_AUSTRIAN          0x03
#define SUBLANG_ITALIAN                  0x01
#define SUBLANG_ITALIAN_SWISS            0x02
#define SUBLANG_NORWEGIAN_BOKMAL         0x01
#define SUBLANG_NORWEGIAN_NYNORSK        0x02
#define SUBLANG_PORTUGUESE               0x02
#define SUBLANG_PORTUGUESE_BRAZILIAN     0x01
#define SUBLANG_SPANISH                  0x01
#define SUBLANG_SPANISH_MEXICAN          0x02
#define SUBLANG_SPANISH_MODERN           0x03
#endif //!SUBLANG_NEUTRAL


#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        101
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#endif

#endif
