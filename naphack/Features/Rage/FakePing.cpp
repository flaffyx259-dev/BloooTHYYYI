#include "FakePing.hpp"
#include "Animations.hpp"
#include "EnginePrediction.hpp"

FakePing g_FakePing;

void FakePing::GetTargetLatency( ) {
	// the reason we want to do this, is because if we are let's say not using ping spike, then toggle 800 pingsike, we are trying to send too much.
	// this, we could get prediction errors that would fullstop us etc.
	// this is negated by just slowly adding to the target value, and using that.

	// get our values.
	bool bPingSpikeEnabled = g_Vars.misc.extended_backtrack || ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled );
	int nPingSpikeAmount = ( g_Vars.misc.ping_spike && g_Vars.misc.ping_spike_key.enabled ) ? g_Vars.misc.ping_spike_amount : 150.f;

	// больше, чем сервер может отмотать, смысла нет - только задержка своих выстрелов
	const int nMaxUseful = static_cast< int >( ( g_Vars.sv_maxunlag->GetFloat( ) - g_Animations.m_fLerpTime ) * 1000.f );
	nPingSpikeAmount = std::clamp( nPingSpikeAmount, 0, std::max( nMaxUseful, 0 ) );

	// ping spike is not enabled, just set it to 0.
	if( !bPingSpikeEnabled ) {
		m_nTargetLatency = 0;

		// we are done here.
		return;
	}

	// our target latency is less than the desired value, increase value.
	if( m_nTargetLatency < nPingSpikeAmount )
		m_nTargetLatency += 5;

	// our target latency is more than the desired value, decrease value.
	else if( m_nTargetLatency > nPingSpikeAmount )
		m_nTargetLatency -= 5;

	// clamp this just to be sure.
	m_nTargetLatency = std::clamp<int>( m_nTargetLatency, 0, nPingSpikeAmount );
	// printf( "target lat: %d, %d\n", g_FakePing.m_nTargetLatency, nPingSpikeAmount );
}

void FakePing::OnSendDatagram( INetChannel *pNetChannel ) {
	if( !pNetChannel || m_Sequences.empty( ) ) {
		return;
	}

	// how much extra latency we still have to fake, in seconds.
	const float flTargetIncrement = m_nTargetLatency / 1000.0f - g_Animations.m_flOutgoingLatency;

	// we are already lagging more than the target, nothing to do.
	if( flTargetIncrement <= 0.f )
		return;

	// walk the history from newest to oldest and take the first entry that is at least
	// flTargetIncrement old.
	//
	// the sequence number and the reliable state MUST come from the same entry: the reliable
	// state is a flip-flop that the server validates against the sequence number it arrived
	// with. writing the number unconditionally and the state only on an exact number match
	// ( what this used to do ) leaves a mismatched pair whenever that number is not in the
	// history, and the server reads that as a dropped packet.
	//
	// selecting by time instead of by exact number also always finds something as long as the
	// history is not empty, which is why RAX does it that way.
	for( const auto &s : m_Sequences ) {
		if( g_pGlobalVars->realtime - s.m_flRealTime < flTargetIncrement )
			continue;

		pNetChannel->m_nInSequenceNr = s.m_nInSequenceNr;
		pNetChannel->m_nInReliableState = s.m_nInReliableState;
		break;
	}
}

void FakePing::UpdateIncomingSequences( INetChannel *pNetChannel ) {
	if( !pNetChannel )
		return;

	// only record strictly increasing sequence numbers.
	// packets that arrived out of order would otherwise land in the history and corrupt it:
	// OnSendDatagram walks this newest-to-oldest and would hand the server a stale pair.
	if( !m_Sequences.empty( ) ) {
		const int nNewest = m_Sequences.front( ).m_nInSequenceNr;

		// нумерация пошла заново (смена карты / реконнект) - вся старая история от другого соединения
		if( pNetChannel->m_nInSequenceNr + 64 < nNewest )
			m_Sequences.clear( );
		// пакет пришёл не по порядку - не пишем
		else if( pNetChannel->m_nInSequenceNr <= nNewest )
			return;
	}

	// store new stuff, newest at the front.
	m_Sequences.emplace_front( g_pGlobalVars->realtime, pNetChannel->m_nInReliableState, pNetChannel->m_nInSequenceNr );

	// чистим по ВРЕМЕНИ, а не по разнице номеров: OnSendDatagram выбирает запись по
	// возрасту, и всё, что старше sv_maxunlag, он уже не возьмёт никогда ( фейк-пинг
	// сверху заклампен тем же sv_maxunlag в GetTargetLatency ). по номерам порог 2048
	// держал ~24 КБ истории при любом тикрейте.
	//
	// m_Sequences упорядочен от новых к старым, поэтому достаточно отрезать хвост.
	// запас в 0.5 с - на случай, если sv_maxunlag сменится на лету.
	const float flKeepTime = g_Vars.sv_maxunlag->GetFloat( ) + 0.5f;

	// erase( ) already returns the next element, so the old `++it` on top of it both
	// skipped an entry and incremented an end( ) iterator once the last one was erased.
	while( !m_Sequences.empty( ) && g_pGlobalVars->realtime - m_Sequences.back( ).m_flRealTime > flKeepTime )
		m_Sequences.pop_back( );
}
