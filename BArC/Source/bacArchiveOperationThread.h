#pragma once
#include "stdafx.h"
#include "KxFramework/KxArchiveEvent.h"
#include "bacBSA.h"

wxDECLARE_EVENT(bacEVT_ARCHIVE_ERROR, wxCommandEvent);
wxDECLARE_EVENT(bacEVT_THREAD_END, wxCommandEvent);

class bacMainWindow;
class KxProgressDialog;
class bacArchiveExtractThreadBase: public wxThread
{
	protected:
		bacMainWindow* m_MainWindow = nullptr;
		const bacBSA* m_Archive = nullptr;
		wxEvtHandler* m_EventHandler = nullptr;
		wxEvtHandler m_ArchiveEventHandler;

		const bacBSA::RecordIndexesList m_ElementsList;
		const wxString m_OutPath;

		size_t m_ItemsProcessed = 0;
		size_t m_ItemsTotal = 0;

	public:
		bacArchiveExtractThreadBase(bacMainWindow* pMainWindow, wxEvtHandler* pEventHandler, const bacBSA::RecordIndexesList& tElements, const wxString& sOutPath);
		~bacArchiveExtractThreadBase();

	protected:
		bsaStatus ExtractFile(size_t nFileIndex, const wxString& sFullPath, const wxString& sRelativePath);

		void SendExtractEvent(size_t nCurrent, size_t nTotal, const wxString& sCurrentFile);
		void SendErrorEvent(bsaStatus nErrorCode);
};

class bacArchiveExtractFilesThread: public bacArchiveExtractThreadBase
{
	public:
		bacArchiveExtractFilesThread(bacMainWindow* pMainWindow, wxEvtHandler* pEventHandler, const bacBSA::RecordIndexesList& tFiles, const wxString& sOutPath);

	private:
		virtual ExitCode Entry() override;
};

class bacArchiveExtractFolderThread: public bacArchiveExtractThreadBase
{
	public:
		bacArchiveExtractFolderThread(bacMainWindow* pMainWindow, wxEvtHandler* pEventHandler, const bacBSA::RecordIndexesList& tFiles, const wxString& sOutPath);

	private:
		virtual ExitCode Entry() override;
};
