#pragma once
#include "stdafx.h"
#include "bacBSA.h"
#include "KxFramework/KxDataView.h"
#include "KxFramework/KxDataViewListModelEx.h"

class bacArchiveFilesViewModel: public KxDataViewListModelEx
{
	private:
		mutable std::unordered_map<wxString, wxString> m_FileTypeMap;

		const bacBSA* m_Archive = nullptr;
		size_t m_FolderIndex = -1;
		size_t m_FileBaseIndex = -1;

	private:
		virtual void OnInitControl() override;

		virtual bool GetItemAttributesByRow(size_t row, const KxDataViewColumn* column, KxDataViewItemAttributes& attribute, KxDataViewCellState cellState) const override;
		virtual void GetValueByRow(wxAny& value, size_t row, const KxDataViewColumn* column) const override;
		virtual bool SetValueByRow(const wxAny& value, size_t row, const KxDataViewColumn* column) override;
		virtual bool CompareByRow(size_t row1, size_t row2, const KxDataViewColumn* column) const override;

	public:
		bacArchiveFilesViewModel();

	public:
		virtual size_t GetItemCount() const override
		{
			return m_Archive ? m_Archive->GetFilesCount(m_FolderIndex) : 0;
		}

		void SetDataVector()
		{
			m_Archive = nullptr;
			m_FolderIndex = -1;
			m_FileBaseIndex = -1;

			ItemsCleared();
		}
		void SetDataVector(const bacBSA* pArchive, size_t nFolderIndex);

		size_t GetFileIndexByRow(size_t nRow) const
		{
			return m_FileBaseIndex + nRow;
		}
		wxString GetTypeName(size_t nFileIndex) const;

		const wxString& GetEntryName(size_t nFileIndex) const
		{
			return m_Archive->GetFilesList()[nFileIndex];
		}
		const wxString& GetEntryName(const KxDataViewItem& tItem) const
		{
			return GetEntryName(GetFileIndexByRow(GetRow(tItem)));
		}
		
		const bsaFileRecord* GetDataEntryByRow(size_t nRow) const
		{
			return GetDataEntry(GetFileIndexByRow(nRow));
		}
		const bsaFileRecord* GetDataEntry(size_t nFileIndex) const
		{
			if (m_Archive && nFileIndex < m_Archive->GetFilesCount())
			{
				return m_Archive->GetFileRecord(nFileIndex);
			}
			return nullptr;
		}
		const bsaFileRecord* GetDataEntry(const KxDataViewItem& tItem) const
		{
			return GetDataEntryByRow(GetRow(tItem));
		}
};
