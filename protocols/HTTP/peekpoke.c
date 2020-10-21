/*
 * peekpoke.c -- the meat of our "simple" peek and poke web server
 */

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* FreeRTOS Protocol includes. */
#include "FreeRTOS_HTTP_commands.h"
#include "FreeRTOS_TCP_server.h"
#include "FreeRTOS_server_private.h"

/* Specifics for the peekpoke server. */
#include "peekpoke.h"

/* First, some helper functions. */

static char *addressToCharPtr( long long int address )
{
	/* convert first to unsigned 32-bit int */
	unsigned int tmp = address & 0xffffffff;
	return (char *) tmp; /* evil! */
}

/* Stateful functions to split a slash-separated string of numbers. */

static const char *buf = NULL;

static void loadString(const char* s)
{
	buf = s;
}

static long long int getNextNumber(void)
{
	/* 
     * Infinite list of todo items that will never happen here:
	 * - Unicode support
	 * - Error handling for non-integer inputs
	 * - Integers that are too big, so have overflows/wraparounds
	 */

	if( buf == NULL )
	{
		return 0;
	}

	char *endPtr = NULL;
	long long int result = strtoll(buf, &endPtr, 0); // overwrites endPtr

	if( endPtr == NULL )
	{
		buf = NULL;
	}
	else if( *endPtr == '/' )
	{
		/*
		 * This is the desired state: the first "invalid" character
		 * is the slash character, so we'll start again next time one
		 * character beyond it. 
		 */
		buf = endPtr + 1;
	}
	else if( endPtr == buf ) {
		/* No valid number found at all! Note absence of error handling. */
		buf = NULL;
		result = 0;
	}
	else
	{
		/* Probably at the 0-terminated end of the string, i.e, *endPtr = 0 */
		buf = endPtr;
	}

	return result;
}

#define STACK_BUFFER_SIZE 1024

size_t peekPokeHandler( HTTPClient_t *pxClient, BaseType_t xIndex, const char *pcURLData, char *pcOutputBuffer, size_t uxBufferLength )
{
	char stackBuffer[STACK_BUFFER_SIZE] = "xyzzy";
	
	switch ( xIndex )
	{
	case ECMD_GET:
		// could be "/hello" or "/peek/address/length"
		if( 0 == strncmp( "/hello", pcURLData, 6) )
		{
			strcpy( pxClient->pxParent->pcContentsType, "text/plain" );

			/* useful for a hacker to have a stack addr */
			snprintf( pcOutputBuffer, uxBufferLength,
					  "It's dark here; you may be eaten by a grue.\n\n&stackBuffer = %p\nSTACK_BUFFER_SIZE = %d\nuxBufferLength = %d\nstackBuffer = %s\n",
					  &stackBuffer, STACK_BUFFER_SIZE, uxBufferLength, stackBuffer );

			return strlen( pcOutputBuffer );
		}
		else if( 0 == strncmp( "/peek/", pcURLData, 6) )
		{
			strcpy( pxClient->pxParent->pcContentsType, "application/octet-stream" );

			loadString(pcURLData + 6);
			long long int memAddress = getNextNumber();
			size_t readLength = getNextNumber();
			const char *mem = addressToCharPtr( memAddress );

			if( memAddress != 0 && readLength != 0 )
			{
				if( readLength > uxBufferLength )
				{
					readLength = uxBufferLength; /* best we can do */
				}

				bzero( pcOutputBuffer, uxBufferLength ); 
				memcpy( pcOutputBuffer, mem, readLength ); /* evil! */

				return readLength;
			}
		}
		break;
	case ECMD_PATCH:
		// could be "/poke/address/length" with body having attack bytes
		if( 0 == strncmp( "/poke/", pcURLData, 6) ) {
			strcpy( pxClient->pxParent->pcContentsType, "text/plain" );

			loadString(pcURLData + 6);
			
			int memAddress = getNextNumber();
			int writeLength = getNextNumber();
			char *mem = addressToCharPtr( memAddress );

			if( memAddress != 0 && writeLength != 0 )
			{
				memcpy( mem, pxClient->pcRestData, writeLength ); /* evil! */
			}

			snprintf( pcOutputBuffer, uxBufferLength, "Wrote %d bytes to %p for 'ya!\n", writeLength, mem );
			return strlen( pcOutputBuffer );
		}
		break;
	default:
		break;
	}

	/* if there's an error */
	return 0;
}
