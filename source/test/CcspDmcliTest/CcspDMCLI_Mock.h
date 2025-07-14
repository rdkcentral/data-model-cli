/*
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
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
#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <mocks/mock_securewrapper.h>
#include <mocks/mock_dslh_dmagnt_exported.h>
#include <mocks/mock_safec_lib.h>
#include <mocks/mock_base_api.h>
#include <mocks/mock_rbus.h>
#include <mocks/mock_rdklogger.h>
#include <mocks/mock_ansc_task.h>
#include <mocks/mock_psm.h>
#include <mocks/mock_messagebus.h>
#include <mocks/mock_telemetry.h>
#include <mocks/mock_ansc_memory.h>
#include<mocks/mock_ansc_wrapper_api.h>
#include<mocks/mock_file_io.h>

extern "C" {
        extern BOOL bVerbose;
        #include "../../MsgBusTestServer/cosa_MsgBusTest_dml.h"
}
using namespace std;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::HasSubstr;
using ::testing::SetArgPointee;
using ::testing::DoAll;

extern SecureWrapperMock *g_securewrapperMock;
extern SafecLibMock *g_safecLibMock;
extern BaseAPIMock *g_baseapiMock;
extern rbusMock *g_rbusMock;
extern AnscWrapperApiMock * g_anscWrapperApiMock;
extern DslhDmagntExportedMock* g_dslhDmagntExportedMock;
extern PsmMock * g_psmMock;
extern MessageBusMock * g_messagebusMock;
extern  telemetryMock *g_telemetryMock;
extern AnscMemoryMock * g_anscMemoryMock;
extern FileIOMock * g_fileIOMock;
