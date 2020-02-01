#include "stdafx.h"
#include "bacBSA.h"
#include "KxFramework/KxString.h"
#include "KxFramework/KxArchiveEvent.h"
#include "KxFramework/KxUtility.h"
#include "ZLib/ZLib.hpp"
#include "LZ4/LZ4.hpp"

const bacBSA bacBSA::NullArchive;

wxString bacBSA::FormatHash(HashType nHash)
{
	return wxString::Format("0x%0llx", nHash);
}

/*
The hash cannot be calculated using / (forward slashes) or upper case characters. Forward slashes
must be changed to backslashes and upper case characters must be converted to lower case.
Folder hash values must not include leading or trailing slashes. While some folder names contain a file extension
(e.g. textures\actors\character\facegendata\facetint\skyrim.esm\), this must not be treated as a
file extension in the calculation. Some parts of the algorithm need to perform calculations on the extension
separately from the filename. The filename substring must be only the filename with no slashes or extension
(e.g. meshes\animbehaviors\doors\doortemplate01.hkx -> doortemplate01). The extension must include the '.'.

http://en.uesp.net/wiki/Tes4Mod:Hash_Calculation
*/
bacBSA::HashType bacBSA::HashPath(const wxString& sSource, bool bFolderPath, wxString* pCorrectedPath)
{
	if (!sSource.IsEmpty())
	{
		using wxCharType = wxString::char_type::value_type;
		wxString sPath = KxString::ToLower(sSource);
		sPath.Replace("/", "\\");

		uint64_t nHash1 = 0;
		uint64_t nHash2 = 0;

		// If this is a file with an extension
		int nExtIndex = -1;
		bool bHasExt = false;
		if (!bFolderPath)
		{
			sPath = sPath.AfterLast('\\');

			nExtIndex = sPath.Find('.', true);
			bHasExt = nExtIndex != wxNOT_FOUND;
		}

		// Hash 1
		if (bHasExt)
		{
			for (size_t i = nExtIndex; i < sPath.Length(); i++)
			{
				nHash1 = (nHash1 * 0x1003f) + (wxCharType)sPath[i];
			}

			// From here on, path must NOT include the file extension.
			sPath.Truncate(nExtIndex);
		}

		if (sPath.Length() > 2)
		{
			for (size_t i = 1; i < sPath.Length() - 2; i++)
			{
				nHash2 = (nHash2 * 0x1003f) + (wxCharType)sPath[i];
			}
		}
		nHash1 += nHash2;
		nHash2 = 0;

		// Move Hash 1 to the upper bits
		nHash1 <<= 32;

		// Hash 2
		nHash2 = (wxCharType)sPath[sPath.Length() - 1];
		nHash2 |= (sPath.Length() > 2) ? (wxCharType)sPath[sPath.Length() - 2] << 8 : 0;
		nHash2 |= sPath.Length() << 16;
		nHash2 |= ((wxCharType)sPath[0] << 24);

		if (bHasExt)
		{
			const int nExtLength = sizeof(uint32_t);
			char sExt[nExtLength + 1] = {'\0'};
			for (size_t i = 0; i < nExtLength; i++)
			{
				size_t nCharIndex = nExtIndex + i;
				if (nCharIndex < sSource.Length())
				{
					sExt[i] = (char)sSource[nCharIndex];
				}
			}

			// Load these 4 bytes as integer
			switch (*(uint32_t*)sExt)
			{
				// 2E 6B 66 00 == .kf\0
				case 0x00666B2E:
				{
					nHash2 |= 0x80;
					break;
				}

				// .nif
				case 0x66696E2E:
				{
					nHash2 |= 0x8000;
					break;
				}

				// .dds
				case 0x7364642E:
				{
					nHash2 |= 0x8080;
					break;
				}

				// .wav
				case 0x7661772E:
				{
					nHash2 |= 0x80000000;
					break;
				}
			};
		}

		KxUtility::SetIfNotNull(pCorrectedPath, sPath);
		return nHash1 + nHash2;
	}
	return 0;
}

bsaStatus bacBSA::IsHeaderOK() const
{
	bsaStatus nWhy = BSA_STATUS_SUCCESS;
	if (std::memcmp(m_Header.Signature, BSA_SIGNATURE, sizeof(m_Header.Signature)) == 0)
	{
		if (m_Header.Offset == sizeof(m_Header))
		{
			if (m_Header.Version == BSA_VERSION_OBLIVION || m_Header.Version == BSA_VERSION_SKYRIM)
			{
				if (!IsFlagsSupported())
				{
					nWhy = BSA_STATUS_FLAGS;
				}
			}
			else
			{
				nWhy = BSA_STATUS_VERSION;
			}
		}
		else
		{
			nWhy = BSA_STATUS_STRUCTURE;
		}
	}
	else
	{
		nWhy = BSA_STATUS_SIGNATURE;
	}
	return nWhy;
}
bool bacBSA::ReadArchive()
{
	return ReadHeader() && ReadFolderRecords() && ReadFileRecords() && ReadFileNamesRecords() && ReadFileDataRecords();
}
bool bacBSA::ReadHeader()
{
	if (m_Stream.ReadObject(m_Header))
	{
		m_Status = IsHeaderOK();
		m_HeaderOk = m_Status == BSA_STATUS_SUCCESS;
		if (!m_HeaderOk)
		{
			m_Header = NullArchive.m_Header;
		}
		return m_HeaderOk;
	}
	else
	{
		m_Status = BSA_STATUS_STREAM;
		return false;
	}
}
bool bacBSA::ReadFolderRecords()
{
	bool bSuccess = false;
	m_Folders.reserve(m_Header.FoldersCount + 1);
	for (size_t i = 0; i < m_Header.FoldersCount; i++)
	{
		if (!m_Stream.ReadObject(m_Folders.emplace_back()))
		{
			m_Folders.pop_back();
			m_Status = BSA_STATUS_FOLDER_RECORDS;
			return false;
		}
	}
	return true;
}
bool bacBSA::ReadFileRecords()
{
	bool bSuccess = false;
	m_Files.reserve(m_Header.FilesCount + 1);
	m_FolderNames.reserve(m_Header.FoldersCount + 1);

	wxMemoryBuffer tBuffer(255);
	for (size_t i = 0; i < m_Header.FoldersCount; i++)
	{
		// Read and save folder name
		auto nLength = m_Stream.ReadObject<uint8_t>();

		tBuffer.SetBufSize(nLength);
		tBuffer.SetDataLen(nLength);
		m_Stream.Read(tBuffer.GetData(), nLength);
		m_FolderNames.push_back(wxString::FromUTF8((const char*)tBuffer.GetData(), nLength - 1));

		// Read file record
		for (size_t nFilesRecordIndex = 0; nFilesRecordIndex < m_Folders[i].Count; nFilesRecordIndex++)
		{
			if (!m_Stream.ReadObject(m_Files.emplace_back()))
			{
				m_Files.pop_back();
				m_Status = BSA_STATUS_FILE_RECORDS;
				return false;
			}
		}
	}
	return true;
}
bool bacBSA::ReadFileNamesRecords()
{
	bool bSuccess = false;
	wxMemoryBuffer tBuffer(255);
	m_FolderNames.reserve(m_Header.FoldersCount + 1);
	for (size_t i = 0; i < m_Header.FilesCount; i++)
	{
		tBuffer.SetDataLen(0);
		
		char c = '\0';
		do
		{
			c = '\0';
			if (!m_Stream.ReadObject(c))
			{
				m_Status = BSA_STATUS_FILE_NAMES;
				return false;
			}
			tBuffer.AppendByte(c);
		}
		while (c);
		m_FileNames.emplace_back(wxString::FromUTF8((const char*)tBuffer.GetData(), tBuffer.GetDataLen() - 1));
	}

	return true;
}
bool bacBSA::ReadFileDataRecords()
{
	m_OriginalSize = m_Stream.Tell();

	bool bSuccess = true;
	m_FileData.reserve(m_Header.FilesCount + 1);
	for (size_t i = 0; i < m_Header.FilesCount; i++)
	{
		const auto& tFileRecord = m_Files[i];
		m_Stream.SeekFromStart(tFileRecord.Offset);
		if (IsFileNamesEmbedded())
		{
			SkipBString(m_Stream);
		}

		bsaFileData tData;
		tData.FileIndex = i;
		tData.RawDataOffset = tFileRecord.Offset;
		tData.OriginalSize = tFileRecord.Size;
		tData.CompressedSize = tFileRecord.Size;
		if (tFileRecord.IsCompressed(m_Header))
		{
			// Read original size
			tData.OriginalSize = m_Stream.ReadObject<uint32_t>();
			m_OriginalSize += tData.OriginalSize;
		}
		else
		{
			// This size is original size
			m_OriginalSize += tFileRecord.Size;
		}
		m_FileData.push_back(tData);
		
		if (!bSuccess)
		{
			m_Status = BSA_STATUS_FILE_DATA;
			return false;
		}
	}
	return true;
}

bool bacBSA::SkipBString(KxFileStream& tStream)
{
	if (uint8_t value = 0; tStream.ReadObject<uint8_t>())
	{
		tStream.Seek(value);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bacBSA::bacBSA()
{
}
bacBSA::bacBSA(const wxString& sFilePath)
{
	Load(sFilePath);
}
bool bacBSA::Load(const wxString& sFilePath)
{
	if (!IsLoaded())
	{
		bool bLoaded = m_Stream.Open(sFilePath, KxFileStream::Access::Read, KxFileStream::Disposition::OpenExisting, KxFileStream::Share::Read);
		m_Loaded = bLoaded;
		if (bLoaded)
		{
			ReadArchive();
		}
		else
		{
			m_Status = BSA_STATUS_STREAM;
		}
		return m_Loaded;
	}
	return false;
}
bacBSA::~bacBSA()
{
	UnLoad();
}
bool bacBSA::UnLoad()
{
	if (IsLoaded())
	{
		m_Header = NullArchive.m_Header;
		m_Folders.clear();
		m_Files.clear();
		m_FolderNames.clear();
		m_FileNames.clear();
		m_FileData.clear();
		m_OriginalSize = NullArchive.m_OriginalSize;
		
		m_Loaded = NullArchive.m_Loaded;
		m_HeaderOk = NullArchive.m_HeaderOk;
		m_Status = NullArchive.m_Status;

		m_Stream.Close();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool bacBSA::IsFlagsSupported() const
{
	// BSA_DEFAULT_COMPRESSED itself is supported, changing it isn't
	if ((m_Header.ArchiveFlags & BSA_REQUIRED))
	{
		bsaArchiveFlags nUnsupportedFlags = BSA_NONE;
		if (m_Header.Version == BSA_VERSION_OBLIVION)
		{
			nUnsupportedFlags = BSA_UNSUPPORTED_FLAGS_OBLIVION;
		}
		else if (m_Header.Version == BSA_VERSION_SKYRIM)
		{
			nUnsupportedFlags = BSA_UNSUPPORTED_FLAGS_SKYRIM;
		}
		return !(m_Header.ArchiveFlags & nUnsupportedFlags);
	}
	return false;
}
bacBSA::RecordIndexesList bacBSA::GetFilesInFolder(size_t nFolderIndex) const
{
	RecordIndexesList tFiles;

	const bsaFolderRecord* pFolderRecord = GetFolderRecord(nFolderIndex);
	if (pFolderRecord)
	{
		size_t nFileIndexBase = 0;
		for (size_t i = 0; i < nFolderIndex; i++)
		{
			nFileIndexBase += GetFolderRecord(i)->Count;
		}

		tFiles.reserve(pFolderRecord->Count);
		for (size_t i = 0; i < pFolderRecord->Count; i++)
		{
			tFiles.push_back(nFileIndexBase + i);
		}
	}

	return tFiles;
}

bsaStatus bacBSA::ExtractFile(const bsaFileRecord* pFileRecord, wxOutputStream& tOutStream, wxEvtHandler* pEventHandler) const
{
	if (pFileRecord)
	{
		KxFileStream tArcStream(m_Stream.GetFileName(), KxFileStream::Access::Read, KxFileStream::Disposition::OpenExisting, KxFileStream::Share::Read);
		if (tArcStream.IsOk())
		{
			// Seek to Compressed/Uncompressed block
			tArcStream.SeekFromStart(pFileRecord->Offset);

			// Skip file name if needed
			if (IsFileNamesEmbedded())
			{
				SkipBString(tArcStream);
			}

			const size_t nTotal = pFileRecord->Size;

			KxArchiveEvent event(KxEVT_ARCHIVE_UNPACK);
			if (pEventHandler)
			{
				event.Allow();
				event.SetEventObject(pEventHandler);
				event.SetMinorTotal(nTotal);
			}

			if (pFileRecord->IsCompressed(m_Header))
			{
				// Skip original size field
				tArcStream.Seek(sizeof(uint32_t));

				ZLibUnpack tUnpacker(tArcStream, tOutStream, nTotal);
				//LZ4Unpack tUnpacker(tArcStream, tOutStream, nTotal);
				tUnpacker.Run(pEventHandler);

				return tUnpacker.IsOK() ? BSA_STATUS_SUCCESS : BSA_STATUS_STREAM;
			}
			else
			{
				const size_t nPartSize = 1024 * 32;
				uint8_t tBuffer[nPartSize] = {0};

				for (size_t nProcessed = 0; nProcessed < nTotal; nProcessed += tArcStream.LastRead())
				{
					if (tArcStream.ReadBuffer(tBuffer, std::min(nPartSize, nTotal)))
					{
						tOutStream.Write(tBuffer, std::min(tArcStream.LastRead(), nTotal - nProcessed));
						event.SetMinorProcessed(tOutStream.LastWrite());
						if (pEventHandler)
						{
							pEventHandler->ProcessEvent(event);
							if (!event.IsAllowed())
							{
								break;
							}
						}

						if (tOutStream.GetLastError() != wxSTREAM_NO_ERROR)
						{
							return BSA_STATUS_STREAM;
						}
					}
					else
					{
						return BSA_STATUS_STREAM;
						break;
					}
				}
				return BSA_STATUS_SUCCESS;
			}
		}
		else
		{
			return BSA_STATUS_STREAM;
		}
	}
	return BSA_STATUS_NO_SUCH_FILE;
}
