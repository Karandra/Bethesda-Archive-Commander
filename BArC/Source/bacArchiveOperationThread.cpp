#include "stdafx.h"
#include "bacArchiveOperationThread.h"
#include "GUI/bacMainWindow.h"
#include "KxFramework/KxProgressDialog.h"
#include "KxFramework/KxTaskDialog.h"
#include "KxFramework/KxFileStream.h"
#include "KxFramework/KxArchiveEvent.h"
#include "KxFramework/KxFile.h"

wxDEFINE_EVENT(bacEVT_ARCHIVE_ERROR, wxCommandEvent);
wxDEFINE_EVENT(bacEVT_THREAD_END, wxCommandEvent);

bacArchiveExtractThreadBase::bacArchiveExtractThreadBase(bacMainWindow* pMainWindow, wxEvtHandler* pEventHandler, const bacBSA::RecordIndexesList& tElements, const wxString& sOutPath)
	:m_EventHandler(pEventHandler),
	m_MainWindow(pMainWindow),
	m_Archive(pMainWindow->GetArchive()),
	m_ElementsList(tElements),
	m_OutPath(sOutPath.Clone()), 
	m_ItemsTotal(tElements.size())
{
	// This will resend synchronous events from archive unpacker as asynchronous
	m_ArchiveEventHandler.Bind(KxEVT_ARCHIVE_UNPACK, [this](KxArchiveEvent& event)
	{
		if (TestDestroy())
		{
			event.Veto();
		}
		else
		{
			SendExtractEvent(event.GetMinorProcessed(), event.GetMinorTotal(), wxEmptyString);
		}
	});
}
bacArchiveExtractThreadBase::~bacArchiveExtractThreadBase()
{
	wxCriticalSectionLocker EnterCS(m_MainWindow->m_ThreadCS);
	m_MainWindow->m_Thread = nullptr;

	m_EventHandler->QueueEvent(new wxCommandEvent(bacEVT_THREAD_END));
}

bsaStatus bacArchiveExtractThreadBase::ExtractFile(size_t nFileIndex, const wxString& sFullPath, const wxString& sRelativePath)
{
	// Update current file name
	SendExtractEvent(0, 0, sRelativePath);

	KxFileStream tOutFile(sFullPath, KxFileStream::Access::Write, KxFileStream::Disposition::CreateAlways, KxFileStream::Share::Read);
	bsaStatus nStatus = m_Archive->ExtractFile(nFileIndex, tOutFile, &m_ArchiveEventHandler);
	m_ItemsProcessed++;
	return nStatus;
}

void bacArchiveExtractThreadBase::SendExtractEvent(size_t nCurrent, size_t nTotal, const wxString& sCurrentFile)
{
	KxArchiveEvent* event = new KxArchiveEvent(KxEVT_ARCHIVE_UNPACK);
	event->SetCurrent(sCurrentFile.Clone());
	event->SetMinorProcessed(nCurrent);
	event->SetMinorTotal(nTotal);
	event->SetMajorProcessed(m_ItemsProcessed);
	event->SetMajorTotal(m_ItemsTotal);

	m_EventHandler->QueueEvent(event);
}
void bacArchiveExtractThreadBase::SendErrorEvent(bsaStatus nErrorCode)
{
	wxCommandEvent* event = new wxCommandEvent(bacEVT_ARCHIVE_ERROR);
	event->SetInt(nErrorCode);
	m_EventHandler->QueueEvent(event);
}

//////////////////////////////////////////////////////////////////////////
bacArchiveExtractFilesThread::bacArchiveExtractFilesThread(bacMainWindow* pMainWindow, wxEvtHandler* pEventHandler, const bacBSA::RecordIndexesList& tFiles, const wxString& sOutPath)
	:bacArchiveExtractThreadBase(pMainWindow, pEventHandler, tFiles, sOutPath)
{
}

wxThread::ExitCode bacArchiveExtractFilesThread::Entry()
{
	if (m_ElementsList.size() == 1)
	{
		size_t nFileIndex = m_ElementsList[0];
		bsaStatus nError = ExtractFile(nFileIndex, m_OutPath, m_Archive->GetFilesList()[nFileIndex]);
		if (nError != BSA_STATUS_SUCCESS)
		{
			SendErrorEvent(nError);
		}
		return (ExitCode)nError;
	}
	else
	{
		for (size_t nFileIndex: m_ElementsList)
		{
			if (!TestDestroy())
			{
				wxString sRelativePath = m_Archive->GetFilesList()[nFileIndex];
				wxString sOutPath = m_OutPath + '\\' + sRelativePath;
				bsaStatus nError = ExtractFile(nFileIndex, sOutPath, sRelativePath);
				if (nError != BSA_STATUS_SUCCESS)
				{
					SendErrorEvent(nError);
					return (ExitCode)nError;
				}
			}
			else
			{
				return (ExitCode)BSA_STATUS_CANCELLED;
			}
		}
	}
	return (ExitCode)BSA_STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
bacArchiveExtractFolderThread::bacArchiveExtractFolderThread(bacMainWindow* pMainWindow, wxEvtHandler* pEventHandler, const bacBSA::RecordIndexesList& tFiles, const wxString& sOutPath)
	:bacArchiveExtractThreadBase(pMainWindow, pEventHandler, tFiles, sOutPath)
{
}

wxThread::ExitCode bacArchiveExtractFolderThread::Entry()
{
	// Calculate total files count
	for (size_t nFolderIndex: m_ElementsList)
	{
		m_ItemsTotal += m_Archive->GetFilesInFolder(nFolderIndex).size();
	}

	for (size_t nFolderIndex: m_ElementsList)
	{
		for (size_t nFileIndex: m_Archive->GetFilesInFolder(nFolderIndex))
		{
			if (!TestDestroy())
			{
				wxString sRelativePath = m_Archive->GetFilesList()[nFileIndex];
				wxString sFilePath = m_OutPath + '\\' + m_Archive->GetFoldersList()[nFolderIndex] + '\\' + sRelativePath;
				KxFile(sFilePath.BeforeLast('\\')).CreateFolder();

				bsaStatus nError = ExtractFile(nFileIndex, sFilePath, sRelativePath);
				if (nError != BSA_STATUS_SUCCESS)
				{
					SendErrorEvent(nError);
					return (ExitCode)nError;
				}
			}
			else
			{
				return (ExitCode)BSA_STATUS_CANCELLED;
			}
		}
	}
	return (ExitCode)BSA_STATUS_SUCCESS;
}
