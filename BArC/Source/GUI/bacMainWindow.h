#pragma once
#include "stdafx.h"
#include "KxFramework/KxFrame.h"
#include "KxFramework/KxTreeView.h"
#include "KxFramework/KxTreeList.h"
#include "KxFramework/KxStatusBarEx.h"
#include "KxFramework/KxAuiToolBar.h"
#include "KxFramework/KxMenu.h"
#include "KxFramework/KxArchiveEvent.h"
#include "KxFramework/KxSplitterWindow.h"
#include "KxFramework/KxDataView.h"

#include "bacApp.h"
#include "bacBSA.h"

class KxDualProgressDialog;
class bacArchiveFilesViewModel;
class bacArchiveFolderRef;
class bacArchiveExtractThreadBase;

class bacMainWindow: public KxFrame
{
	friend class bacArchiveExtractThreadBase;

	public:
		static bacMainWindow& Get()
		{
			return *bacApp::Get().GetMainWindow();
		}

	private:
		enum ArcStatus
		{
			SelectedFolder,
			SelectedFile,
			Total,

			MAX
		};
		typedef std::unique_ptr<bacBSA> ArcPtr;

	private:
		// Controls
		KxStatusBarEx* m_StatusBar = nullptr;
		wxMenuBar* m_MenuBar = nullptr;
		KxAuiToolBar* m_ToolBar = nullptr;
		KxSplitterWindow* m_ViewSplitter = nullptr;
		KxTreeList* m_FoldersView = nullptr;
		bacArchiveFilesViewModel* m_FilesViewModel = nullptr;

		// Layout
		wxBoxSizer* m_Sizer;

		// Menus
		KxMenu* m_MenuFile = nullptr;
		KxMenuItem* m_MenuFile_CloseArchive = nullptr;

		KxMenu* m_MenuCommands = nullptr;
		KxMenuItem* m_MenuCommands_Extract = nullptr;
		KxMenuItem* m_MenuCommands_ExtractFolder = nullptr;
		KxMenuItem* m_MenuCommands_ExtractFolderRecursive = nullptr;

		KxMenu* m_MenuLanguage = nullptr;

		// ToolBar buttons
		KxAuiToolBarItem* m_TB_InfoButton = nullptr;
		KxAuiToolBarItem* m_TB_TestButton = nullptr;

		// Current loaded file
		wxString m_ArchiveName;
		ArcPtr m_Archive;
		std::unordered_map<wxString, bacArchiveFolderRef> m_ArchiveContent;

		// Extracting thread
		KxDualProgressDialog* m_ThreadStatusDialog = nullptr;
		bacArchiveExtractThreadBase* m_Thread = nullptr;
		wxCriticalSection m_ThreadCS;

	private:
		void CreateMenus();
		void CreateToolBar();
		void CreateLayout();
		void CreateFolderView();
		void CreateFileView();

	private:
		void CloseArchiveView();

		void OpenArchiveView(ArcPtr& pArchive);
		void ArchiveViewConfigureUI();
		void LoadArchiveFiles(bacArchiveFolderRef& tFolderInfo);
		void LoadArchiveStatus();

		void BeginFileExtract(const KxDataViewItem::Vector& tItems, const wxString& sOutPath);
		void BeginFolderExtract(const KxTreeListItem& tRootItem, const wxString& sOutPath, bool bRecursive = false);
		void BeginExtract();

	public:
		static const int LayoutDefaultBorder = 3;

		bacMainWindow();
		virtual ~bacMainWindow();

	public:
		bool IsArchiveOpen() const
		{
			return m_Archive && m_Archive->IsLoaded();
		}
		
		// Get image by extension (dds, nif, wav, ...)
		bacIMG::Enum GetImageByType(const wxString& sExt);

		wxString GetArchiveName() const
		{
			return m_ArchiveName;
		}
		const bacBSA* GetArchive() const
		{
			return m_Archive.get();
		}

	private:
		void OnOpenFile(wxCommandEvent& event);
		void OnSelectFile(KxDataViewEvent& event);
		void OnActivateFile(KxDataViewEvent& event);
		void OnFileContextMenu(KxDataViewEvent& event);
		void OnSelectFolder(wxTreeListEvent& event);

		void OnTestArchive(KxAuiToolBarEvent& event);

		void OnExtractFileCommand(wxCommandEvent& event);
		void OnExtractFolderCommand(wxCommandEvent& event);
		
		void OnExtractionEnd(wxCommandEvent& event);
		void OnExtractionCancel(wxNotifyEvent& event);
		void OnExtractionError(wxCommandEvent& event);
		void OnExtractionStatusChange(KxArchiveEvent& event);
};

template<class T> class bacArchiveItemRef
{
	protected:
		T m_Value;

	public:
		bacArchiveItemRef() {}
		bacArchiveItemRef(T v)
			:m_Value(v)
		{
		}

	public:
		T GetIndex() const
		{
			return m_Value;
		}
};
class bacArchiveFolderRef: public wxClientData, public bacArchiveItemRef<size_t>
{
	public:
		KxTreeListItem Item;
		const bool m_HasFiles = false;

	public:
		bacArchiveFolderRef(size_t i, bool bHasFiles, const KxTreeListItem& tItem)
			:bacArchiveItemRef(i), m_HasFiles(bHasFiles), Item(tItem)
		{
			if (!bHasFiles)
			{
				m_Value = (size_t)-1;
			}
		}

	public:
		bool HasFiles() const
		{
			return m_HasFiles;
		}
};
class bacArchiveFileRef: public wxClientData, public bacArchiveItemRef<size_t>
{
	public:
		bacArchiveFileRef(size_t i)
			:bacArchiveItemRef(i)
		{
		}
};
