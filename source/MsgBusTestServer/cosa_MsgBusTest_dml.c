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


/**************************************************************************

    module: cosa_MsgBusTest_dml.c

    For COSA Data Model Library Development

    -------------------------------------------------------------------

    description:

        This file implementes back-end apis for the 
		COSA Data Model Library - MsgBusTestServer Component	
    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    author:

        Rajmohan Thangarajan

    -------------------------------------------------------------------

    revision:

        Sep 5, 2018    initial revision.

**************************************************************************/

#include "ansc_platform.h"
#include "cosa_MsgBusTest_dml.h"
#include  "safec_lib_common.h"

char ParamString[64]= {"THROUGHPUTTEST555555"};
int ParamInt = 5;

BOOL MsgBusTest_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* strValue)
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t rc       = -1;
    int     ind      = -1;
    rc = strcmp_s("ParamString",strlen("ParamString"),ParamName,&ind);
    ERR_CHK(rc);
    if ((!ind) && (rc == EOK))
    {
	AnscCopyString(ParamString, strValue);
	   
        return TRUE;
    }
    return FALSE;
}

ULONG
MsgBusTest_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    UNREFERENCED_PARAMETER(hInsContext);
    UNREFERENCED_PARAMETER(pUlSize);
    errno_t rc       = -1;
    int     ind      = -1;
    /* check the parameter name and return the corresponding value */
    rc = strcmp_s("ParamString",strlen("ParamString"),ParamName,&ind);
    ERR_CHK(rc);
    if ((!ind) && (rc == EOK))
    {
	AnscCopyString(pValue, ParamString);
        return 0;
    }
	return 1;
}


BOOL MsgBusTest_SetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int value)
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t rc       = -1;
    int     ind      = -1;
    rc = strcmp_s("ParamInteger",strlen("ParamInteger"),ParamName,&ind);
    ERR_CHK(rc);
    if ((!ind) && (rc == EOK))
    {
		ParamInt = value;
		return TRUE;
    }
	return FALSE;
}

BOOL MsgBusTest_GetParamIntValue(ANSC_HANDLE hInsContext, char* ParamName, int* pInt)
{
    UNREFERENCED_PARAMETER(hInsContext);
    errno_t rc       = -1;
    int     ind      = -1;
    rc = strcmp_s("ParamInteger",strlen("ParamInteger"),ParamName,&ind);
    ERR_CHK(rc);
    if ((!ind) && (rc == EOK))
    {
        *pInt = ParamInt;
        return TRUE;
    }
    return FALSE;
}
