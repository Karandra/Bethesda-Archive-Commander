#include "stdafx.h"
#include "LZ4.hpp"

#include <lib\lz4.h>
#pragma comment(lib, "liblz4_static.lib")

////////////////////////////////////////////////////////////////////////
//// LZ4Base
////////////////////////////////////////////////////////////////////////
LZ4Base::LZ4Base(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize)
	:m_StreamSource(tSourceStream), m_StreamOut(tOutStream), m_TotalSize(nTotalSize)
{
}
LZ4Base::~LZ4Base()
{
}

KxArchiveEvent LZ4Base::CreateEvent(wxEvtHandler* pEventHandler, wxEventType nType)
{
	KxArchiveEvent event(nType);
	if (pEventHandler)
	{
		event.Allow();
		event.SetEventObject(pEventHandler);
		event.SetMinorTotal(m_TotalSize);
	}
	return event;
}
bool LZ4Base::SendEvent(wxEvtHandler* pEventHandler, KxArchiveEvent& event, size_t nProcessed)
{
	if (pEventHandler)
	{
		event.SetMinorProcessed(nProcessed);
		m_EventHandler->ProcessEvent(event);
		if (!event.IsAllowed())
		{
			m_Status = ERROR_CANCELLED;
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////
//// LZ4Unpack
////////////////////////////////////////////////////////////////////////
LZ4Unpack::LZ4Unpack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize)
	:LZ4Base(tSourceStream, tOutStream, nTotalSize)
{
	m_ZStream = LZ4_createStreamDecode();
}
LZ4Unpack::~LZ4Unpack()
{
	if (m_ZStream)
	{
		LZ4_freeStreamDecode(static_cast<LZ4_streamDecode_t*>(m_ZStream));
	}
}

void LZ4Unpack::Run(wxEvtHandler* pEventHandler)
{
	if (IsOK())
	{
		auto pZStream = static_cast<LZ4_streamDecode_t*>(m_ZStream);
		if (LZ4_setStreamDecode(pZStream, nullptr, 0))
		{
			if (m_TotalSize != 0)
			{
				auto event = CreateEvent(pEventHandler, KxEVT_ARCHIVE_UNPACK);

				const size_t nDecodingBufferSize = LZ4_COMPRESSBOUND(PartSize);
				const size_t nDecodingBufferCount = 2;
				size_t nDecodingBufferIndex = 0;

				uint8_t tDataBuffer[PartSize] = {nullptr};
				uint8_t tDecodingBuffer[nDecodingBufferCount][nDecodingBufferSize] = {nullptr};

				size_t nReadSize = std::min(PartSize, m_TotalSize);
				size_t nProcessed = 0;

				do
				{
					m_StreamSource.Read(tDataBuffer, std::min(nReadSize, m_TotalSize - nProcessed));
					if (m_StreamSource.IsOk())
					{
						char* pOutBuffer = (char*)tDecodingBuffer[nDecodingBufferIndex];
						size_t nOutBufferSize = m_StreamSource.LastRead();
						int nOutSize = LZ4_decompress_safe_continue(pZStream, (const char*)tDataBuffer, pOutBuffer, nOutBufferSize, nDecodingBufferSize);

						if (nOutSize > 0)
						{
							nProcessed += nOutSize;
							if (SendEvent(pEventHandler, event, nProcessed))
							{
								if (!m_StreamOut.Write(pOutBuffer, nOutSize).IsOk())
								{
									m_Status = ERROR_WRITE_FAULT;
								}
							}
						}
						else
						{
							m_Status = ERROR_FUNCTION_FAILED;
						}
					}

					nDecodingBufferIndex = (nDecodingBufferIndex + 1) % nDecodingBufferCount;
				}
				while (m_Status == ERROR_SUCCESS && nProcessed < m_TotalSize);

				// If all data is processed
				if (nProcessed == m_TotalSize)
				{
					m_Status = ERROR_SUCCESS;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//// LZ4Pack
////////////////////////////////////////////////////////////////////////
LZ4Pack::LZ4Pack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize, int nLevel)
	:LZ4Base(tSourceStream, tOutStream, nTotalSize), m_CompressionLevel((CompressionLevel)nLevel)
{
	m_ZStream = LZ4_createStream();
}
LZ4Pack::~LZ4Pack()
{
	if (m_ZStream)
	{
		LZ4_freeStream(static_cast<LZ4_stream_t*>(m_ZStream));
	}
}

void LZ4Pack::Run(wxEvtHandler* pEventHandler)
{
	if (IsOK())
	{
		auto pZStream = static_cast<LZ4_stream_t*>(m_ZStream);
		LZ4_resetStream(pZStream);

		if (m_TotalSize != 0)
		{
			auto event = CreateEvent(pEventHandler, KxEVT_ARCHIVE_UNPACK);

			const size_t nDecodingBufferSize = LZ4_COMPRESSBOUND(PartSize);
			const size_t nDecodingBufferCount = 2;
			size_t nDecodingBufferIndex = 0;

			uint8_t tInputBuffer[nDecodingBufferCount][PartSize] = {nullptr};
			uint8_t tOutputBuffer[nDecodingBufferSize] = {nullptr};

			size_t nReadSize = std::min(PartSize, m_TotalSize);
			size_t nProcessed = 0;

			do
			{
				char* pInputBuffer = (char*)tInputBuffer[nDecodingBufferIndex];
				m_StreamSource.Read(pInputBuffer, std::min(nReadSize, m_TotalSize - nProcessed));
				if (m_StreamSource.IsOk())
				{
					size_t nOutBufferSize = m_StreamSource.LastRead();
					int nOutSize = LZ4_compress_fast_continue(pZStream, pInputBuffer, (char*)tOutputBuffer, nOutBufferSize, nDecodingBufferSize, m_CompressionLevel);

					if (nOutSize > 0)
					{
						nProcessed += nOutSize;
						if (SendEvent(pEventHandler, event, nProcessed))
						{
							if (!m_StreamOut.Write(tOutputBuffer, nOutSize).IsOk())
							{
								m_Status = ERROR_WRITE_FAULT;
							}
						}
					}
					else
					{
						m_Status = ERROR_FUNCTION_FAILED;
					}
				}

				nDecodingBufferIndex = (nDecodingBufferIndex + 1) % nDecodingBufferCount;
			}
			while (m_Status == ERROR_SUCCESS && nProcessed < m_TotalSize);

			// If all data is processed
			if (nProcessed == m_TotalSize)
			{
				m_Status = ERROR_SUCCESS;
			}
		}
	}
}
