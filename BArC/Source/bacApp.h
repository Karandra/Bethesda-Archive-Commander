#pragma once
#include "KxFramework/KxApp.h"
#include "KxFramework/KxImageList.h"
#include "KxFramework/KxMenu.h"
#include "KxFramework/KxINI.h"
#include "KxFramework/KxTranslation.h"
class bacMainWindow;

class bacApp: public KxApp<wxApp, bacApp>
{
	public:
		static bacApp& Get()
		{
			return *static_cast<bacApp*>(GetInstance());
		}
		static KxMenuItem* CreateMenuItem(const wxString& sLabel, bacIMG::Enum nID = bacIMG::NONE);

	private:
		bacMainWindow* m_MainWindow = nullptr;
		KxImageList m_ImageList;
		KxINI m_AppOptions;

		KxTranslation m_Translation;
		KxTranslation::AvailableMap m_AvailableTranslations;
		wxString m_CurrentLocale;

	private:
		void InitImageList();

	public:
		bacApp();
		virtual ~bacApp();

	public:
		virtual bool OnInit() override;
		virtual int OnExit() override;

		wxString GetCurrentLocale() const
		{
			return m_CurrentLocale;
		}
		void SetCurrentLocale(const wxString& lcoale)
		{
			m_CurrentLocale = lcoale;
		}

		const KxTranslation::AvailableMap& GetAlailableTranslations() const
		{
			return m_AvailableTranslations;
		}

		bacMainWindow* GetMainWindow()
		{
			return m_MainWindow;
		}
		KxImageList* GetImageList()
		{
			return &m_ImageList;
		}
		const KxImageList* GetImageList() const
		{
			return &m_ImageList;
		}
		wxBitmap GetBitmap(bacIMG::Enum nID)
		{
			return m_ImageList.GetBitmap(nID);
		}
};
wxDECLARE_APP(bacApp);
