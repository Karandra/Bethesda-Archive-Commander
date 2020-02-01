#include "stdafx.h"
#include "ZLib.hpp"
#include "KxFramework/KxArchiveEvent.h"
//#pragma comment(lib, "zlibstat.lib")

////////////////////////////////////////////////////////////////////////
//// ZLibBase
////////////////////////////////////////////////////////////////////////
ZLibBase::ZLibBase(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize)
	:m_StreamSource(tSourceStream), m_StreamOut(tOutStream), m_TotalSize(nTotalSize)
{

}

void ZLibBase::RunGeneric(wxEvtHandler* pEventHandler, bool bCompress)
{
	if (m_TotalSize != 0)
	{
		KxArchiveEvent event(KxArchiveEvent::EvtProcess);
		if (pEventHandler)
		{
			event.Allow();
			event.SetEventObject(pEventHandler);
			event.SetMinorTotal(m_TotalSize);
		}

		int nStatus = Z_OK;
		uint8_t tInBuffer[PartSize] = {0};
		uint8_t tOutBuffer[PartSize] = {0};

		size_t nReadSize = std::min(PartSize, m_TotalSize);
		size_t nProcessed = 0;
		int nFlushMode = Z_NO_FLUSH;

		bool bContinue = false;
		do
		{
			m_ZStream.avail_in = m_StreamSource.Read(tInBuffer, std::min(nReadSize, m_TotalSize - nProcessed)).LastRead();
			nProcessed += m_ZStream.avail_in;

			if (m_StreamSource.IsOk() && m_ZStream.avail_in != 0)
			{
				m_ZStream.next_in = tInBuffer;
				if (bCompress)
				{
					nFlushMode = nProcessed == m_TotalSize ? Z_FINISH : Z_NO_FLUSH;
				}

				do
				{
					m_ZStream.avail_out = PartSize;
					m_ZStream.next_out = tOutBuffer;

					// Pack or unpack
					if (bCompress)
					{
						nStatus = deflate(&m_ZStream, nFlushMode);
					}
					else
					{
						nStatus = inflate(&m_ZStream, nFlushMode);
					}

					if (pEventHandler)
					{
						event.SetMinorProcessed(nProcessed);
						event.SetCRC(m_ZStream.adler);

						pEventHandler->ProcessEvent(event);
						if (!event.IsAllowed())
						{
							m_Status = Z_DATA_ERROR;
							return;
						}
					}

					if (nStatus != Z_NEED_DICT && nStatus != Z_DATA_ERROR && nStatus != Z_MEM_ERROR)
					{
						uint32_t nSize = PartSize - m_ZStream.avail_out;
						if (!m_StreamOut.Write(tOutBuffer, nSize).IsOk())
						{
							m_Status = Z_DATA_ERROR;
							return;
						}
					}
					else
					{
						m_Status = nStatus;
						return;
					}
				}
				while (m_ZStream.avail_out == 0);
			}
			else
			{
				break;
			}

			if (bCompress)
			{
				bContinue = nFlushMode != Z_FINISH;
			}
			else
			{
				bContinue = nStatus != Z_STREAM_END;
			}
		}
		while (bContinue && nProcessed < m_TotalSize);
	}
	else
	{
		m_Status = Z_DATA_ERROR;
	}
}

////////////////////////////////////////////////////////////////////////
//// ZLibExtract
////////////////////////////////////////////////////////////////////////
ZLibUnpack::ZLibUnpack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize)
	:ZLibBase(tSourceStream, tOutStream, nTotalSize)
{
	m_Status = inflateInit(&m_ZStream);
}
ZLibUnpack::~ZLibUnpack()
{
	inflateEnd(&m_ZStream);
}

void ZLibUnpack::Run(wxEvtHandler* pEventHandler)
{
	RunGeneric(pEventHandler, false);
}

////////////////////////////////////////////////////////////////////////
//// ZLibCompress
////////////////////////////////////////////////////////////////////////
ZLibPack::ZLibPack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize, int nLevel)
	:ZLibBase(tSourceStream, tOutStream, nTotalSize)
{
	// Check compression level
	if (nLevel < LEVEL_NONE || nLevel > LEVEL_MAX || nLevel != LEVEL_DEFAULT)
	{
		nLevel = LEVEL_DEFAULT;
	}

	m_Status = deflateInit(&m_ZStream, nLevel);
}
ZLibPack::~ZLibPack()
{
	deflateEnd(&m_ZStream);
}

void ZLibPack::Run(wxEvtHandler* pEventHandler)
{
	RunGeneric(pEventHandler, true);
}
