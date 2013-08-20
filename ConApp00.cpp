// ConApp00.cpp

#include "stdafx.h"
#include <windows.h>
#include <iostream>

#include "CLogReader.h"
#include "CLogReaderPro.h"

int main( int argc, const char* argv[] )
{
    if( argc != 3 )
    {
        printf( "Usage: application.exe [Text_File] [Wildcard_String]\n" );
        return 0;
    }

    //CLogReader logReader( true );  //CRT version
    CLogReaderPro logReader( true ); //Map version
    if( !logReader.Open( argv[1] ))
    {
        printf( "Error: can't open file: '%s'\n", argv[1] );
        return 0;
    }

    if( !logReader.SetFilter( argv[2] ) )
    {
        printf( "Error: can't set filter\n" );
        logReader.Close();
        return 0;
    }

    if( !SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) )
    {
        DWORD dwError = GetLastError();
        printf( "SetPriority error %d\n", dwError );
    }

    const int BUF_SIZE = 1024;
    char buffer[BUF_SIZE];

    LARGE_INTEGER start, finish, freq;
    QueryPerformanceFrequency( &freq );
    QueryPerformanceCounter( &start );

    while( logReader.GetNextLine( buffer, BUF_SIZE ) )
    {
        printf_s( ">> '%s'\n", buffer );
    }

    QueryPerformanceCounter( &finish );
    printf( "Execution time: %f\n", (( finish.QuadPart - start.QuadPart ) / (double) freq.QuadPart ) );
    system( "pause" );

    logReader.Close();
    return 0;
}
