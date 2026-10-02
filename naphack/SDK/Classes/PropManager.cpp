#include "PropManager.hpp"
#include "../../Loader/Security/Security.hpp"

#include "../../pandora.hpp"

namespace Engine
{

  PropManager g_PropManager;

  PropManager::~PropManager( ) {
	 this->Destroy( );
  }

  bool PropManager::Create( IBaseClientDLL* client ) {
	 if( !client ) {
#ifdef DEV
		Win32::Error( XorStr( "IBaseClientDLL is nullptr" ) );
#endif
		return false;
	 }

	 auto entry = client->GetAllClasses( );

	 if( !entry ) {
#ifdef DEV
		Win32::Error( XorStr( "ClientClass is nullptr" ) );
#endif
		return false;
	 }

	 while( entry ) {
		auto table = entry->m_pRecvTable;

		if( table )
		  this->m_pTables.emplace_back( table );

		entry = entry->m_pNext;
	 }

	 if( this->m_pTables.empty( ) ) {
#ifdef DEV
		Win32::Error( XorStr( "std::vector<RecvTable*> is empty" ) );
#endif
		return false;
	 }

	 g_protection.check_idk( );

	/* for ( auto clientclass = client->GetAllClasses( );
		   clientclass != nullptr;
		   clientclass = clientclass->m_pNext ) {

		if ( clientclass->m_pRecvTable ) {
		   database.emplace_back( LoadTable( clientclass->m_pRecvTable ) );
		}
	 }*/

	 int cur_id = -1;
	 for( auto clientclass = g_pServer->GetAllClasses( );
		  clientclass != nullptr;
		  clientclass = clientclass->m_pNext ) {

		 m_client_ids[ clientclass->m_pNetworkName ] = ++cur_id;
	 }

	 return true;
  }

  PropManager::netvar_table PropManager::LoadTable( RecvTable* recvTable ) {
	 auto table = netvar_table{};

	 table.offset = 0;
	 table.name = recvTable->m_pNetTableName;

	 if ( isdigit( recvTable->m_pNetTableName[ 0 ] ) )
		table.name = recvTable->m_pNetTableName;

	 for ( auto i = 0; i < recvTable->m_nProps; ++i ) {
		auto prop = &recvTable->m_pProps[ i ];

		if ( !prop )
		   continue;

		bool digit = isdigit( prop->m_pVarName[ 0 ] );
		if ( digit && prop->m_RecvType != DPT_DataTable )
		   continue;

		if ( strcmp( XorStr( "baseclass" ), prop->m_pVarName ) == 0 )
		   continue;

		if ( prop->m_RecvType == DPT_DataTable && prop->m_pDataTable ) {
		   table.child_tables.emplace_back( LoadTable( prop->m_pDataTable ) );
		   table.child_tables.back( ).offset = prop->m_Offset;
		   table.child_tables.back( ).prop = prop;
		} else {
		   table.child_props.emplace_back( prop );
		}
	 }
	 return table;
  }

  int PropManager::GetClientID( std::string networkname )
  {
	  return m_client_ids[ networkname ];
  }

  void PropManager::Destroy( ) {
	 this->m_pTables.clear( );
  }

  int PropManager::GetOffset( const std::string& table_name, const std::string& prop_name ) {
	 return ( this->GetProp( table_name, prop_name ) );
  }

  RecvVarProxyFn PropManager::Hook( const RecvVarProxyFn hooked, const std::string& table_name, const std::string& prop_name ) {
	 RecvProp* prop = nullptr;

	 if( !GetProp( table_name, prop_name, &prop ) ) {
#ifdef DEV
		Win32::Warning( XorStr( "Prop '%s::%s' not found (PropManager::%s)" ), table_name.c_str( ), prop_name.c_str( ) );
#endif
		return nullptr;
	 }

	 auto restore = prop->m_ProxyFn;

	 prop->m_ProxyFn = hooked;

	 return restore;
  }

  int PropManager::GetProp( const std::string& table_name, const std::string& prop_name, RecvProp** out_prop /*= nullptr */ ) {
	  const auto hash = hash_32_fnv1a( std::string( table_name ).append( prop_name ).data( ) );

	  bool bFound = false;
	  if( m_cached_offsets.find( hash ) != m_cached_offsets.end( ) ) {
		  bFound = true;
	  }

	  auto nOffset = bFound && !out_prop ? m_cached_offsets.at( hash ) : this->GetProp( this->GetTable( table_name ), prop_name, out_prop );

	 if( nOffset != 0 && !bFound ) {
		 m_cached_offsets.insert( { hash, nOffset } );
	 }

	 return nOffset;
  }

  int PropManager::GetProp( RecvTable* table, const std::string& prop_name, RecvProp** out_prop /*= nullptr */ ) {
	 int extra = 0;

	 for( int i = 0; i < table->m_nProps; i++ ) {
		auto prop = &table->m_pProps[i];
		auto child = prop->m_pDataTable;

		if( child && child->m_nProps ) {
		  int add = GetProp( child, prop_name, out_prop );

		  if( add )
			 extra += ( prop->m_Offset + add );
		}

		if( prop_name.compare( prop->m_pVarName ) == 0 ) {
		  if( out_prop )
			 *out_prop = prop;

		  return ( prop->m_Offset + extra );
		}
	 }

	 return extra;
  }

  RecvTable* PropManager::GetTable( const std::string& name ) {
	 if( !this->m_pTables.empty( ) ) {
		for( auto& table : m_pTables ) {
		  if( name.compare( table->m_pNetTableName ) == 0 )
			 return table;
		}
	 }
#ifdef DEV
	 Win32::Warning( XorStr( "Prop table '%s' not found" ), name.c_str( ) );
#endif
	 return nullptr;
  }

}
