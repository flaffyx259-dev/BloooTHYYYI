#pragma once
#include "../../SDK/sdk.hpp"

struct Packet_t {
	int nEntIndex;
	uint32_t uHash;
	int p1, p2, p3, p4;

	int unused1, unused2;
	//unsigned int user_4;

	void Encrypt( ) {
		//uHash *= 5;
	}

	void Decrypt( ) {
		//uHash /= 5;
	}
};

struct VoiceData_t {
	uint32_t xuid_low{ };
	uint32_t xuid_high{ };
	int32_t sequence_bytes{ };
	uint32_t section_number{ };
	uint32_t uncompressed_sample_offset{ };

	__forceinline uint8_t *GetRawData( ) {
		return ( uint8_t * )this;
	}
};

struct CSVCMsg_VoiceData {
	char pad_0000[ 8 ]; //0x0000
	int32_t client; //0x0008
	int32_t audible_mask; //0x000C
	uint32_t xuid_low{ };
	uint32_t xuid_high{ };
	void *voide_data_; //0x0018
	int32_t proximity; //0x001C
	//int32_t caster; //0x0020
	int32_t format; //0x0020
	int32_t sequence_bytes; //0x0024
	uint32_t section_number; //0x0028
	uint32_t uncompressed_sample_offset; //0x002C

	__forceinline VoiceData_t GetData( ) {
		VoiceData_t cdata;
		cdata.xuid_low = xuid_low;
		cdata.xuid_high = xuid_high;
		cdata.sequence_bytes = sequence_bytes;
		cdata.section_number = section_number;
		cdata.uncompressed_sample_offset = uncompressed_sample_offset;
		return cdata;
	}
};

struct CCLCMsg_VoiceData {
	uint32_t INetMessage_Vtable; //0x0000
	char pad_0004[ 4 ]; //0x0004
	uint32_t CCLCMsg_VoiceData_Vtable; //0x0008
	char pad_000C[ 8 ]; //0x000C
	void *data; //0x0014
	uint32_t xuid_low{ };
	uint32_t xuid_high{ };
	int32_t format; //0x0020
	int32_t sequence_bytes; //0x0024
	uint32_t section_number; //0x0028
	uint32_t uncompressed_sample_offset; //0x002C
	int32_t cached_size; //0x0030

	uint32_t flags; //0x0034

	uint8_t no_stack_overflow[ 0xFF ];

	__forceinline void SetData( VoiceData_t *cdata ) {
		xuid_low = cdata->xuid_low;
		xuid_high = cdata->xuid_high;
		sequence_bytes = cdata->sequence_bytes;
		section_number = cdata->section_number;
		uncompressed_sample_offset = cdata->uncompressed_sample_offset;
	}
};

struct _String_t {
	char data[ 16 ]{ };
	uint32_t current_len = 0;
	uint32_t max_len = 15;
};

class Communication {
	bool SendDataMessage( VoiceData_t *data );
public:
	void SendUpdatePacket( int nIndex = 0, Vector vecOrigin = Vector( 0, 0, 0 ) );
	bool OnMessageHook( std::function<bool( )> oSVCMsg_VoiceData, void *msg );
};

extern Communication g_Communication;