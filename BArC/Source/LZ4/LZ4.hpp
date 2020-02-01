#pragma once
#include "stdafx.h"
#include "KxFramework/KxFileOperationEvent.h"
#include "KxFramework/KxArchiveEvent.h"

class LZ4Base
{
	protected:
		static const size_t PartSize = 1024 * 16;

	protected:
		void* m_ZStream = nullptr;
		int m_Status = ERROR_SUCCESS;

		size_t m_TotalSize = 0;
		wxInputStream& m_StreamSource;
		wxOutputStream& m_StreamOut;

		KxArchiveEvent m_Event;
		wxEvtHandler* m_EventHandler = nullptr;

	protected:
		KxArchiveEvent CreateEvent(wxEvtHandler* pEventHandler, wxEventType nType);
		bool SendEvent(wxEvtHandler* pEventHandler, KxArchiveEvent& event, size_t nProcessed);

	public:
		LZ4Base(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize);
		virtual ~LZ4Base();

	public:
		bool IsOK() const
		{
			return m_ZStream && m_Status == ERROR_SUCCESS;
		}
		int GetStatus() const
		{
			return m_Status;
		}

		virtual void Run(wxEvtHandler* pEventHandler = nullptr) = 0;
};

class LZ4Unpack: public LZ4Base
{
	public:
		LZ4Unpack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize);
		virtual ~LZ4Unpack();

	public:
		virtual void Run(wxEvtHandler* pEventHandler = nullptr) override;
};

class LZ4Pack: public LZ4Base
{
	public:
		enum CompressionLevel: int
		{
			LEVEL_MIN = 0,
			LEVEL_MAX = 12,
			LEVEL_DEFAULT = 1,
		};

	private:
		CompressionLevel m_CompressionLevel = LEVEL_DEFAULT;

	public:
		LZ4Pack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize, int nLevel = LEVEL_DEFAULT);
		virtual ~LZ4Pack();

	public:
		virtual void Run(wxEvtHandler* pEventHandler = nullptr) override;
};
