#ifndef SIZED_TYPES

#define SIZED_TYPES

// ================================================================
// Sized types

typedef char                  Int8;
typedef unsigned char         UInt8;

typedef short                 Int16;
typedef unsigned short        UInt16;

typedef int                   Int32;
typedef unsigned int          UInt32;

typedef long long             Int64;
typedef unsigned long long    UInt64;

// ================================================================
// Conversions between bits, bytes (8 bits) and words (32 bits)

#define bytes_to_bits(nB) ((nB) * 8)

#define bits_to_bytes(nb) (((nb) + 7) / 8)

#define words_to_bits(nw)  ((nw) * 32)

#define bits_to_words(nb)  (((nb) + 31) / 32)

#define words_to_bytes(nw) ((nw) * 4)

#define bytes_to_words(nB) (((nB) + 3) / 4)

// Conversions between bit-offset and byte-offset/byte-index or word-offset/word-index

#define bit_offset_to_byte_offset(b) ((b) & 7)
#define bit_offset_to_byte_index(b)  ((b) >> 3)

#define bit_offset_to_word_offset(b) ((b) & 31)
#define bit_offset_to_word_index(b)  ((b) >> 5)

// Sized type aliases used by Dini code

typedef UInt16 u16;
typedef UInt32 u32;
typedef UInt64 u64;

#endif
