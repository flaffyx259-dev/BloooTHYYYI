#pragma once

#include "../../SDK/sdk.hpp"
#include "../../SDK/Valve/CBaseHandle.hpp"
#include <deque>

class C_CSPlayer;
class C_WeaponCSBaseGun;
class C_BaseViewModel;

struct unpred_vars_t {
	bool in_prediction{};
	bool first_time_predicted{};

	int random_seed{};
	int prediction_player{};

	float recoil_index{};
	float accuracy_penalty{};

	float curtime{};
	float frametime{};

	int flags{};

	Vector velocity{};

	CUserCmd *predicted_cmd{};
	CUserCmd updated_cmd{};

	CBaseHandle ground_entity{};

	void store( C_CSPlayer *pLocal );
	void restore( C_CSPlayer *pLocal );

	__forceinline void reset( ) {
		in_prediction = false;
		first_time_predicted = false;

		random_seed = 0;
		prediction_player = 0;

		recoil_index = 0.f;
		accuracy_penalty = 0.f;

		curtime = 0.f;
		frametime = 0.f;

		flags = 0;

		velocity.Init( );

		predicted_cmd = nullptr;
		updated_cmd = {};

		ground_entity = 0;
	}
};

struct viewmodel_info_t {
	bool looking_at_weapon{};

	int cmd_tick{};
	int sequence{};
	int animation_parity{};
	int new_sequence_parity{};
	int model_index{};

	float anim_time{};
	float old_anim_time{};

	float cycle{};
	float old_cycle{};

	C_WeaponCSBaseGun *active_weapon{};

	void store( CUserCmd *cmd, C_BaseViewModel *viewmodel );
	void reset( );
};

struct networked_vars_t {
	bool done = false;
	bool walking{};
	bool scoped{};

	int command_number{};
	int tickbase{};
	int move_state{};
	int move_type{};
	int flags{};

	float forwardmove{};
	float sidemove{};

	float spread{};
	float inaccuracy{};
	float duck_amount{};

	//float stamina{};
	float recoil_index{};
	float acc_penalty{};
	float lower_body_yaw{};
	float thirdperson_recoil{};
	float fall_velocity{};
	float velocity_modifier{};

	Vector origin{};
	Vector abs_origin{};
	Vector velocity{};
	Vector viewoffset{};
	QAngle aimpunch{};
	QAngle aimpunch_vel{};
	QAngle viewpunch{};
	QAngle viewangles{};
	Vector ladder_normal{};
	Vector base_velocity{};
	Vector network_origin{};

	CBaseHandle ground_entity{};

	void store( C_CSPlayer *pLocal, CUserCmd *cmd, bool no_ground_entity = false );
	void restore( C_CSPlayer *pLocal, bool animations = false );

	__forceinline void reset( ) {
		done = false;
		walking = false;
		scoped = false;

		tickbase = 0;
		flags = 0;

		duck_amount = 0.f;

		spread = 0.f;
		inaccuracy = 0.f;
		//stamina = 0.f;
		fall_velocity = 0.f;
		velocity_modifier = 0.f;

		recoil_index = 0.f;
		acc_penalty = 0.f;
		lower_body_yaw = 0.f;
		thirdperson_recoil = 0.f;

		forwardmove = 0.f;
		sidemove = 0.f;

		origin.Init( );
		abs_origin.Init( );
		velocity.Init( );
		viewoffset.Init( );
		aimpunch.Init( );
		aimpunch_vel.Init( );
		viewpunch.Init( );
		viewangles.Init( );
		ladder_normal.Init( );
		base_velocity.Init( );
		network_origin.Init( );

		ground_entity = 0;
	}
};

class c_engine_prediction {
private:
	CMoveData move_data{};

	networked_vars_t initial_vars{};
	unpred_vars_t unpred_vars{};
	networked_vars_t networked_vars[ 150 ]{};
	networked_vars_t restore_vars[ 150 ]{};
	viewmodel_info_t viewmodel_info[ 150 ]{};

	struct compressed_netvars_t {
		bool filled{};
		int cmd_number{};
		int tickbase{};

		float fall_velocity{};
		float velocity_modifier{};
		float stamina{};

		QAngle aimpunch{};
		QAngle viewpunch{};
		QAngle aimpunch_vel{};
		Vector viewoffset{};
		Vector origin{};
		Vector base_velocity{};
		Vector velocity{};
		Vector network_origin{};

		void store( int tick );

		void reset( ) {
			filled = false;
			cmd_number = 0;
			tickbase = 0;

			fall_velocity = 0.f;
			velocity_modifier = 0.f;
			stamina = 0.f;

			aimpunch.Init( );
			viewpunch.Init( );
			aimpunch_vel.Init( );
			viewoffset.Init( );
			origin.Init( );
			base_velocity.Init( );
			velocity.Init( );
			network_origin.Init( );
		}
	};

	struct ping_backup_t {
		int cs_tickcount{};
		int tickcount{};
		float curtime{};
		float frametime{};

		void store( );
		void restore( );

		void reset( ) {
			cs_tickcount = 0;
			tickcount = 0;

			curtime = 0.f;
			frametime = 0.f;
		}
	};

	compressed_netvars_t compressed_netvars[ 150 ]{};

	std::string last_sound_name{};
public:
	float ideal_inaccuracy{};
	bool in_prediction{};

	bool fake_datagram;
	bool m_bReadPackets;

	std::vector<int> last_outgoing_commands{};

	bool pred_error_occured{};
	float velocity_modifier{};
	ping_backup_t ping_backup{};

	int m_iSeqDiff;
	CUserCmd *m_pLastCmd;


	__forceinline networked_vars_t *get_networked_vars( int cmd ) {
		return &networked_vars[ cmd % 150 ];
	}

	__forceinline networked_vars_t *get_vars_to_restore( int cmd ) {
		return &restore_vars[ cmd % 150 ];
	}

	__forceinline networked_vars_t *get_initial_vars( ) {
		return &initial_vars;
	}

	__forceinline unpred_vars_t *get_unpredicted_vars( ) {
		return &unpred_vars;
	}

	__forceinline CMoveData *get_move_data( ) {
		return &move_data;
	}

	__forceinline std::string &get_last_sound_name( ) {
		return last_sound_name;
	}

	__forceinline void reset( ) {
		in_prediction = false;
		move_data = {};
		last_sound_name = "";
		pred_error_occured = false;
		velocity_modifier = 0.f;
		ping_backup.reset( );

		unpred_vars.reset( );
		initial_vars.reset( );

		for( auto &i : networked_vars )
			i.reset( );

		for( auto &i : restore_vars )
			i.reset( );

		for( auto &i : viewmodel_info )
			i.reset( );

		for( auto &i : compressed_netvars )
			i.reset( );
	}

	bool available( );

	bool should_reduce_ping( );

	void update_ping_values( bool final_tick );

	__forceinline compressed_netvars_t *get_compressed_netvars( int cmd ) {
		return &compressed_netvars[ cmd % 150 ];
	}

	void fix_netvars( int tick );
	void store_netvars( int tick );

	void update( );
	void start( C_CSPlayer *pLocal, CUserCmd* pCmd );
	void update_viewmodel_info( CUserCmd *cmd );
	void fix_viewmodel( int stage );
	void ReadPackets( bool bFinal );
	void end( C_CSPlayer *pLocal );
};

extern c_engine_prediction g_Prediction;