// SPDX-FileCopyrightText: 2024 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "include_gtest.h"
#include "inc_libCZI.h"
#include "../libCZI/utilities.h"

using namespace libCZI;
using namespace std;

struct AzureUriAndExpectedResultFixture : public testing::TestWithParam<tuple<wstring, map<wstring, wstring>>> { };

TEST_P(AzureUriAndExpectedResultFixture, TokenizeAzureUriScheme_ValidCases)
{
    const auto parameters = GetParam();
    const auto tokens = Utilities::TokenizeAzureUriString(get<0>(parameters));
    EXPECT_EQ(tokens, get<1>(parameters));
}

INSTANTIATE_TEST_SUITE_P(
    AzureBlobStream,
    AzureUriAndExpectedResultFixture,
    testing::Values(
    // third tile on top, second tile in the middle, first tile at the bottom
    make_tuple(L"a=b;c=d;d=e", map<wstring, wstring> { { L"a", L"b" }, { L"c", L"d" }, { L"d", L"e" } }),
    make_tuple(L"a=b x;c= d ;d=e  ;o=   w", map<wstring, wstring> { { L"a", L"b x" }, { L"c", L" d " }, { L"d", L"e  " }, { L"o", L"   w" } }),
    make_tuple(L" a =b;c =d; d =e", map<wstring, wstring> { { L" a ", L"b" }, { L"c ", L"d" }, { L" d ", L"e" } }),
    make_tuple(LR"(a=\;\;;c=\\d;d=e)", map<wstring, wstring> { { L"a", L";;" }, { L"c", LR"(\\d)" }, { L"d", L"e" } }),
    make_tuple(LR"(c=\\d)", map<wstring, wstring> { { L"c", LR"(\\d)" } }),
    make_tuple(LR"(\;a=abc)", map<wstring, wstring> { { LR"(;a)", L"abc" } }),
    make_tuple(LR"(\;a\==abc)", map<wstring, wstring> { { LR"(;a=)", L"abc" } }),
    make_tuple(LR"(c=\\d\=\;)", map<wstring, wstring> { { L"c", LR"(\\d=;)" } })
));

struct IllFormedAzureUriAndExpectedErrorFixture : public testing::TestWithParam<wstring> { };

TEST_P(IllFormedAzureUriAndExpectedErrorFixture, TokenizeAzureUriScheme_InvalidCases)
{
    const auto parameter = GetParam();
    EXPECT_THROW(Utilities::TokenizeAzureUriString(parameter), std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    AzureBlobStream,
    IllFormedAzureUriAndExpectedErrorFixture,
    testing::Values(
    L"xxx",
    L"=xxx",
    LR"(a\=xxx)",
    L";",
    L"=",
    L"=;",
    LR"(\=\;)",
    L"a=b;c=d;k="
));
