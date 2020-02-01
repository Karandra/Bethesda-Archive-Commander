#pragma once
#define _CRT_SECURE_NO_DEPRECATE 1

#if _WIN64
#pragma comment(lib, "Bin/KxFramework x64.lib")
#else
#pragma comment(lib, "Bin/KxFramework.lib")
#endif

/* KxFramework */
#include <KxFramework/KxFramework.h>
#include <KxFramework/KxWinUndef.h>

#include "KxFramework/KxApp.h"
#include "KxFramework/KxTranslation.h"
#include "KxFramework/KxSystem.h"
#include "KxFramework/KxSystemSettings.h"

#include "KxFramework/KxFrame.h"
#include "KxFramework/KxDialog.h"
#include "KxFramework/KxTaskDialog.h"

#include "KxFramework/KxLabel.h"
#include "KxFramework/KxAuiToolBar.h"


template<class vID> wxString T(const vID& s)
{
	bool bSuccess = false;
	wxString sOut = KxTranslation::GetCurrent().GetString(s, &bSuccess);
	if (bSuccess)
	{
		return sOut;
	}
	else
	{
		return wxString::Format("$T(%s)", s);
	}
}
inline wxString T(int nID)
{
	wxString sOut = KxUtility::GetStandardLocalizedString(nID);
	if (!sOut.IsEmpty())
	{
		return sOut;
	}
	else
	{
		return wxString::Format("$T(%d)", nID);
	}
}
inline wxString T(wxStandardID nID)
{
	return T((int)nID);
}
inline wxString T(KxStandardID nID)
{
	return T((int)nID);
}

enum bsaStatus;
inline wxString T(bsaStatus nID)
{
	return T(wxString::Format("BSA.Status%d", (int)nID));
}

struct bacIMG
{
	enum Enum: int
	{
		NONE = -1,

		box,
		box_document,
		cactus,
		chain,
		diamond,
		disk,
		disks,
		document,
		document_binary,
		document_code,
		document_config,
		document_flash_movie,
		document_image,
		document_text,
		document_text_image,
		document_xaml,
		document_zipper,
		ear_listen,
		edit,
		film,
		film_timeline,
		folder_horizontal,
		folder_horizontal_open,
		folder_open,
		folder_plus,
		folder_zipper,
		information_frame,
		map,
		maps,
		music,
		pictures,
		plug,
		plug_disconnect,
		script,
		script_binary,
		script_code,
		stickman_run,
		speaker_volume,
		tree,
		ui_check_boxes_list,
		ui_layered_pane,
		ui_menu,

		MAX,
	};
};
