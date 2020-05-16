// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CXMLWriter.h"
#include <wchar.h>
#include "irrString.h"
#include "IrrCompileConfig.h"

namespace irr
{
namespace io
{


//! Constructor
CXMLWriter::CXMLWriter(IWriteFile* file)
: File(file), Tabs(0), TextWrittenLast(false)
{
	#ifdef _DEBUG
	setDebugName("CXMLWriter");
	#endif
	
	if (File)
		File->grab();
}



//! Destructor
CXMLWriter::~CXMLWriter()
{
	if (File)
		File->drop();
}



//! Writes a xml 1.0 header like <?xml version="1.0"?>
void CXMLWriter::writeXMLHeader()
{
	if (!File)
		return;

	if (sizeof(wchar_t)==2)
	{
		const u16 h = 0xFEFF;
		File->write(&h, 2);
	}
	else
	{
		const u32 h = 0x0000FEFF;
		File->write(&h, sizeof(wchar_t));
	}

	const wchar_t* const p = L"<?xml version=\"1.0\"?>";
	File->write(p, wcslen(p)*sizeof(wchar_t));

	writeLineBreak();
	TextWrittenLast = false;
}



//! Writes an xml element with maximal 5 attributes
void CXMLWriter::writeElement(const wchar_t* name, bool empty,
	const wchar_t* attr1Name, const wchar_t* attr1Value,
	const wchar_t* attr2Name, const wchar_t* attr2Value,
	const wchar_t* attr3Name, const wchar_t* attr3Value,
	const wchar_t* attr4Name, const wchar_t* attr4Value,
	const wchar_t* attr5Name, const wchar_t* attr5Value)
{
	if (!File || !name)
		return;

	if (Tabs > 0)
	{
		for (int i=0; i<Tabs; ++i)
			File->write(L"\t", sizeof(wchar_t));
	}
	
	// write name

	File->write(L"<", sizeof(wchar_t));
	File->write(name, wcslen(name)*sizeof(wchar_t));

	// write attributes

	writeAttribute(attr1Name, attr1Value);
	writeAttribute(attr2Name, attr2Value);
	writeAttribute(attr3Name, attr3Value);
	writeAttribute(attr4Name, attr4Value);
	writeAttribute(attr5Name, attr5Value);

	// write closing tag
	if (empty)
		File->write(L" />", 3*sizeof(wchar_t));
	else
	{
		File->write(L">", sizeof(wchar_t));
		++Tabs;
	}
	
	TextWrittenLast = false;
}

//! Writes an xml element with any number of attributes
void CXMLWriter::writeElement(const wchar_t* name, bool empty,
				  core::array<core::stringw> &names,
				  core::array<core::stringw> &values)
{
	if (!File || !name)
		return;

	if (Tabs > 0)
	{
		for (int i=0; i<Tabs; ++i)
			File->write(L"\t", sizeof(wchar_t));
	}
	
	// write name

	File->write(L"<", sizeof(wchar_t));
	File->write(name, wcslen(name)*sizeof(wchar_t));

	// write attributes
	u32 i=0;
	for (; i < names.size() && i < values.size(); ++i)
		writeAttribute(names[i].c_str(), values[i].c_str());

	// write closing tag
	if (empty)
		File->write(L" />", 3*sizeof(wchar_t));
	else
	{
		File->write(L">", sizeof(wchar_t));
		++Tabs;
	}
	
	TextWrittenLast = false;
}


void CXMLWriter::writeAttribute(const wchar_t* name, const wchar_t* value)
{
	if (!name || !value)
		return;

	File->write(L" ", sizeof(wchar_t));
	File->write(name, wcslen(name)*sizeof(wchar_t));
	File->write(L"=\"", 2*sizeof(wchar_t));
	writeText(value);
	File->write(L"\"", sizeof(wchar_t));
}


//! Writes a comment into the xml file
void CXMLWriter::writeComment(const wchar_t* comment)
{
	if (!File || !comment)
		return;

	File->write(L"<!--", 4*sizeof(wchar_t));
	writeText(comment);
	File->write(L"-->", 3*sizeof(wchar_t));
}


//! Writes the closing tag for an element. Like </foo>
void CXMLWriter::writeClosingTag(const wchar_t* name)
{
	if (!File || !name)
		return;

	--Tabs;

	if (Tabs > 0 && !TextWrittenLast)
	{
		for (int i=0; i<Tabs; ++i)
			File->write(L"\t", sizeof(wchar_t));
	}

	File->write(L"</", 2*sizeof(wchar_t));
	File->write(name, wcslen(name)*sizeof(wchar_t));
	File->write(L">", sizeof(wchar_t));
	TextWrittenLast = false;
}



const CXMLWriter::XMLSpecialCharacters XMLWSChar[] = 
{
	{ L'&', L"&amp;" },
	{ L'<', L"&lt;" },
	{ L'>', L"&gt;" },
	{ L'"', L"&quot;" },
	{ L'\0', 0 }
};


//! Writes a text into the file. All occurrences of special characters like
//! & (&amp;), < (&lt;), > (&gt;), and " (&quot;) are automaticly replaced.
void CXMLWriter::writeText(const wchar_t* text)
{
	if (!File || !text)
		return;

	// TODO: we have to get rid of that reserve call as well as it slows down xml-writing seriously.
	// Making a member-variable would work, but a lot of memory would stay around after writing.
	// So the correct solution is probably using fixed block here and always write when that is full.
	core::stringw s;
	s.reserve(wcslen(text)+1);	
	const wchar_t* p = text;

	while(*p)
	{
		// check if it is matching
		bool found = false;
		for (s32 i=0; XMLWSChar[i].Character != '\0'; ++i)
			if (*p == XMLWSChar[i].Character)
			{
				s.append(XMLWSChar[i].Symbol);
				found = true;
				break;
			}

		if (!found)
			s.append(*p);
		++p;
	}

	// write new string
	File->write(s.c_str(), s.size()*sizeof(wchar_t));
	TextWrittenLast = true;
}


//! Writes a line break
void CXMLWriter::writeLineBreak()
{
	if (!File)
		return;

#if defined(_IRR_OSX_PLATFORM_)
	File->write(L"\r", sizeof(wchar_t));
#elif defined(_IRR_WINDOWS_API_)
	File->write(L"\r\n", 2*sizeof(wchar_t));
#else
	File->write(L"\n", sizeof(wchar_t));
#endif

}


} // end namespace irr
} // end namespace io

