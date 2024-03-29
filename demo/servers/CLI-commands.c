/*
 *  FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
 *  All rights reserved
 *
 *  VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.
 *
 *  This file is part of the FreeRTOS distribution.
 *
 *  FreeRTOS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (version 2) as published by the
 *  Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.
 *
 ***************************************************************************
 *  >>!   NOTE: The modification to the GPL is included to allow you to     !<<
 *  >>!   distribute a combined work that includes FreeRTOS without being   !<<
 *  >>!   obliged to provide the source code for proprietary components     !<<
 *  >>!   outside of the FreeRTOS kernel.                                   !<<
 ***************************************************************************
 *
 *  FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  Full license text is available on the following
 *  link: http://www.freertos.org/a00114.html
 *
 ***************************************************************************
 *                                                                       *
 *    FreeRTOS provides completely free yet professionally developed,    *
 *    robust, strictly quality controlled, supported, and cross          *
 *    platform software that is more than just the market leader, it     *
 *    is the industry's de facto standard.                               *
 *                                                                       *
 *    Help yourself get started quickly while simultaneously helping     *
 *    to support the FreeRTOS project by purchasing a FreeRTOS           *
 *    tutorial book, reference manual, or both:                          *
 *    http://www.FreeRTOS.org/Documentation                              *
 *                                                                       *
 ***************************************************************************
 *
 *  http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
 *  the FAQ page "My application does not run, what could be wrong?".  Have you
 *  defined configASSERT()?
 *
 *  http://www.FreeRTOS.org/support - In return for receiving this top quality
 *  embedded software for free we request you assist our global community by
 *  participating in the support forum.
 *
 *  http://www.FreeRTOS.org/training - Investing in training allows your team to
 *  be as productive as possible as early as possible.  Now you can receive
 *  FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
 *  Ltd, and the world's leading authority on the world's leading RTOS.
 *
 *  http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
 *  including FreeRTOS+Trace - an indispensable productivity tool, a DOS
 *  compatible FAT file system, and our tiny thread aware UDP/IP stack.
 *
 *  http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
 *  Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.
 *
 *  http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
 *  Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
 *  licenses offer ticketed support, indemnification and commercial middleware.
 *
 *  http://www.SafeRTOS.com - High Integrity Systems also provide a safety
 *  engineered and independently SIL3 certified version for use in safety and
 *  mission critical applications that require provable dependability.
 *
 *  1 tab == 4 spaces!
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/* FreeRTOS+TCP includes, just to make the stats available to the CLI
 * commands. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

#ifdef ipconfigUSE_FAT_LIBDL
/* FreeRTOS-libdl includes to dynamically load and link objects */
    #include <dlfcn.h>
    #include <rtl/rtl-trace.h>
#endif

/* The example IP trace macros are included here so the definitions are
 * available in all the FreeRTOS+TCP source files. */
#include "DemoIPTrace.h"

#ifndef  configINCLUDE_TRACE_RELATED_CLI_COMMANDS
    #define configINCLUDE_TRACE_RELATED_CLI_COMMANDS    0
#endif

#include "portstatcounters.h"

/*
 * Implements the run-time-stats command.
 */
static BaseType_t prvTaskStatsCommand( char * pcWriteBuffer,
                                       size_t xWriteBufferLen,
                                       const char * pcCommandString );

/*
 * Implements the task-stats command.
 */
static BaseType_t prvRunTimeStatsCommand( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString );

/*
 * Implements the echo-three-parameters command.
 */
static BaseType_t prvThreeParameterEchoCommand( char * pcWriteBuffer,
                                                size_t xWriteBufferLen,
                                                const char * pcCommandString );

/*
 * Implements the echo-parameters command.
 */
static BaseType_t prvParameterEchoCommand( char * pcWriteBuffer,
                                           size_t xWriteBufferLen,
                                           const char * pcCommandString );

/*
 * Defines a command that prints out IP address information.
 */
static BaseType_t prvDisplayIPConfig( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString );

/*
 * Defines a command that prints out the gathered demo debug stats.
 */
static BaseType_t prvDisplayIPDebugStats( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString );

/*
 * Defines a command that sends an ICMP ping request to an IP address.
 */
static BaseType_t prvPingCommand( char * pcWriteBuffer,
                                  size_t xWriteBufferLen,
                                  const char * pcCommandString );

/*
 * Defines a command that calls FreeRTOS_netstat().
 */
static BaseType_t prvNetStatCommand( char * pcWriteBuffer,
                                     size_t xWriteBufferLen,
                                     const char * pcCommandString );

/*
 * Defines a command that sends a shutdown signal to the underlying platform.
 */
static BaseType_t prvShutDownCommand( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString );

/*
 * Defines a command that starts reading performance counters
 */
static BaseType_t prvStartPerformanceCounters( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString );
/*
 * Defines a command that ends and diffs performance counters
 */
static BaseType_t prvEndPerformanceCounters( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString );

/*
 * Defines a command that prints performance counters between two points
 */
static BaseType_t prvPrintPerformanceCounters( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString );

/*
 * Defines a command that dynamically loads and links object files
 */
static BaseType_t prvDlOpenCommand( char * pcWriteBuffer,
                                    size_t xWriteBufferLen,
                                    const char * pcCommandString );

/*
 * Defines a command that dynamically loads, links and calls a function within a compartment
 */
static BaseType_t prvExeCommand( char * pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char * pcCommandString );

/*
 * Defines a command to trigger faults
 */
static BaseType_t prvFaultCommand( char * pcWriteBuffer,
                                   size_t xWriteBufferLen,
                                   const char * pcCommandString ) __attribute__((optnone));

/*
 * Implements the "trace start" and "trace stop" commands;
 */
#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
    static BaseType_t prvStartStopTraceCommand( char * pcWriteBuffer,
                                                size_t xWriteBufferLen,
                                                const char * pcCommandString );
#endif

/* Structure that defines the "ip-config" command line command. */
static const CLI_Command_Definition_t xIPConfig =
{
    "ip-config",
    "ip-config:\r\n Displays IP address configuration\r\n\r\n",
    prvDisplayIPConfig,
    0
};

#if configINCLUDE_DEMO_DEBUG_STATS != 0
    /* Structure that defines the "ip-debug-stats" command line command. */
    static const CLI_Command_Definition_t xIPDebugStats =
    {
        "ip-debug-stats",       /* The command string to type. */
        "ip-debug-stats:\r\n Shows some IP stack stats useful for debug - an example only.\r\n\r\n",
        prvDisplayIPDebugStats, /* The function to run. */
        0                       /* No parameters are expected. */
    };
#endif /* configINCLUDE_DEMO_DEBUG_STATS */

/* Structure that defines the "run-time-stats" command line command.   This
 * generates a table that shows how much run time each task has */
static const CLI_Command_Definition_t xRunTimeStats =
{
    "run-time-stats",       /* The command string to type. */
    "run-time-stats:\r\n Displays a table showing how much processing time each FreeRTOS task has used\r\n\r\n",
    prvRunTimeStatsCommand, /* The function to run. */
    0                       /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command.  This generates
 * a table that gives information on each task in the system. */
static const CLI_Command_Definition_t xTaskStats =
{
    "task-stats",        /* The command string to type. */
    "task-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n\r\n",
    prvTaskStatsCommand, /* The function to run. */
    0                    /* No parameters are expected. */
};

/* Structure that defines the "echo_3_parameters" command line command.  This
 * takes exactly three parameters that the command simply echos back one at a
 * time. */
static const CLI_Command_Definition_t xThreeParameterEcho =
{
    "echo-3-parameters",
    "echo-3-parameters <param1> <param2> <param3>:\r\n Expects three parameters, echos each in turn\r\n\r\n",
    prvThreeParameterEchoCommand, /* The function to run. */
    3                             /* Three parameters are expected, which can take any value. */
};

/* Structure that defines the "echo_parameters" command line command.  This
 * takes a variable number of parameters that the command simply echos back one at
 * a time. */
static const CLI_Command_Definition_t xParameterEcho =
{
    "echo-parameters",
    "echo-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n\r\n",
    prvParameterEchoCommand, /* The function to run. */
    -1                       /* The user can enter any number of commands. */
};

#ifdef ipconfigUSE_TCP
    #if ( ipconfigUSE_TCP == 1 )

/* Structure that defines the "task-stats" command line command.  This generates
 * a table that gives information on each task in the system. */
        static const CLI_Command_Definition_t xNetStats =
        {
            "net-stats",       /* The command string to type. */
            "net-stats:\r\n Calls FreeRTOS_netstat()\r\n\r\n",
            prvNetStatCommand, /* The function to run. */
            0                  /* No parameters are expected. */
        };
    #endif /* ipconfigUSE_TCP == 1 */
#endif /* ifdef ipconfigUSE_TCP */

#if ipconfigSUPPORT_OUTGOING_PINGS == 1

/* Structure that defines the "ping" command line command.  This takes an IP
 * address or host name and (optionally) the number of bytes to ping as
 * parameters. */
    static const CLI_Command_Definition_t xPing =
    {
        "ping",
        "ping <ipaddress> <optional:bytes to send>:\r\n for example, ping 192.168.0.3 8, or ping www.example.com\r\n\r\n",
        prvPingCommand, /* The function to run. */
        -1              /* Ping can take either one or two parameter, so the number of parameters has to be determined by the ping command implementation. */
    };

#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1

/* Structure that defines the "trace" command line command.  This takes a single
 * parameter, which can be either "start" or "stop". */
    static const CLI_Command_Definition_t xStartStopTrace =
    {
        "trace",
        "trace [start | stop]:\r\n Starts or stops a trace recording for viewing in FreeRTOS+Trace\r\n\r\n",
        prvStartStopTraceCommand, /* The function to run. */
        1                         /* One parameter is expected.  Valid values are "start" and "stop". */
    };
#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

/* Structure that defines the "shutdown" command line command */
    static const CLI_Command_Definition_t xShutDown =
    {
        "shutdown",
        "shutdown:\r\n Send a shutdown signal to the underlying platform\r\n\r\n",
        prvShutDownCommand, /* The function to run. */
        0                   /* No parameters are expected. */
    };

    static cheri_riscv_hpms start_hpms;
    static cheri_riscv_hpms end_hpms;

/* Structure that defines the "start_hpms" command line command */
    static const CLI_Command_Definition_t xStartHPMs =
    {
        "start_hpms",
        "start_hpms:\r\n Read and save HPMs at a start sampling point\r\n\r\n",
        prvStartPerformanceCounters, /* The function to run. */
        0                   /* No parameters are expected. */
    };

/* Structure that defines the "end_hpms" command line command */
    static const CLI_Command_Definition_t xEndHPMs =
    {
        "end_hpms",
        "end_hpms:\r\n Read and save HPMs at an end samping point\n\r\n",
        prvEndPerformanceCounters, /* The function to run. */
        0                   /* No parameters are expected. */
    };

/* Structure that defines the "print_hpms" command line command */
    static const CLI_Command_Definition_t xPrintHPMs =
    {
        "print_hpms",
        "print_hpms:\r\n Print diff HPMs between start_hpms and end_hpms command\r\n\r\n",
        prvPrintPerformanceCounters, /* The function to run. */
        0                   /* No parameters are expected. */
    };
/*-----------------------------------------------------------*/

#ifdef ipconfigUSE_FAT_LIBDL
    /* Structure that defines the "dlopen" command line command */
    static const CLI_Command_Definition_t xDlOpen =
    {
        "dlopen",
        "dlopen <object_file>:\r\n dynamically load and link <object_file> against this image \r\n\r\n",
        prvDlOpenCommand, /* The function to run. */
        1                 /* One parameter is expected. Valid values are files */
    };
/*-----------------------------------------------------------*/

/* Structure that defines the "exe" command line command */
    static const CLI_Command_Definition_t xExe =
    {
        "exe",
        "exe <object_file> <function> <[args]>:\r\n call function from object_file compartment  \r\n\r\n",
        prvExeCommand, /* The function to run. */
        -1             /* Besides object_file and function, the other can enter any number of args */
    };
#endif /* ifdef ipconfigUSE_FAT_LIBDL */
/*-----------------------------------------------------------*/

/* Structure that defines the "fault" command line command */
    static const CLI_Command_Definition_t xFault =
    {
        "fault",
        "fault:\r\n trigger a fault\r\n\r\n",
        prvFaultCommand, /* The function to run. */
        0                /* No parameters are expected. */
    };
/*-----------------------------------------------------------*/

void vRegisterCLICommands( void )
{
    static BaseType_t xCommandRegistered = pdFALSE;

    /* Prevent commands being registered more than once. */
    if( xCommandRegistered == pdFALSE )
    {
        /* Register all the command line commands defined immediately above. */
        FreeRTOS_CLIRegisterCommand( &xTaskStats );
        FreeRTOS_CLIRegisterCommand( &xRunTimeStats );
        FreeRTOS_CLIRegisterCommand( &xThreeParameterEcho );
        FreeRTOS_CLIRegisterCommand( &xParameterEcho );
        FreeRTOS_CLIRegisterCommand( &xIPDebugStats );
        FreeRTOS_CLIRegisterCommand( &xIPConfig );

        #if ipconfigSUPPORT_OUTGOING_PINGS == 1
            {
                FreeRTOS_CLIRegisterCommand( &xPing );
            }
        #endif /* ipconfigSUPPORT_OUTGOING_PINGS */

        #ifdef ipconfigUSE_TCP
            {
                #if ipconfigUSE_TCP == 1
                    {
                        FreeRTOS_CLIRegisterCommand( &xNetStats );
                    }
                #endif /* ipconfigUSE_TCP == 1 */
            }
        #endif /* ifdef ipconfigUSE_TCP */

        #if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1
            {
                FreeRTOS_CLIRegisterCommand( &xStartStopTrace );
            }
        #endif

            FreeRTOS_CLIRegisterCommand( &xShutDown );

            FreeRTOS_CLIRegisterCommand( &xStartHPMs );
            FreeRTOS_CLIRegisterCommand( &xEndHPMs );
            FreeRTOS_CLIRegisterCommand( &xPrintHPMs );

        #ifdef ipconfigUSE_FAT_LIBDL
            FreeRTOS_CLIRegisterCommand( &xDlOpen );
            FreeRTOS_CLIRegisterCommand( &xExe );
        #endif

        FreeRTOS_CLIRegisterCommand( &xFault );

        xCommandRegistered = pdTRUE;
    }
}
/*-----------------------------------------------------------*/

static BaseType_t prvTaskStatsCommand( char * pcWriteBuffer,
                                       size_t xWriteBufferLen,
                                       const char * pcCommandString )
{
    const char * const pcHeader = "Task          State  Priority  Stack	#\r\n************************************************\r\n";

    /* Remove compile time warnings about unused parameters, and check the
     * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
     * write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Generate a table of task stats. */
    strcpy( pcWriteBuffer, pcHeader );
    vTaskList( pcWriteBuffer + strlen( pcHeader ) );

    /* There is no more data to return after this single string, so return
     * pdFALSE. */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvRunTimeStatsCommand( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString )
{
    const char * const pcHeader = "Task            Abs Time      % Time\r\n****************************************\r\n";

    /* Remove compile time warnings about unused parameters, and check the
     * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
     * write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    ( void ) xWriteBufferLen;
    configASSERT( pcWriteBuffer );

    /* Generate a table of task stats. */
    strcpy( pcWriteBuffer, pcHeader );
#if (configGENERATE_RUN_TIME_STATS == 1)
    vTaskGetRunTimeStats( pcWriteBuffer + strlen( pcHeader ) );
#endif

    /* There is no more data to return after this single string, so return
     * pdFALSE. */
    return pdFALSE;
}
/*-----------------------------------------------------------*/

static BaseType_t prvThreeParameterEchoCommand( char * pcWriteBuffer,
                                                size_t xWriteBufferLen,
                                                const char * pcCommandString )
{
    const char * pcParameter;
    BaseType_t xParameterStringLength, xReturn;
    static BaseType_t lParameterNumber = 0;

    /* Remove compile time warnings about unused parameters, and check the
     * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
     * write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    configASSERT( pcWriteBuffer );

    if( lParameterNumber == 0 )
    {
        /* The first time the function is called after the command has been
         * entered just a header string is returned. */
        snprintf( pcWriteBuffer, xWriteBufferLen, "The three parameters were:\r\n" );

        /* Next time the function is called the first parameter will be echoed
         * back. */
        lParameterNumber = 1L;

        /* There is more data to be returned as no parameters have been echoed
         * back yet. */
        xReturn = pdPASS;
    }
    else
    {
        /* Obtain the parameter string. */
        pcParameter = FreeRTOS_CLIGetParameter
                      (
            pcCommandString,                        /* The command string itself. */
            lParameterNumber,                       /* Return the next parameter. */
            &xParameterStringLength                 /* Store the parameter string length. */
                      );

        /* Sanity check something was returned. */
        configASSERT( pcParameter );

        /* Return the parameter string. */
        memset( pcWriteBuffer, 0x00, xWriteBufferLen );
        snprintf( pcWriteBuffer, xWriteBufferLen, "%d: ", ( int ) lParameterNumber );
        strncat( pcWriteBuffer, pcParameter, xParameterStringLength );
        strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

        /* If this is the last of the three parameters then there are no more
         * strings to return after this one. */
        if( lParameterNumber == 3L )
        {
            /* If this is the last of the three parameters then there are no more
             * strings to return after this one. */
            xReturn = pdFALSE;
            lParameterNumber = 0L;
        }
        else
        {
            /* There are more parameters to return after this one. */
            xReturn = pdTRUE;
            lParameterNumber++;
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

static BaseType_t prvParameterEchoCommand( char * pcWriteBuffer,
                                           size_t xWriteBufferLen,
                                           const char * pcCommandString )
{
    const char * pcParameter;
    BaseType_t xParameterStringLength, xReturn;
    static BaseType_t lParameterNumber = 0;

    /* Remove compile time warnings about unused parameters, and check the
     * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
     * write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    configASSERT( pcWriteBuffer );

    if( lParameterNumber == 0 )
    {
        /* The first time the function is called after the command has been
         * entered just a header string is returned. */
        snprintf( pcWriteBuffer, xWriteBufferLen, "The parameters were:\r\n" );

        /* Next time the function is called the first parameter will be echoed
         * back. */
        lParameterNumber = 1L;

        /* There is more data to be returned as no parameters have been echoed
         * back yet. */
        xReturn = pdPASS;
    }
    else
    {
        /* Obtain the parameter string. */
        pcParameter = FreeRTOS_CLIGetParameter
                      (
            pcCommandString,                        /* The command string itself. */
            lParameterNumber,                       /* Return the next parameter. */
            &xParameterStringLength                 /* Store the parameter string length. */
                      );

        if( pcParameter != NULL )
        {
            /* Return the parameter string. */
            memset( pcWriteBuffer, 0x00, xWriteBufferLen );
            snprintf( pcWriteBuffer, xWriteBufferLen, "%d: ", ( int ) lParameterNumber );
            strncat( pcWriteBuffer, pcParameter, xParameterStringLength );
            strncat( pcWriteBuffer, "\r\n", strlen( "\r\n" ) );

            /* There might be more parameters to return after this one. */
            xReturn = pdTRUE;
            lParameterNumber++;
        }
        else
        {
            /* No more parameters were found.  Make sure the write buffer does
             * not contain a valid string. */
            pcWriteBuffer[ 0 ] = 0x00;

            /* No more data to return. */
            xReturn = pdFALSE;

            /* Start over the next time this command is executed. */
            lParameterNumber = 0;
        }
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

#ifdef ipconfigUSE_TCP

    #if ipconfigUSE_TCP == 1

        static BaseType_t prvNetStatCommand( char * pcWriteBuffer,
                                             size_t xWriteBufferLen,
                                             const char * pcCommandString )
        {
            ( void ) pcWriteBuffer;
            ( void ) xWriteBufferLen;
            ( void ) pcCommandString;

            FreeRTOS_netstat();
            snprintf( pcWriteBuffer, xWriteBufferLen, "FreeRTOS_netstat() called - output uses FreeRTOS_printf\r\n" );
            return pdFALSE;
        }

    #endif /* ipconfigUSE_TCP == 1 */

#endif /* ifdef ipconfigUSE_TCP */
/*-----------------------------------------------------------*/

#if ipconfigSUPPORT_OUTGOING_PINGS == 1

    static BaseType_t prvPingCommand( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString )
    {
        char * pcParameter;
        BaseType_t lParameterStringLength, xReturn;
        uint32_t ulIPAddress, ulBytesToPing;
        const uint32_t ulDefaultBytesToPing = 8UL;
        char cBuffer[ 16 ];

        /* Remove compile time warnings about unused parameters, and check the
         * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
         * write buffer length is adequate, so does not check for buffer overflows. */
        ( void ) pcCommandString;
        configASSERT( pcWriteBuffer );

        /* Start with an empty string. */
        pcWriteBuffer[ 0 ] = 0x00;

        /* Obtain the number of bytes to ping. */
        pcParameter = ( char * ) FreeRTOS_CLIGetParameter
                      (
            pcCommandString,                                /* The command string itself. */
            2,                                              /* Return the second parameter. */
            &lParameterStringLength                         /* Store the parameter string length. */
                      );

        if( pcParameter == NULL )
        {
            /* The number of bytes was not specified, so default it. */
            ulBytesToPing = ulDefaultBytesToPing;
        }
        else
        {
            ulBytesToPing = atol( pcParameter );
        }

        /* Obtain the IP address string. */
        pcParameter = ( char * ) FreeRTOS_CLIGetParameter
                      (
            pcCommandString,                                /* The command string itself. */
            1,                                              /* Return the first parameter. */
            &lParameterStringLength                         /* Store the parameter string length. */
                      );

        /* Sanity check something was returned. */
        configASSERT( pcParameter );

        /* Attempt to obtain the IP address.   If the first character is not a
         * digit, assume the host name has been passed in. */
        if( ( *pcParameter >= '0' ) && ( *pcParameter <= '9' ) )
        {
            ulIPAddress = FreeRTOS_inet_addr( pcParameter );
        }
        else
        {
            /* Terminate the host name. */
            pcParameter[ lParameterStringLength ] = 0x00;

            /* Attempt to resolve host. */
            ulIPAddress = FreeRTOS_gethostbyname( pcParameter );
        }

        /* Convert IP address, which may have come from a DNS lookup, to string. */
        FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );

        if( ulIPAddress != 0 )
        {
            xReturn = FreeRTOS_SendPingRequest( ulIPAddress, ( uint16_t ) ulBytesToPing, portMAX_DELAY );
        }
        else
        {
            xReturn = pdFALSE;
        }

        if( xReturn == pdFALSE )
        {
            snprintf( pcWriteBuffer, xWriteBufferLen, "%s", "Could not send ping request\r\n" );
        }
        else
        {
            snprintf( pcWriteBuffer, xWriteBufferLen, "Ping sent to %s with identifier %d\r\n", cBuffer, xReturn );
        }

        return pdFALSE;
    }
    /*-----------------------------------------------------------*/

#endif /* ipconfigSUPPORT_OUTGOING_PINGS */

#if configINCLUDE_DEMO_DEBUG_STATS != 0

    static BaseType_t prvDisplayIPDebugStats( char * pcWriteBuffer,
                                              size_t xWriteBufferLen,
                                              const char * pcCommandString )
    {
        static BaseType_t xIndex = -1;
        extern ExampleDebugStatEntry_t xIPTraceValues[];
        BaseType_t xReturn;

        /* Remove compile time warnings about unused parameters, and check the
         * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
         * write buffer length is adequate, so does not check for buffer overflows. */
        ( void ) pcCommandString;
        configASSERT( pcWriteBuffer );

        xIndex++;

        if( xIndex < xExampleDebugStatEntries() )
        {
            snprintf( pcWriteBuffer, xWriteBufferLen, "%s %d\r\n", xIPTraceValues[ xIndex ].pucDescription, ( int ) xIPTraceValues[ xIndex ].ulData );
            xReturn = pdPASS;
        }
        else
        {
            /* Reset the index for the next time it is called. */
            xIndex = -1;

            /* Ensure nothing remains in the write buffer. */
            pcWriteBuffer[ 0 ] = 0x00;
            xReturn = pdFALSE;
        }

        return xReturn;
    }
    /*-----------------------------------------------------------*/

#endif /* configINCLUDE_DEMO_DEBUG_STATS */

static BaseType_t prvDisplayIPConfig( char * pcWriteBuffer,
                                      size_t xWriteBufferLen,
                                      const char * pcCommandString )
{
    static BaseType_t xIndex = 0;
    BaseType_t xReturn;
    uint32_t ulAddress;

    /* Remove compile time warnings about unused parameters, and check the
     * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
     * write buffer length is adequate, so does not check for buffer overflows. */
    ( void ) pcCommandString;
    configASSERT( pcWriteBuffer );

    switch( xIndex )
    {
        case 0:
            FreeRTOS_GetAddressConfiguration( &ulAddress, NULL, NULL, NULL );
            snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nIP address " );
            xReturn = pdTRUE;
            xIndex++;
            break;

        case 1:
            FreeRTOS_GetAddressConfiguration( NULL, &ulAddress, NULL, NULL );
            snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nNet mask " );
            xReturn = pdTRUE;
            xIndex++;
            break;

        case 2:
            FreeRTOS_GetAddressConfiguration( NULL, NULL, &ulAddress, NULL );
            snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nGateway address " );
            xReturn = pdTRUE;
            xIndex++;
            break;

        case 3:
            FreeRTOS_GetAddressConfiguration( NULL, NULL, NULL, &ulAddress );
            snprintf( pcWriteBuffer, xWriteBufferLen, "\r\nDNS server address " );
            xReturn = pdTRUE;
            xIndex++;
            break;

        default:
            ulAddress = 0;
            snprintf( pcWriteBuffer, xWriteBufferLen, "\r\n\r\n" );
            xReturn = pdFALSE;
            xIndex = 0;
            break;
    }

    if( ulAddress != 0 )
    {
        FreeRTOS_inet_ntoa( ulAddress, &( pcWriteBuffer[ strlen( pcWriteBuffer ) ] ) );
    }

    return xReturn;
}
/*-----------------------------------------------------------*/

#if configINCLUDE_TRACE_RELATED_CLI_COMMANDS == 1

    static BaseType_t prvStartStopTraceCommand( char * pcWriteBuffer,
                                                size_t xWriteBufferLen,
                                                const char * pcCommandString )
    {
        const char * pcParameter;
        BaseType_t lParameterStringLength;

        /* Remove compile time warnings about unused parameters, and check the
         * write buffer is not NULL.  NOTE - for simplicity, this example assumes the
         * write buffer length is adequate, so does not check for buffer overflows. */
        ( void ) pcCommandString;
        configASSERT( pcWriteBuffer );

        /* Obtain the parameter string. */
        pcParameter = FreeRTOS_CLIGetParameter
                      (
            pcCommandString,                        /* The command string itself. */
            1,                                      /* Return the first parameter. */
            &lParameterStringLength                 /* Store the parameter string length. */
                      );

        /* Sanity check something was returned. */
        configASSERT( pcParameter );

        /* There are only two valid parameter values. */
        if( strncmp( pcParameter, "start", strlen( "start" ) ) == 0 )
        {
            /* Start or restart the trace. */
            vTraceStop();
            vTraceClear();
            vTraceStart();

            snprintf( pcWriteBuffer, xWriteBufferLen, "Trace recording (re)started.\r\n" );
        }
        else if( strncmp( pcParameter, "stop", strlen( "stop" ) ) == 0 )
        {
            /* End the trace, if one is running. */
            vTraceStop();
            snprintf( pcWriteBuffer, xWriteBufferLen, "Stopping trace recording.\r\n" );
        }
        else
        {
            snprintf( pcWriteBuffer, xWriteBufferLen, "Valid parameters are 'start' and 'stop'.\r\n" );
        }

        /* There is no more data to return after this single string, so return
         * pdFALSE. */
        return pdFALSE;
    }

#endif /* configINCLUDE_TRACE_RELATED_CLI_COMMANDS */

    static BaseType_t prvShutDownCommand( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString )
    {
        _exit( 0 );
        return pdTRUE;
    }
    /*-----------------------------------------------------------*/

    static BaseType_t prvStartPerformanceCounters( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString )
    {
        PortStatCounters_ReadAll(&start_hpms);
        return pdFALSE;
    }

    static BaseType_t prvEndPerformanceCounters( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString )
    {
        PortStatCounters_ReadAll(&end_hpms);
        return pdFALSE;
    }

    static BaseType_t prvPrintPerformanceCounters( char * pcWriteBuffer,
                                          size_t xWriteBufferLen,
                                          const char * pcCommandString )
    {
        PortStatCounters_DiffAll(&start_hpms, &end_hpms, &end_hpms);
        for (int i = 0; i < COUNTERS_NUM; i++) {
            printf("HPM %s: %" PRIu64 "\n", hpm_names[i], end_hpms.counters[i]);
        }
        return pdFALSE;
    }
    /*-----------------------------------------------------------*/

#ifdef ipconfigUSE_FAT_LIBDL
    static BaseType_t prvDlOpenCommand( char * pcWriteBuffer,
                                        size_t xWriteBufferLen,
                                        const char * pcCommandString )
    {
        const char * pcParameter;
        BaseType_t lParameterStringLength;

        /* Obtain the parameter string. */
        pcParameter = FreeRTOS_CLIGetParameter
                      (
            pcCommandString,                        /* The command string itself. */
            1,                                      /* Return the first parameter. */
            &lParameterStringLength                 /* Store the parameter string length. */
                      );

        if( !dlopen( pcParameter, RTLD_NOW | RTLD_GLOBAL ) )
        {
            memset( pcWriteBuffer, 0x00, xWriteBufferLen );
            snprintf( pcWriteBuffer, xWriteBufferLen, "%s: %s ", "Failed to open", pcParameter );
        }

        /* There is no more data to return after this single string, so return
         * pdFALSE. */
        return pdFALSE;
    }
    /*-----------------------------------------------------------*/

    static BaseType_t prvExeCommand( char * pcWriteBuffer,
                                       size_t xWriteBufferLen,
                                       const char * pcCommandString )
    {
        typedef int (* call_t)( int argc, char **argv);
        const char * pcParameter1;
        const char * pcParameter2;
        int arg_index = 0;
        char *argv[10] = {NULL};
        call_t call = NULL;

        void * obj_handle;
        BaseType_t lParameterStringLength;

        /* Obtain the parameter string. */
        pcParameter1 = FreeRTOS_CLIGetParameter
                       (
            pcCommandString,                        /* The command string itself. */
            1,                                      /* Return the first parameter. */
            &lParameterStringLength                 /* Store the parameter string length. */
                       );

        memcpy( pcWriteBuffer, pcParameter1, lParameterStringLength );
        pcWriteBuffer[ lParameterStringLength ] = 0;

        char * secondparam = pcWriteBuffer + lParameterStringLength + 1;

        pcParameter2 = FreeRTOS_CLIGetParameter
                       (
            pcCommandString,                        /* The arg string itself. */
            2,                                      /* Return the second parameter. */
            &lParameterStringLength                 /* Store the parameter string length. */
                       );

        memcpy( secondparam, pcParameter2, lParameterStringLength );
        secondparam[ lParameterStringLength ] = 0;

        while(pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, arg_index + 3, &lParameterStringLength)) {
            char *arg = pvPortMalloc(lParameterStringLength + 1);
            arg[lParameterStringLength] = 0;
            memcpy(arg, pcParameter2, lParameterStringLength);
            argv[arg_index] = arg;
            arg_index++;
        }

        #if DEBUG
            rtems_rtl_trace_set_mask( RTEMS_RTL_TRACE_UNRESOLVED );
        #endif /* DEBUG */

        obj_handle = dlopen( pcWriteBuffer, RTLD_NOW | RTLD_GLOBAL );

        if( !obj_handle )
        {
            snprintf( pcWriteBuffer, xWriteBufferLen, "%s", "Failed to open file\n" );
            return pdFALSE;
        }

        call = dlsym( obj_handle, secondparam );

        if( !call )
        {
            printf( "Error: Couldn't find %s function in %s object \n", pcParameter2, pcParameter1 );
            return pdFALSE;
        }

        /* Jump to the module's function */
        call( arg_index, argv );

        dlclose( obj_handle );

        for( int i = 0; i < arg_index; i++ )
            vPortFree( argv[i] );

        /* There is no more data to return after this single string, so return
         * pdFALSE. */
        return pdFALSE;
    }
#endif /* ifdef ipconfigUSE_FAT_LIBDL */

    static BaseType_t prvFaultCommand( char * pcWriteBuffer,
                                       size_t xWriteBufferLen,
                                       const char * pcCommandString )
    {
        int* invalid_pointer = NULL;
        *invalid_pointer = 0;
        return pdTRUE;
    }
       /*-----------------------------------------------------------*/
