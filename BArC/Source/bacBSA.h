#pragma once
#include "stdafx.h"
#include "bacArchiveCommon.h"
#include "KxFramework/KxUtility.h"
#include "KxFramework/KxFileStream.h"

struct bsaHeader
{
	char Signature[4] = {'\000'};
	uint32_t Version = 0;
	uint32_t Offset = 0; // Should be sizeof(bsaHeader)
	bsaArchiveFlags ArchiveFlags = BSA_NONE;
	uint32_t FoldersCount = 0;
	uint32_t FilesCount = 0;
	uint32_t TotalFolderNameLength = 0;
	uint32_t TotalFileNameLength = 0;
	bsaContentFlags FileFlags = BSA_CONTENT_NONE;
};
struct bsaFolderRecord
{
	uint64_t NameHash = 0;
	uint32_t Count = 0;
	uint32_t Offset = 0;
};
struct bsaFileRecord
{
	uint64_t NameHash = 0;
	uint32_t Size = 0;
	uint32_t Offset = 0;

	bool IsCompressed(const bsaHeader& tHeader) const
	{
		// This is definitely not the way BSA stores its "compressed" status for files
		// Because this doesn't work at all
		// 
		// const uint32_t nCompressedMask = 1 << 30;
		// return (Size & nCompressedMask) && !(tHeader.ArchiveFlags & BSA_DEFAULT_COMPRESSED);
		return tHeader.ArchiveFlags & BSA_DEFAULT_COMPRESSED;
	}
	void SetCompressed(bsaHeader& tHeader, bool bCompressed)
	{
		const uint32_t nCompressedMask = 1 << 30;
		if (tHeader.ArchiveFlags & BSA_DEFAULT_COMPRESSED)
		{
			KxUtility::ModFlagRef(Size, nCompressedMask, !bCompressed);
		}
		else
		{
			KxUtility::ModFlagRef(Size, nCompressedMask, bCompressed);
		}
	}
};
struct bsaFileData
{
	wxFileOffset RawDataOffset = 0;
	uint32_t OriginalSize = 0;
	uint32_t CompressedSize = 0;
	size_t FileIndex = 0;

	float GetRatio() const
	{
		if (OriginalSize != 0)
		{
			return (float)CompressedSize/(float)OriginalSize;
		}
		return 0;
	}
};

class bacBSA
{
	public:
		static const bacBSA NullArchive;

		typedef std::vector<wxString> FolderNamesList;
		typedef std::vector<wxString> FileNamesList;
		typedef std::vector<size_t> RecordIndexesList;
		typedef uint64_t HashType;
		static wxString FormatHash(HashType nHash);
		static HashType HashPath(const wxString& sSource, bool bFolderPath, wxString* pCorrectedPath = nullptr);

	private:
		KxFileStream m_Stream;
		bsaHeader m_Header;
		std::vector<bsaFolderRecord> m_Folders;
		std::vector<bsaFileRecord> m_Files;
		FolderNamesList m_FolderNames;
		FileNamesList m_FileNames;
		std::vector<bsaFileData> m_FileData;
		wxFileOffset m_OriginalSize = 0;
		
		bool m_Loaded = false;
		bool m_HeaderOk = false;
		bsaStatus m_Status = BSA_STATUS_UNKNOWN;

	private:
		bsaStatus IsHeaderOK() const;
		bool ReadArchive();
		bool ReadHeader();
		bool ReadFolderRecords();
		bool ReadFileRecords();
		bool ReadFileNamesRecords();
		bool ReadFileDataRecords();

		bool IsFileNamesEmbedded() const
		{
			return m_Header.Version == BSA_VERSION_SKYRIM && m_Header.ArchiveFlags & BSA_EMBED_FILE_NAMES;
		}
		static bool SkipBString(KxFileStream& tStream);

	public:
		bacBSA();
		bacBSA(const wxString& sFilePath);
		virtual bool Load(const wxString& sFilePath);
		virtual ~bacBSA();
		virtual bool UnLoad();

	public:
		// Is file loaded. Doesn't tell is file are valid BSA
		bool IsLoaded() const
		{
			return m_Loaded;
		}
		
		// Is loaded file valid and its format are supported
		bool IsOK() const
		{
			return m_HeaderOk && GetStatus() == BSA_STATUS_SUCCESS;
		}
		bsaStatus GetStatus() const
		{
			return m_Status;
		}
		
		// Is archive can be opened even if some errors occurred?
		bool CanIgnoreErrors() const
		{
			return m_HeaderOk;
		}

		bool IsFlagsSupported() const;
		bsaArchiveFlags GetArchiveFlags() const
		{
			return m_Header.ArchiveFlags;
		}
		bsaContentFlags GetContentFlags() const
		{
			return m_Header.FileFlags;
		}
		int GetVersion() const
		{
			return m_Header.Version;
		}
		
		float GetArchiveRatio() const
		{
			if (GetArchiveOriginalSize() != 0)
			{
				return (float)GetArchiveCompressedSize()/(float)GetArchiveOriginalSize();
			}
			return 0;
		}
		wxFileOffset GetArchiveCompressedSize() const
		{
			return m_Stream.GetLength();
		}
		wxFileOffset GetArchiveOriginalSize() const
		{
			return m_OriginalSize;
		}

		uint32_t GetFoldersCount() const
		{
			return m_Header.FoldersCount;
		}
		uint32_t GetFilesCount() const
		{
			return m_Header.FilesCount;
		}
		uint32_t GetFilesCount(size_t nFolderIndex) const
		{
			if (const bsaFolderRecord* pFolder = GetFolderRecord(nFolderIndex))
			{
				return pFolder->Count;
			}
			return 0;
		}
		
		const FileNamesList& GetFilesList() const
		{
			return m_FileNames;
		}
		const FolderNamesList& GetFoldersList() const
		{
			return m_FolderNames;
		}
		RecordIndexesList GetFilesInFolder(size_t nFolderIndex) const;
		
		const bsaHeader& GetHeader() const
		{
			return m_Header;
		}
		const bsaFolderRecord* GetFolderRecord(size_t i) const
		{
			if (i < m_Folders.size())
			{
				return &m_Folders[i];
			}
			return nullptr;
		}
		const bsaFileRecord* GetFileRecord(size_t i) const
		{
			if (i < m_Files.size())
			{
				return &m_Files[i];
			}
			return nullptr;
		}
		const bsaFileData* GetFileDataRecord(size_t i) const
		{
			if (i < m_FileData.size())
			{
				return &m_FileData[i];
			}
			return nullptr;
		}

		bsaStatus ExtractFile(size_t nFileIndex, wxOutputStream& tOutStream, wxEvtHandler* pEventHandler = nullptr) const
		{
			auto pFileRecord = GetFileRecord(nFileIndex);
			return pFileRecord ? ExtractFile(pFileRecord, tOutStream, pEventHandler) : BSA_STATUS_NO_SUCH_FILE;
		}
		bsaStatus ExtractFile(const bsaFileRecord* pFileRecord, wxOutputStream& tOutStream, wxEvtHandler* pEventHandler = nullptr) const;
};
