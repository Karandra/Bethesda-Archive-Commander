#include "stdafx.h"
#include "bacMainWindow.h"

union Ext
{
	public:
		struct
		{
			char Char[sizeof(uint64_t)];
		};
		uint64_t Int = 0;
		
	public:
		Ext(uint64_t v = 0)
			:Int(v)
		{
		}

	public:
		operator uint64_t() const
		{
			return Int;
		}
		bool operator==(uint64_t v) const
		{
			return Int == v;
		}
};

bacIMG::Enum bacMainWindow::GetImageByType(const wxString& sExt)
{
	auto ExtToInt = [](const wxString& sExt) -> uint64_t
	{
		Ext tExt;
		size_t nLength = sExt.Length();
		for (size_t i = 0; i < sizeof(Ext); i++)
		{
			char c = 0;
			if (i < nLength && sExt[i].GetAsChar(&c))
			{
				tExt.Char[i] = std::tolower(c);
			}
		}
		return tExt.Int;
	};

	using ImgRecord = struct
	{
		bacIMG::Enum Image;
		Ext Ext;
	};
	static ImgRecord tExts[] =
	{
		{bacIMG::diamond, ExtToInt("nif")},
		{bacIMG::stickman_run, ExtToInt("kf")},
		{bacIMG::stickman_run, ExtToInt("hkx")},

		{bacIMG::document_image, ExtToInt("dds")},
		{bacIMG::document_image, ExtToInt("tga")},
		{bacIMG::pictures, ExtToInt("png")},

		{bacIMG::script_code, ExtToInt("psc")},
		{bacIMG::script_binary, ExtToInt("pex")},

		{bacIMG::music, ExtToInt("xwm")},
		{bacIMG::music, ExtToInt("mp3")},
		{bacIMG::music, ExtToInt("ogg")},
		{bacIMG::ear_listen, ExtToInt("fuz")},
		{bacIMG::speaker_volume, ExtToInt("wav")},
		{bacIMG::chain, ExtToInt("lip")},

		{bacIMG::ui_layered_pane, ExtToInt("gfx")},
		{bacIMG::document_flash_movie, ExtToInt("swf")},

		{bacIMG::ui_menu, ExtToInt("lst")},
		{bacIMG::document_xaml, ExtToInt("xml")},
		{bacIMG::document_config, ExtToInt("ini")},
		{bacIMG::document_text, ExtToInt("txt")},
		{bacIMG::edit, ExtToInt("strings")},
		{bacIMG::edit, ExtToInt("dlstrings")},
		{bacIMG::edit, ExtToInt("ilstrings")},

		{bacIMG::film, ExtToInt("bik")},
		{bacIMG::film, ExtToInt("bk2")},

		{bacIMG::box, ExtToInt("bsa")},
		{bacIMG::box, ExtToInt("ba2")},
		{bacIMG::plug, ExtToInt("esp")},
		{bacIMG::plug_disconnect, ExtToInt("esm")},

		{bacIMG::tree, ExtToInt("spt")},
		{bacIMG::map, ExtToInt("btt")},
		{bacIMG::map, ExtToInt("bto")},
		{bacIMG::map, ExtToInt("btr")},
		{bacIMG::maps, ExtToInt("lod")},
		{bacIMG::chain, ExtToInt("seq")},
		{bacIMG::box_document, ExtToInt("bsl")},
		{bacIMG::cactus, ExtToInt("gid")},
	};

	auto nExtValue = ExtToInt(sExt);
	for (size_t i = 0; i < ARRAYSIZE(tExts); i++)
	{
		if (tExts[i].Ext == nExtValue)
		{
			return tExts[i].Image;
		}
	}
	return bacIMG::document;
}
