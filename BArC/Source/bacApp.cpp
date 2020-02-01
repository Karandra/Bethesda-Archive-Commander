#include "stdafx.h"
#include "bacApp.h"
#include "bacBSA.h"
#include "GUI/bacMainWindow.h"
#include "KxFramework/KxFile.h"
#include "KxFramework/KxSystemSettings.h"

KxMenuItem* bacApp::CreateMenuItem(const wxString& sLabel, bacIMG::Enum nID)
{
	auto pItem = new KxMenuItem(T(sLabel));
	if (nID != bacIMG::NONE)
	{
		pItem->SetBitmap(Get().GetBitmap(nID));
	}
	return pItem;
}

void bacApp::InitImageList()
{
	m_ImageList.Create(16, 16);
	auto Add = [this](const char* sName) -> int
	{
		return m_ImageList.Add(wxBitmap(wxString::Format("UI\\Icons\\%s.png", sName), wxBITMAP_TYPE_PNG));
	};

	// These names should follow order of bacIMG enum
	Add("box");
	Add("box_document");
	Add("cactus");
	Add("chain");
	Add("diamond");
	Add("disk");
	Add("disks");
	Add("document");
	Add("document_binary");
	Add("document_code");
	Add("document_config");
	Add("document_flash_movie");
	Add("document_image");
	Add("document_text");
	Add("document_text_image");
	Add("document_xaml");
	Add("document_zipper");
	Add("ear_listen");
	Add("edit");
	Add("film");
	Add("film_timeline");
	Add("folder_horizontal");
	Add("folder_horizontal_open");
	Add("folder_open");
	Add("folder_plus");
	Add("folder_zipper");
	Add("information_frame");
	Add("map");
	Add("maps");
	Add("music");
	Add("pictures");
	Add("plug");
	Add("plug_disconnect");
	Add("script");
	Add("script_binary");
	Add("script_code");
	Add("stickman_run");
	Add("speaker_volume");
	Add("tree");
	Add("ui_check_boxes_list");
	Add("ui_layered_pane");
	Add("ui_menu");
}

bacApp::bacApp()
{
	SetAppName("BArC");
	SetAppDisplayName("Bethesda Archive Commander");
	InitImageList();
}
bacApp::~bacApp()
{

}

bool bacApp::OnInit()
{
	KxFileStream optionsFile(wxString("Settings.ini"), KxFileStream::Access::Read);
	m_AppOptions.Load(optionsFile);

	m_AvailableTranslations = KxTranslation::FindTranslationsInDirectory(KxFile("Lang").GetFullPath());
	if (!m_AvailableTranslations.empty())
	{
		wxString locale = m_AppOptions.GetValue("General", "Language", KxTranslation::GetSystemPreferredLocale());
		if (m_AvailableTranslations.count(locale))
		{
			m_CurrentLocale = locale;
		}
		else
		{
			m_CurrentLocale = m_AvailableTranslations.begin()->first;
		}

		m_Translation.LoadFromFile("Lang\\" + m_CurrentLocale + ".xml");
		KxTranslation::SetCurrent(m_Translation);
	}
	else
	{
		wxMessageBox("No translations found. Terminating", "Error", wxOK|wxICON_ERROR, nullptr);
		return false;
	}

	m_MainWindow = new bacMainWindow();
	m_MainWindow->Show();
	return true;
}

int bacApp::OnExit()
{
	m_AppOptions.SetValue("General", "Language", m_CurrentLocale);

	KxFileStream optionsFile(wxString("Settings.ini"), KxFileStream::Access::Read, KxFileStream::Disposition::CreateAlways);
	m_AppOptions.Save(optionsFile);

	return 0;
}

wxIMPLEMENT_APP(bacApp);
