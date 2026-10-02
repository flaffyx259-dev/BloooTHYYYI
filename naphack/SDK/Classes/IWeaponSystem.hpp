#pragma once
#include "WeaponInfo.hpp"

class IWeaponSystem {
public:
    CCSWeaponInfo* GetWeaponInfo( int nItemDefinitionIndex );
};