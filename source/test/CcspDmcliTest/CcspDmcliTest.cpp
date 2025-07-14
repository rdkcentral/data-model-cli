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

#include "CcspDMCLI_Mock.h"

#define BUSCLIENT_MAX_COUNT_SUPPORTED 20

extern "C"
{
    int dmcli_main(int argc, char *argv[]);

typedef struct 
{
    char * pathname;
    char * val1;  // type or notify
    char * val2;  // value or accesslist
}TRIPLE_VALUE, *PTRIPLE_VALUE;

typedef struct
{
    char * command;
    char * val1; // commit or nextlevel.
    TRIPLE_VALUE  result[BUSCLIENT_MAX_COUNT_SUPPORTED+1];
}CMD_CONTENT, *PCMD_CONTENT;

struct param_rtt
{
  char *param_name;
  int rtt;
};

extern char subsystem_prefix[32];

int apply_cmd(PCMD_CONTENT pInputCmd );
int analyse_cmd(char **args,PCMD_CONTENT pInputCmd );
int analyse_interactive_cmd (char *inputLine, char **args);
char *ccspReturnValueToString(unsigned long ret);
void signal_interrupt (int i);
int param_rtt_cmp (const void *c1, const void *c2);
void ccsp_exception_handler(int sig, siginfo_t *info, void *context);
}

class DmcliTest : public ::testing::Test {
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

TEST_F(DmcliTest, Main_call_arg_lessthan_2) {
    int argc = 1;
    char *argv[] = {0};
    int ret = dmcli_main(argc, argv);
    EXPECT_EQ(0, ret);
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_help) {
    char *argv[]    = {"dmcli_main", "help"};
    int ret = dmcli_main(2, argv);
    EXPECT_EQ(0, ret);
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_not_help_return_in_strcmp_s) {
    char *argv[]    = {"dmcli_main", "test"};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(1)
    ));

    int ret = dmcli_main(2, argv);
    EXPECT_EQ(0, ret);
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_not_help_return_in_strcmp_s_simu) {
    char *argv[]    = {"simu", "simu"};

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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillOnce(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(1),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strncpy_s_chk(_, _, _, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(-1))
        .WillOnce(testing::Return(0));


    char expectedIfList[] = "setcommit";

    EXPECT_CALL(*g_fileIOMock, fgets(_, _, _))
        .WillRepeatedly(testing::DoAll(
            testing::SetArrayArgument<0>(expectedIfList, expectedIfList + strlen(expectedIfList) + 1),
            Return(static_cast<char*>(expectedIfList))
        ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    int ret = dmcli_main(2, argv);
    EXPECT_EQ(-1, ret);
  
    

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_not_help_return_in_ccsp_message_bus_init) {
    char *argv[]    = {"dmcli_main", "test"};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    char expectedIfList[] = "setcommit";

    EXPECT_CALL(*g_fileIOMock, fgets(_, _, _))
        .WillRepeatedly(testing::DoAll(
            testing::SetArrayArgument<0>(expectedIfList, expectedIfList + strlen(expectedIfList) + 1),
            Return(static_cast<char*>(expectedIfList))
        ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(-1))
        .WillOnce(testing::Return(0));

    int ret = dmcli_main(2, argv);
    EXPECT_EQ(-1, ret);
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_not_help_return_in_ret_MINUS2) {
    char *argv[]    = {"dmcli_main", "test"};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    char expectedIfList[] = "exit";

    EXPECT_CALL(*g_fileIOMock, fgets(_, _, _))
        .WillRepeatedly(testing::DoAll(
            testing::SetArrayArgument<0>(expectedIfList, expectedIfList + strlen(expectedIfList) + 1),
            Return(static_cast<char*>(expectedIfList))
        ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    int ret = dmcli_main(2, argv);
    EXPECT_EQ(0, ret);
}

TEST_F(DmcliTest, Main_call_arg_equalto_4_and_argv_is_not_help_return_in_bFirstError) {
    char *argv[]    = {0};

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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    int ret = dmcli_main(4, argv);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_not_help_return_in_last_fgets) {
    char *argv[]    = {0};

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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    char expectedIfList[] = "setcommit";

     EXPECT_CALL(*g_fileIOMock, fgets(_, _, _)).Times(2)
    .WillOnce(testing::DoAll(
        testing::SetArrayArgument<0>(expectedIfList, expectedIfList + strlen(expectedIfList) + 1),
        Return(static_cast<char*>(expectedIfList))
    ))
    .WillOnce(Return(nullptr));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    int ret = dmcli_main(2, argv);
    EXPECT_EQ(-1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(DmcliTest, Main_call_arg_equalto_2_and_argv_is_not_help_return_in_END) {
    char *argv[]    = {"Main", "test", "help", "help"};

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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Init(_, _, _, _, _))
        .Times(testing::AtLeast(1))
        .WillOnce(testing::Return(0));

    EXPECT_CALL(*g_messagebusMock, CCSP_Message_Bus_Exit(_))
        .Times(testing::AtLeast(1));

    int ret = dmcli_main(4, argv);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;
}

TEST_F(DmcliTest, apply_cmd_setvalues) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("bool");
    pInputCmd->result[0].val2 = strdup("1");

    pInputCmd->command = strdup("setvalues");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};
    pInputCmd->val1 = strdup("false");

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .Times(testing::AtLeast(1))
    .WillRepeatedly(testing::Invoke([](const char* s1, size_t s1max, const char* s2, int* result, size_t s2max, size_t n) {
        if (strcmp(s1, "bool") == 0) {
            if (strcmp(s2, "bool") == 0) {
                *result = 0;
                return 0;
            } else {
                *result = 1;
                return 1;
            }
        }
        else if (strcmp(s1, "void") == 0) {
            *result = 1;
            return 1;
        }
        else if (strcmp(s1, "1") == 0) {
            if(strcmp(s2, "1") == 0) {
                *result = 0;
                return 0;
            }
            else {
                *result = 1;
                return 1;
            }
        }
        else if (strcmp(s1, "0") == 0 ) {
            if(strcmp(s2, "0") == 0) {
                *result = 0;
                return 0;
            }
            else {
                *result = 1;
                return 1;
            }
        }
        else {
            *result = 0;
            return 0;
        }
    }));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(testing::Invoke([](const char* str) {
        if (strcmp(str, "0") == 0) {
            return strdup("0");
        } else if (strcmp(str, "1") == 0) {
            return strdup("1");
        } else {
            return strdup("status");
        }
    }));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    char mockValue[] = "false";
    char dstPath[64]="/com/cisco/spvtg/ccsp/wifi";
      EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setParameterValues(_, _, _, _, _, _, _, _, _))
    .Times(2)
    .WillRepeatedly(testing::DoAll(
        testing::WithArg<8>([&](char** faultParam){
        *faultParam = mockValue;
        }),
    Return(CCSP_SUCCESS)));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = strdup("0");
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_setvalues_CCSP_MESSAGE_BUS_NOT_EXIST) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("commit");
    pInputCmd->result[1].val1 = strdup("set1");


    pInputCmd->command = strdup("set1");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(192)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_setvalues_CCSP_ERR_NOT_CONNECT) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("commit");
    pInputCmd->result[1].val1 = strdup("set1");


    pInputCmd->command = strdup("set1");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(190)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_setvalues_CCSP_ERR_TIMEOUT) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("commit");
    pInputCmd->result[1].val1 = strdup("set1");


    pInputCmd->command = strdup("set1");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(191)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_psmset) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("bool");
    pInputCmd->result[0].val2 = strdup("1");


    pInputCmd->command = strdup("psmset");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .Times(testing::AtLeast(1))
    .WillRepeatedly(testing::Invoke([](const char* s1, size_t s1max, const char* s2, int* result, size_t s2max, size_t n) {
        if (strcmp(s1, "bool") == 0) {
            if (strcmp(s2, "bool") == 0) {
                *result = 0;
                return 0;
            } else {
                *result = 1;
                return 1;
            }
        }
        else if (strcmp(s1, "void") == 0) {
            *result = 1;
            return 1;
        }
        else if (strcmp(s1, "1") == 0) {
            if(strcmp(s2, "1") == 0) {
                *result = 0;
                return 0;
            }
            else {
                *result = 1;
                return 1;
            }
        }
        else if (strcmp(s1, "0") == 0 ) {
            if(strcmp(s2, "0") == 0) {
                *result = 0;
                return 0;
            }
            else {
                *result = 1;
                return 1;
            }
        }
        else {
            *result = 0;
            return 0;
        }
    }));

    EXPECT_CALL(*g_safecLibMock, _strcpy_s_chk(_, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(_)).WillRepeatedly(testing::Invoke([](const char* str) {
        if (strcmp(str, "0") == 0) {
            return strdup("0");
        } else if (strcmp(str, "1") == 0) {
            return strdup("1");
        } else {
            return strdup("status");
        }
    }));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_psmMock, PSM_Set_Record_Value2(_, _, _, _,_))
        .WillRepeatedly(::testing::Return(100));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = strdup("0");
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_table) {
    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("commit");
    pInputCmd->result[1].val1 = strdup("addtable");


    pInputCmd->command = strdup("addtable");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcpy_s_chk(_, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(StrEq((*parameterVal)->parameterName))).WillRepeatedly(Return((char*)status));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4, 5>([&ppComponents](componentStruct_t ***outComponents, int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_baseapiMock, PSM_Del_Record(_, _, _))
    .WillRepeatedly(::testing::Return(100));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_DeleteTblRow(_, _, _, _, _))
    .WillRepeatedly(::testing::Return(100));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_AddTblRow(_, _, _, _, _, _))
    .WillRepeatedly(::testing::Return(100));


    EXPECT_CALL(*g_psmMock, PSM_Set_Record_Value2(_, _, _, _,_))
        .WillRepeatedly(::testing::Return(100));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[1].val1 = strdup("deltable");
    pInputCmd->command = strdup("deltable");

    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[1].val1 = strdup("psmdel");
    pInputCmd->command = strdup("psmdel");

    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}


TEST_F(DmcliTest, apply_cmd_psmset_with_subsystem_prefix) {

    const char *status = "success";
    componentStruct_t ** ppComponents = NULL;
    parameterValStruct_t ** parameterVal = NULL;
    
    strcpy(subsystem_prefix, "eRT.");

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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("bool");
    pInputCmd->result[0].val2 = strdup("1");

    pInputCmd->command = strdup("psmset");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcpy_s_chk(_, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(StrEq((*parameterVal)->parameterName))).WillRepeatedly(Return((char*)status));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_psmMock, PSM_Set_Record_Value2(_, _, _, _,_))
        .WillRepeatedly(::testing::Return(100));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_setcommit) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("setcommit");
    pInputCmd->result[1].val1 = strdup("setcommit");


    pInputCmd->command = strdup("setcommit");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    int ret_strcmp = 0;

    EXPECT_CALL(*g_safecLibMock, _strcpy_s_chk(_, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(StrEq((*parameterVal)->parameterName))).WillRepeatedly(Return((char*)status));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setCommit(_, _, _, _, _, _))
    .WillRepeatedly(testing::Return(100));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4, 5>([&ppComponents](componentStruct_t ***outComponents, int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_setattributes) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("setattributes");
    pInputCmd->result[1].val1 = strdup("setattributes");
    pInputCmd->result[0].val2 = strdup("acs");


    pInputCmd->command = strdup("setattributes");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_baseapiMock,CcspBaseIf_setParameterAttributes(_,_,_,_,_,_))
    .WillRepeatedly(testing::Return(100));

    EXPECT_CALL(*g_safecLibMock, _strcpy_s_chk(_, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(*g_safecLibMock, _strcat_s_chk(_, _, _, _))
    .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(StrEq((*parameterVal)->parameterName))).WillRepeatedly(Return((char*)status));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_setCommit(_, _, _, _, _, _))
    .WillRepeatedly(testing::Return(100));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4, 5>([&ppComponents](componentStruct_t ***outComponents, int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = strdup("xmpp");
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = strdup("cli");
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = strdup("webgui");
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = strdup("anybody");
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    pInputCmd->result[0].val2 = NULL;
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}


TEST_F(DmcliTest, apply_cmd_getnames) {

    const char *status = "success";
     parameterInfoStruct_t ** parameterVal = NULL;

    parameterVal = (parameterInfoStruct_t **)malloc(sizeof(parameterInfoStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = (parameterInfoStruct_t *)malloc(sizeof(parameterInfoStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->writable = 1;

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("getnames");
    pInputCmd->result[1].val1 = strdup("getnames");
    pInputCmd->val1 = strdup("false");

    pInputCmd->command = strdup("getnames");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    //AnscFreeMemoryOrig
    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterNames(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2, parameterInfoStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}


TEST_F(DmcliTest, apply_cmd_getvalues) {

    const char *status = "success";
      parameterValStruct_t ** parameterVal = NULL;

    parameterVal = ( parameterValStruct_t **)malloc(sizeof( parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = ( parameterValStruct_t *)malloc(sizeof( parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("getvalues");
    pInputCmd->result[1].val1 = strdup("getvalues");

    pInputCmd->command = strdup("getvalues");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_retv) {
    const char *status = "success";
    parameterValStruct_t ** parameterVal = NULL;
    componentStruct_t ** ppComponents = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);
    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    parameterVal = ( parameterValStruct_t **)malloc(sizeof( parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = ( parameterValStruct_t *)malloc(2*sizeof( parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("retv");
    pInputCmd->result[1].val1 = strdup("retv");

    pInputCmd->command = strdup("retv");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4,5>([&ppComponents](componentStruct_t ***outComponents,int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 0;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_sgetvalues) {

    const char *status = "success";
    parameterValStruct_t ** parameterVal = NULL;
    componentStruct_t ** ppComponents = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);
    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    parameterVal = ( parameterValStruct_t **)malloc(sizeof( parameterValStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = ( parameterValStruct_t *)malloc(sizeof( parameterValStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("sgetvalues");
    pInputCmd->result[1].val1 = strdup("sgetvalues");

    pInputCmd->command = strdup("sgetvalues");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_baseapiMock, free_parameterValStruct_t(_,_,_))
        .WillRepeatedly(Return());

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4, 5>([&ppComponents](componentStruct_t ***outComponents, int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterValues(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterValStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());
    
    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}


TEST_F(DmcliTest, apply_cmd_getattributes) {

    const char *status = "success";
    parameterAttributeStruct_t ** parameterVal = NULL;

    parameterVal = ( parameterAttributeStruct_t **)malloc(sizeof( parameterAttributeStruct_t *));
    ASSERT_NE(parameterVal, nullptr);

    *parameterVal  = ( parameterAttributeStruct_t *)malloc(sizeof( parameterAttributeStruct_t));
    ASSERT_NE(*parameterVal, nullptr);

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->accessControlBitmask = 0x0;
    (*parameterVal)->notification = 1;

    componentStruct_t ** ppComponents = NULL;

    ppComponents = (componentStruct_t **)malloc(sizeof(componentStruct_t *));
    ASSERT_NE(ppComponents, nullptr);
    *ppComponents = (componentStruct_t *)malloc(sizeof(componentStruct_t));
    ASSERT_NE(*ppComponents, nullptr);

    (*ppComponents)->componentName = strdup("Component1");
    (*ppComponents)->dbusPath = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_dbus_path = strdup("/com/example/component1");
    (*ppComponents)->remoteCR_name = strdup("com.example.component1");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("getattributes");
    pInputCmd->result[1].val1 = strdup("getattributes");

    pInputCmd->command = strdup("getattributes");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4, 5>([&ppComponents](componentStruct_t ***outComponents, int *size2) {
            *outComponents = ppComponents;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_getParameterAttributes(_, _, _, _, _, _, _))
     .WillRepeatedly(testing::DoAll(
        testing::WithArgs<5, 6>([&parameterVal](int *size2,  parameterAttributeStruct_t ***outComponents) {
            *outComponents = parameterVal;
            *size2 = 1;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return());
    
    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    (*parameterVal)->accessControlBitmask = 0x1;
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    (*parameterVal)->accessControlBitmask = 0x2;
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    (*parameterVal)->accessControlBitmask = 0x4;
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    (*parameterVal)->accessControlBitmask = 0xFFFFFFFF;
    ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, apply_cmd_psmget) {

    const char *status = "success";
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

    (*parameterVal)->parameterName = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    (*parameterVal)->parameterValue = strdup("erouter0");

    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    pInputCmd->result[0].val1 = strdup("commit");
    pInputCmd->result[1].val1 = strdup("psmget");


    pInputCmd->command = strdup("psmget");
    pInputCmd->result[0].pathname = strdup("Device.DeviceInfo.X_COMCAST-COM_WAN_IP");
    pInputCmd->result[1].pathname = {0};

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    EXPECT_CALL(*g_safecLibMock, _strcpy_s_chk(_, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    EXPECT_CALL(*g_anscWrapperApiMock, AnscCloneString(StrEq((*parameterVal)->parameterName))).WillRepeatedly(Return((char*)status));

    EXPECT_CALL(*g_baseapiMock, CcspBaseIf_discComponentSupportingNamespace(_, _, _, _, _, _))
    .WillRepeatedly(testing::DoAll(
        testing::WithArgs<4>([&ppComponents](componentStruct_t ***outComponents) {
            *outComponents = ppComponents;
        }),
        testing::Return(100)
    ));

    EXPECT_CALL(*g_psmMock, PSM_Get_Record_Value2(_, _, _, _,_))
        .WillRepeatedly(::testing::Return(100));

    EXPECT_CALL(*g_anscMemoryMock, AnscFreeMemoryOrig(_))
        .WillRepeatedly(Return()); 

    int ret = apply_cmd(pInputCmd);
    EXPECT_EQ(1, ret);

    free(*ppComponents);
    *ppComponents = NULL;
    free(ppComponents);
    ppComponents = NULL;

    free(*parameterVal);
    *parameterVal = NULL;
    free(parameterVal);
    parameterVal = NULL;

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_args_null) {
    char *args[] = {0};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setsub) {
    char *args[] = {"setsub", "subsystem"};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strncpy_s_chk(_, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArrayArgument<0>("subsystem", "subsystem" + strlen("subsystem") + 1),
            ::testing::Return(0)
        ));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(-3, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setsub_with_strcpy_fail) {
    char *args[] = {"setsub", "subsystem"};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strncpy_s_chk(_, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArrayArgument<0>("subsystem", "subsystem" + strlen("subsystem") + 1),
            ::testing::Return(-1)
        ));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(-3, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_pCmd_fail) {
    char *args[] = {"\0"};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setvalues_pPathname_argsNextNull) {
    char *args[] = {"setvalues", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setvalues_validDataType_notequal_1) {
    char *args[] = {"setvalues", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "string", "erouter0", "commit", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setvalues_validDataType_notequal_1_pType_afterArgsNull) {
    char *args[] = {"setvalues", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "string", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setvalues_validDataType_notequal_1_pType_afterArgsNotNull) {
    char *args[] = {"setvalues", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "string", "erouter0", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmset_pPathname_argsNextNull) {
    char *args[] = {"psmset", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmset_validDataType_notequal_1) {
    char *args[] = {"psmset", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "string", "erouter0", "commit", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmset_validDataType_notequal_1_pType_afterArgsNull) {
    char *args[] = {"psmset", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "string", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmset_validDataType_notequal_1_pType_afterArgsNotNull) {
    char *args[] = {"psmset", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "string", "erouter0", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _,_))
    .WillRepeatedly(testing::DoAll(
        testing::SetArgPointee<3>(0),
        Return(0)
    ));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}


TEST_F(DmcliTest, analyse_cmd_getvalues_argsNextNull) {
    char *args[] = {"getvalues", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_getvalues_argsNextNotNull) {
    char *args[] = {"getvalues", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_sgetvalues_argsNextNull) {
    char *args[] = {"sgetvalues", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_sgetvalues_argsNextNotNull) {
    char *args[] = {"sgetvalues", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_retv_argsNextNull) {
    char *args[] = {"retv", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_retv_argsNextNotNull) {
    char *args[] = {"retv", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmget_argsNextNull) {
    char *args[] = {"psmget", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmget_argsNextNotNull) {
    char *args[] = {"psmget", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_addtable_argsNextNull) {
    char *args[] = {"addtable", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_addtable_argsNextNotNull) {
    char *args[] = {"addtable", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_deltable_argsNextNull) {
    char *args[] = {"deltable", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_deltable_argsNextNotNull) {
    char *args[] = {"deltable", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmdel_argsNextNull) {
    char *args[] = {"psmdel", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_psmdel_argsNextNotNull) {
    char *args[] = {"psmdel", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_getnames_argsNextNull) {
    char *args[] = {"getnames", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_getnames_argsNextNotNull_pNextlevel_invalidValue) {
    char *args[] = {"getnames", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "t123", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_getnames_argsNextNotNull_pNextlevel_true) {
    char *args[] = {"getnames", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "true", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_getattributes_argsNextNull) {
    char *args[] = {"getattributes", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_getattributes_argsNextNotNull) {
    char *args[] = {"getattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNull) {
    char *args[] = {"setattributes", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNotNull_pPathname_afterArgsNull) {
    char *args[] = {"setattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNotNull_pPathname_afterArgsNotNull_pNotify_unchange) {
    char *args[] = {"setattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "unchange", "acs", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNotNull_pPathname_afterArgsNotNull_pNotify_Not_VALID) {
    char *args[] = {"setattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "1234", "unchange", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNotNull_pPathname_afterArgsNotNull_pAccesslist_not_valid) {
    char *args[] = {"setattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "off", "cs1", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNotNull_pPathname_afterArgsNotNull_pAccesslist_acs) {
    char *args[] = {"setattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "passive", "acs", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setattributes_argsNextNotNull_pPathname_afterArgsNotNull_pAccesslist_NULL) {
    char *args[] = {"setattributes", "Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "active", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    EXPECT_CALL(*g_safecLibMock, _strcmp_s_chk(_, _, _, _, _, _))
        .WillRepeatedly(::testing::Return(0));

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_setcommit) {
    char *args[] = {"setcommit", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_exit) {
    char *args[] = {"exit", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(-2, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_quit) {
    char *args[] = {"quit", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(-2, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_help) {
    char *args[] = {"help", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(-1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_questionMark) {
    char *args[] = {"?", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(-1, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_cmd_invalid) {
    char *args[] = {"invalid", NULL};
    PCMD_CONTENT pInputCmd = (PCMD_CONTENT)malloc(sizeof(CMD_CONTENT));
    ASSERT_NE(pInputCmd, nullptr);

    int ret = analyse_cmd(args, pInputCmd);
    EXPECT_EQ(0, ret);

    free(pInputCmd);
    pInputCmd = NULL;
}

TEST_F(DmcliTest, analyse_interactive_cmd_null) {
    char **args = NULL;
    char *inputLine = NULL;

    int ret = analyse_interactive_cmd(inputLine, args);
    EXPECT_EQ(0, ret);
}

TEST_F(DmcliTest, analyse_interactive_cmd) {
    char **args = (char **)malloc(100 * sizeof(char *));
    char inputLine[] = "setvalues Device.DeviceInfo.X_COMCAST-COM_WAN_IP string \"erouter0 \"";
    ASSERT_NE(args, nullptr);

    int ret = analyse_interactive_cmd(inputLine, args);
    EXPECT_EQ(1, ret);

    free(args);
    args = NULL;
}

TEST_F(DmcliTest, ccspReturnValueToString) {
    unsigned long ret = 0;
    char *retValue = ccspReturnValueToString(ret);
    EXPECT_STREQ("UNKNOWN MESSAGE TYPE", retValue);
}

TEST_F(DmcliTest, ccspReturnValueToString_valid) {
    unsigned long ret = 100;
    char *retValue = ccspReturnValueToString(ret);
    EXPECT_STREQ("CCSP_SUCCESS", retValue);
}

TEST_F(DmcliTest, signal_interrupt) {
    signal_interrupt(1);
}

TEST_F(DmcliTest, param_rtt_cmp) {

    struct param_rtt a = {"Device.DeviceInfo.X_COMCAST-COM_WAN_IP", 1};
    struct param_rtt b = {"Device.DeviceInfo.X_COMCAST-COM_WAN_IP", 2};

    int ret = param_rtt_cmp((const void*)&a,(const void*)&b);
    EXPECT_EQ(1, ret);
}

TEST_F(DmcliTest, ccsp_exception_handler) {
        int sig = 1;
        siginfo_t *info = (siginfo_t *)malloc(sizeof(siginfo_t));
        ASSERT_NE(info, nullptr);

        info->si_addr = (void *)0x12345678;
        info->si_code = 1;
        info->si_errno = 1;

        void *context = NULL;

        EXPECT_NO_THROW({
            ccsp_exception_handler(sig, info, context);
        });

        free(info);
        info = NULL;
}