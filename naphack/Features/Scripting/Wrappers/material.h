#pragma once

#include "entity.h"
#include "../../Visuals/Models.hpp"

namespace Wrappers::Material {

	class LuaMaterialVar {
	public:
		LuaMaterialVar( IMaterialVar *pMaterialVar ) {
			m_pMaterialVar = pMaterialVar;
		}

		void SetIntValue( int value ) {
			if( !m_pMaterialVar )
				return;

			m_pMaterialVar->SetIntValue( value );
		}

		void SetFloatValue( float value ) {
			if( !m_pMaterialVar )
				return;

			m_pMaterialVar->SetFloatValue( value );
		}

		void SetStringValue( const char *value ) {
			if( !m_pMaterialVar || !value )
				return;

			m_pMaterialVar->SetStringValue( value );
		}

		void SetVecValue( int value1, int value2, int value3 ) {
			if( !m_pMaterialVar )
				return;

			m_pMaterialVar->SetVecValue( value1, value2, value3 );
		}

	private:
		IMaterialVar *m_pMaterialVar;
	};

	class LuaMaterial {
	public:
		LuaMaterial( IMaterial *pMaterial ) {
			m_pMaterial = pMaterial;
		}

		void ModulateColor( float flRed, float flGreen, float flBlue ) {
			if( !m_pMaterial )
				return;

			m_pMaterial->ColorModulate( flRed, flGreen, flBlue );
		}

		void ModulateAlpha( float flAlpha ) {
			if( !m_pMaterial )
				return;

			m_pMaterial->AlphaModulate( flAlpha );
		}

		void SetMaterialFlag( int flag, bool on ) {
			if( !m_pMaterial )
				return;

			m_pMaterial->SetMaterialVarFlag( MaterialVarFlags_t( flag ), on );
		}

		LuaMaterialVar FindMaterialVar( const char *varName ) {
			return LuaMaterialVar( m_pMaterial->FindVar( varName, nullptr ) );
		}

		bool IsValid( ) {
			return m_pMaterial != nullptr;
		}

		IMaterial *GetMaterial( ) {
			return m_pMaterial;
		}

	private:
		IMaterial *m_pMaterial;
	};

	class ModelDrawContext {
	public:
		ModelDrawContext( C_CSPlayer *pPlayer, std::string modelName, std::function<void( )> originalFunc ) {
			m_pPlayer = pPlayer;
			m_ModelName = modelName;
			m_OriginalDME = originalFunc;
		}

		Wrappers::Entity::CEntity GetEntity( ) {
			return Wrappers::Entity::CEntity( m_pPlayer );
		}

		std::string GetModelName( ) {
			return m_ModelName;
		}

		void DrawModel( ) {
			m_OriginalDME( );
		}

		void ForceMaterialOverride( LuaMaterial material ) {
			if( !material.IsValid( ) )
				return;

			g_pModelRender->ForcedMaterialOverride( material.GetMaterial( ) );
			g_pStudioRender->m_pForcedMaterial = material.GetMaterial( );
		}

	private:
		C_CSPlayer *m_pPlayer;
		std::string m_ModelName;
		std::function<void( )> m_OriginalDME;
	};

	__forceinline LuaMaterial CreateMaterial( const std::string &name, const std::string &vmt, const std::string &type ) {
		//const auto material = LuaMaterial( g_Chams.CreateMaterial( name, vmt, type ) );
		//return material;
		
		return nullptr;
	}

	__forceinline LuaMaterial FindMaterialNoGroup( const std::string &name ) {
		const auto pMaterial = g_pMaterialSystem->FindMaterial( name.data( ), nullptr );
		const auto material = LuaMaterial( pMaterial );

		return material;
	}
	__forceinline LuaMaterial FindMaterial( const std::string &name, const std::string &group ) {
		const auto pMaterial = g_pMaterialSystem->FindMaterial( name.data( ), group.data( ) );
		const auto material = LuaMaterial( pMaterial );

		return material;
	}
}