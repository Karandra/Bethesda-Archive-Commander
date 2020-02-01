#include "stdafx.h"
#include "bacArchiveFilesViewModel.h"
#include "bacMainWindow.h"
#include "KxFramework/KxFile.h"
#include "KxFramework/KxFileOperationEvent.h"
#include "KxFramework/KxFileBrowseDialog.h"
#include "KxFramework/KxTextBoxDialog.h"
#include "KxFramework/KxString.h"
#include "KxFramework/KxShell.h"

enum ColumnID
{
	Name,
	Type,
	SizeOriginal,
	SizeCompressed,
	Hash,
};

void bacArchiveFilesViewModel::OnInitControl()
{
	KxDataViewColumnFlags columnFlags = KxDV_COL_DEFAULT_FLAGS| (IsVirtualListModel() ? KxDV_COL_NONE : KxDV_COL_SORTABLE);

	GetView()->AppendColumn<KxDataViewBitmapTextRenderer>(T("FilesView.Name"), ColumnID::Name, KxDATAVIEW_CELL_INERT, 300, columnFlags);
	GetView()->AppendColumn<KxDataViewTextRenderer>(T("FilesView.SizeOriginal"), ColumnID::SizeOriginal, KxDATAVIEW_CELL_INERT, 115, columnFlags);
	GetView()->AppendColumn<KxDataViewTextRenderer>(T("FilesView.Type"), ColumnID::Type, KxDATAVIEW_CELL_INERT, 150, columnFlags);
	GetView()->AppendColumn<KxDataViewTextRenderer>(T("FilesView.SizeCompressed"), ColumnID::SizeCompressed, KxDATAVIEW_CELL_INERT, 115, columnFlags);
	GetView()->AppendColumn<KxDataViewTextRenderer>(T("FilesView.Hash"), ColumnID::Hash, KxDATAVIEW_CELL_INERT, 115, columnFlags);

	GetView()->Bind(KxEVT_DATAVIEW_COLUMN_HEADER_CLICK, [this](KxDataViewEvent& event)
	{
		if (!event.GetColumn())
		{
			if (KxDataViewColumn* column = GetView()->GetSortingColumn())
			{
				column->ResetSorting();
			}
		}
		event.Skip();
	});
}

bool bacArchiveFilesViewModel::GetItemAttributesByRow(size_t row, const KxDataViewColumn* column, KxDataViewItemAttributes& attribute, KxDataViewCellState cellState) const
{
	return false;
	if (column->GetID() == ColumnID::Type)
	{
		attribute.SetBackgroundColor(KxColor(50, 0, 75));
		return true;
	}
	return false;
}
void bacArchiveFilesViewModel::GetValueByRow(wxAny& value, size_t row, const KxDataViewColumn* column) const
{
	size_t nFileIndex = GetFileIndexByRow(row);
	const bsaFileRecord* pEntry = GetDataEntry(nFileIndex);
	if (pEntry)
	{
		static const wxString ms_SizeErrorLabel = T(KxID_ERROR);

		switch (int columnID = column->GetID())
		{
			case ColumnID::Name:
			{
				const wxString& sName = GetEntryName(nFileIndex);
				int nImage = bacMainWindow::Get().GetImageByType(sName.AfterLast('.'));

				value = KxDataViewBitmapTextValue(sName, bacApp::Get().GetImageList()->GetBitmap(nImage));
				break;
			}
			case ColumnID::Type:
			{
				value = GetTypeName(nFileIndex);
				break;
			}
			case ColumnID::SizeOriginal:
			case ColumnID::SizeCompressed:
			{
				const bsaFileData* pFileDataRecord = m_Archive->GetFileDataRecord(nFileIndex);
				int nRatio = 100.0 * pFileDataRecord->GetRatio();

				if (columnID == ColumnID::SizeOriginal)
				{
					value = KxFile::FormatFileSize(pFileDataRecord->OriginalSize, 2, ms_SizeErrorLabel);
				}
				else
				{
					value = wxString::Format("%s, (%d%%)", KxFile::FormatFileSize(pFileDataRecord->CompressedSize, 2, ms_SizeErrorLabel), nRatio);
				}
				break;
			}
			case ColumnID::Hash:
			{
				value = bacBSA::FormatHash(pEntry->NameHash);
				break;
			}
		};
	}
}
bool bacArchiveFilesViewModel::SetValueByRow(const wxAny& value, size_t row, const KxDataViewColumn* column)
{
	return false;
}
bool bacArchiveFilesViewModel::CompareByRow(size_t row1, size_t row2, const KxDataViewColumn* column) const
{
	size_t nFileIndex1 = m_FileBaseIndex + row1;
	const bsaFileRecord* pEntry1 = GetDataEntry(nFileIndex1);

	size_t nFileIndex2 = m_FileBaseIndex + row2;
	const bsaFileRecord* pEntry2 = GetDataEntry(nFileIndex2);

	if (pEntry1 && pEntry2)
	{
		switch (int columnID = column->GetID())
		{
			case ColumnID::Name:
			{
				return GetEntryName(nFileIndex1) < GetEntryName(nFileIndex2);
			}
			case ColumnID::Type:
			{
				return GetTypeName(nFileIndex1) < GetTypeName(nFileIndex2);
			}
			case ColumnID::SizeOriginal:
			case ColumnID::SizeCompressed:
			{
				const bsaFileData* pFileDataRecord1 = m_Archive->GetFileDataRecord(nFileIndex1);
				const bsaFileData* pFileDataRecord2 = m_Archive->GetFileDataRecord(nFileIndex2);

				if (columnID == ColumnID::SizeCompressed)
				{
					return pFileDataRecord1->CompressedSize < pFileDataRecord2->CompressedSize;
				}
				else
				{
					return pFileDataRecord1->OriginalSize < pFileDataRecord2->OriginalSize;
				}
			}
			case ColumnID::Hash:
			{
				return pEntry1->NameHash < pEntry2->NameHash;
			}
		};
	}
	return row1 < row2;
}

bacArchiveFilesViewModel::bacArchiveFilesViewModel()
{
	SetDataViewFlags(KxDataViewCtrl::DefaultStyle|KxDV_VERT_RULES|KxDV_ALTERNATING_ROW_COLORS|KxDV_MULTIPLE_SELECTION);
}

wxString bacArchiveFilesViewModel::GetTypeName(size_t nFileIndex) const
{
	wxString sExt = GetEntryName(nFileIndex).AfterLast('.');
	auto it = m_FileTypeMap.find(sExt);
	if (it != m_FileTypeMap.end())
	{
		return it->second;
	}
	else
	{
		wxString sTypeName = KxShell::GetTypeName(sExt);
		m_FileTypeMap.insert_or_assign(sExt, sTypeName);
		return sTypeName;
	}
}

void bacArchiveFilesViewModel::SetDataVector(const bacBSA* pArchive, size_t nFolderIndex)
{
	m_Archive = pArchive;
	m_FolderIndex = nFolderIndex;

	const bsaFolderRecord* pFolderRecord = m_Archive->GetFolderRecord(m_FolderIndex);
	if (pFolderRecord && pFolderRecord->Count != 0)
	{
		size_t nFileIndexBase = 0;
		for (size_t i = 0; i < m_FolderIndex; i++)
		{
			nFileIndexBase += m_Archive->GetFolderRecord(i)->Count;
		}

		m_FileBaseIndex = nFileIndexBase;
	}
	RefreshItems();
}
