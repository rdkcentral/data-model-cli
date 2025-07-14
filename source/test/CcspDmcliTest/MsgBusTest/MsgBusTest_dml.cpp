/*
* If not stated otherwise in this file or this component's LICENSE file the
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

#include "../CcspDMCLI_Mock.h"

extern "C" {
}

class MsgBusTest_dml : public ::testing::Test {
    protected:
    void SetUp() override {
        g_securewrapperMock = new SecureWrapperMock();
        g_safecLibMock = new SafecLibMock();
        g_anscMemoryMock = new AnscMemoryMock();
        g_baseapiMock = new BaseAPIMock();
        g_rbusMock = new rbusMock();
        g_anscWrapperApiMock = new AnscWrapperApiMock();
        g_dslhDmagntExportedMock = new DslhDmagntExportedMock();
        g_psmMock = new PsmMock();
        g_messagebusMock = new MessageBusMock();
        g_fileIOMock   = new FileIOMock();
    }
    void TearDown() override {
        delete g_securewrapperMock;
        delete g_safecLibMock;
        delete g_anscMemoryMock;
        delete g_baseapiMock;
        delete g_rbusMock;
        delete g_anscWrapperApiMock;
        delete g_dslhDmagntExportedMock;
        delete g_psmMock;
        delete g_messagebusMock;
        delete g_fileIOMock;
        g_securewrapperMock = nullptr;
        g_safecLibMock = nullptr;
        g_anscMemoryMock = nullptr;
        g_baseapiMock = nullptr;
        g_rbusMock = nullptr;
        g_anscWrapperApiMock = nullptr;
        g_dslhDmagntExportedMock = nullptr;
        g_psmMock = nullptr;
        g_messagebusMock = nullptr;
        g_fileIOMock = nullptr;
    }
};

TEST_F(MsgBusTest_dml, MsgBusTest_SetParamStringValue_Positive)
{
    char ParamName[] = "ParamString";
    char strValue[] = "TestString";
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));
    BOOL ret = MsgBusTest_SetParamStringValue(NULL, ParamName, strValue);
    EXPECT_EQ(TRUE, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_SetParamStringValue_Negative)
{
    char ParamName[] = "Not_ParamString";
    char strValue[] = "TestString";
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(1)
    ));
    BOOL ret = MsgBusTest_SetParamStringValue(NULL, ParamName, strValue);
    EXPECT_EQ(FALSE, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_GetParamStringValue_Positive)
{
    char ParamName[] = "ParamString";
    char pValue[100] = {0};
    ULONG pUlSize = 100;
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));
    ULONG ret = MsgBusTest_GetParamStringValue(NULL, ParamName, pValue, &pUlSize);
    EXPECT_EQ(0, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_GetParamStringValue_Negative)
{
    char ParamName[] = "Not_ParamString";
    char pValue[100] = {0};
    ULONG pUlSize = 100;
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(1)
    ));
    ULONG ret = MsgBusTest_GetParamStringValue(NULL, ParamName, pValue, &pUlSize);
    EXPECT_EQ(1, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_SetParamIntValue_Positive)
{
    char ParamName[] = "ParamInteger";
    int value = 10;
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));
    BOOL ret = MsgBusTest_SetParamIntValue(NULL, ParamName, value);
    EXPECT_EQ(TRUE, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_SetParamIntValue_Negative)
{
    char ParamName[] = "Not_ParamInteger";
    int value = 10;
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(1)
    ));
    BOOL ret = MsgBusTest_SetParamIntValue(NULL, ParamName, value);
    EXPECT_EQ(FALSE, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_GetParamIntValue_Positive)
{
    char ParamName[] = "ParamInteger";
    int pInt = 0;
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));
    BOOL ret = MsgBusTest_GetParamIntValue(NULL, ParamName, &pInt);
    EXPECT_EQ(TRUE, ret);
}

TEST_F(MsgBusTest_dml, MsgBusTest_GetParamIntValue_Negative)
{
    char ParamName[] = "Not_ParamInteger";
    int pInt = 0;
    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(1)
    ));
    BOOL ret = MsgBusTest_GetParamIntValue(NULL, ParamName, &pInt);
    EXPECT_EQ(FALSE, ret);
}

