// CLogReaderPro.cpp

#include "stdafx.h"
#include "CLogReaderPro.h"

const SIZE_T MAX_MAP_SIZE = 70 * 1024 * 1024;

CLogReaderPro::CLogReaderPro( bool isCaseSensitive )
    : mMapped( INVALID_HANDLE_VALUE )
    , mFile( INVALID_HANDLE_VALUE )
    , mFilter( NULL )
    , mBuffer( NULL )
    , mFilePosHi( 0L )
    , mFilePosLo( 0L )
    , lpMapAddress( NULL )
    , mCaseSensitive( isCaseSensitive )
{
}

CLogReaderPro::~CLogReaderPro()
{
    Close();
}

bool CLogReaderPro::Open( const char* fileName )
{
    mFile = CreateFileA(
        fileName,
        FILE_READ_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
    );

    if( INVALID_HANDLE_VALUE == mFile )
        return false;

    mMapped = CreateFileMapping( mFile, NULL, PAGE_READONLY, 0, 0, NULL );
    if( INVALID_HANDLE_VALUE == mMapped )
    {
        Close();
        return false;
    }

    return true;
}

void CLogReaderPro::Close()
{
    if( !mBuffer )
        UnmapViewOfFile( mBuffer );

    CloseHandle( mMapped );
    CloseHandle( mFile );

    mFile = INVALID_HANDLE_VALUE;
    mMapped = INVALID_HANDLE_VALUE;
    mBuffer = NULL;
}

bool CLogReaderPro::SetFilter( const char* filter )
{
    size_t len = strnlen_s( filter, MAX_FILTER_SIZE );
    mFilter = (char*) malloc( len + 1 );
    if (0 != strcpy_s( mFilter, len + 1, filter ))
	{
		return false;
	}


	// preprocesss pattern
	mStrLenMin = 0;

	bool vw = false;
	for (char *ps(mFilter), *pd(mFilter);;)
	{
		bool w(false);
		while (*ps == '*')
		{
			w = true;
			*pd = *ps;
			++ps;
		}
		if (!w)
		{
			*pd = *ps;
			if (*ps == '\0')
				break;
			++mStrLenMin;
			++ps;
		}
		else
		{
			vw = true;
		}
		++pd;
	}
	mStrLenMax = vw ? -1 : mStrLenMin;

	// skip empty lines
	if (mStrLenMin == 0)
		mStrLenMin = 1;

	return true;

}
/*
0       1       2       3       4       5       6       7
XXXXXXXXXXXXXXXXXXXXXXnoooooonXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
                XXXXXnoooooonXXXXXXXXXXX
                       x----x
                            XXXXXnoooooonXXXXXXXXXXX
*/
bool CLogReaderPro::GetNextLine( char* buf, const int bufsize )
{
    SYSTEM_INFO SysInfo;
    GetSystemInfo( &SysInfo );
    DWORD dwSysGran = SysInfo.dwAllocationGranularity;
    DWORD dwMapPos = 0;
    DWORD dwMapSize = 0;
    DWORD dwBufPos = 1;
    int bufpos = 0;
    int iMapDelta = 0;
    DWORD dwFileSizeLo = GetFileSize( mFile, NULL );
    DWORD dwBufSize;
    DWORD dwAvailable = 0;
    buf[0] = '\0';

    do
    {
        if( dwBufPos >= dwAvailable )
        {
            dwBufSize = MAX_MAP_SIZE;
            if( dwFileSizeLo - mFilePosLo < MAX_MAP_SIZE )
                dwBufSize = dwFileSizeLo - mFilePosLo;

            dwMapPos = (mFilePosLo / dwSysGran) * dwSysGran;
            dwMapSize = (mFilePosLo % dwSysGran) + dwBufSize;
            if( lpMapAddress )
                UnmapViewOfFile( lpMapAddress );

            lpMapAddress = (char*) MapViewOfFile( mMapped, FILE_MAP_READ, 0, dwMapPos, dwMapSize );
            if( !lpMapAddress )
                return false;

            iMapDelta = mFilePosLo - dwMapPos;
            mBuffer = (char*) lpMapAddress + iMapDelta;
            dwBufPos = 0;
            dwAvailable = dwBufSize;
            mFilePosLo += dwBufSize;
        }

        if( mBuffer[dwBufPos] == '\r' || mBuffer[dwBufPos] == '\n' )
        {
            if(PreMatch(buf, bufpos) && GeneralTextCompare( buf, mCaseSensitive ) )
            {
                mFilePosLo = mFilePosLo - dwBufSize + dwBufPos + 1;
                return true;
            }

            bufpos = 0;
            buf[bufpos] = '\0';
        }
        else
        {
            if( bufpos >= bufsize - 1 )
            {
                buf[bufpos] = '\0';
                return false;
            }

            buf[bufpos] = mBuffer[dwBufPos];
            ++bufpos;
            buf[bufpos] = '\0';
        }

        ++dwBufPos;
    } while( dwAvailable > 0 );

    return false;
}

bool CLogReaderPro::PreMatch(char* , unsigned cText)
{
	return cText >= mStrLenMin && cText <= mStrLenMax;
}


bool CLogReaderPro::IsUndefined(char ch)
{
    return ch == '?';
}

bool CLogReaderPro::IsEqual(char tameChar, char patChar)
{
    return tameChar == patChar || IsUndefined(patChar);
}
//Ripped from IBM wildcard algo, modified by me.
template<bool bCaseSensitive>
bool CLogReaderPro::GeneralTextCompare(
    char  * __restrict pTameText,
    char  * __restrict pWildText,
    char cAltTerminator
    )
{
    bool bMatch = true;
    char* pAfterLastWild = NULL;
    char* pAfterLastTame = NULL;
    char t, w;

    while( 1 )
    {
        t = *pTameText;
        w = *pWildText;

        if( !t || t == cAltTerminator )
        {
            if( !w || w == cAltTerminator )
            {
                break;
            }
            else if( w == '*' )
            {
                pWildText++;
                continue;
            }

            else if( pAfterLastTame )
            {
                if( !( *pAfterLastTame ) || *pAfterLastTame == cAltTerminator )
                {
                    bMatch = false;
                    break;
                }

                pTameText = pAfterLastTame++;
                pWildText = pAfterLastWild;
                continue;
            }

            bMatch = false;
            break;
        }
        else
        {
            if( !bCaseSensitive )
            {
                if (t >= 'A' && t <= 'Z')
                {
                    t += ('a' - 'A');
                }

                if (w >= 'A' && w <= 'Z')
                {
                    w += ('a' - 'A');
                }
            }

            if( !IsEqual(t, w) )
            {
                if( w == '*' )
                {
                    pAfterLastWild = ++pWildText;
                    pAfterLastTame = pTameText;
                    w = *pWildText;

                    if( !w || w == cAltTerminator )
                    {
                        break;
                    }
                    continue;
                }
                else if ( pAfterLastWild )
                {
                    if( pAfterLastWild != pWildText )
                    {
                        pWildText = pAfterLastWild;
                        w = *pWildText;

                        if( !bCaseSensitive && w >= 'A' && w <= 'Z' )
                        {
                            w += ('a' - 'A');
                        }

                        if( IsEqual(t, w) )
                        {
                            pWildText++;
                        }
                    }
                    pTameText++;
                    continue;
                }
                else
                {
                    bMatch = false;
                    break;
                }
            }
        }
        pTameText++;
        pWildText++;
    }
    return bMatch;
}

bool CLogReaderPro::GeneralTextCompare(
    char *   pTameText,
	bool bCaseSensitive
)
{
	return bCaseSensitive ? GeneralTextCompare<true>(pTameText, mFilter) : GeneralTextCompare<false>(pTameText, mFilter);
}
