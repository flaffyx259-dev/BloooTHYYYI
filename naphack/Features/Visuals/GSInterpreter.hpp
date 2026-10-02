#pragma once
#include "../../SDK/sdk.hpp"

using _DWORD = int;

#undef LOBYTE

#define SLOBYTE(x)   (*((__int8*)&(x)))
#define LODWORD(x)  (*((int*)&(x)))  // low dword
#define LOBYTE(x)   (*((char*)&(x)))   // low byte

// what the fuck
inline unsigned int esoterik_array_two[ ] = {
 0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535, 131071, 262143, 524287, 1048575, 2097151, 4194303, 8388607, 16777215, 33554431, 67108863, 134217727, 268435455, 536870911, 1073741823, 2147483647, -1, 1869180533, 110, 0, -2147483647
};
struct struct_dword18 {
    char gap0[ 16 ];
    size_t size;
};

struct voice_data {
    char gap0[ 8 ];
    int client;
    char gapC[ 12 ];
    struct_dword18 *voice_data;
    int gap1C;
    int dword20;
    int dword24;
    int dword28;
    int dword2C;
    char gap30[ 4 ];
    int caster_flags;
};

struct premium_shared_esp {
    int *field_0;
    int field_4;
    int field_8;
    int field_C;
    bool field_10;
    bool field_11;
};

struct funny_t {
    int _1;
    int _2;
    int _3;
    int _4;
};

struct SomeSharedESPData {
    int gap0;
    bool byte4;
    char field_5;
    char field_6;
    char field_7;
    int field_8;
    int field_C;
    int dword10;
    int dword14;
    unsigned int *punsigned_int18;
    unsigned int *field_1C;
    funny_t punsigned_int1C;
    int aaaaaa;
    int aaaaaa_part2;
    unsigned int dlwsakd;
};

extern void setup_esp_data( SomeSharedESPData *_ecx, int a2 );

class GSInterpreter {
public:
    bool ReceiveSharedPackets( voice_data *msg );
};

extern GSInterpreter g_GSInterpreter;