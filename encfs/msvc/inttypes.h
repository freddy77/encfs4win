#ifndef ENCFS_INTTYPES_H
#define ENCFS_INTTYPES_H

typedef unsigned char    uint8_t;
typedef signed   char     int8_t;
typedef unsigned short   uint16_t;
typedef signed   short    int16_t;
typedef unsigned int     uint32_t;
typedef signed   int      int32_t;
typedef unsigned __int64 uint64_t;
typedef signed   __int64  int64_t;

#define PRIi16 "i"
#define PRId16 "d"
#define PRIo16 "o"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"

#define PRIi32 "i"
#define PRId32 "d"
#define PRIo32 "o"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"

#define PRIi64 "I64i"
#define PRId64 "I64d"
#define PRIo64 "I64o"
#define PRIu64 "I64u"
#define PRIx64 "I64x"
#define PRIX64 "I64X"

#endif // ENCFS_INTTYPES_H
