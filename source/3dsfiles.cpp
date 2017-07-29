#include <algorithm>

#include <unistd.h>
#include <dirent.h>

#include <stdio.h>

#include "port.h"
#include "3dsfiles.h"
#include "3dsmenu.h"
#include "7zip/7z.h"
#include "7zip/7zBuf.h"
#include "7zip/7zAlloc.h"
#include "7zip/7zFile.h"
#include "7zip/7zCrc.h"
#include "unzip.h"

#include "memmap.h"

static char currentDir[_MAX_PATH] = "";

//----------------------------------------------------------------------
// Initialize the library
//----------------------------------------------------------------------
void file3dsInitialize(void)
{
    mkdir("/3ds",0777);
    mkdir("/3ds/data",0777);
    mkdir("/3ds/data/snes9x_3ds",0777);
    getcwd(currentDir, _MAX_PATH);
}


//----------------------------------------------------------------------
// Gets the current directory.
//----------------------------------------------------------------------
char *file3dsGetCurrentDir(void)
{
    return currentDir;
}


//----------------------------------------------------------------------
// Go up to the parent directory.
//----------------------------------------------------------------------
void file3dsGoToParentDirectory(void)
{
    int len = strlen(currentDir);

    if (len > 1)
    {
        for (int i = len - 2; i>=0; i--)
        {
            if (currentDir[i] == '/')
            {
                currentDir[i + 1] = 0;
                break;
            }
        }
    }
}

//----------------------------------------------------------------------
// Checks if file exists.
//----------------------------------------------------------------------
bool IsFileExists(const char * filename) {
    if (FILE * file = fopen(filename, "r")) {
        fclose(file);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------
// Go up to the child directory.
//----------------------------------------------------------------------
void file3dsGoToChildDirectory(char *childDir)
{
    strncat(currentDir, childDir, _MAX_PATH);
    strncat(currentDir, "/", _MAX_PATH);
}


//----------------------------------------------------------------------
// Gets the extension of a given file.
//----------------------------------------------------------------------
char *file3dsGetExtension(char *filePath)
{
    int len = strlen(filePath);
    char *extension = &filePath[len];

    for (int i = len - 1; i >= 0; i--)
    {
        if (filePath[i] == '.')
        {
            extension = &filePath[i + 1];
            break;
        }
    }
    return extension;
}


//----------------------------------------------------------------------
// Case-insensitive check for substring.
//----------------------------------------------------------------------
char* stristr( char* str1, const char* str2 )
{
    char* p1 = str1 ;
    const char* p2 = str2 ;
    char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( *p1 ) == tolower( *p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( tolower( *p1 ) == tolower( *p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? r : 0 ;
}

//----------------------------------------------------------------------
// Load all ROM file names (up to 512 ROMs)
//
// Specify a comma separated list of extensions.
//
//----------------------------------------------------------------------
std::vector<std::string> file3dsGetFiles(char *extensions, int maxFiles)
{
    std::vector<std::string> files;
    char buffer[_MAX_PATH];

    struct dirent* dir;
    DIR* d = opendir(currentDir);

    if (strlen(currentDir) > 1)
    {
        // Insert the parent directory.
        snprintf(buffer, _MAX_PATH, "\x01 ..");   
        files.push_back(buffer);
    }

    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            char *dot = strrchr(dir->d_name, '.');

            if (dir->d_name[0] == '.')
                continue;
            if (dir->d_type == DT_DIR)
            {
                snprintf(buffer, _MAX_PATH, "\x01 %s", dir->d_name);
                files.push_back(buffer);
            }
            if (dir->d_type == DT_REG)
            {
                char *ext = file3dsGetExtension(dir->d_name);

                if (!stristr(extensions, ext))
                    continue;

                files.push_back(dir->d_name);
            }
        }
        closedir(d);
    }

    std::sort(files.begin(), files.end());

    return files;
}



static const ISzAlloc g_Alloc = { SzAlloc, SzFree };

static int Buf_EnsureSize(CBuf *dest, size_t size)
{
  if (dest->size >= size)
    return 1;
  Buf_Free(dest, &g_Alloc);
  return Buf_Create(dest, size, &g_Alloc);
}


#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))

#define _UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))

#define _UTF8_HEAD(n, val) ((Byte)(_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((Byte)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

static size_t Utf16_To_Utf8_Calc(const UInt16 *src, const UInt16 *srcLim)
{
  size_t size = 0;
  for (;;)
  {
    UInt32 val;
    if (src == srcLim)
      return size;
    
    size++;
    val = *src++;
   
    if (val < 0x80)
      continue;

    if (val < _UTF8_RANGE(1))
    {
      size++;
      continue;
    }

    if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
    {
      UInt32 c2 = *src;
      if (c2 >= 0xDC00 && c2 < 0xE000)
      {
        src++;
        size += 3;
        continue;
      }
    }

    size += 2;
  }
}

static Byte *Utf16_To_Utf8(Byte *dest, const UInt16 *src, const UInt16 *srcLim)
{
  for (;;)
  {
    UInt32 val;
    if (src == srcLim)
      return dest;
    
    val = *src++;
    
    if (val < 0x80)
    {
      *dest++ = (char)val;
      continue;
    }

    if (val < _UTF8_RANGE(1))
    {
      dest[0] = _UTF8_HEAD(1, val);
      dest[1] = _UTF8_CHAR(0, val);
      dest += 2;
      continue;
    }

    if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
    {
      UInt32 c2 = *src;
      if (c2 >= 0xDC00 && c2 < 0xE000)
      {
        src++;
        val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
        dest[0] = _UTF8_HEAD(3, val);
        dest[1] = _UTF8_CHAR(2, val);
        dest[2] = _UTF8_CHAR(1, val);
        dest[3] = _UTF8_CHAR(0, val);
        dest += 4;
        continue;
      }
    }
    
    dest[0] = _UTF8_HEAD(2, val);
    dest[1] = _UTF8_CHAR(1, val);
    dest[2] = _UTF8_CHAR(0, val);
    dest += 3;
  }
}

static SRes Utf16_To_Utf8Buf(CBuf *dest, const UInt16 *src, size_t srcLen)
{
  size_t destLen = Utf16_To_Utf8_Calc(src, src + srcLen);
  destLen += 1;
  if (!Buf_EnsureSize(dest, destLen))
    return SZ_ERROR_MEM;
  *Utf16_To_Utf8(dest->data, src, src + srcLen) = 0;
  return SZ_OK;
}

static SRes Utf16_To_Char(CBuf *buf, const UInt16 *s)
{
  unsigned len = 0;
  for (len = 0; s[len] != 0; len++);
  return Utf16_To_Utf8Buf(buf, s, len);

}
int sZPos=0;
int sZPackedsize=0;
static SRes LookToRead3ds_Look_Exact(const ILookInStream *pp, const void **buf, size_t *size)
{
  sZPos+=*size;
  SRes res = SZ_OK;
  CLookToRead2 *p = MY_container_of(pp, CLookToRead2, vt);
  
  size_t size2 = p->size - p->pos;
  if (size2 == 0 && *size != 0)
  {
    menu3dsUpdateDialogProgress(sZPackedsize-sZPos, sZPackedsize);
    p->pos = 0;
    p->size = 0;
    if (*size > p->bufSize)
      *size = p->bufSize;
    res = ISeekInStream_Read(p->realStream, p->buf, size);
    size2 = p->size = *size;
  }
  if (*size > size2)
    *size = size2;
  *buf = p->buf + p->pos;
  return res;
}

int FileLoader3ds (uint8* buffer, const char* filename, int32 maxsize)
{
	int32 TotalFileSize = 0;
	int len = 0;
	int nFormat=FILE_DEFAULT;
 
	char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char name [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    char fname [_MAX_PATH + 1];

	unsigned long FileSize = 0;
	int current_pos;
	
    
    
	_splitpath (filename, drive, dir, name, ext);
    _makepath (fname, drive, dir, name, ext);
	
	if (strcasecmp (ext, "zip") == 0)
		nFormat = FILE_ZIP;
    if (strcasecmp (ext, "7z") == 0)
		nFormat = FILE_7ZIP;
    if (strcasecmp (ext, "gz") == 0)
        nFormat = FILE_GZIP;

    if(nFormat==FILE_ZIP)
	{
        unzFile file=NULL;
        unzFile zip_file = 0;    
        unz_file_info unzinfo;
        char snes_file[256];
        char *p;
        uint8 *ptr=buffer;

        file = unzOpen(fname);

	    zip_file = unzOpen(filename);
        if (!zip_file) return FALSE;
        unzGoToFirstFile (zip_file);
 
        for (;;)
        {
        	if (unzGetCurrentFileInfo(zip_file, &unzinfo, snes_file, sizeof(snes_file), NULL, 0, NULL, 0) != UNZ_OK)
                return FALSE;

            p = (char*)(strrchr(snes_file, '.') + 1);

            if (strcasecmp(p, "smc") == 0)
                break;
            if (strcasecmp(p, "sfc") == 0)
                break;
            if (strcasecmp(p, "swc") == 0)
                break;
            if (strcasecmp(p, "fig") == 0)
                break;
            if (unzGoToNextFile(zip_file) != UNZ_OK)
                return FALSE;
         }                
         unzOpenCurrentFile (zip_file);
         
		FileSize = unzinfo.uncompressed_size;	 	 	 

		unzReadCurrentFile(zip_file,(void*)(ptr), FileSize, &menu3dsUpdateDialogProgress);

        unzCloseCurrentFile (zip_file);
        unzClose (zip_file);

        int calc_size = (FileSize / 0x2000) * 0x2000;

	    if ((FileSize - calc_size == 512 && !Settings.ForceNoHeader) ||
			Settings.ForceHeader)
	    {
			memmove (ptr, ptr + 512, calc_size);
			Memory.HeaderCount++;
			FileSize -= 512;
	    }
	    TotalFileSize += FileSize;
		strcpy (Memory.ROMFilename, fname);

	}
	else if(nFormat==FILE_7ZIP)
	{
		uint8 *ptr=buffer;

        ISzAlloc allocImp;
        ISzAlloc allocTempImp;
        CFileInStream archiveStream;
        CLookToRead2 lookStream;
        CSzArEx db;
        SRes res;
        UInt16 *temp = NULL;
        size_t tempSize = 0;
        allocImp = g_Alloc;
        allocTempImp = g_Alloc;
        InFile_Open(&archiveStream.file, filename);
        if(archiveStream.file.file==0)
            return FALSE;
        FileInStream_CreateVTable(&archiveStream);
        LookToRead2_CreateVTable(&lookStream, False);
        // to show progress
        lookStream.vt.Look = LookToRead3ds_Look_Exact;

        lookStream.buf = NULL;

        res = SZ_OK;
        {
            #define kInputBufSize ((size_t)1 << 18)
            lookStream.buf = (Byte *)ISzAlloc_Alloc(&allocImp, kInputBufSize);
            if (!lookStream.buf)
                res = SZ_ERROR_MEM;
            else
            {
                lookStream.bufSize = kInputBufSize;
                lookStream.realStream = &archiveStream.vt;
                LookToRead2_Init(&lookStream);
            }
        }
        CrcGenerateTable();
            
        SzArEx_Init(&db);
            
        if (res == SZ_OK)
            res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
        else
            return FALSE;
        
        if (res == SZ_OK)
        {
            int i;
            UInt32 blockIndex = 0xFFFFFFFF;
            Byte *outBuffer = 0;
            size_t outBufferSize = 0;
            size_t offset = 0;
            size_t outSizeProcessed = 0;

            for (i = 0; i < db.NumFiles; i++)
            {
                size_t len;
                if (SzArEx_IsDir(&db, i))
                    continue;
                len = SzArEx_GetFileNameUtf16(&db, i, NULL);
                if (len > tempSize)
                {
                    SzFree(NULL, temp);
                    tempSize = len;
                    temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
                    if (!temp)
                    {
                        res = SZ_ERROR_MEM;
                        return FALSE;;
                    }
                }
                SzArEx_GetFileNameUtf16(&db, i, temp);
                CBuf buf;
                SRes res2;
                Buf_Init(&buf);
                res2 = Utf16_To_Char(&buf, temp);

                if (res2 != SZ_OK)
                    return FALSE;

                //printf((const char *)buf.data);
                char *p = (char*)(strrchr((const char *)buf.data, '.') + 1);

                if (strcasecmp(p, "smc") == 0)
                    break;
                if (strcasecmp(p, "sfc") == 0)
                    break;
                if (strcasecmp(p, "swc") == 0)
                    break;
                if (strcasecmp(p, "fig") == 0)
                    break;

                Buf_Free(&buf, &g_Alloc);
  
            }
            if (i==db.NumFiles)
                return FALSE;
            FileSize = SzArEx_GetFileSize(&db, i);
            sZPackedsize=db.db.PackPositions[i+1]-db.db.PackPositions[i];
            sZPos=0;

            res = SzArEx_Extract(&db, &lookStream.vt, i,
                &blockIndex, &outBuffer, &outBufferSize,
                &offset, &outSizeProcessed,
                &allocImp, &allocTempImp);
            if (res != SZ_OK)
                return FALSE;

            memcpy(ptr,outBuffer,outBufferSize);

            int calc_size = (FileSize / 0x2000) * 0x2000;

            if ((FileSize - calc_size == 512 && !Settings.ForceNoHeader) ||
                Settings.ForceHeader)
            {
                memmove (ptr, ptr + 512, calc_size);

                Memory.HeaderCount++;
                FileSize -= 512;
            }
            TotalFileSize += FileSize;
            strcpy (Memory.ROMFilename, fname);
            ISzAlloc_Free(&allocImp, outBuffer);
            SzFree(NULL, temp);
            SzArEx_Free(&db, &allocImp);
            ISzAlloc_Free(&allocImp, lookStream.buf);
    
        }
        else
             return FALSE;
		
	}
    else if(nFormat==FILE_GZIP)
    {
        FILE *f = fopen(fname, "rb");
        if(f==NULL)
            return NULL;
        fseek(f, -4, SEEK_END);
        FileSize = fgetc(f) | (fgetc(f) << 8) | (fgetc(f) << 16) | (fgetc(f) << 24);
        fclose(f);
        current_pos=FileSize;
    
        gzFile ROMFile;
		if ((ROMFile = gzopen (fname, "rb")) == NULL)
			return NULL;
		strcpy (Memory.ROMFilename, fname);
		
		Memory.HeaderCount = 0;
		uint8 *ptr = buffer;
		bool8 more = FALSE;
    

    
        while (current_pos>0) {
            menu3dsUpdateDialogProgress(current_pos,FileSize);
            if (current_pos>0xF000) {
                gzread(ROMFile,ptr,0xF000);
                ptr+=0xF000;
                current_pos-=0xF000;
            } else {
                gzread(ROMFile,ptr,current_pos);
                ptr+=current_pos;
                current_pos=0;
            }
        }
        menu3dsUpdateDialogProgress(current_pos,FileSize);
        ptr=ROM;
        gzclose(ROMFile);
        
        int calc_size = (FileSize / 0x2000) * 0x2000;
    
        if ((FileSize - calc_size == 512 && !Settings.ForceNoHeader) ||
            Settings.ForceHeader)
        {
            memmove (ptr, ptr + 512, calc_size);
            Memory.HeaderCount++;
            FileSize -= 512;
        }
        
        ptr += FileSize;
        TotalFileSize += FileSize;
    
	}
	else if(nFormat==FILE_DEFAULT)
    {
		// any other roms go here
        STREAM ROMFile;
		if ((ROMFile = fopen (fname, "rb")) == NULL)
			return (0);
		
		strcpy (Memory.ROMFilename, fname);
		
		Memory.HeaderCount = 0;
		uint8 *ptr = buffer;
		bool8 more = FALSE;
		do
		{
			//FileSize = READ_STREAM (ptr, maxsize + 0x200 - (ptr - ROM), ROMFile);
			//CLOSE_STREAM (ROMFile);
			fseek(ROMFile,0,SEEK_END);
	    	FileSize=ftell(ROMFile);
	   		 fseek(ROMFile,0,SEEK_SET);
			current_pos=FileSize;

			while (current_pos>0) {
				menu3dsUpdateDialogProgress(current_pos,FileSize);
	    		if (current_pos>0xF000) {
	    			fread(ptr,0xF000,1,ROMFile);
	    			ptr+=0xF000;
	    			current_pos-=0xF000;
	    		} else {
	    			fread(ptr,current_pos,1,ROMFile);
	    			ptr+=current_pos;
	    			current_pos=0;
	    		}
	    	}
	    	menu3dsUpdateDialogProgress(current_pos,FileSize);
	    	ptr=ROM;
	    	fclose(ROMFile);
			
			int calc_size = (FileSize / 0x2000) * 0x2000;
		
			if ((FileSize - calc_size == 512 && !Settings.ForceNoHeader) ||
				Settings.ForceHeader)
			{
				memmove (ptr, ptr + 512, calc_size);
				Memory.HeaderCount++;
				FileSize -= 512;
			}
			
			ptr += FileSize;
			TotalFileSize += FileSize;
		

			// check for multi file roms

			if (ptr - ROM < maxsize + 0x200 &&
				(isdigit (ext [0]) && ext [1] == 0 && ext [0] < '9'))
			{
				more = TRUE;
				ext [0]++;
				_makepath (fname, drive, dir, name, ext);
			}
			else if (ptr - ROM < maxsize + 0x200 &&
					(((len = strlen (name)) == 7 || len == 8) &&
					strncasecmp (name, "sf", 2) == 0 &&
					isdigit (name [2]) && isdigit (name [3]) && isdigit (name [4]) &&
					isdigit (name [5]) && isalpha (name [len - 1])))
			{
				more = TRUE;
				name [len - 1]++;
				_makepath (fname, drive, dir, name, ext);
			}
			else
				more = FALSE;

		} while (more && (ROMFile = OPEN_STREAM (fname, "rb")) != NULL);
    
	}

	return TotalFileSize;

}
