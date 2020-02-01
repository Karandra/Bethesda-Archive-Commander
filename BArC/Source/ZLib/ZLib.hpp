#pragma once
#include "stdafx.h"
#include "KxFramework/KxFileOperationEvent.h"
#include "zlib/zlib.h"

class ZLibBase
{
	protected:
		static const size_t PartSize = 1024 * 16;

	protected:
		z_stream m_ZStream = {nullptr};
		int m_Status = Z_OK;

		size_t m_TotalSize = 0;
		wxInputStream& m_StreamSource;
		wxOutputStream& m_StreamOut;
		wxEvtHandler* m_EventHandler = nullptr;

	protected:
		void RunGeneric(wxEvtHandler* pEventHandler, bool bCompress);

	public:
		ZLibBase(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize);

	public:
		bool IsOK() const
		{
			return m_Status == Z_OK;
		}
		
		virtual void Run(wxEvtHandler* pEventHandler = nullptr) = 0;
};

class ZLibUnpack: public ZLibBase
{
	public:
		ZLibUnpack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize);
		virtual ~ZLibUnpack();

	public:
		virtual void Run(wxEvtHandler* pEventHandler = nullptr) override;
};

class ZLibPack: public ZLibBase
{
	public:
		enum CompressionLevel
		{
			LEVEL_NONE = Z_NO_COMPRESSION,
			LEVEL_MIN = Z_BEST_SPEED,
			LEVEL_MAX = Z_BEST_COMPRESSION,
			LEVEL_DEFAULT = Z_DEFAULT_COMPRESSION,
		};

	public:
		ZLibPack(wxInputStream& tSourceStream, wxOutputStream& tOutStream, size_t nTotalSize, int nLevel = LEVEL_DEFAULT);
		virtual ~ZLibPack();

	public:
		virtual void Run(wxEvtHandler* pEventHandler = nullptr) override;
};
