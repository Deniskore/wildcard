// CLogReader.cpp

#include "stdafx.h"

#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include "CLogReader.h"

CLogReader::CLogReader( bool isCaseSensitive )
    : mFile( NULL )
    , mFilter( NULL )
    , mCaseSensitive( isCaseSensitive ),
	mStrLenMin(0),
	mStrLenMax(0)
{
}

CLogReader::~CLogReader()
{
	if (mFilter)
		free(mFilter);
    Close();
}

bool CLogReader::Open( const char* fileName )
{
    errno_t error = fopen_s( &mFile, fileName, "rb" );
    if( error )
        return false;

	//setvbuf(mFile, NULL, _IONBF, 0);
    return true;
}

void CLogReader::Close()
{
    if( !mFile )
        return;

    fclose( mFile );
    mFile = NULL;
}



bool CLogReader::SetFilter( const char* filter )
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
|XXXXXnXXsoYeXXXnXXccccXXXXXXXXXXXXXXXXXXXXX| = file
    |XnXXsoYeXXXnXXc|                         = fstr
         soYe
         x--x
        *so?e*
       x--------x                             = buf
*/

bool CLogReader::GetNextLine( char* buf, const int bufsize )
{
    const int FSTR_SIZE = 65535;
    char fstr[FSTR_SIZE + 1];
    fstr[FSTR_SIZE] = '\0';

    int bufpos = 0;
    size_t fpos = 0;
    size_t nread = 0;

    buf[0] = '\0';
	

    do
    {
        if( fpos >= nread )
        {
            nread = fread(&fstr, sizeof(char), FSTR_SIZE, mFile);
            fpos = 0;
        }

        if( fstr[fpos] == '\r' || fstr[fpos] == '\n' )
        {

            buf[bufpos] = '\0';

            if(PreMatch(buf, bufpos) && GeneralTextCompare(buf, mCaseSensitive) )
            {
                long filepos = ftell( mFile );
                return 0 == fseek( mFile, filepos - nread + fpos + 1, SEEK_SET );
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

            buf[bufpos] = fstr[fpos];
            ++bufpos;
        }

        ++fpos;
    } while( nread > 0 );

    return false;
}

bool CLogReader::PreMatch(char* , unsigned cText)
{
	return cText >= mStrLenMin && cText <= mStrLenMax;
}


bool CLogReader::IsUndefined(char ch)
{
    return ch == '?';
}

bool CLogReader::IsEqual(char tameChar, char patChar)
{
    return tameChar == patChar || IsUndefined(patChar);
}
//Ripped from IBM wildcard algo, modified by me.
template<bool bCaseSensitive>
bool CLogReader::GeneralTextCompare(
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
                if( !(*pAfterLastTame) || *pAfterLastTame == cAltTerminator )
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

bool CLogReader::GeneralTextCompare(
    char *   pTameText,
	bool bCaseSensitive
)
{
	return bCaseSensitive ? GeneralTextCompare<true>(pTameText, mFilter) : GeneralTextCompare<false>(pTameText, mFilter);
}

