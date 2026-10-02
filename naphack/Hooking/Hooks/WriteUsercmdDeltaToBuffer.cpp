#include "../Hooked.hpp"
#include "../../SDK/sdk.hpp"
#include "../../SDK/Classes/Exploits.hpp"
#include "../../SDK/Displacement.hpp"

#include "../../Features/Rage/TickbaseShift.hpp"


bool __fastcall Hooked::WriteUsercmdDeltaToBuffer( void* ecx, void* edx, int nSlot, void* pBuf, int nFrom, int nTo, bool newCmd ) {
	//if( g_TickbaseController.m_iShiftAmount <= 0 ) {
		return oWriteUsercmdDeltaToBuffer( ecx, nSlot, pBuf, nFrom, nTo, newCmd );
	//}

	if( nFrom != -1 )
		return true;

   // g_TickbaseController.m_bDontCommunicate = true;

    uintptr_t frame_ptr{ };
    __asm {
        mov frame_ptr, ebp;
    }

    int* pNumBackupCommands = ( int* )( ( std::uintptr_t )frame_ptr + 0xFD8 );
    int* m_nNewCmdsPtr = ( int* )( ( std::uintptr_t )frame_ptr + 0xFDC );
    int m_nNewCmds = *m_nNewCmdsPtr;

	int m_nTotalNewCmds = g_TickbaseController.m_iShiftAmount + m_nNewCmds;

    g_TickbaseController.m_iShiftAmount = 0;

	if( m_nTotalNewCmds > 62 )
		m_nTotalNewCmds = 62;

	nFrom = -1;

	*m_nNewCmdsPtr = m_nTotalNewCmds;
    *pNumBackupCommands = 0;

	int m_nNextCmd = g_pClientState->m_nLastOutgoingCommand( ) + g_pClientState->m_nChokedCommands( ) + 1;

	nTo = m_nNextCmd - m_nNewCmds + 1;
    if( nTo > m_nNextCmd ) {
    Run:
        CUserCmd* m_pCmd = g_pInput->GetUserCmd( nTo );
        if( m_pCmd ) {
            CUserCmd m_nToCmd = *m_pCmd, m_nFromCmd = *m_pCmd;
            m_nToCmd.command_number++;
            m_nToCmd.tick_count += 64 + 2 * 64;

            if( m_nNewCmds <= m_nTotalNewCmds ) {
                int m_shift = m_nTotalNewCmds - m_nNewCmds + 1;

                do {
                   // WriteUsercmd( (bf_write*)pBuf, &m_nToCmd, &m_nFromCmd );
                    m_nToCmd.command_number++;
                    m_nToCmd.tick_count++;

                    --m_shift;
                } while( m_shift );
            }
        }
        g_TickbaseController.m_bDontCommunicate = false;
        return true;
    }
    else {
        while( oWriteUsercmdDeltaToBuffer( ecx, nSlot, pBuf, nFrom, nTo, true ) ) {

            nFrom = nTo++;

            if( nTo > m_nNextCmd )
                goto Run;

        }

        g_TickbaseController.m_bDontCommunicate = false;

        return false;
    }

    g_TickbaseController.m_bDontCommunicate = false;

    return true;
}