#pragma once
#include "stdafx.h"

static const int BSA_VERSION_OBLIVION = 103;
static const int BSA_VERSION_SKYRIM = 104;
static const int BSA_VERSION_SKYRIM_SE = 105;
static const char BSA_SIGNATURE[] = "BSA\000";

enum bsaArchiveFlags: uint32_t
{
	BSA_NONE = 0,
	BSA_INCLUDE_DIRECTORY_NAMES = 1 << 0,
	BSA_INCLUDE_FILE_NAMES = 1 << 1,
	BSA_DEFAULT_COMPRESSED = 1 << 2,
	BSA_RETAIN_DIRECTORY_NAMES = 1 << 3,
	BSA_RETAIN_FILE_NAMES = 1 << 4,
	BSA_RETAIN_FILE_OFFSETS = 1 << 5,
	BSA_XBOX360 = 1 << 6,
	BSA_RETAIN_STRINGS_DURING_STARTUP = 1 << 7,
	BSA_EMBED_FILE_NAMES = 1 << 8,
	BSA_CODEC_XMEM = 1 << 9,

	BSA_REQUIRED = BSA_INCLUDE_DIRECTORY_NAMES|BSA_INCLUDE_FILE_NAMES,
	BSA_CHANGEABLE = BSA_NONE,
	BSA_UNSUPPORTED_FLAGS_OBLIVION = BSA_XBOX360,
	BSA_UNSUPPORTED_FLAGS_SKYRIM = BSA_XBOX360|BSA_CODEC_XMEM,
};
enum bsaContentFlags: uint32_t
{
	BSA_CONTENT_NONE = 0,
	BSA_CONTENT_MESHES = 1 << 0,
	BSA_CONTENT_TEXTURES = 1 << 1,
	BSA_CONTENT_MENUS = 1 << 2,
	BSA_CONTENT_SOUNDS = 1 << 3,
	BSA_CONTENT_VOICES = 1 << 4,
	BSA_CONTENT_SHADERS = 1 << 5,
	BSA_CONTENT_TREES = 1 << 6,
	BSA_CONTENT_FONTS = 1 << 7,
	BSA_CONTENT_MISC = 1 << 8,
	BSA_CONTENT_CTL = 1 << 9,
};

enum bsaStatus: int
{
	BSA_STATUS_SUCCESS = 0,

	BSA_STATUS_FIRST_GENRIC = 100,
	BSA_STATUS_UNKNOWN, // Unknown error
	BSA_STATUS_UNSUPPORTED, // Unsupported format, compression, flags, whatever
	BSA_STATUS_NO_SUCH_FILE, // No file with requested ID
	BSA_STATUS_NO_SUCH_FOLDER, // Same, but for folder
	BSA_STATUS_COMPRESSION, // Compression error
	BSA_STATUS_CANCELLED, // Operation aborted by user
	BSA_STATUS_NO_FILES_TO_EXTRACT,

	/* Header errors */
	BSA_STATUS_FIRST_HEADER = 200,
	BSA_STATUS_STREAM, // Stream internal error
	BSA_STATUS_SIGNATURE, // Bad signature (not "BSA\000")
	BSA_STATUS_STRUCTURE, // Invalid structure (currently bsaHeader::Offset != sizeof(bsaHeader))
	BSA_STATUS_VERSION, // Only 104
	BSA_STATUS_FLAGS, // See bsaArchiveFlags::BSA_UNSUPPORTED_FLAGS

	/* File structure errors */
	BSA_STATUS_FIRST_STRUCTURE = 300,
	BSA_STATUS_FOLDER_RECORDS,
	BSA_STATUS_FILE_RECORDS,
	BSA_STATUS_FILE_NAMES,
	BSA_STATUS_FILE_DATA,
};

struct bsaBZString
{
	uint8_t Length = 0;
	const char* Data = nullptr;
};
struct bsaWZString
{
	uint16_t Length = 0;
	const char* Data = nullptr;
};
