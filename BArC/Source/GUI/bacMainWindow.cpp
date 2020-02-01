#include "stdafx.h"
#include "bacMainWindow.h"
#include "bacApp.h"
#include "KxFramework/KxDualProgressDialog.h"
#include "KxFramework/KxFileBrowseDialog.h"
#include "KxFramework/KxProgressDialog.h"
#include "KxFramework/KxArchiveEvent.h"
#include "KxFramework/KxTaskDialog.h"
#include "KxFramework/KxString.h"
#include "KxFramework/KxFile.h"
#include "KxFramework/KxDataView.h"

#include "bacArchiveFilesViewModel.h"
#include "bacArchiveInfoWindow.h"
#include "bacArchiveOperationThread.h"

void bacMainWindow::CreateMenus()
{
	/* File */
	m_MenuFile = new KxMenu();

	m_MenuFile->Add(bacApp::CreateMenuItem("Menus.File.OpenArchive", bacIMG::folder_open))->
		Bind(KxEVT_MENU_SELECT, &bacMainWindow::OnOpenFile, this);

	m_MenuFile_CloseArchive = m_MenuFile->Add(bacApp::CreateMenuItem("Menus.File.CloseArchive"));
	m_MenuFile_CloseArchive->Enable(false);
	m_MenuFile_CloseArchive->Bind(KxEVT_MENU_SELECT, [this](wxCommandEvent& event)
	{
		m_MenuFile_CloseArchive->Enable(false);
		CloseArchiveView();
	});
	m_MenuFile->AddSeparator();

	m_MenuFile->Add(bacApp::CreateMenuItem("Menus.File.ExitApp"))->Bind(KxEVT_MENU_SELECT, [](wxCommandEvent&)
	{
		wxExit();
	});

	/* Commands */
	m_MenuCommands = new KxMenu();

	m_MenuCommands_Extract = m_MenuCommands->Add(bacApp::CreateMenuItem("Menus.Commands.Extract"));
	m_MenuCommands_Extract->Bind(KxEVT_MENU_SELECT, &bacMainWindow::OnExtractFileCommand, this);
	m_MenuCommands_Extract->Enable(false);
	m_MenuCommands->AddSeparator();

	m_MenuCommands_ExtractFolderRecursive = m_MenuCommands->Add(bacApp::CreateMenuItem("Menus.Commands.ExtractFolderRecursive"));
	m_MenuCommands_ExtractFolderRecursive->Bind(KxEVT_MENU_SELECT, &bacMainWindow::OnExtractFolderCommand, this);
	m_MenuCommands_ExtractFolderRecursive->Enable(false);

	m_MenuCommands_ExtractFolder = m_MenuCommands->Add(bacApp::CreateMenuItem("Menus.Commands.ExtractFolder"));
	m_MenuCommands_ExtractFolder->Bind(KxEVT_MENU_SELECT, &bacMainWindow::OnExtractFolderCommand, this);
	m_MenuCommands_ExtractFolder->Enable(false);

	/* Language */
	m_MenuLanguage = new KxMenu();
	for (const auto&[locale, path]: bacApp::Get().GetAlailableTranslations())
	{
		KxMenuItem* item = m_MenuLanguage->Add(new KxMenuItem(KxTranslation::GetLanguageFullName(locale), wxEmptyString, wxITEM_CHECK));
		item->Check(locale == bacApp::Get().GetCurrentLocale());

		item->Bind(KxEVT_MENU_SELECT, [this, locale](KxMenuEvent& event)
		{
			bacApp::Get().SetCurrentLocale(locale);
			wxMessageBox(T("App.LangChange"), GetTitle(), wxOK|wxICON_INFORMATION, this);
		});
	}

	/* Add to MenuBar */
	m_MenuBar->Append(m_MenuFile, T("Menus.File"));
	m_MenuBar->Append(m_MenuCommands, T("Menus.Commands"));
	m_MenuBar->Append(m_MenuLanguage, T("Menus.Language"));
}
void bacMainWindow::CreateLayout()
{
	m_Sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_Sizer);

	m_Sizer->Add(m_ToolBar, 0, wxEXPAND);
	m_Sizer->Add(m_ViewSplitter, 1, wxEXPAND|wxALL, LayoutDefaultBorder);

	m_ViewSplitter->SetMinimumPaneSize(50);
	m_ViewSplitter->SplitVertically(m_FoldersView, m_FilesViewModel->GetView(), 200);

	m_StatusBar->SetStatusWidths({-3, -2, -2});
}
void bacMainWindow::CreateToolBar()
{
	m_ToolBar = new KxAuiToolBar(this, wxID_ANY, KxAuiToolBar::DefaultStyle|wxAUI_TB_TEXT);

	m_TB_InfoButton = m_ToolBar->AddTool(T("ToolBar.Info"), wxGetApp().GetBitmap(bacIMG::information_frame));
	m_TB_InfoButton->Bind(KxEVT_AUI_TOOLBAR_CLICK, [this](KxAuiToolBarEvent& event)
	{
		bacArchiveInfoWindow(this).ShowModal();
	});

	m_TB_TestButton = m_ToolBar->AddTool(T("ToolBar.TestArchive"), wxGetApp().GetBitmap(bacIMG::ui_check_boxes_list));
	m_TB_TestButton->Bind(KxEVT_AUI_TOOLBAR_CLICK, &bacMainWindow::OnTestArchive, this);

	m_ToolBar->Realize();
	m_ToolBar->Disable();
}
void bacMainWindow::CreateFolderView()
{
	m_FoldersView = new KxTreeList(m_ViewSplitter, wxID_ANY, KxTreeList::DefaultStyle|KxTL_DCLICK_EXPAND);
	m_FoldersView->AddColumn(T("FilesView.Type.Folder"), wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxCOL_SORTABLE);

	m_FoldersView->SetSortColumn(0);
	m_FoldersView->SetImageList(bacApp::Get().GetImageList());

	m_FoldersView->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &bacMainWindow::OnSelectFolder, this);
}
void bacMainWindow::CreateFileView()
{
	m_FilesViewModel = new bacArchiveFilesViewModel();
	m_FilesViewModel->Create(m_ViewSplitter);

	m_FilesViewModel->GetView()->Bind(KxEVT_DATAVIEW_ITEM_SELECTED, &bacMainWindow::OnSelectFile, this);
	m_FilesViewModel->GetView()->Bind(KxEVT_DATAVIEW_ITEM_ACTIVATED, &bacMainWindow::OnActivateFile, this);
	m_FilesViewModel->GetView()->Bind(KxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &bacMainWindow::OnFileContextMenu, this);
}

bacMainWindow::bacMainWindow()
	:KxFrame(nullptr, wxID_ANY, wxGetApp().GetAppDisplayName(), wxDefaultPosition, wxSize(900, 550))
{
	SetIcon(wxIcon("UI\\Icons\\Box.png", wxBITMAP_TYPE_PNG));

	m_StatusBar = new KxStatusBarEx(this, wxID_ANY, ArcStatus::MAX, KxStatusBarEx::DefaultStyle|KxSBE_SEPARATORS_ENABLED);
	SetStatusBar(m_StatusBar);

	m_MenuBar = new wxMenuBar();
	SetMenuBar(m_MenuBar);

	m_ViewSplitter = new KxSplitterWindow(this, wxID_NONE);

	CreateToolBar();
	CreateFolderView();
	CreateFileView();
	CreateLayout();
	CreateMenus();

	Bind(bacEVT_THREAD_END, &bacMainWindow::OnExtractionEnd, this);
	Bind(bacEVT_ARCHIVE_ERROR, &bacMainWindow::OnExtractionError, this);
	Bind(KxEVT_ARCHIVE_UNPACK, &bacMainWindow::OnExtractionStatusChange, this);

	m_ThreadStatusDialog = new KxDualProgressDialog(this, wxID_NONE, T("ExtractArchive.Caption"), wxDefaultPosition, wxDefaultSize, KxBTN_NONE);
	m_ThreadStatusDialog->AddButton(wxID_CANCEL);
	m_ThreadStatusDialog->GetPB1()->SetRange(100);
	m_ThreadStatusDialog->GetPB2()->SetRange(100);
	m_ThreadStatusDialog->Bind(KxEVT_STDDIALOG_BUTTON, &bacMainWindow::OnExtractionCancel, this);
	m_ThreadStatusDialog->Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent& event)
	{
		event.Veto();
	});
}
bacMainWindow::~bacMainWindow()
{
	m_ThreadStatusDialog->Close(true);
}

void bacMainWindow::CloseArchiveView()
{
	SetTitle(wxGetApp().GetAppDisplayName());
	m_FoldersView->GetRoot().RemoveChildren();
	m_FilesViewModel->SetDataVector();
	m_ArchiveContent.clear();
	m_ToolBar->Disable();
	m_MenuCommands_Extract->Enable(false);

	m_StatusBar->SetStatusText(wxEmptyString, ArcStatus::SelectedFolder);
	m_StatusBar->SetStatusText(wxEmptyString, ArcStatus::SelectedFile);
	m_StatusBar->SetStatusText(wxEmptyString, ArcStatus::Total);

	m_Archive.reset();
}

void bacMainWindow::OpenArchiveView(ArcPtr& pArchive)
{
	// Close current archive if any
	m_FoldersView->Freeze();
	CloseArchiveView();

	// Assign new archive and load it
	m_Archive = std::move(pArchive);
	ArchiveViewConfigureUI();

	// Add item to tree and create item data for it
	auto AddItem = [this](KxTreeListItem tParentItem, const wxString& sName, const wxString& sFullPath, size_t nFolderIndex, bool bHasFiles)
	{
		auto tItem = tParentItem.Add(sName);
		tItem.SetImage(bacIMG::folder_horizontal, bacIMG::folder_horizontal_open);

		auto pData = new bacArchiveFolderRef(nFolderIndex, bHasFiles, tItem);
		m_ArchiveContent.emplace(std::make_pair(sFullPath, *pData));
		tItem.SetData(pData);

		return tItem;
	};
	
	// Check if specified folder has any files (exist in the archive) and correct its index
	auto HasFiles = [](const auto& tPathsArray, const wxString& sPath, size_t& nIndex)
	{
		auto tIt = std::find(tPathsArray.cbegin(), tPathsArray.cend(), sPath);
		if (tIt != tPathsArray.end())
		{
			nIndex = tIt - tPathsArray.begin();
			return true;
		}
		else
		{
			nIndex = (size_t)-1;
			return false;
		}
	};

	wxString sFolderLabel = T("FilesView.Type.Folder");
	KxTreeListItem tRoot = AddItem(m_FoldersView->GetRoot(), GetArchiveName(), wxEmptyString, 0, false);

	// Folder names and folder records arrays have same order (files arrays too)
	const auto& tArchiveFolders = m_Archive->GetFoldersList();
	for (size_t i = 0; i < tArchiveFolders.size(); i++)
	{
		const auto& sFolderPath = tArchiveFolders[i];
		if (!sFolderPath.IsEmpty() && sFolderPath != '.')
		{
			auto sFolderPathParts = KxString::Split(sFolderPath, "\\");

			wxString sFullPath;
			size_t nPartsCount = sFolderPathParts.size();
			for (size_t k = 0; k < nPartsCount; k++)
			{
				// Save previous path
				wxString sPrevPath = sFullPath;

				// Get previous tree node
				KxTreeListItem tPrevNode;
				auto tNode = m_ArchiveContent.find(sPrevPath);
				if (tNode == m_ArchiveContent.end())
				{
					tPrevNode = tRoot;
				}
				else
				{
					tPrevNode = tNode->second.Item;
				}

				// Construct new path part
				const auto& sPart = sFolderPathParts[k];
				if (k != 0)
				{
					sFullPath.Append("\\");
				}
				sFullPath.Append(sPart);

				// Check folder existence and adjust its index 
				size_t nRealFolderIndex = i;
				bool bHasFiles = HasFiles(tArchiveFolders, sFullPath, nRealFolderIndex);

				// If this folder is not present in tree, add it
				tNode = m_ArchiveContent.find(sFullPath);
				if (tNode == m_ArchiveContent.end())
				{
					AddItem(tPrevNode, sPart, sFullPath, nRealFolderIndex, bHasFiles);
				}
				else
				{
					// If it already added, set its item data to correct one
					auto& tNodeItem = tNode->second;
					if (tNodeItem.Item.IsOK())
					{
						auto pData = new bacArchiveFolderRef(nRealFolderIndex, bHasFiles, tNodeItem.Item);
						tNodeItem.Item.SetData(pData);
					}
				}
			}
		}
		else
		{
			size_t nIndex = i;
			bool bHasFiles = HasFiles(tArchiveFolders, sFolderPath, nIndex);
			tRoot.SetData(new bacArchiveFolderRef(nIndex, bHasFiles, tRoot));
		}
	}

	tRoot.Expand();
	m_FoldersView->Thaw();
}
void bacMainWindow::LoadArchiveStatus()
{
	uint32_t nFoldersCount = m_Archive->GetFoldersCount();
	uint32_t nFilesCount = m_Archive->GetFilesCount();
	wxString sTotalSize = KxFile::FormatFileSize(m_Archive->GetArchiveCompressedSize(), 2);
	m_StatusBar->SetStatusText(wxString::Format(T("FilesView.Status.Total"), nFoldersCount, sTotalSize, nFilesCount), ArcStatus::Total);
}
void bacMainWindow::ArchiveViewConfigureUI()
{
	SetTitle(wxString::Format("%s - %s", m_ArchiveName, wxGetApp().GetAppDisplayName()));
	LoadArchiveStatus();

	m_ToolBar->Enable();
	m_MenuFile_CloseArchive->Enable(true);
}
void bacMainWindow::LoadArchiveFiles(bacArchiveFolderRef& tFolderInfo)
{
	wxBusyCursor tBusy;
	m_FilesViewModel->SetDataVector(m_Archive.get(), tFolderInfo.GetIndex());
}

// Events
void bacMainWindow::OnOpenFile(wxCommandEvent& event)
{
	KxFileBrowseDialog tDialog(this, KxID_NONE, KxFBD_OPEN);
	tDialog.AddFilter("*.bsa", T("FileFilter.BSA"));
	tDialog.AddFilter("*", T("FileFilter.AllFiles"));

	auto Open = [this](ArcPtr& pArchive, const wxString& sFilePath)
	{
		m_ArchiveName = KxFile(sFilePath).GetFullName();
		OpenArchiveView(pArchive);
	};

	// Try to load specified file. If archive is loaded successfully, set it as current and delete the old one.
	// Else discard it leaving current loaded untouched.
	if (tDialog.ShowModal() == wxID_OK)
	{
		wxString sFilePath = tDialog.GetResult();

		wxBusyCursor tBusy;
		ArcPtr pArchive = std::unique_ptr<bacBSA>(new bacBSA(sFilePath));
		if (pArchive && pArchive->IsOK())
		{
			Open(pArchive, sFilePath);
		}
		else
		{
			KxTaskDialog tMsg(this, wxID_ANY, T(KxID_ERROR), wxEmptyString, pArchive->CanIgnoreErrors() ? KxBTN_YES|KxBTN_NO : KxBTN_OK);
			if (pArchive->CanIgnoreErrors())
			{
				tMsg.SetMainIcon(KxICON_WARNING);
				tMsg.SetMessage(T("BSA.InvalidFormatCanIgnore"));
			}
			else
			{
				tMsg.SetMainIcon(KxICON_ERROR);
				tMsg.SetMessage(T("BSA.InvalidFormatMessage"));
			}
			tMsg.SetExMessage(T(pArchive->GetStatus()));
			tMsg.SetOptionEnabled(KxTD_EXMESSAGE_IN_FOOTER);

			if (tMsg.ShowModal() == wxID_YES && pArchive->CanIgnoreErrors())
			{
				Open(pArchive, sFilePath);
			}
		}
	}
}
void bacMainWindow::OnSelectFile(KxDataViewEvent& event)
{
	m_MenuCommands_Extract->Enable(false);

	wxString sStatus;
	KxDataViewItem item = event.GetItem();

	if (item.IsOK())
	{
		const bsaFileRecord* pFileRecord = m_FilesViewModel->GetDataEntry(item);
		if (pFileRecord)
		{
			size_t row = m_FilesViewModel->GetRow(item);
			size_t fileIndex = m_FilesViewModel->GetFileIndexByRow(row);

			wxString sSize = KxFile::FormatFileSize(pFileRecord->Size, 2);
			sStatus.Printf(T("FilesView.Status.Selected"), fileIndex, m_FilesViewModel->GetEntryName(item), sSize, pFileRecord->Offset);

			m_MenuCommands_Extract->Enable(true);
		}
	}

	m_StatusBar->SetStatusText(sStatus, ArcStatus::SelectedFile);
}
void bacMainWindow::OnActivateFile(KxDataViewEvent& event)
{
	OnExtractFileCommand(event);
}
void bacMainWindow::OnFileContextMenu(KxDataViewEvent& event)
{
	m_MenuCommands->Show(m_FilesViewModel->GetView());
}
void bacMainWindow::OnSelectFolder(wxTreeListEvent& event)
{
	bool bAllowClear = true;
	KxTreeListItem tItem(*m_FoldersView, event.GetItem());
	if (tItem.IsOK())
	{
		wxString sStatus;
		bacArchiveFolderRef* pItemData = static_cast<bacArchiveFolderRef*>(tItem.GetData());
		if (pItemData)
		{
			wxString sFolder;
			if (pItemData->HasFiles())
			{
				sFolder = m_ArchiveName + "\\" + m_Archive->GetFoldersList()[pItemData->GetIndex()];

				bAllowClear = false;
				LoadArchiveFiles(*pItemData);
			}
			else
			{
				KxTreeListItem tCurrentItem = pItemData->Item;
				do
				{
					sFolder.Prepend(tCurrentItem.GetLabel());
					sFolder.Prepend('\\');

					tCurrentItem = tCurrentItem.GetParent();
				}
				while (tCurrentItem.IsOK());
				sFolder.Remove(0, 2);
			}

			wxString sIndex;
			wxString sExData;
			if (pItemData->HasFiles())
			{
				sIndex = " #" + std::to_string(pItemData->GetIndex());

				auto pFolderRecord = m_Archive->GetFolderRecord(pItemData->GetIndex());
				uint32_t nSize = pFolderRecord->Count;
				uint32_t nOffset = pFolderRecord->Offset;
				bacBSA::HashType nHash = pFolderRecord->NameHash;

				sExData.Printf(", %s: %u, %s: %u, %s: %s", T("FilesView.FolderSize"), nSize, T("FilesView.Offset"), nOffset, T("FilesView.Hash"), m_Archive->FormatHash(nHash));
			}

			sStatus.Printf("%s%s: \"%s\"%s", T("FilesView.Type.Folder"), sIndex, sFolder, sExData);
		}

		if (bAllowClear)
		{
			m_FilesViewModel->SetDataVector();
		}
		m_StatusBar->SetStatusText(sStatus, ArcStatus::SelectedFolder);
	}

	m_MenuCommands_Extract->Enable(false);
	m_MenuCommands_ExtractFolder->Enable(tItem.IsOK());
	m_MenuCommands_ExtractFolderRecursive->Enable(tItem.IsOK());
}

void bacMainWindow::OnTestArchive(KxAuiToolBarEvent& event)
{
	class Thread: public wxThread
	{
		private:
			KxStringVector m_ErrorFolders;
			KxStringVector m_ErrorFiles;

			bacBSA* m_Archive = nullptr;
			bacMainWindow* m_MainWindow = nullptr;
			KxProgressDialog* m_StatusDialog = nullptr;

		protected:
			void Test(const KxStringVector& tList, const wxString& sLabel, bool bFolder)
			{
				for (size_t i = 0; i < tList.size(); i++)
				{
					const wxString& sPath = tList[i];
					if (!sPath.IsEmpty())
					{
						m_MainWindow->CallAfter([this, i, size = tList.size(), label = sLabel.Clone(), path = sPath.Clone()]()
						{
							m_StatusDialog->SetValue(i, size);
							m_StatusDialog->SetLabel(wxString::Format("%s: \"%s", label, path));
							m_StatusDialog->SetAutoSize(false);

							if (i % 1000)
							{
								m_StatusDialog->Update();
							}
						});

						bacBSA::HashType nArcHash = bFolder ? m_Archive->GetFolderRecord(i)->NameHash : m_Archive->GetFileRecord(i)->NameHash;;
						bacBSA::HashType nTestHash = m_Archive->HashPath(sPath, bFolder);

						if (nArcHash != nTestHash)
						{
							if (bFolder)
							{
								m_ErrorFolders.push_back(sPath);
							}
							else
							{
								m_ErrorFiles.push_back(sPath);
							}
						}
					}
				}
			};

			virtual ExitCode Entry() override
			{
				Test(m_Archive->GetFoldersList(), T("TestArchive.CheckingFolder"), true);
				Test(m_Archive->GetFilesList(), T("TestArchive.CheckingFile"), false);

				return 0;
			}
			virtual void OnExit() override
			{
				bool bOK = false;
				wxString sExMessage;

				if (m_ErrorFolders.empty() && m_ErrorFiles.empty())
				{
					bOK = true;
				}
				else
				{
					bOK = false;

					wxString sFilesList;
					wxString sFoldersList;
					if (!m_ErrorFiles.empty())
					{
						sFilesList = T("FilesView.Type.Files") + ":\r\n" + KxString::Join(m_ErrorFiles, "\r\n");
					}
					if (!m_ErrorFolders.empty())
					{
						sFoldersList = T("FilesView.Type.Folders") + ":\r\n" + KxString::Join(m_ErrorFolders, "\r\n");
					}
					sExMessage = sFilesList + "\r\n\r\n" + sFoldersList;
				}

				m_StatusDialog->Destroy();
				m_MainWindow->CallAfter([this, mainWindow = m_MainWindow, bOK, sExMessage = sExMessage.Clone()]()
				{
					KxTaskDialog dialog(mainWindow, wxID_NONE, T("TestArchive.Caption"));
					if (bOK)
					{
						dialog.SetMainIcon(KxICON_INFORMATION);
						dialog.SetMessage(T("TestArchive.NoErrors"));
					}
					else
					{
						dialog.SetMainIcon(KxICON_ERROR);
						dialog.SetMessage(T("TestArchive.Error"));
						dialog.SetExMessage(sExMessage);
					}

					dialog.ShowModal();
					mainWindow->Enable();
				});
			}

		public:
			Thread(bacBSA* archive, KxProgressDialog* dialog, bacMainWindow* window)
				:m_Archive(archive), m_StatusDialog(dialog), m_MainWindow(window)
			{
			}
	};

	wxString sCaption = wxString::Format("%s - \"%s\"", T("TestArchive.Caption"), m_ArchiveName);
	KxProgressDialog* statusDialog = new KxProgressDialog(this, wxID_NONE, sCaption, wxDefaultPosition, wxDefaultSize, KxBTN_NONE);
	statusDialog->SetWindowResizeSide(wxHORIZONTAL);
	statusDialog->SetRange(100);
	statusDialog->Show();
	Disable();

	Thread* thread = new Thread(m_Archive.get(), statusDialog, this);
	if (thread->Run() != wxTHREAD_NO_ERROR)
	{
		statusDialog->Destroy();
		Enable();
		delete thread;
	}
}

void bacMainWindow::OnExtractFileCommand(wxCommandEvent& event)
{
	KxDataViewItem::Vector tItemsList;
	if (m_FilesViewModel->GetView()->GetSelections(tItemsList) != 0)
	{
		KxFileBrowseDialog tDialog;
		if (tItemsList.size() == 1)
		{
			tDialog.Create(this, KxID_NONE, KxFBD_SAVE);

			wxString sLabel = m_FilesViewModel->GetEntryName(tItemsList.front());
			wxString sExt = sLabel.AfterLast('.');

			tDialog.AddFilter(wxString::Format("*.%s", sExt), wxString::Format("%s %s", T("FilesView.Type.File"), sExt.Upper()));
			tDialog.AddFilter("*", T("FileFilter.AllFiles"));
			tDialog.SetDefaultExtension(sExt);
			tDialog.SetFileName(sLabel);
		}
		else
		{
			tDialog.Create(this, KxID_NONE, KxFBD_OPEN_FOLDER);
		}

		if (tDialog.ShowModal() == wxID_OK)
		{
			BeginFileExtract(tItemsList, tDialog.GetResult());
		}
	}
}
void bacMainWindow::OnExtractFolderCommand(wxCommandEvent& event)
{
	KxTreeListItem tRoot(*m_FoldersView, m_FoldersView->GetSelection());
	if (tRoot.IsOK())
	{
		KxFileBrowseDialog tDialog(this, KxID_NONE, KxFBD_OPEN_FOLDER);
		if (tDialog.ShowModal() == wxID_OK)
		{
			BeginFolderExtract(tRoot, tDialog.GetResult(), event.GetId() == m_MenuCommands_ExtractFolderRecursive->GetId());
		}
	}
}

void bacMainWindow::OnExtractionStatusChange(KxArchiveEvent& event)
{
	wxString sCurrentFile = event.GetCurrent();
	if (sCurrentFile.IsEmpty())
	{
		m_ThreadStatusDialog->GetPB1()->SetValue(event.GetMinorProcessed(), event.GetMinorTotal());
		m_ThreadStatusDialog->GetPB2()->SetValue(event.GetMajorProcessed(), event.GetMajorTotal());
	}
	else
	{
		auto nCurrent = event.GetMajorProcessed();
		auto nMax = event.GetMajorTotal();
		wxString sLabel = wxString::Format("%s: %llu/%llu\r\n%s: %s", T("ExtractArchive.LabelProcessed"), nCurrent, nMax, T("ExtractArchive.Label"), sCurrentFile);

		m_ThreadStatusDialog->SetLabel(sLabel);
	}
	m_ThreadStatusDialog->SetAutoSize(false);
}
void bacMainWindow::OnExtractionCancel(wxNotifyEvent& event)
{
	if (m_Thread && event.GetId() == wxID_CANCEL)
	{
		m_Thread->Delete();
	}
	event.Veto();
}
void bacMainWindow::OnExtractionEnd(wxCommandEvent& event)
{
	m_ThreadStatusDialog->Hide();

	Enable();
	SetFocus();
};
void bacMainWindow::OnExtractionError(wxCommandEvent& event)
{
	wxWindow* pParent = this;
	if (m_ThreadStatusDialog->IsVisible())
	{
		pParent = m_ThreadStatusDialog;
	}
	KxTaskDialog(pParent, wxID_ANY, T(KxID_ERROR), T((bsaStatus)event.GetInt()), KxBTN_OK, KxICON_ERROR).ShowModal();
};

void bacMainWindow::BeginFileExtract(const KxDataViewItem::Vector& tItems, const wxString& sOutPath)
{
	if (!tItems.empty())
	{
		bacBSA::RecordIndexesList tElements;
		tElements.reserve(tItems.size());
		for (const auto& item: tItems)
		{
			tElements.push_back(m_FilesViewModel->GetFileIndexByRow(m_FilesViewModel->GetRow(item)));
		}

		m_Thread = new bacArchiveExtractFilesThread(this, this->GetEventHandler(), tElements, sOutPath);
		BeginExtract();
	}
	else
	{
		wxCommandEvent event(bacEVT_ARCHIVE_ERROR);
		event.SetInt(BSA_STATUS_NO_FILES_TO_EXTRACT);
		HandleWindowEvent(event);
	}
}
void bacMainWindow::BeginFolderExtract(const KxTreeListItem& tRootItem, const wxString& sOutPath, bool bRecursive)
{
	bacBSA::RecordIndexesList tElements;
	auto AddFolder = [this, &tElements](const KxTreeListItem& tItem)
	{
		bacArchiveFolderRef* pItemData = static_cast<bacArchiveFolderRef*>(tItem.GetData());
		if (pItemData)
		{
			if (pItemData->GetIndex() != (size_t)-1)
			{
				tElements.push_back(pItemData->GetIndex());
			}
		}
	};

	AddFolder(tRootItem);
	if (bRecursive)
	{
		std::function<void(const KxTreeListItem&)> Recurse;
		Recurse = [&Recurse, &AddFolder, &tElements](const KxTreeListItem& tItem)
		{
			KxTreeListItem tCurrentItem = tItem;
			while (tCurrentItem.IsOK())
			{
				AddFolder(tCurrentItem);

				KxTreeListItem tChild = tCurrentItem.GetFirstChild();
				if (tChild.IsOK())
				{
					Recurse(tChild);
				}
				tCurrentItem = tCurrentItem.GetNextSibling();
			}
		};
		Recurse(tRootItem.GetFirstChild());
	}

	m_Thread = new bacArchiveExtractFolderThread(this, this->GetEventHandler(), tElements, sOutPath);
	BeginExtract();
}
void bacMainWindow::BeginExtract()
{
	// Disable main window as progress dialog is modeless and doesn't disables its parent automatically
	// Re-enabled in OnExtractionEnd()
	Disable();
	m_ThreadStatusDialog->SetAutoSize(true);
	m_ThreadStatusDialog->Show();

	// m_Thread will become nullptr after thread is successfully finished
	if (m_Thread->Run() != wxTHREAD_NO_ERROR)
	{
		delete m_Thread;
		m_Thread = nullptr;
	}
}
