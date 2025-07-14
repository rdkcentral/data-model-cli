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
    int MsgBusTestClientApp_main(int argc, char *argv[]);
    extern int nsDiscTest;
    extern int optind;
    extern int nsDiscOnGetSet;
    extern int verbose ;
    extern int interleaved;
}

class MsgBusTestClientApp : public ::testing::Test {
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

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_help) {

    const char *status = "success";
    char *argv[] = {"MsgBusTestClientApp_main","--help"};
    int argc = 2;
    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ(0, ret);
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_timeout_case0) {
    char *argv[] = {"MsgBusTestClientApp_main", "--v","0"};
    int argc = 2;

    parameterValStruct_t ** parameterVal = NULL;

    parameterVal = ( parameterValStruct_t **)malloc(sizeof( parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = (parameterValStruct_t *)malloc(sizeof(parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    (*parameterVal)->parameterName = strdup("Device.MsgBusTest.ParamString");
    (*parameterVal)->parameterValue = strdup("TestValue");

    char mockValue[] = "false";
    char dstPath[64]="/com/cisco/spvtg/ccsp/wifi";
    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setParameterValues(_, _, _, _, _, _, _, _, _))
    .Times(1)
    .WillRepeatedly(testing::DoAll(
        testing::WithArg<8>([&](char** faultParam){
        *faultParam = mockValue;
        }),
    Return(CCSP_SUCCESS)));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    )); 

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(Return((char*)"success"));

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = MsgBusTestClientApp_main(argc, argv);
    
    EXPECT_EQ(0, ret);

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_nsDiscTest_return) {
    char *argv[] = {"MsgBusTestClientApp_main","new","--timeout", "1", "--setCount", "5", "--getCount", "3", "--nsDiscTestCount", "1"};
    int argc = 10;
    nsDiscTest = 1;
    
    componentStruct_t ** ppComponents = NULL;
     parameterValStruct_t ** parameterVal = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);

    parameterVal = (parameterValStruct_t **)malloc(sizeof(parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = (parameterValStruct_t *)malloc(sizeof(parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    (*parameterVal)->parameterName = strdup("Device.MsgBusTest.ParamString");
    (*parameterVal)->parameterValue = strdup("TestValue");

    char mockValue[] = "false";
    char dstPath[64]="/com/cisco/spvtg/ccsp/wifi";
    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setParameterValues(_, _, _, _, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArg<8>([&](char** faultParam){
        *faultParam = mockValue;
        }),
    Return(CCSP_SUCCESS)));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(Return((char*)"success"));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4,5>([&ppComponents](componentStruct_t ***outComponents,int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, free_componentStruct_t(_,_,_))
        .WillRepeatedly(Return());

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ( 0, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}


TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_ccspBusInit_minus1) {
    optind = 0;
    char *argv[] = {"MsgBusTestClientApp_main","new","--timeout", "1", "--setCount", "5", "--getCount", "3", "--nsDiscTestCount", "2"};
    int argc = 10;

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(-1));

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ( -1, ret);

}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_timeout_case_t_return_minus1) {
    optind = 0;
   char *argv[] = {"MsgBusTestClientApp_main","ai", "--timeout",NULL,"--setCount", "5", "--getCount", "3", "--nsDiscTestCount", "2"};
    int argc = 9;

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ(-1, ret);
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_timeout_case_s_return_minus1) {
    optind = 0;
   char *argv[] = {"MsgBusTestClientApp_main","ai", "--timeout","1","--setCount", NULL, "--getCount", "3", "--nsDiscTestCount", "2"};
    int argc = 9;

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ(-1, ret);
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_timeout_case_g_return_minus1) {
    optind = 0;
   char *argv[] = {"MsgBusTestClientApp_main","ai", "--timeout","1","--setCount", "5", "--getCount", NULL, "--nsDiscTestCount", "2"};
    int argc = 9;

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ(-1, ret);
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_return_timeout_case_d_return_minus1) {
    optind = 0;
   char *argv[] = {"MsgBusTestClientApp_main","ai", "--timeout","1","--setCount", "5", "--getCount", "3", "--nsDiscTestCount", NULL};
    int argc = 10;

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ(-1, ret);
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_last_0) {
    nsDiscOnGetSet = 1;
    optind = 0;
    char *argv[] = {"MsgBusTestClientApp_main","new","--timeout", "1", "--setCount", "1", "--getCount", "1", "--nsDiscTestCount", "1"};
    int argc = 10;
    nsDiscTest = 0;
    
    componentStruct_t ** ppComponents = NULL;
     parameterValStruct_t ** parameterVal = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);

    parameterVal = (parameterValStruct_t **)malloc(sizeof(parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = (parameterValStruct_t *)malloc(sizeof(parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    (*parameterVal)->parameterName = strdup("Device.MsgBusTest.ParamString");
    (*parameterVal)->parameterValue = strdup("TestValue");

    char mockValue[] = "false";
    char dstPath[64]="/com/cisco/spvtg/ccsp/wifi";
    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setParameterValues(_, _, _, _, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArg<8>([&](char** faultParam){
        *faultParam = mockValue;
        }),
    Return(CCSP_SUCCESS)));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(Return((char*)"success"));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4,5>([&ppComponents](componentStruct_t ***outComponents,int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, free_componentStruct_t(_,_,_))
        .WillRepeatedly(Return());

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ( 0, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_abort_at_interleaved) {
    char *argv[] = {"MsgBusTestClientApp_main","new","--timeout", "1", "--setCount", "5", "--getCount", "3", "--nsDiscTestCount", "1"};
    int argc = 10;
    nsDiscTest = 1;
    interleaved = 1;
    optind = 0;
    
    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ( -1, ret);
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_CcspBaseIf_setParameterValues_fail) {
    nsDiscOnGetSet = 1;
    optind = 0;
    char *argv[] = {"MsgBusTestClientApp_main","new","--timeout", "1", "--setCount", "1", "--getCount", "1", "--nsDiscTestCount", "1"};
    int argc = 10;
    nsDiscTest = 0;
    interleaved = 1;
    
    componentStruct_t ** ppComponents = NULL;
     parameterValStruct_t ** parameterVal = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);

    parameterVal = (parameterValStruct_t **)malloc(sizeof(parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = (parameterValStruct_t *)malloc(sizeof(parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    (*parameterVal)->parameterName = strdup("Device.MsgBusTest.ParamString");
    (*parameterVal)->parameterValue = strdup("TestValue");

    char mockValue[] = "false";
    char dstPath[64]="/com/cisco/spvtg/ccsp/wifi";
    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setParameterValues(_, _, _, _, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArg<8>([&](char** faultParam){
        *faultParam = mockValue;
        }),
    Return(-1)));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(-1)
    ));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(Return((char*)"success"));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4,5>([&ppComponents](componentStruct_t ***outComponents,int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, free_componentStruct_t(_,_,_))
        .WillRepeatedly(Return());

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ( 0, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(MsgBusTestClientApp, MsgBusTestClientApp_main_CcspBaseIf_discComponentSupportingNamespace_fail) {
    nsDiscOnGetSet = 1;
    optind = 0;
    char *argv[] = {"MsgBusTestClientApp_main","new","--timeout", "1", "--setCount", "1", "--getCount", "1", "--nsDiscTestCount", "1"};
    int argc = 10;
    nsDiscTest = 0;
    interleaved = 1;
    
    componentStruct_t ** ppComponents = NULL;
     parameterValStruct_t ** parameterVal = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);

    parameterVal = (parameterValStruct_t **)malloc(sizeof(parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = (parameterValStruct_t *)malloc(sizeof(parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    (*parameterVal)->parameterName = strdup("Device.MsgBusTest.ParamString");
    (*parameterVal)->parameterValue = strdup("TestValue");

    char mockValue[] = "false";
    char dstPath[64]="/com/cisco/spvtg/ccsp/wifi";
    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setParameterValues(_, _, _, _, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArg<8>([&](char** faultParam){
        *faultParam = mockValue;
        }),
    Return(-1)));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(-1)
    ));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(Return((char*)"success"));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4,5>([&ppComponents](componentStruct_t ***outComponents,int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(-1)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, free_componentStruct_t(_,_,_))
        .WillRepeatedly(Return());

    int ret = MsgBusTestClientApp_main(argc, argv);
    EXPECT_EQ( 0, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}