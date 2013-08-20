// CLogReaderPro.h

#pragma once
#include <windows.h>

class CLogReaderPro
{
public:
    CLogReaderPro( bool isCaseSensitive = true );
    ~CLogReaderPro();

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
    HANDLE mFile;
    HANDLE mMapped;
    char* mFilter;
    char* mBuffer;
    DWORD mFilePosHi;
    DWORD mFilePosLo;
    LPVOID lpMapAddress;
    bool mCaseSensitive;
	unsigned mStrLenMin, mStrLenMax;
};

