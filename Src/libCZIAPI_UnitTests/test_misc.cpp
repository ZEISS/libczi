// SPDX-FileCopyrightText: 2025 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <libCZIApi.h>

TEST(CZIAPI_Misc, GetVersionInfoAndCheckResult)
{
    LibCZIVersionInfoInterop version_info;
    const LibCZIApiErrorCode result = libCZI_GetLibCZIVersionInfo(&version_info);
    EXPECT_EQ(result, LibCZIApi_ErrorCode_OK);
    EXPECT_GE(version_info.major, 0);
    EXPECT_GE(version_info.minor, 0);
    EXPECT_GE(version_info.patch, 0);
    EXPECT_GE(version_info.tweak, 0);
}

TEST(CZIAPI_Misc, GetBuildInformationAndCheckResult)
{
    LibCZIBuildInformationInterop build_info;
    const LibCZIApiErrorCode result = libCZI_GetLibCZIBuildInformation(&build_info);
    EXPECT_EQ(result, LibCZIApi_ErrorCode_OK);

    // Well, the strings could be all null (in case the build information is not available), so there is
    //  not much we can check here.
    EXPECT_TRUE(build_info.compilerIdentification == nullptr || strlen(build_info.compilerIdentification) < 1024);
    EXPECT_TRUE(build_info.repositoryUrl == nullptr || strlen(build_info.repositoryUrl) < 1024);
    EXPECT_TRUE(build_info.repositoryBranch == nullptr || strlen(build_info.repositoryBranch) < 1024);
    EXPECT_TRUE(build_info.repositoryTag == nullptr || strlen(build_info.repositoryTag) < 1024);

    libCZI_Free(build_info.compilerIdentification);
    libCZI_Free(build_info.repositoryUrl);
    libCZI_Free(build_info.repositoryBranch);
    libCZI_Free(build_info.repositoryTag);
}
