#include "vector2d.hpp"
#include "../variables.hpp"

Vector2D::Vector2D( float x, float y ) {
  this->Set( x, y );
}

Vector2D::Vector2D( const Vector2D& v ) {
  this->Set( v.x, v.y );
}

Vector2D::Vector2D( const float* v ) {
  this->Set( v[0], v[1] );
}

void Vector2D::Set( float x /*= 0.0f*/, float y /*= 0.0f */ ) {
  this->x = x;
  this->y = y;

  // almost all menu elements use a Vector2D for their pos/size
  //if( g_Vars.globals.m_bAllowDPIScale ) {
	 // switch( g_Vars.menu.dpi_menu % 5 ) {
		//  case 0:
		//	  break;
		//  case 1:
		//	  this->x *= 1.25f;
		//	  this->y *= 1.25f;
		//	  break;
		//  case 2:
		//	  this->x *= 1.5f;
		//	  this->y *= 1.5f;
		//	  break;
		//  case 3:
		//	  this->x *= 1.75f;
		//	  this->y *= 1.75f;
		//	  break;
		//  case 4:
		//	  this->x *= 2.f;
		//	  this->y *= 2.f;
		//	  break;
	 // }
  //}
}

float Vector2D::Dot( const Vector2D& v ) const {
  return ( this->x * v.x +
	 this->y * v.y );
}

float Vector2D::LengthSquared( ) const {
  return ( this->Dot( *this ) );
}

float Vector2D::Length( ) const {
  return ( std::sqrt( this->LengthSquared( ) ) );
}

float Vector2D::operator [] ( const std::uint32_t index ) {
  return ( ( ( float* )this )[index] );
}

const float Vector2D::operator [] ( const std::uint32_t index ) const {
  return ( ( ( const float* )this )[index] );
}

Vector2D& Vector2D::operator = ( const Vector2D& v ) {
  this->Set( v.x, v.y );

  return ( *this );
}

Vector2D& Vector2D::operator = ( const float* v ) {
  this->Set( v[0], v[1] );

  return ( *this );
}

Vector2D& Vector2D::operator += ( const Vector2D& v ) {
  this->x += v.x;
  this->y += v.y;

  return ( *this );
}

Vector2D& Vector2D::operator -= ( const Vector2D& v ) {
  this->x -= v.x;
  this->y -= v.y;

  return ( *this );
}

Vector2D& Vector2D::operator *= ( const Vector2D& v ) {
  this->x *= v.x;
  this->y *= v.y;

  return ( *this );
}

Vector2D& Vector2D::operator /= ( const Vector2D& v ) {
  this->x /= v.x;
  this->y /= v.y;

  return ( *this );
}

Vector2D& Vector2D::operator += ( float fl ) {
  this->x += fl;
  this->y += fl;

  return ( *this );
}

Vector2D& Vector2D::operator -= ( float fl ) {
  this->x -= fl;
  this->y -= fl;

  return ( *this );
}

Vector2D& Vector2D::operator *= ( float fl ) {
  this->x *= fl;
  this->y *= fl;

  return ( *this );
}

Vector2D& Vector2D::operator /= ( float fl ) {
  this->x /= fl;
  this->y /= fl;

  return ( *this );
}

bool Vector2D::operator<( const Vector2D& in ) const{
    return ( x < in.x&& y < in.y );
}

bool Vector2D::operator>( const Vector2D& in ) const{
    return ( x > in.x && y > in.y );
}

Vector2D Vector2D::operator + ( const Vector2D& v ) const {
  return { this->x + v.x,
			this->y + v.y };
}

Vector2D Vector2D::operator - ( const Vector2D& v ) const {
  return { this->x - v.x,
			this->y - v.y };
}

Vector2D Vector2D::operator * ( const Vector2D& v ) const {
  return { this->x * v.x,
			this->y * v.y };
}

Vector2D Vector2D::operator / ( const Vector2D& v ) const {
  return { this->x / v.x,
			this->y / v.y };
}

Vector2D Vector2D::operator + ( float fl ) const {
  return { this->x + fl,
			this->y + fl };
}

Vector2D Vector2D::operator - ( float fl ) const {
  return { this->x - fl,
			this->y - fl };
}

Vector2D Vector2D::operator * ( float fl ) const {
  return { this->x * fl,
			this->y * fl };
}

Vector2D Vector2D::operator / ( float fl ) const {
  return { this->x / fl,
			this->y / fl };
}

bool Vector2D::operator==( const Vector2D & v ) const {
  return v.x == x && v.y == y;
}

bool Vector2D::operator==( const float v ) const {
  return x == v && v == y;
}

bool Vector2D::operator!=( const Vector2D & v ) const {
  return v.x != x || v.y != y;
}

bool Vector2D::operator!=( const float v ) const {
  return x != v || v != y;
}

bool Vector2D::operator<( const Vector2D& v ) {
	return { this->x < v.x&&
		this->y < v.y };
}

bool Vector2D::operator>( const Vector2D& v ) {
	return { this->x > v.x &&
		this->y > v.y};
}

bool Vector2D::operator<=( const Vector2D& v ) {
	return { this->x <= v.x &&
		this->y <= v.y };
}

bool Vector2D::operator>=( const Vector2D& v ) {
	return { this->x >= v.x &&
		this->y >= v.y };
}
