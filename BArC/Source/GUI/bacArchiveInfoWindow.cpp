#include "stdafx.h"
#include "bacArchiveInfoWindow.h"
#include "bacMainWindow.h"

#include "KxFramework/KxLabel.h"
#include "KxFramework/KxFile.h"

void bacArchiveInfoWindow::DisplayInfo(wxPanel* hPanel)
{
	using namespace std;
	auto AddLabel = [this, hPanel](const wxString& sValue, bool bAddColon = false)
	{
		auto pLabel = new KxLabel(hPanel, wxID_ANY, bAddColon ? sValue + ':' : sValue);
		return m_InfoSizer->Add(pLabel);
	};
	auto AddLabelL = [this, AddLabel](const wxString& sValue, bool bAddColon = false)
	{
		return AddLabel(T(sValue), bAddColon);
	};

	const bacBSA* pArchive = m_MainWindow->GetArchive();
	const bsaHeader& tArchiveHeader = pArchive->GetHeader();

	AddLabelL("ArchiveInfo.Version", true);
	AddLabel(to_string(tArchiveHeader.Version));

	AddLabelL("ArchiveInfo.CompressedSize", true);
	AddLabel(KxFile::FormatFileSize(pArchive->GetArchiveCompressedSize(), 2));

	AddLabelL("ArchiveInfo.OriginalSize", true);
	AddLabel(KxFile::FormatFileSize(pArchive->GetArchiveOriginalSize(), 2));
	
	int nRatio = pArchive->GetArchiveRatio() * 100;
	m_ArchiveRatioView->SetValue(nRatio);
	AddLabelL("ArchiveInfo.Ratio", true);
	AddLabel(to_string(nRatio) + '%');

	AddLabelL("ArchiveInfo.FoldersCount", true);
	AddLabel(to_string(tArchiveHeader.FoldersCount));

	AddLabelL("ArchiveInfo.FilesCount", true);
	AddLabel(to_string(tArchiveHeader.FilesCount));

	AddLabelL("ArchiveInfo.TotalFolderNamesLength", true);
	AddLabel(to_string(tArchiveHeader.TotalFolderNameLength));

	AddLabelL("ArchiveInfo.TotalFileNamesLength", true);
	AddLabel(to_string(tArchiveHeader.TotalFileNameLength));
}
void bacArchiveInfoWindow::AddUnknownFlags(KxTreeList* hControl, size_t nStartIndex, uint32_t nFlags)
{
	for (size_t nIndex = nStartIndex; nIndex < sizeof(nFlags) * 8; nIndex++)
	{
		uint32_t nValue = (uint32_t)1 << nIndex;
		auto tItem = hControl->GetRoot().Add({T("ArchiveInfo.BitNum") + std::to_string(nIndex), std::to_string(nValue)});
		tItem.SetChecked(nFlags & nValue);
	}
}
KxTreeList* bacArchiveInfoWindow::CreateFlagListCtrl()
{
	KxTreeList* hFlagsView = new KxTreeList(m_TabsWindow, wxID_NONE, KxTreeList::DefaultStyle|wxTL_CHECKBOX|wxTL_NO_HEADER);
	hFlagsView->AddColumn("Name", 250);
	hFlagsView->AddColumn("Value", 75);
	hFlagsView->AddColumn("Constant");

	return hFlagsView;
}

wxWindow* bacArchiveInfoWindow::CreateTab_Info()
{
	KxPanel* hPanel = new KxPanel(m_TabsWindow, KxID_NONE);
	hPanel->SetBackgroundColour(KxSystemSettings::GetColor(wxSYS_COLOUR_WINDOW));

	wxSizer* pMainSizer = new wxBoxSizer(wxHORIZONTAL);
	hPanel->SetSizer(pMainSizer);

	m_ArchiveRatioView = new KxProgressBar(hPanel, wxID_NONE, 100, wxGA_VERTICAL);
	pMainSizer->Add(m_ArchiveRatioView, 0, wxEXPAND|wxLEFT|wxTOP|wxBOTTOM, m_MainWindow->LayoutDefaultBorder);

	m_InfoSizer = new wxFlexGridSizer(2, 0, 0);
	pMainSizer->Add(m_InfoSizer, 0, wxEXPAND|wxLEFT, m_MainWindow->LayoutDefaultBorder);

	DisplayInfo(hPanel);
	return hPanel;
}
wxWindow* bacArchiveInfoWindow::CreateTab_ArchiveFlags()
{
	KxTreeList* hFlagsView = CreateFlagListCtrl();
	auto pArchive = m_MainWindow->GetArchive();
	auto nArchiveFlags = pArchive->GetArchiveFlags();

	size_t nFlagIndex = 0;
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_INCLUDE_DIRECTORY_NAMES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_INCLUDE_FILE_NAMES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_DEFAULT_COMPRESSED);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_RETAIN_DIRECTORY_NAMES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_RETAIN_FILE_NAMES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_RETAIN_FILE_OFFSETS);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_XBOX360);
	if (pArchive->GetVersion() == BSA_VERSION_SKYRIM)
	{
		BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_RETAIN_STRINGS_DURING_STARTUP);
		BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_EMBED_FILE_NAMES);
		BAC_ADD_FLAG(hFlagsView, nFlagIndex, nArchiveFlags, BSA_CODEC_XMEM);
	}
	AddUnknownFlags(hFlagsView, nFlagIndex, nArchiveFlags);

	hFlagsView->Bind(wxEVT_TREELIST_ITEM_CHECKED, [hFlagsView](wxTreeListEvent& event)
	{
		KxTreeListItem(*hFlagsView, event.GetItem()).SetChecked(event.GetOldCheckedState());
	});

	return hFlagsView;
}
wxWindow* bacArchiveInfoWindow::CreateTab_ContentFlags()
{
	KxTreeList* hFlagsView = CreateFlagListCtrl();
	auto pArchive = m_MainWindow->GetArchive();
	auto nFlags = pArchive->GetContentFlags();

	size_t nFlagIndex = 0;
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_MESHES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_TEXTURES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_MENUS);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_SOUNDS);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_VOICES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_SHADERS);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_TREES);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_FONTS);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_MISC);
	BAC_ADD_FLAG(hFlagsView, nFlagIndex, nFlags, BSA_CONTENT_CTL);
	AddUnknownFlags(hFlagsView, nFlagIndex, nFlags);

	hFlagsView->Bind(wxEVT_TREELIST_ITEM_CHECKED, [hFlagsView](wxTreeListEvent& event)
	{
		KxTreeListItem(*hFlagsView, event.GetItem()).SetChecked(event.GetOldCheckedState());
	});

	return hFlagsView;
}

bacArchiveInfoWindow::bacArchiveInfoWindow(bacMainWindow* pParent)
	:m_MainWindow(pParent)
{
	wxString sCaption = wxString::Format("%s - \"%s\"", T("ArchiveInfo.DialogCaption"), pParent->GetArchiveName());
	if (KxStdDialog::Create(pParent, wxID_NONE, sCaption, wxDefaultPosition, wxDefaultSize, KxBTN_OK))
	{
		SetDefaultBackgroundColor();
		GetContentWindow()->SetBackgroundColour(GetBackgroundColour());

		m_TabsWindow = new KxAuiNotebook(GetContentWindow(), wxID_ANY);
		PostCreate(GetPosition());

		m_TabsWindow->AddPage(CreateTab_Info(), T("ArchiveInfo.Tab.Info"), true);
		m_TabsWindow->AddPage(CreateTab_ArchiveFlags(), T("ArchiveInfo.Tab.ArchiveFlags"));
		m_TabsWindow->AddPage(CreateTab_ContentFlags(), T("ArchiveInfo.Tab.ContentFlags"));

		SetMainIcon(KxICON_NONE);
		AdjustWindow(wxDefaultPosition, wxSize(600, 400));
	}
}
bacArchiveInfoWindow::~bacArchiveInfoWindow()
{
}
