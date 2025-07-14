##########################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
AM_CFLAGS = -fPIC -fno-exceptions -Os -Wall -Wextra

AM_CXXFLAGS = -std=c++11

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = CcspDmcliTest_gtest.bin

CcspDmcliTest_gtest_bin_CPPFLAGS = -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/gtest \
                               -I${top_srcdir}/gtest/include \
                               -I${top_srcdir}/source \
                               -I${top_srcdir}/source/include \
                               -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/dbus-1.0 \
                               -I$(PKG_CONFIG_SYSROOT_DIR)$(libdir)/dbus-1.0/include \
                               -I$(PKG_CONFIG_SYSROOT_DIR)$(includedir)/ccsp \
                               -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/rbus \
                               -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/libsafec \
                               -I${PKG_CONFIG_SYSROOT_DIR}$(includedir) \
                               -DUNIT_TEST_DOCKER_SUPPORT
CcspDmcliTest_gtest_bin_SOURCES = gtest_main.cpp \
                                      CcspDmcliTest.cpp \
                                      MsgBusTest/MsgBusTestClientApp.cpp \
                                      MsgBusTest/MsgBusTest_dml.cpp \
                                      CcspDMCLI_Mock.cpp \
                                      $(top_builddir)/source/ccsp_message_bus_client_tool.c \
                                      $(top_builddir)/source/MsgBusTestClient/MsgBusTestClientApp.c \
                                      $(top_builddir)/source/MsgBusTestServer/cosa_MsgBusTest_dml.c

CcspDmcliTest_gtest_bin_LDADD = $(HOME)/usr/local/lib/libmock_ccsp_common.la \
                               $(HOME)/usr/local/lib/libmock_ethsw_hal.la \
                               $(HOME)/usr/local/lib/libmock_safec_lib.la \
                               $(HOME)/usr/local/lib/libmock_psm.la \
                               $(HOME)/usr/local/lib/libmock_telemetry.la \
                               $(HOME)/usr/local/lib/libmock_secure_wrapper.la \
                               $(HOME)/usr/local/lib/libmock_file_io.la


CcspDmcliTest_gtest_bin_LDFLAGS = -lgtest -lgmock -lgcov -lpthread

AM_CFLAGS += -fprofile-arcs -ftest-coverage
AM_CXXFLAGS += -fprofile-arcs -ftest-coverage
CcspDmcliTest_gtest_bin_LDFLAGS += --coverage
