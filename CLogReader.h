// CLogReader.h

#pragma once
#include <stdio.h>

class CLogReader
{
public:
    CLogReader( bool isCaseSensitive = true );
    ~CLogReader();

    bool Open( const char* fileName );
    void Close();

    bool SetFilter( const char* filter );
    bool GetNextLine( char* buf, const int bufsize );

private:
    static bool IsUndefined( char ch );
    static bool IsEqual( char tameChar, char patChar );

    bool GeneralTextCompare(
        char *   pTameText,
		bool bCaseSensitive = false
    );

	template<bool bCaseSensitive>
    static bool GeneralTextCompare(
        char *   __restrict pTameText,
        char *  __restrict  pWildText,
	    char cAltTerminator = '\0'
    );

	bool PreMatch(char* pTameText, unsigned cText);

private:
    static const int MAX_FILTER_SIZE = 1024;

private:
    FILE* mFile;
    char* mFilter;
    bool mCaseSensitive;
	unsigned mStrLenMin, mStrLenMax;
};
