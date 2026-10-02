#include "GSInterpreter.hpp"
#include "../../pandora.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "Visuals.hpp"
#include "../Rage/Resolver.hpp"

// AHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHA
// AHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHA
// AHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHA
// AHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHAAHAHAHHAHA

GSInterpreter g_GSInterpreter;

void setup_esp_data( SomeSharedESPData *_ecx, int a2 ) {
	int v3; // ebx
	int v4; // edi
	unsigned int v5; // ebp
	int v6; // ecx
	signed int v7; // edi
	unsigned int *v8; // eax
	unsigned int v9; // edx
	int v10; // ecx
	int v11; // edx
	int v12; // eax
	int v13; // eax
	int v14; // eax

	v3 = 1;
	v4 = 0;
	if( _ecx->field_8 < 0 ) {
		v4 = _ecx->field_8;
		_ecx->byte4 = 1;
	}
	v5 = _ecx->field_C & 3;
	if( _ecx->field_C >= 4u && ( !v5 || v4 / 8 >= ( int )v5 ) ) {
		v6 = _ecx->punsigned_int1C._1;
		v7 = v4 - 8 * v5;
		v8 = ( unsigned int * )( v5 + v6 + 4 * ( v7 / 32 ) );
		_ecx->punsigned_int18 = v8;
		if( v6 ) {
			_ecx->dword14 = 32;
			if( v8 != _ecx->field_1C ) {
				if( v8 <= _ecx->field_1C ) {
					v9 = *v8;
					v3 = 32;
					_ecx->punsigned_int18 = v8 + 1;
				LABEL_11:
					_ecx->dword10 = v9 >> ( v7 & 0x1F );
					if( v3 >= 32 - ( v7 & 0x1F ) )
						v3 = 32 - ( v7 & 0x1F );
					goto LABEL_23;
				}
				_ecx->byte4 = 1;
				v3 = 32;
			LABEL_10:
				v9 = 0;
				goto LABEL_11;
			}
			_ecx->punsigned_int18 = v8 + 1;
		}
		_ecx->dword14 = 1;
		goto LABEL_10;
	}
	v10 = _ecx->punsigned_int1C._1;
	if( v10 ) {
		v11 = *( unsigned __int8 * )v10++;
		_ecx->dword10 = v11;
		if( v5 > 1 ) {
			v12 = *( unsigned __int8 * )v10++;
			v13 = v11 | ( v12 << 8 );
			_ecx->dword10 = v13;
			v11 = v13;
		}
		if( v5 > 2 ) {
			v14 = *( unsigned __int8 * )v10++;
			_ecx->dword10 = v11 | ( v14 << 16 );
		}
	}
	_ecx->punsigned_int18 = ( unsigned int * )v10;
	_ecx->dword10 >>= v4 & 0x1F;
	v3 = 8 * v5 - ( v4 & 0x1F );
LABEL_23:
	_ecx->dword14 = v3;
}

float sub_6ACAF3F1( SomeSharedESPData *_ecx ) {
	char v2; // bl
	float result; // xmm0_4
	int v4; // edi
	int v5; // eax
	unsigned int *v6; // eax
	unsigned int v7; // esi
	int v8; // ecx
	int v9; // eax
	unsigned int *v10; // eax
	unsigned int v11; // esi
	int v12; // eax
	unsigned int *v13; // eax
	unsigned int v14; // esi
	unsigned int *v15; // eax
	unsigned int *v16; // eax
	unsigned int *v17; // eax
	unsigned int v18; // eax
	unsigned int v19; // esi
	unsigned int *v20; // eax
	unsigned int *v21; // eax
	unsigned int *v22; // eax
	int v23; // ecx
	unsigned int v24; // eax
	int v25; // [esp+8h] [ebp-18h]
	int v26; // [esp+10h] [ebp-10h]
	int v27; // [esp+10h] [ebp-10h]
	int v28; // [esp+14h] [ebp-Ch]
	unsigned int v29; // [esp+14h] [ebp-Ch]
	char v30; // [esp+18h] [ebp-8h]
	char v31; // [esp+1Fh] [ebp-1h]

	v2 = 1;
	result = 0.0;
	v26 = _ecx->dword10 & 1;
	v4 = 32;
	v5 = _ecx->dword14 - 1;
	if( _ecx->dword14 == 1 ) {
		v6 = _ecx->punsigned_int18;
		_ecx->dword14 = 32;
		if( v6 == _ecx->field_1C ) {
			v7 = 0;
			_ecx->punsigned_int18 = v6 + 1;
			v5 = 1;
		}
		else {
			if( v6 <= _ecx->field_1C ) {
				v7 = *v6;
				_ecx->punsigned_int18 = v6 + 1;
			}
			else {
				_ecx->byte4 = 1;
				v7 = 0;
			}
			v5 = 32;
		}
	}
	else {
		v7 = _ecx->dword10 >> 1;
	}
	_ecx->dword10 = v7;
	v8 = v7 & 1;
	v9 = v5 - 1;
	_ecx->dword14 = v9;
	if( v9 ) {
		v11 = v7 >> 1;
	}
	else {
		v10 = _ecx->punsigned_int18;
		_ecx->dword14 = 32;
		if( v10 == _ecx->field_1C ) {
			_ecx->dword14 = 1;
			_ecx->punsigned_int18 = v10 + 1;
			v11 = 0;
			v9 = 1;
		}
		else {
			if( v10 <= _ecx->field_1C ) {
				v11 = *v10;
				_ecx->punsigned_int18 = v10 + 1;
			}
			else {
				_ecx->byte4 = 1;
				v11 = 0;
			}
			v9 = 32;
		}
	}
	_ecx->dword10 = v11;
	if( v26 || v8 ) {
		v25 = v11 & 1;
		v12 = v9 - 1;
		v30 = v12;
		_ecx->dword14 = v12;
		if( v12 ) {
			v14 = v11 >> 1;
			v4 = v12;
		}
		else {
			v13 = _ecx->punsigned_int18;
			_ecx->dword14 = 32;
			if( v13 == _ecx->field_1C ) {
				_ecx->dword14 = 1;
				_ecx->punsigned_int18 = v13 + 1;
				v14 = 0;
				v12 = 1;
				v4 = 1;
				v30 = 1;
			}
			else if( v13 <= _ecx->field_1C ) {
				v14 = *v13;
				_ecx->punsigned_int18 = v13 + 1;
				v12 = 32;
				v30 = 32;
			}
			else {
				v12 = 32;
				_ecx->byte4 = 1;
				v30 = 32;
				v14 = 0;
			}
		}
		_ecx->dword10 = v14;
		if( !v26 ) {
		LABEL_47:
			if( !v8 )
				goto LABEL_66;
			if( v4 < 5 ) {
				v22 = _ecx->punsigned_int18;
				v23 = 5 - v4;
				if( v22 == _ecx->field_1C ) {
					_ecx->dword14 = 1;
					_ecx->punsigned_int18 = v22 + 1;
					LOBYTE( v4 ) = 1;
					_ecx->byte4 = 1;
					v24 = 0;
				}
				else if( v22 <= _ecx->field_1C ) {
					v24 = *v22;
					++_ecx->punsigned_int18;
					v2 = _ecx->byte4;
				}
				else {
					_ecx->byte4 = 1;
					v24 = 0;
				}
				_ecx->dword10 = v24;
				if( v2 ) {
					v8 = 0;
				}
				else {
					_ecx->dword14 = 32 - v23;
					_ecx->dword10 = v24 >> v23;
					v8 = v14 | ( ( v24 & esoterik_array_two[ v23 ] ) << v4 );
				}
				goto LABEL_66;
			}
			v8 = v14 & 0x1F;
			_ecx->dword14 = v4 - 5;
			if( v4 != 5 ) {
				v19 = v14 >> 5;
			LABEL_57:
				_ecx->dword10 = v19;
			LABEL_66:
				result = ( float )v8 * 0.03125 + ( double )v26;
				if( v25 )
					result = -result;
				return result;
			}
			v20 = _ecx->punsigned_int18;
			_ecx->dword14 = 32;
			if( v20 == _ecx->field_1C ) {
				v21 = v20 + 1;
				_ecx->dword14 = 1;
				v19 = 0;
			}
			else {
				if( v20 > _ecx->field_1C ) {
					_ecx->byte4 = 1;
					v19 = 0;
					goto LABEL_57;
				}
				v19 = *v20;
				v21 = v20 + 1;
			}
			_ecx->punsigned_int18 = v21;
			goto LABEL_57;
		}
		if( v12 < 14 ) {
			v27 = 14 - v12;
			v17 = _ecx->punsigned_int18;
			if( v17 == _ecx->field_1C ) {
				_ecx->dword14 = 1;
				_ecx->punsigned_int18 = v17 + 1;
				v4 = 1;
				v18 = 0;
				_ecx->byte4 = 1;
				v29 = 0;
				v30 = 1;
				v31 = 1;
			}
			else if( v17 <= _ecx->field_1C ) {
				v18 = *v17;
				++_ecx->punsigned_int18;
				v29 = v18;
				v31 = _ecx->byte4;
			}
			else {
				v18 = 0;
				_ecx->byte4 = 1;
				v29 = 0;
				v31 = 1;
			}
			_ecx->dword10 = v18;
			if( v31 ) {
				v28 = 0;
			}
			else {
				v18 >>= v27;
				v4 = 32 - v27;
				_ecx->dword10 = v18;
				v28 = v14 | ( ( esoterik_array_two[ v27 ] & v29 ) << v30 );
				_ecx->dword14 = 32 - v27;
			}
			v14 = v18;
			goto LABEL_46;
		}
		v28 = v14 & 0x3FFF;
		v4 = v12 - 14;
		_ecx->dword14 = v12 - 14;
		if( v12 != 14 ) {
			v14 >>= 14;
		LABEL_36:
			_ecx->dword10 = v14;
		LABEL_46:
			v26 = v28 + 1;
			goto LABEL_47;
		}
		v15 = _ecx->punsigned_int18;
		v4 = 32;
		_ecx->dword14 = 32;
		if( v15 == _ecx->field_1C ) {
			v16 = v15 + 1;
			_ecx->dword14 = 1;
			v14 = 0;
			v4 = 1;
		}
		else {
			if( v15 > _ecx->field_1C ) {
				_ecx->byte4 = 1;
				v14 = 0;
				goto LABEL_36;
			}
			v14 = *v15;
			v16 = v15 + 1;
		}
		_ecx->punsigned_int18 = v16;
		goto LABEL_36;
	}
	return result;
}

bool Main( voice_data *msg ) {
	int flags; // ecx
	int v8; // edi
	unsigned int *v9; // edx
	unsigned int v10; // esi
	unsigned __int16 v11; // ax
	unsigned int *v12; // ecx
	int v13; // eax
	unsigned int v14; // eax
	unsigned int v15; // esi
	unsigned int *v16; // edx
	unsigned int v17; // ecx
	float v18; // xmm0_4
	unsigned int v19; // xmm1_4
	int v20; // ecx
	unsigned int *v21; // edi
	unsigned int v22; // edx
	unsigned int *v23; // eax
	unsigned int v24; // esi
	int v25; // edx
	int v26; // eax
	int *v27; // esi
	float *v28; // edi
	SomeSharedESPData v30; // [esp-64h] [ebp-70h] BYREF
	float v31; // [esp-24h] [ebp-30h]
	int v32; // [esp-20h] [ebp-2Ch]
	unsigned int v33; // [esp-1Ch] [ebp-28h]
	unsigned int v34; // [esp-18h] [ebp-24h]
	float v35; // [esp-14h] [ebp-20h]
	int v36; // [esp-10h] [ebp-1Ch]
	int v37; // [esp-Ch] [ebp-18h]
	unsigned int v38; // [esp-8h] [ebp-14h]
	unsigned int *v39; // [esp-4h] [ebp-10h]
	int v40; // [esp+0h] [ebp-Ch]
	int v41; // [esp+4h] [ebp-8h]

	static auto sv_deadtalk = g_pCVar->FindVar( XorStr( "sv_deadtalk" ) );
	static auto sv_full_alltalk = g_pCVar->FindVar( XorStr( "sv_full_alltalk" ) );

	v35 = *( float * )&msg;
	if( !sv_deadtalk->GetBool( ) && !sv_full_alltalk->GetBool( ) ) {
		return 0;
	}

	flags = msg->caster_flags;
	if( ( flags & 0x10 ) != 0 ) {
		if( msg->voice_data->size )
			return 0;
	}
	if( ( flags & 0x40 ) == 0 )
		return 0;
	if( msg->dword20 )
		return 0;
	if( ( flags & 0x185 ) != 0x185 )
		return 0;
	v30.punsigned_int1C._2 = msg->dword28;
	v30.punsigned_int1C._3 = msg->dword24;
	v30.punsigned_int1C._4 = msg->dword2C;
	v30.aaaaaa = *( _DWORD * )&msg->gapC[ 4 ];
	v30.aaaaaa_part2 = *( _DWORD * )&msg->gapC[ 8 ];
	v30.punsigned_int1C._1 = ( int )&v30.punsigned_int1C._2;
	v30.punsigned_int18 = ( unsigned int * )&v30.punsigned_int1C._2;
	v30.gap0 = 0;
	v30.field_C = 0x14;
	v30.field_8 = 0xA0;
	v30.byte4 = 0;
	v30.field_1C = &v30.dlwsakd;
	setup_esp_data( &v30, 0x185 );
	v8 = v30.dword14;
	v9 = v30.punsigned_int18;
	if( ( int )v30.dword14 >= 16 ) {
		v8 = v30.dword14 - 16;
		v30.dword14 = v8;
		if( v8 ) {
			v10 = HIWORD( v30.dword10 );
		LABEL_26:
			v11 = v30.dword10;
		LABEL_34:
			v12 = v30.field_1C;
			goto LABEL_35;
		}
		v8 = 32;
		v30.dword14 = 32;
		if( v30.punsigned_int18 == v30.field_1C ) {
			v9 = v30.punsigned_int18 + 1;
			v10 = 0;
			v8 = 1;
			v30.dword14 = 1;
		}
		else {
			if( v30.punsigned_int18 > v30.field_1C ) {
				v10 = 0;
				v30.byte4 = 1;
				goto LABEL_26;
			}
			v10 = *v30.punsigned_int18;
			v9 = v30.punsigned_int18 + 1;
		}
		v30.punsigned_int18 = v9;
		goto LABEL_26;
	}
	v12 = v30.field_1C;
	v33 = 16 - v30.dword14;
	if( v30.punsigned_int18 == v30.field_1C ) {
		v10 = 0;
		v9 = v30.punsigned_int18 + 1;
		v8 = 1;
		++v30.punsigned_int18;
		v30.dword14 = 1;
		v30.byte4 = 1;
	}
	else if( v30.punsigned_int18 <= v30.field_1C ) {
		v10 = *v30.punsigned_int18;
		v9 = ++v30.punsigned_int18;
		if( !v30.byte4 ) {
			v34 = ( ( v10 & esoterik_array_two[ 16 - v30.dword14 ] ) << SLOBYTE( v30.dword14 ) ) | v30.dword10;
			v11 = v34;
			v8 = 32 - v33;
			v10 >>= v33;
			v30.dword14 = 32 - v33;
			goto LABEL_34;
		}
	}
	else {
		v10 = 0;
		v30.byte4 = 1;
	}
	v11 = 0;
LABEL_35:
	v34 = v11;
	v13 = v11 - 0xBEED;
	if( !v13 || v13 == 2 ) {
		v33 = *( _DWORD * )( LODWORD( v35 ) + 8 );
		if( ( WORD )v34 != 0xBEEF )
			return 1;

		if( v8 < 7 ) {
			v33 = 7 - v8;
			if( v9 == v12 ) {
				v30.dword10 = 0;
				v30.dword14 = 1;
				v30.punsigned_int18 = v9 + 1;
				v30.byte4 = 1;
			}
			else if( v9 <= v12 ) {
				v17 = *v9;
				v34 = v17;
				v30.dword10 = v17;
				v30.punsigned_int18 = v9 + 1;
				if( !v30.byte4 ) {
					LODWORD( v35 ) = ( ( v17 & esoterik_array_two[ 7 - v8 ] ) << v8 ) | v10;
					v30.dword14 = 32 - v33;
					v30.dword10 = v34 >> v33;
					v14 = LODWORD( v35 );
					goto LABEL_59;
				}
			}
			else {
				v30.dword10 = 0;
				v30.byte4 = 1;
			}
			v14 = 0;
		LABEL_59:
			v33 = v14 + 1;
			if( v14 > 0x3F )
				return 1;
			v35 = sub_6ACAF3F1( &v30 );
			*( float * )&v34 = sub_6ACAF3F1( &v30 );
			*( float * )&v30.dlwsakd = v35;
			*( &v30.dlwsakd + 1 ) = v34;
			v18 = sub_6ACAF3F1( &v30 );
			v31 = v18;
			v19 = v34;
			*( float * )&v34 = v35;
			v20 = 0x7F800000;
			if( ( LODWORD( v35 ) & 0x7F800000 ) == 0x7F800000 )
				return 1;
			if( v35 <= -16384.0 )
				return 1;
			if( v35 >= 16384.0 )
				return 1;
			v34 = v19;
			if( ( v19 & 0x7F800000 ) == 0x7F800000 )
				return 1;
			if( *( float * )&v19 <= -16384.0 )
				return 1;
			if( *( float * )&v19 >= 16384.0 )
				return 1;
			*( float * )&v34 = v18;
			if( ( LODWORD( v18 ) & 0x7F800000 ) == 0x7F800000 || v18 <= -16384.0 || v18 >= 16384.0 )
				return 1;
			v21 = v30.punsigned_int18;
			v37 = v30.dword14;
			if( ( int )v30.dword14 >= 32 ) {
				v35 = *( float * )&v30.dword10;
				v37 = v30.dword14 - 32;
				if( v30.dword14 == 32 ) {
					v37 = 32;
					v23 = v30.field_1C;
					if( v30.punsigned_int18 != v30.field_1C ) {
						if( v30.punsigned_int18 <= v30.field_1C ) {
							LOBYTE( v23 ) = v30.byte4;
							v22 = *v30.punsigned_int18;
							v21 = v30.punsigned_int18 + 1;
							v39 = v23;
						}
						else {
							v22 = 0;
							LOBYTE( v39 ) = 1;
						}
						goto LABEL_77;
					}
					v21 = v30.punsigned_int18 + 1;
					v37 = 1;
				}
				LOBYTE( v20 ) = v30.byte4;
				v22 = 0;
				v39 = ( unsigned int * )v20;
			LABEL_77:
				v38 = v22;
				v36 = v22;
				goto LABEL_86;
			}
			v34 = 32 - v30.dword14;
			if( v30.punsigned_int18 == v30.field_1C ) {
				v21 = v30.punsigned_int18 + 1;
				LOBYTE( v39 ) = 1;
				v37 = 1;
			}
			else {
				if( v30.punsigned_int18 <= v30.field_1C ) {
					v24 = *v30.punsigned_int18;
					v21 = v30.punsigned_int18 + 1;
					LOBYTE( v20 ) = v30.byte4;
					v38 = v24;
					v36 = v24;
					v39 = ( unsigned int * )v20;
					if( !v30.byte4 ) {
						LODWORD( v35 ) = ( ( v24 & esoterik_array_two[ v34 ] ) << SLOBYTE( v30.dword14 ) ) | v30.dword10;
						v37 = 32 - v34;
						v38 = v24 >> v34;
						v36 = v24 >> v34;
					LABEL_86:
						auto asdsad = ( int )abs( LODWORD( v35 ) - g_pEngine->GetServerTick( ) );
						auto dasdsad = ( int )( float )( ( float )( 1.0 / g_pGlobalVars->interval_per_tick ) + 0.5 );

						// NOTE - MICHAL
						// TODO, FIX THIS!!!!!!!!!!!!!! (idk if it will cause issues)
						if( /*asdsad <= dasdsad*/true ) {
							if( v37 >= 7 ) {
								v25 = v36 & 0x7F;
								if( v37 == 7 && v21 != v30.field_1C ) {
									v26 = ( unsigned __int8 )v39;
									if( v21 > v30.field_1C )
										v26 = 1;
									v39 = ( unsigned int * )v26;
								}
								goto LABEL_99;
							}
							if( v21 == v30.field_1C || v21 > v30.field_1C ) {
								LOBYTE( v39 ) = 1;
							}
							else if( !( bool )v39 ) {
								v25 = v38 | ( ( *v21 & esoterik_array_two[ 7 - v37 ] ) << v37 );
							LABEL_99:
								if( ( unsigned int )( v25 - 1 ) <= 124 && !( bool )v39 ) {
									if( v33 > 0 && v33 <= 64 ) {

										auto local = C_CSPlayer::GetLocalPlayer( );

										if( local != nullptr ) {
											Vector origin = Vector( *( float * )&v30.dlwsakd, *( float * )&v19, v31 );
											if( !origin.IsZero( ) ) {
												g_ExtendedVisuals.m_arrOverridePlayers.at( v33 ).m_eOverrideType = EOverrideType::ESP_SHARED;
												g_ExtendedVisuals.m_arrOverridePlayers.at( v33 ).m_vecLastOrigin = g_ExtendedVisuals.m_arrOverridePlayers.at( v33 ).m_vecOrigin;
												g_ExtendedVisuals.m_arrOverridePlayers.at( v33 ).m_vecOrigin = origin;
												g_ExtendedVisuals.m_arrOverridePlayers.at( v33 ).m_flReceiveTime = g_pGlobalVars->realtime;
												// g_Resolver.m_arrResolverData.at( v33 ).m_nProxyDetectCount++;
												// printf( "receive (skeet): %d - x(%.f), y(%.f), z(%.f)\n", v33, origin.x, origin.y, origin.z );
											}
											// g_pDebugOverlay->AddBoxOverlay( g_ExtendedVisuals.m_arrOverridePlayers.at( player->EntIndex( ) ).m_vecOrigin, { -5,-5,-5 }, { 5,5,5 }, {}, 0, 255, 0, 255, 0.1f );
										}
									}
								}
								return 1;
							}
							v25 = 0;
							goto LABEL_99;
						}
						return 1;
					}
				LABEL_84:
					v35 = 0.0;
					goto LABEL_86;
				}
				LOBYTE( v39 ) = 1;
			}
			v38 = 0;
			v36 = 0;
			goto LABEL_84;
		}
		v14 = v10 & 0x7F;
		v30.dword14 = v8 - 7;
		if( v8 != 7 ) {
			v15 = v10 >> 7;
		LABEL_51:
			v30.dword10 = v15;
			goto LABEL_59;
		}
		v30.dword14 = 32;
		if( v9 == v12 ) {
			v16 = v9 + 1;
			v30.dword14 = 1;
			v15 = 0;
		}
		else {
			if( v9 > v12 ) {
				v30.byte4 = 1;
				v15 = 0;
				goto LABEL_51;
			}
			v15 = *v9;
			v16 = v9 + 1;
		}
		v30.punsigned_int18 = v16;
		goto LABEL_51;
	}
	return 0;
}

bool GSInterpreter::ReceiveSharedPackets( voice_data *msg ) {
	return Main( msg );
}