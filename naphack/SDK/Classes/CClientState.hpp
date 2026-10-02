#pragma once

#define CONCAT_IMPL( x, y ) x##y
#define MARCO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define PAD( size ) char MARCO_CONCAT( _pad, __COUNTER__ ) [ size ]

class CClientState
{
	class CClockDriftManager {
	public:
		float m_ClockOffsets[ 16 ];
		unsigned int m_iCurClockOffset;
		unsigned int m_nServerTick;
		unsigned int m_nClientTick;
	};

public:
  int& m_nDeltaTick( );
  int& m_nLastOutgoingCommand( );
  int& m_nChokedCommands( );
  int& m_nLastCommandAck( );
  bool& m_bIsHLTV( );
  int& m_nMaxClients( );

  CClockDriftManager& m_ClockDriftManager( );

  PAD( 0x9C );
  void *net_channel;
  int challenge_nr;
  PAD( 0x4 );
  double connect_time;
  int retry_number;
  PAD( 0x54 );
  int signon_state;
  PAD( 0x4 );
  double next_cmd_time;
  int server_count;
  int current_sequence;
  PAD( 0x8 );
  CClockDriftManager clock_drift_mgr;
  int delta_tick;

  PAD( 19240 );
  int old_tickcount;
  float tick_remainder;
  float frame_time;
  int last_outgoing_command;
  int choked_commands;
  int last_command_ack;
  int last_server_tick;
  int command_ack;
};
