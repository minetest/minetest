// Copyright (C) 2002-2012 Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// Code contributed by skreamz

#include "IrrCompileConfig.h"

#ifdef __IRR_COMPILE_WITH_WAD_ARCHIVE_LOADER_

#include "CWADReader.h"
#include "os.h"
#include "coreutil.h"

namespace irr
{
namespace io
{

//! Constructor
CArchiveLoaderWAD::CArchiveLoaderWAD( io::IFileSystem* fs)
: FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("CArchiveLoaderWAD");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
bool CArchiveLoaderWAD::isALoadableFileFormat(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "wad" );
}


//! Creates an archive from the filename
/** \param file File handle to check.
\return Pointer to newly created archive, or 0 upon error. */
IFileArchive* CArchiveLoaderWAD::createArchive(const io::path& filename, bool ignoreCase, bool ignorePaths) const
{
	IFileArchive *archive = 0;
	io::IReadFile* file = FileSystem->createAndOpenFile(filename);

	if (file)
	{
		archive = createArchive ( file, ignoreCase, ignorePaths );
		file->drop ();
	}
	
	return archive;
}

//! creates/loads an archive from the file.
//! \return Pointer to the created archive. Returns 0 if loading failed.
IFileArchive* CArchiveLoaderWAD::createArchive(io::IReadFile* file, bool ignoreCase, bool ignorePaths) const
{
	IFileArchive *archive = 0;
	if ( file )
	{
		file->seek ( 0 );
		archive = new CWADReader(file, ignoreCase, ignorePaths);
	}
	return archive;
}


//! Check if the file might be loaded by this class
/** Check might look into the file.
\param file File handle to check.
\return True if file seems to be loadable. */
bool CArchiveLoaderWAD::isALoadableFileFormat(io::IReadFile* file) const
{
	SWADFileHeader header;
	memset(&header, 0, sizeof(header));

	file->read( &header.tag, 4 );

	return !strncmp ( header.tag, "WAD2", 4 ) || !strncmp ( header.tag, "WAD3", 4 );
}

//! Check to see if the loader can create archives of this type.
bool CArchiveLoaderWAD::isALoadableFileFormat(E_FILE_ARCHIVE_TYPE fileType) const
{
	return fileType == EFAT_WAD;
}

void createDir ( const c8 *full );


/*!
	WAD Reader
*/
CWADReader::CWADReader(IReadFile* file, bool ignoreCase, bool ignorePaths)
: CFileList((file ? file->getFileName() : io::path("")), ignoreCase, ignorePaths), File(file)
{
	#ifdef _DEBUG
	setDebugName("CWADReader");
	#endif

	if (File)
	{
		File->grab();

		Base = File->getFileName();
		Base.replace ( '\\', '/' );

		// scan local headers
		scanLocalHeader();

		sort();
	}

#if 0
	for ( u32 i = 0; i < FileList.size(); ++i )
	{
		SWADFileEntry &e = FileList[i];
		char buf[128];
		snprintf ( buf, 128, "c:\\h2\\%s", e.wadFileName.c_str() );

		createDir ( buf );
		FILE * f = fopen ( buf, "wb" );
		if ( 0 == f )
			continue;

		u8 * mem = new u8 [ e.header.disksize ];
		File->seek ( e.header.filepos );
		File->read ( mem, e.header.disksize );
		fwrite ( mem, e.header.disksize, 1, f );
		delete [] mem;
		fclose ( f );
	}
#endif

}


CWADReader::~CWADReader()
{
	if (File)
		File->drop();
}


//! return the id of the file Archive
const io::path& CWADReader::getArchiveName () const
{ 
	return Base;
}

const IFileList* CWADReader::getFileList() const
{
	return this;
}


//! scans for a local header, returns false if there is no more local file header.
bool CWADReader::scanLocalHeader()
{
	SWADFileEntryOriginal entry;
	SWADFileEntry save;

	memset(&Header, 0, sizeof(SWADFileHeader));
	File->read(&Header, sizeof(SWADFileHeader));

	if ( 0 == strncmp ( Header.tag, "WAD2", 4 ) )
		WadType = WAD_FORMAT_QUAKE2;
	else
	if ( 0 == strncmp ( Header.tag, "WAD3", 4 ) )
		WadType = WAD_FORMAT_HALFLIFE;
	else
		WadType = WAD_FORMAT_UNKNOWN;

	if ( WadType == WAD_FORMAT_UNKNOWN )
		return false;

#ifdef __BIG_ENDIAN__
	Header.numlumps = os::Byteswap::byteswap(Header.numlumps);
	Header.infotableofs = os::Byteswap::byteswap(Header.infotableofs);
#endif

	File->seek ( Header.infotableofs );

	c8 buf[16];
	for ( u32 i = 0; i < Header.numlumps; ++i )
	{
		// read entry
		File->read(&entry, sizeof ( SWADFileEntryOriginal ));
		entry.name[ sizeof ( entry.name ) - 1 ] = 0;

		save.header = entry;
		save.wadFileName = "/";
		save.wadFileName += entry.name;

		if ( WadType == WAD_FORMAT_HALFLIFE )
		{
			// don't know about the types! i'm guessing
			switch ( entry.type )
			{
				case WAD_TYP_MIPTEX_HALFLIFE:
					save.wadFileName += ".wal2";
					break;
				default:
					snprintf ( buf, 16, ".%02d", entry.type );
					save.wadFileName += buf;
					break;
			}
		}
		else
		if ( WadType == WAD_FORMAT_QUAKE2 )
		{
			switch ( entry.type )
			{
				case WAD_TYP_MIPTEX: save.wadFileName += ".miptex"; break;
				case WAD_TYP_SOUND: save.wadFileName += ".sound"; break;
				case WAD_TYP_PALETTE: save.wadFileName += ".palette"; break;
				case WAD_TYP_QTEX: save.wadFileName += ".qtex"; break;
				case WAD_TYP_QPIC: save.wadFileName += ".qpic"; break;
				case WAD_TYP_FONT: save.wadFileName += ".font"; break;
				default:
					snprintf ( buf, 16, ".%02d", entry.type );
					save.wadFileName += buf;
					break;
			}
		}

		// add file to list
		addItem(save.wadFileName,save.header.filepos, save.header.disksize, false );
		//FileInfo.push_back(save);
	}
	return true;
}


//! opens a file by file name
IReadFile* CWADReader::createAndOpenFile(const io::path& filename)
{
	s32 index = findFile(filename, false);

	if (index != -1)
		return createAndOpenFile(index);

	return 0;
}


//! opens a file by index
IReadFile* CWADReader::createAndOpenFile(u32 index)
{
	if (index >= Files.size() )
		return 0;

	const SFileListEntry &entry = Files[index];
	return createLimitReadFile( entry.FullName, File, entry.Offset, entry.Size );
}



} // end namespace io
} // end namespace irr


#endif // __IRR_COMPILE_WITH_WAD_ARCHIVE_LOADER_

