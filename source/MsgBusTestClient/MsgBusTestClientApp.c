/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ccsp_message_bus.h>
#include <ccsp_base_api.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include "ccsp_memory.h"
#include <ccsp_custom.h>
#include <dslh_definitions_database.h>
#include <sys/ucontext.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <ansc_platform.h>

#define  COMPONENT_ID_TESTSERVER "eRT.com.cisco.spvtg.ccsp.testserver"
#define  COMPONENT_PATH_TESTSERVER "/com/cisco/spvtg/ccsp/testserver"

#ifdef UNIT_TEST_DOCKER_SUPPORT
#define STATIC
#else
#define STATIC static
#endif

unsigned long timeout_uS = 1000 * 1000; // 1 Sec
unsigned long setCount = 1;
unsigned long getCount = 1;
unsigned long nsDiscTestCount = 1;
int verbose = 0;
int interleaved = 0;
int nsDiscOnGetSet = 0;
int nsDiscTest = 0;

typedef struct metrics_ {
unsigned long count;
unsigned long min;
unsigned long max;
unsigned long total;
double average;
} metrics;

static long long timespec_sub(struct timespec * recent, struct timespec * old)
{
    struct timespec difference;
    difference.tv_sec = recent->tv_sec - old->tv_sec;
    difference.tv_nsec = recent->tv_nsec - old->tv_nsec;
    if(0 > difference.tv_nsec)
    {
        difference.tv_sec--;
        difference.tv_nsec += 1000000000ll;
    }

    return (difference.tv_sec * 1000000ll + difference.tv_nsec / 1000ll);
}

static int discoverComp(void *handle, char *namespace, char **name, char **path, long long *elapsedTime) {
    int ret = 0;
    int size = 0;
    componentStruct_t **ppComponents = NULL;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    ret = CcspBaseIf_discComponentSupportingNamespace(handle,
            "eRT.com.cisco.spvtg.ccsp.CR", namespace, "", &ppComponents, &size);
    clock_gettime(CLOCK_MONOTONIC, &end);
    *elapsedTime = timespec_sub(&end, &start);

    if(ret == CCSP_SUCCESS && size >= 1) {
        *name = AnscCloneString(ppComponents[0]->componentName);
        *path = AnscCloneString(ppComponents[0]->dbusPath);
        free_componentStruct_t(handle, size, ppComponents);
        return 1;
    } else {
        if(verbose || nsDiscTest)
            printf("CcspBaseIf_discComponentSupportingNamespace Failed for namespace=%s, Err : %d, size : %d\n", namespace, ret, size);
        return 0;
    }
}

#ifdef UNIT_TEST_DOCKER_SUPPORT
int MsgBusTestClientApp_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    int ret = 0;
    int size = 0;
    void *bus_handle = NULL;
    unsigned char bTmp = 1;
    char *parameterNames[5] = {NULL};
    parameterValStruct_t **parameterVal = NULL;
    char * pFaultParameter = NULL;
    char *pCfg = CCSP_MSG_BUS_CFG;
    long long tC = 0, gC = 0, sC = 0;
    long long discTime = 0, tDiff = 0;

    metrics sMetrics = {0, 0xFFFFFFFF, 0, 0, 0.0};
    metrics gMetrics = {0, 0xFFFFFFFF, 0, 0, 0.0};
    metrics nsDiscMetrics = {0, 0xFFFFFFFF, 0, 0, 0.0};

    static struct option long_options[] = {
        {"timeout",              required_argument, 0, 't'},
        {"setCount",             required_argument, 0, 's'},
        {"getCount",            required_argument, 0, 'g'},
        {"nsDiscTestCount",            required_argument, 0, 'd'},

        {"v",  no_argument,       &verbose, 1},
        {"interleaved",  no_argument,       &interleaved, 1},
        {"nsDiscOnGetSet",  no_argument,       &nsDiscOnGetSet, 1},
        {"nsDiscTest",  no_argument,       &nsDiscTest, 1},
        {0, 0, 0, 0}
    };

    if(argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("\n\
Usage : \n\
* %s [--timeout <in_micro_seconds=1Sec>] [--setCount <n=1>] [--getCount <n=1>] [--interleaved=0] [--nsDiscOnGetSet=0] [--nsDiscTest=0] [--nsDiscTestCount=1]\n\
\n", argv[0]);
        return 0;
    }

    while (1) {
        int option_index = 0;

        int c = getopt_long (argc, argv, "t:s:g:d:",
                long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 't':
                 /* Coverity Issue Fix - CID:110523 : Forward NULL*/
                 if (optarg == NULL) {
        		printf("optarg == NULL, missing optional argument in case 't' \n");
       			 return -1;
    		}
                	timeout_uS = atof(optarg) * 1000 * 1000;
                break;

            case 's':
                  /* Coverity Issue Fix - CID:110523 : Forward NULL */
            	  if (optarg == NULL) {
        	    printf("optarg == NULL, missing optional argument in case 's' \n");
       			 return -1;
    		}
                	setCount = atoi(optarg);
                break;

            case 'g':
                  /* Coverity Issue Fix - CID:110523 : Forward NULL*/
                  if (optarg == NULL) {
        		printf("optarg == NULL, missing optional argument in case 'g' \n");
        		return -1;
    		}
                	getCount = atoi(optarg);
                break;

            case 'd':
                  /* Coverity Issue Fix - CID:110523 : Forward NULL*/
            	  if (optarg == NULL) {
       			 printf("optarg == NULL, missing optional argument in case 'd' \n");
       			 return -1;
    		}
                	nsDiscTestCount = atoi(optarg);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }
    }

    printf("Configurations, timeout_uS=%lu, setCount=%lu, getCount=%lu, isInterleavedOperation=%d, nsDiscOnGetSet=%d, nsDiscTest=%d, nsDiscTestCount=%lu\n",
            timeout_uS, setCount, getCount, interleaved, nsDiscOnGetSet, nsDiscTest, nsDiscTestCount);

    if(interleaved && getCount != setCount) {
        printf("getCount must be equal to setCount for interleaved get & set test\n");
#ifndef UNIT_TEST_DOCKER_SUPPORT
        exit(-1);
#else
        return -1;
#endif
    }

    // we begin the initiation of dbus
    ret = CCSP_Message_Bus_Init("ccsp.busclient", pCfg, &bus_handle, (CCSP_MESSAGE_BUS_MALLOC)Ansc_AllocateMemory_Callback, Ansc_FreeMemory_Callback);
    if ( ret == -1 )
    {
        printf("CCSP_Message_Bus_Init Error\n");
        return -1;
    }

    // Do Discovery Test
    if(nsDiscTest) {
        char *ppDestComponentName = NULL;
        char * ppDestPath = NULL;

        for(tC = 0; (unsigned)tC < nsDiscTestCount; tC++) {
            ppDestPath = NULL;
            ppDestComponentName = NULL;

            if(discoverComp(bus_handle, "Device.MsgBusTest.ParamString", &ppDestComponentName, &ppDestPath, &discTime)) {
                nsDiscMetrics.count++;
                if((unsigned)discTime < nsDiscMetrics.min)
                    nsDiscMetrics.min = discTime;
                if((unsigned)discTime > nsDiscMetrics.max)
                    nsDiscMetrics.max = discTime;
                nsDiscMetrics.total += discTime;

                printf("Discovery of Device.MsgBusTest.ParamString (attempt-%lld) takes %lld uSec\n", tC+1, discTime);
            }

            usleep(timeout_uS);
        }

        if(nsDiscTestCount) {
             /* Coverity Issue Fix - CID:110655 : Divide By Zero*/
            if (nsDiscMetrics.count < 1) {
        	nsDiscMetrics.count = 1;
    	    }
            nsDiscMetrics.average = nsDiscMetrics.total * 1.0 / nsDiscMetrics.count;
            printf("Namespace Discovery, count=%lu, minTime=%lu, maxTime=%lu, total=%lu, average=%lf, units in uSec\n",
                    nsDiscMetrics.count, nsDiscMetrics.min, nsDiscMetrics.max, nsDiscMetrics.total, nsDiscMetrics.average);
        }
        /* Coverity Fix CID:110503 RESOURCE_LEAK */
#ifndef UNIT_TEST_DOCKER_SUPPORT
        free(ppDestComponentName);
        ppDestComponentName = NULL;
        free(ppDestPath);              //CID:155507
        ppDestPath = NULL;
#endif
        return 0;
    }

    // Do Get / Set Test
    do {
        for(sC = tC; (unsigned)sC < setCount; sC++) {
            parameterValStruct_t val[5] = {{0}};
            val[0].parameterName = AnscCloneString("Device.MsgBusTest.ParamString");
            val[0].type = ccsp_string;
            val[0].parameterValue = AnscCloneString("TestValue");

            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            // Discover the component providing the namespace
            char *ppDestComponentName = NULL;
            char * ppDestPath = NULL;
            if(nsDiscOnGetSet) {
                discoverComp(bus_handle, "Device.MsgBusTest.ParamString", &ppDestComponentName, &ppDestPath, &discTime);

                if(verbose)
                    printf("Comp = %s, Path = %s\n", ppDestComponentName ? ppDestComponentName : "NIL", ppDestPath ? ppDestPath : "NIL");
            } else {
                ppDestComponentName = COMPONENT_ID_TESTSERVER;
                ppDestPath = COMPONENT_PATH_TESTSERVER;
            }

            // Perform Set operation
            ret = CcspBaseIf_setParameterValues( bus_handle, ppDestComponentName, ppDestPath,
                    0, DSLH_MPA_ACCESS_CONTROL_CLIENTTOOL, val, 1, bTmp, &pFaultParameter);
            clock_gettime(CLOCK_MONOTONIC, &end);

            if ( ret != CCSP_SUCCESS ) {
                printf("Device.MsgBusTest.ParamString set failed in attempt %lld\n", sC+1);
            } else {
                tDiff = timespec_sub(&end, &start);

                sMetrics.count++;
                if((unsigned)tDiff < sMetrics.min)
                    sMetrics.min = tDiff;
                if((unsigned)tDiff > sMetrics.max)
                    sMetrics.max = tDiff;
                sMetrics.total += tDiff;

                printf("Device.MsgBusTest.ParamString set, attempt-%lld takes %lld uSec, nsDisc %lld uSec\n", sC+1, tDiff, discTime);
            }

            if ( pFaultParameter != NULL )
                AnscFreeMemory(pFaultParameter);

            usleep(timeout_uS);

            if(nsDiscOnGetSet)
             {
 #ifndef UNIT_TEST_DOCKER_SUPPORT
                free(ppDestPath);          //CID:155507
                ppDestPath = NULL;
#endif
             }

            if(interleaved)
                break;
        }

        parameterNames[0]="Device.MsgBusTest.ParamString";
        for(gC = tC; (unsigned)gC < getCount; gC++) {

            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            // Discover the component providing the namespace
            char *ppDestComponentName = NULL;
            char *ppDestPath = NULL;
            if(nsDiscOnGetSet) {
                discoverComp(bus_handle, "Device.MsgBusTest.ParamString", &ppDestComponentName, &ppDestPath, &discTime);

                if(verbose)
                    printf("Comp = %s, Path = %s\n", ppDestComponentName ? ppDestComponentName : "NIL", ppDestPath ? ppDestPath : "NIL");
            } else {
                ppDestComponentName = COMPONENT_ID_TESTSERVER;
                ppDestPath = COMPONENT_PATH_TESTSERVER;
            }

            // Perfor get operation
            ret = CcspBaseIf_getParameterValues( bus_handle, ppDestComponentName, ppDestPath, parameterNames, 1, &size , &parameterVal);
            clock_gettime(CLOCK_MONOTONIC, &end);

            if(ret != CCSP_SUCCESS) {
                printf("Device.MsgBusTest.ParamString get failed in attempt %lld\n", gC+1);
            } else {
                tDiff = timespec_sub(&end, &start);

                gMetrics.count++;
                if((unsigned)tDiff < gMetrics.min)
                    gMetrics.min = tDiff;
                if((unsigned)tDiff > gMetrics.max)
                    gMetrics.max = tDiff;
                gMetrics.total += tDiff;

                printf("Device.MsgBusTest.ParamString get, attempt-%lld takes %lld uSec, nsDisc %lld uSec\n", gC+1, tDiff, discTime);
            }

            free_parameterValStruct_t(bus_handle, 1, parameterVal);

            usleep(timeout_uS);


            if(nsDiscOnGetSet)
              {
#ifndef UNIT_TEST_DOCKER_SUPPORT
                free(ppDestPath);       //CID:155507  
                ppDestPath = NULL;
#endif
              }

            if(interleaved)
                break;

        }

        if(interleaved) {
            if((unsigned)tC < getCount) // getCount == seCount
                tC++;
            else
                break;
        } else {
            break;
        }
    } while(1);

    if(setCount) {
        /* Coverity Issue Fix - CID:110455 : Divide By Zero */
       if (sMetrics.count < 1) {
        	sMetrics.count = 1;
    	}
	sMetrics.average = sMetrics.total * 1.0 / sMetrics.count;
        printf("Set Parameter, count=%lu, minTime=%lu, maxTime=%lu, total=%lu, average=%lf, units in uSec\n",
                sMetrics.count, sMetrics.min, sMetrics.max, sMetrics.total, sMetrics.average);
    }

    if(getCount) {
	/* Coverity Issue Fix - CID:110571 : Divide By Zero */
	 if (gMetrics.count < 1) {
        	gMetrics.count = 1;
    	}
        gMetrics.average = gMetrics.total * 1.0 / gMetrics.count;
        printf("Get Parameter, count=%lu, minTime=%lu, maxTime=%lu, total=%lu, average=%lf, units in uSec\n",
                gMetrics.count, gMetrics.min, gMetrics.max, gMetrics.total, gMetrics.average);
    }
       /* Coverity Fix CID:110518 RESOURCE_LEAK */
       free(bus_handle);
       bus_handle  = NULL;

    return 0;
}