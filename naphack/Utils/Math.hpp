#pragma once
#include "../SDK/sdk.hpp"
#include <DirectXMath.h>

#define RAD2DEG(x) DirectX::XMConvertToDegrees(x)
#define DEG2RAD(x) DirectX::XMConvertToRadians(x)

static const float invtwopi = 0.1591549f;
static const float twopi = 6.283185f;
static const float threehalfpi = 4.7123889f;
static const float pi = 3.141593f;
static const float halfpi = 1.570796f;
static const __m128 signmask = _mm_castsi128_ps( _mm_set1_epi32( 0x80000000 ) );

static const __declspec( align( 16 ) ) float null[ 4 ] = { 0.f, 0.f, 0.f, 0.f };
static const __declspec( align( 16 ) ) float _pi2[ 4 ] = { 1.5707963267948966192f, 1.5707963267948966192f, 1.5707963267948966192f, 1.5707963267948966192f };
static const __declspec( align( 16 ) ) float _pi[ 4 ] = { 3.141592653589793238f, 3.141592653589793238f, 3.141592653589793238f, 3.141592653589793238f };

typedef __declspec( align( 16 ) ) union {
	float f[ 4 ];
	__m128 v;
} m128;

__forceinline __m128 sqrt_ps( const __m128 squared ) {
	return _mm_sqrt_ps( squared );
}

__forceinline __m128 cos_52s_ps( const __m128 x ) {
	const auto c1 = _mm_set1_ps( 0.9999932946f );
	const auto c2 = _mm_set1_ps( -0.4999124376f );
	const auto c3 = _mm_set1_ps( 0.0414877472f );
	const auto c4 = _mm_set1_ps( -0.0012712095f );
	const auto x2 = _mm_mul_ps( x, x );
	return _mm_add_ps( c1, _mm_mul_ps( x2, _mm_add_ps( c2, _mm_mul_ps( x2, _mm_add_ps( c3, _mm_mul_ps( c4, x2 ) ) ) ) ) );
}

__forceinline __m128 cos_ps( __m128 angle ) {
	angle = _mm_andnot_ps( signmask, angle );
	angle = _mm_sub_ps( angle, _mm_mul_ps( _mm_cvtepi32_ps( _mm_cvttps_epi32( _mm_mul_ps( angle, _mm_set1_ps( invtwopi ) ) ) ), _mm_set1_ps( twopi ) ) );

	auto cosangle = angle;
	cosangle = _mm_xor_ps( cosangle, _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( halfpi ) ), _mm_xor_ps( cosangle, _mm_sub_ps( _mm_set1_ps( pi ), angle ) ) ) );
	cosangle = _mm_xor_ps( cosangle, _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( pi ) ), signmask ) );
	cosangle = _mm_xor_ps( cosangle, _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( threehalfpi ) ), _mm_xor_ps( cosangle, _mm_sub_ps( _mm_set1_ps( twopi ), angle ) ) ) );

	auto result = cos_52s_ps( cosangle );
	result = _mm_xor_ps( result, _mm_and_ps( _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( halfpi ) ), _mm_cmplt_ps( angle, _mm_set1_ps( threehalfpi ) ) ), signmask ) );
	return result;
}

__forceinline __m128 sin_ps( const __m128 angle ) {
	return cos_ps( _mm_sub_ps( _mm_set1_ps( halfpi ), angle ) );
}

__forceinline void sincos_ps( __m128 angle, __m128 *sin, __m128 *cos ) {
	const auto anglesign = _mm_or_ps( _mm_set1_ps( 1.f ), _mm_and_ps( signmask, angle ) );
	angle = _mm_andnot_ps( signmask, angle );
	angle = _mm_sub_ps( angle, _mm_mul_ps( _mm_cvtepi32_ps( _mm_cvttps_epi32( _mm_mul_ps( angle, _mm_set1_ps( invtwopi ) ) ) ), _mm_set1_ps( twopi ) ) );

	auto cosangle = angle;
	cosangle = _mm_xor_ps( cosangle, _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( halfpi ) ), _mm_xor_ps( cosangle, _mm_sub_ps( _mm_set1_ps( pi ), angle ) ) ) );
	cosangle = _mm_xor_ps( cosangle, _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( pi ) ), signmask ) );
	cosangle = _mm_xor_ps( cosangle, _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( threehalfpi ) ), _mm_xor_ps( cosangle, _mm_sub_ps( _mm_set1_ps( twopi ), angle ) ) ) );

	auto result = cos_52s_ps( cosangle );
	result = _mm_xor_ps( result, _mm_and_ps( _mm_and_ps( _mm_cmpge_ps( angle, _mm_set1_ps( halfpi ) ), _mm_cmplt_ps( angle, _mm_set1_ps( threehalfpi ) ) ), signmask ) );
	*cos = result;

	const auto sinmultiplier = _mm_mul_ps( anglesign, _mm_or_ps( _mm_set1_ps( 1.f ), _mm_and_ps( _mm_cmpgt_ps( angle, _mm_set1_ps( pi ) ), signmask ) ) );
	*sin = _mm_mul_ps( sinmultiplier, sqrt_ps( _mm_sub_ps( _mm_set1_ps( 1.f ), _mm_mul_ps( result, result ) ) ) );
}


namespace Math {
	bool IntersectSegmentToSegment( Vector s1, Vector s2, Vector k1, Vector k2, float radius );
	bool IntersectSegmentSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, const Vector &vecSphereCenter, float radius );
	float segment_to_segment( const Vector &p1, const Vector &p2, const Vector &q1, const Vector &q2, float &invariant1, float &invariant2 );
	bool IntersectSegmentCapsule( const Vector &start, const Vector &end, const Vector &min, const Vector &max, float radius );
	bool IntersectionBoundingBox( const Vector &start, const Vector &dir, const Vector &min, const Vector &max, Vector *hit_point = nullptr );

	void Rotate( std::array<Vector2D, 3> &points, float rotation );

	int RoundToMultiple( int in, int multiple );

	void AngleVectors( const QAngle &angles, Vector &forward, Vector &right, Vector &up );

	void VectorAngles( const Vector &forward, Vector &angles );

	void SinCos( float a, float *s, float *c );

	void AngleVectors( const QAngle &angles, Vector &forward );

	float GetFov( const QAngle &viewAngle, const Vector &start, const Vector &end );

	float AngleNormalize( float angle );

	float ApproachAngle( float target, float value, float speed );

	void VectorTransform( const Vector &in1, const matrix3x4_t &in2, Vector &out );

	__forceinline static bool intersect_line_with_bb( Vector &start, Vector &end, Vector &min, Vector &max ) {
		float d1, d2, f;
		auto start_solid = true;
		auto t1 = -1.0f, t2 = 1.0f;

		const float s[ 3 ] = { start.x, start.y, start.z };
		const float e[ 3 ] = { end.x, end.y, end.z };
		const float mi[ 3 ] = { min.x, min.y, min.z };
		const float ma[ 3 ] = { max.x, max.y, max.z };

		for( auto i = 0; i < 6; i++ ) {
			if( i >= 3 ) {
				const auto j = i - 3;

				d1 = s[ j ] - ma[ j ];
				d2 = d1 + e[ j ];
			}
			else {
				d1 = -s[ i ] + mi[ i ];
				d2 = d1 - e[ i ];
			}

			if( d1 > 0.0f && d2 > 0.0f )
				return false;

			if( d1 <= 0.0f && d2 <= 0.0f )
				continue;

			if( d1 > 0 )
				start_solid = false;

			if( d1 > d2 ) {
				f = d1;
				if( f < 0.0f )
					f = 0.0f;

				f /= d1 - d2;
				if( f > t1 )
					t1 = f;
			}
			else {
				f = d1 / ( d1 - d2 );
				if( f < t2 )
					t2 = f;
			}
		}

		return start_solid || ( t1 < t2 &&t1 >= 0.0f );
	}


	__forceinline static void vector_i_transform( const Vector &in1, const matrix3x4_t &in2, Vector &out ) {
		out.x = ( in1.x - in2[ 0 ][ 3 ] ) * in2[ 0 ][ 0 ] + ( in1.y - in2[ 1 ][ 3 ] ) * in2[ 1 ][ 0 ] + ( in1.z - in2[ 2 ][ 3 ] ) * in2[ 2 ][ 0 ];
		out.y = ( in1.x - in2[ 0 ][ 3 ] ) * in2[ 0 ][ 1 ] + ( in1.y - in2[ 1 ][ 3 ] ) * in2[ 1 ][ 1 ] + ( in1.z - in2[ 2 ][ 3 ] ) * in2[ 2 ][ 1 ];
		out.z = ( in1.x - in2[ 0 ][ 3 ] ) * in2[ 0 ][ 2 ] + ( in1.y - in2[ 1 ][ 3 ] ) * in2[ 1 ][ 2 ] + ( in1.z - in2[ 2 ][ 3 ] ) * in2[ 2 ][ 2 ];
	}

	__forceinline static void vector_i_rotate( const Vector &in1, const matrix3x4_t &in2, Vector &out ) {
		out.x = in1.x * in2[ 0 ][ 0 ] + in1.y * in2[ 1 ][ 0 ] + in1.z * in2[ 2 ][ 0 ];
		out.y = in1.x * in2[ 0 ][ 1 ] + in1.y * in2[ 1 ][ 1 ] + in1.z * in2[ 2 ][ 1 ];
		out.z = in1.x * in2[ 0 ][ 2 ] + in1.y * in2[ 1 ][ 2 ] + in1.z * in2[ 2 ][ 2 ];
	}

	void SmoothAngle( QAngle src, QAngle &dst, float factor );

	QAngle CalcAngle( Vector src, Vector dst, bool bruh = false );

	Vector GetSmoothedVelocity( float min_delta, Vector a, Vector b );

	float AngleDiff( float src, float dst );

	float SmoothStepBounds( float edge0, float edge1, float x );

	float ClampCycle( float flCycleIn );

	float Approach( float target, float value, float speed );

	float Bias( float x, float biasAmt );

	float RemapValClamped( float val, float A, float B, float C, float D );

	__forceinline static matrix3x4_t angle_matrix( const QAngle angles ) {
		matrix3x4_t result{};

		m128 angle, sin, cos;
		angle.f[ 0 ] = DEG2RAD( angles.x );
		angle.f[ 1 ] = DEG2RAD( angles.y );
		angle.f[ 2 ] = DEG2RAD( angles.z );
		sincos_ps( angle.v, &sin.v, &cos.v );

		result[ 0 ][ 0 ] = cos.f[ 0 ] * cos.f[ 1 ];
		result[ 1 ][ 0 ] = cos.f[ 0 ] * sin.f[ 1 ];
		result[ 2 ][ 0 ] = -sin.f[ 0 ];

		const auto crcy = cos.f[ 2 ] * cos.f[ 1 ];
		const auto crsy = cos.f[ 2 ] * sin.f[ 1 ];
		const auto srcy = sin.f[ 2 ] * cos.f[ 1 ];
		const auto srsy = sin.f[ 2 ] * sin.f[ 1 ];

		result[ 0 ][ 1 ] = sin.f[ 0 ] * srcy - crsy;
		result[ 1 ][ 1 ] = sin.f[ 0 ] * srsy + crcy;
		result[ 2 ][ 1 ] = sin.f[ 2 ] * cos.f[ 0 ];

		result[ 0 ][ 2 ] = sin.f[ 0 ] * crcy + srsy;
		result[ 1 ][ 2 ] = sin.f[ 0 ] * crsy - srcy;
		result[ 2 ][ 2 ] = cos.f[ 2 ] * cos.f[ 0 ];

		return result;
	}

	__forceinline static Vector vector_rotate( const Vector &in1, const matrix3x4_t &in2 ) {
		return Vector( in1.Dot( in2[ 0 ] ), in1.Dot( in2[ 1 ] ), in1.Dot( in2[ 2 ] ) );
	}

	__forceinline static Vector vector_rotate( const Vector &in1, const QAngle &in2 ) {
		const auto matrix = angle_matrix( in2 );
		return vector_rotate( in1, matrix );
	}

	__forceinline float DotProductXD( const float *v1, const float *v2 ) {
		return v1[ 0 ] * v2[ 0 ] + v1[ 1 ] * v2[ 1 ] + v1[ 2 ] * v2[ 2 ];
	}

	__forceinline void VectorRotate( const float *in1, const matrix3x4_t &in2, float *out ) {
		out[ 0 ] = DotProductXD( in1, in2[ 0 ] );
		out[ 1 ] = DotProductXD( in1, in2[ 1 ] );
		out[ 2 ] = DotProductXD( in1, in2[ 2 ] );
	}

	__forceinline void VectorRotate( const Vector &in1, const matrix3x4_t &in2, Vector &out ) {
		VectorRotate( &in1.x, in2, &out.x );
	}

	__forceinline void AngleMatrix( const QAngle angles, matrix3x4_t &matrix ) {
		float sr, sp, sy, cr, cp, cy;

		sy = sin( DEG2RAD( angles[ 1 ] ) );
		cy = cos( DEG2RAD( angles[ 1 ] ) );

		sp = sin( DEG2RAD( angles[ 0 ] ) );
		cp = cos( DEG2RAD( angles[ 0 ] ) );

		sr = sin( DEG2RAD( angles[ 2 ] ) );
		cr = cos( DEG2RAD( angles[ 2 ] ) );

		//matrix = (YAW * PITCH) * ROLL
		matrix[ 0 ][ 0 ] = cp * cy;
		matrix[ 1 ][ 0 ] = cp * sy;
		matrix[ 2 ][ 0 ] = -sp;

		float crcy = cr * cy;
		float crsy = cr * sy;
		float srcy = sr * cy;
		float srsy = sr * sy;

		matrix[ 0 ][ 1 ] = sp * srcy - crsy;
		matrix[ 1 ][ 1 ] = sp * srsy + crcy;
		matrix[ 2 ][ 1 ] = sr * cp;

		matrix[ 0 ][ 2 ] = ( sp * crcy + srsy );
		matrix[ 1 ][ 2 ] = ( sp * crsy - srcy );
		matrix[ 2 ][ 2 ] = cr * cp;

		matrix[ 0 ][ 3 ] = 0.0f;
		matrix[ 1 ][ 3 ] = 0.0f;
		matrix[ 2 ][ 3 ] = 0.0f;
	}

	__forceinline void MatrixSetColumn( const Vector &in, int column, matrix3x4_t &out ) {
		out[ 0 ][ column ] = in.x;
		out[ 1 ][ column ] = in.y;
		out[ 2 ][ column ] = in.z;
	}

	__forceinline void AngleMatrix( const QAngle &angles, const Vector &position, matrix3x4_t &matrix_out ) {
		AngleMatrix( angles, matrix_out );
		MatrixSetColumn( position, 3, matrix_out );
	}

	__forceinline void VectorRotate( const Vector &in1, const QAngle &in2, Vector &out ) {
		matrix3x4_t matRotate;
		AngleMatrix( in2, matRotate );
		VectorRotate( in1, matRotate, out );
	}

	__forceinline void MatrixCopy( const matrix3x4_t &source, matrix3x4_t &target ) {
		for( int i = 0; i < 3; i++ ) {
			for( int j = 0; j < 4; j++ ) {
				target[ i ][ j ] = source[ i ][ j ];
			}
		}
	}

	__forceinline void MatrixMultiply( matrix3x4_t &in1, const matrix3x4_t &in2 ) {
		matrix3x4_t out;
		if( &in1 == &out ) {
			matrix3x4_t in1b;
			MatrixCopy( in1, in1b );
			MatrixMultiply( in1b, in2 );
			return;
		}
		if( &in2 == &out ) {
			matrix3x4_t in2b;
			MatrixCopy( in2, in2b );
			MatrixMultiply( in1, in2b );
			return;
		}
		out[ 0 ][ 0 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 0 ] +
			in1[ 0 ][ 2 ] * in2[ 2 ][ 0 ];
		out[ 0 ][ 1 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 1 ] +
			in1[ 0 ][ 2 ] * in2[ 2 ][ 1 ];
		out[ 0 ][ 2 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 2 ] +
			in1[ 0 ][ 2 ] * in2[ 2 ][ 2 ];
		out[ 0 ][ 3 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 3 ] +
			in1[ 0 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 0 ][ 3 ];
		out[ 1 ][ 0 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 0 ] +
			in1[ 1 ][ 2 ] * in2[ 2 ][ 0 ];
		out[ 1 ][ 1 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 1 ] +
			in1[ 1 ][ 2 ] * in2[ 2 ][ 1 ];
		out[ 1 ][ 2 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 2 ] +
			in1[ 1 ][ 2 ] * in2[ 2 ][ 2 ];
		out[ 1 ][ 3 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 3 ] +
			in1[ 1 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 1 ][ 3 ];
		out[ 2 ][ 0 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 0 ] +
			in1[ 2 ][ 2 ] * in2[ 2 ][ 0 ];
		out[ 2 ][ 1 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 1 ] +
			in1[ 2 ][ 2 ] * in2[ 2 ][ 1 ];
		out[ 2 ][ 2 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 2 ] +
			in1[ 2 ][ 2 ] * in2[ 2 ][ 2 ];
		out[ 2 ][ 3 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 3 ] +
			in1[ 2 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 2 ][ 3 ];

		in1 = out;
	}

	__forceinline void ConcatTransforms( const matrix3x4_t &in1, const matrix3x4_t &in2, matrix3x4_t &out ) {
		if( &in1 == &out ) {
			matrix3x4_t in1b;
			MatrixCopy( in1, in1b );
			ConcatTransforms( in1b, in2, out );
			return;
		}

		if( &in2 == &out ) {
			matrix3x4_t in2b;
			MatrixCopy( in2, in2b );
			ConcatTransforms( in1, in2b, out );
			return;
		}

		out[ 0 ][ 0 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 0 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 0 ];
		out[ 0 ][ 1 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 1 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 1 ];
		out[ 0 ][ 2 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 2 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 2 ];
		out[ 0 ][ 3 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 3 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 0 ][ 3 ];

		out[ 1 ][ 0 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 0 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 0 ];
		out[ 1 ][ 1 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 1 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 1 ];
		out[ 1 ][ 2 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 2 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 2 ];
		out[ 1 ][ 3 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 3 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 1 ][ 3 ];

		out[ 2 ][ 0 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 0 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 0 ];
		out[ 2 ][ 1 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 1 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 1 ];
		out[ 2 ][ 2 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 2 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 2 ];
		out[ 2 ][ 3 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 3 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 3 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 3 ] + in1[ 2 ][ 3 ];
	}

	__forceinline static float Interpolate( const float from, const float to, const float percent ) {
		return to * percent + from * ( 1.f - percent );
	}

	// Returns A + (B-A)*flPercent.
	// float Lerp( float flPercent, float A, float B );
	template <class T>
	__forceinline T Lerp( float flPercent, T const &A, T const &B ) {
		return A + ( B - A ) * flPercent;
	}


	__forceinline static Vector Interpolate( const Vector from, const Vector to, const float percent ) {
		return to * percent + from * ( 1.f - percent );
	}

	__forceinline static void MatrixSetOrigin( Vector pos, matrix3x4_t &matrix ) {
		matrix[ 0 ][ 3 ] = pos.x;
		matrix[ 1 ][ 3 ] = pos.y;
		matrix[ 2 ][ 3 ] = pos.z;
	}

	__forceinline static Vector MatrixGetOrigin( const matrix3x4_t &src ) {
		return { src[ 0 ][ 3 ], src[ 1 ][ 3 ], src[ 2 ][ 3 ] };
	}

	__forceinline void VectorScale( const float *in, float scale, float *out ) {
		out[ 0 ] = in[ 0 ] * scale;
		out[ 1 ] = in[ 1 ] * scale;
		out[ 2 ] = in[ 2 ] * scale;
	}

	struct CapsuleCollider {
		Vector min;
		Vector max;
		float radius;

		bool Intersect( const Vector &a, const Vector &b ) const;
	};

	// mixed types involved.
	template < typename T >
	T Clamp( const T &val, const T &minVal, const T &maxVal ) {
		if( ( T )val < minVal )
			return minVal;
		else if( ( T )val > maxVal )
			return maxVal;
		else
			return val;
	}

	template < typename T >
	T Hermite_Spline(
		T p1,
		T p2,
		T d1,
		T d2,
		float t ) {
		float tSqr = t * t;
		float tCube = t * tSqr;

		float b1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
		float b2 = 1.0f - b1; // -2*tCube+3*tSqr;
		float b3 = tCube - 2 * tSqr + t;
		float b4 = tCube - tSqr;

		T output;
		output = p1 * b1;
		output += p2 * b2;
		output += d1 * b3;
		output += d2 * b4;

		return output;
	}

	template < typename T >
	T Hermite_Spline( T p0, T p1, T p2, float t ) {
		return Hermite_Spline( p1, p2, p1 - p0, p2 - p1, t );
	}

	// wide -> multi-byte
	__forceinline std::string WideToMultiByte( const std::wstring &str ) {
		std::string ret;
		int         str_len;

		// check if not empty str
		if( str.empty( ) )
			return { };

		// count size
		str_len = WideCharToMultiByte( CP_UTF8, 0, str.data( ), ( int )str.size( ), 0, 0, 0, 0 );

		// setup return value
		ret = std::string( str_len, 0 );

		// final conversion
		WideCharToMultiByte( CP_UTF8, 0, str.data( ), ( int )str.size( ), &ret[ 0 ], str_len, 0, 0 );

		return ret;
	}

	// multi-byte -> wide
	__forceinline std::wstring MultiByteToWide( const std::string &str ) {
		std::wstring    ret;
		int		        str_len;

		// check if not empty str
		if( str.empty( ) )
			return { };

		// count size
		str_len = MultiByteToWideChar( CP_UTF8, 0, str.data( ), ( int )str.size( ), nullptr, 0 );

		// setup return value
		ret = std::wstring( str_len, 0 );

		// final conversion
		MultiByteToWideChar( CP_UTF8, 0, str.data( ), ( int )str.size( ), &ret[ 0 ], str_len );

		return ret;
	}
}
