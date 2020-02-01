#pragma once
#include "KxFramework/KxStdDialog.h"
#include "KxFramework/KxProgressBar.h"
#include "KxFramework/KxAuiNotebook.h"

#include "bacBSA.h"

#define BAC_ADD_FLAG(control, index, flags, name)																\
	control->GetRoot().Add({T("ArchiveInfo."#name), std::to_string(name), #name}).SetChecked(flags & name);	\
	index++	

class KxTreeList;
class bacMainWindow;
class bacArchiveInfoWindow: public KxStdDialog
{
	private:
		bacMainWindow* m_MainWindow = nullptr;
		KxAuiNotebook* m_TabsWindow = nullptr;
		wxFlexGridSizer* m_InfoSizer = nullptr;
		KxProgressBar* m_ArchiveRatioView = nullptr;

	private:
		virtual int GetViewSizerProportion() const override
		{
			return 1;
		}
		virtual wxOrientation GetViewSizerOrientation() const override
		{
			return wxHORIZONTAL;
		}
		virtual wxOrientation GetViewLabelSizerOrientation() const override
		{
			return wxHORIZONTAL;
		}
		virtual bool IsEnterAllowed(wxKeyEvent& tEvent, wxWindowID* pID) const override
		{
			return true;
		}

	private:
		void DisplayInfo(wxPanel* hPanel);
		void AddUnknownFlags(KxTreeList* hControl, size_t nStartIndex, uint32_t nFlags);
		KxTreeList* CreateFlagListCtrl();

		wxWindow* CreateTab_Info();
		wxWindow* CreateTab_ArchiveFlags();
		wxWindow* CreateTab_ContentFlags();

	public:
		bacArchiveInfoWindow(bacMainWindow* pParent);
		virtual ~bacArchiveInfoWindow();

	public:
		virtual wxWindow* GetDialogMainCtrl() const override
		{
			return m_TabsWindow;
		}
		
};

