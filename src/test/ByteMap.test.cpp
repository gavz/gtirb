//===- ByteMap.test.cpp -----------------------------------------*- C++ -*-===//
//
//  Copyright (C) 2018 GrammaTech, Inc.
//
//  This code is licensed under the MIT license. See the LICENSE file in the
//  project root for license terms.
//
//  This project is sponsored by the Office of Naval Research, One Liberty
//  Center, 875 N. Randolph Street, Arlington, VA 22203 under contract #
//  N68335-17-C-0700.  The content of the information does not necessarily
//  reflect the position or policy of the Government and no official
//  endorsement should be inferred.
//
//===----------------------------------------------------------------------===//
#include <gtirb/ByteMap.hpp>
#include <gtirb/Context.hpp>
#include <proto/ByteMap.pb.h>
#include <gsl/span>
#include <gtest/gtest.h>
#include <iterator>
#include <type_traits>

using namespace gtirb;

template <typename RangeTy> static bool empty(RangeTy&& R) {
  return std::distance(std::forward<RangeTy>(R).begin(),
                       std::forward<RangeTy>(R).end()) == 0;
}

TEST(Unit_ByteMap, newRegion) {
  ByteMap B;
  std::vector<std::byte> data = {std::byte(1), std::byte(2), std::byte(3),
                                 std::byte(4)};

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(data))));

  EXPECT_EQ(B.data(Addr(1000), data.size()), data);
  EXPECT_TRUE(empty(B.data(Addr(999), data.size())));
  EXPECT_TRUE(empty(B.data(Addr(1000), data.size() + 1)));
}

TEST(Unit_ByteMap, setEmptyData) {
  ByteMap B;
  std::vector<std::byte> EmptyData;

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(EmptyData))));
  EXPECT_EQ(B.data(Addr(1000), 0), EmptyData);
  EXPECT_TRUE(empty(B.data(Addr(1000), 0)));
}

TEST(Unit_ByteMap, secondRegionAfter) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};
  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));
  EXPECT_TRUE(B.setData(Addr(2000), as_bytes(gsl::make_span(Data2))));

  EXPECT_EQ(B.data(Addr(1000), Data1.size()), Data1);
  EXPECT_EQ(B.data(Addr(2000), Data2.size()), Data2);
  EXPECT_TRUE(empty(B.data(Addr(999), Data1.size())));
  EXPECT_TRUE(empty(B.data(Addr(1000), Data1.size() + 1)));
  EXPECT_TRUE(empty(B.data(Addr(1999), Data2.size())));
  EXPECT_TRUE(empty(B.data(Addr(2000), Data2.size() + 1)));
}

TEST(Unit_ByteMap, secondRegionBefore) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};

  EXPECT_TRUE(B.setData(Addr(2000), as_bytes(gsl::make_span(Data2))));
  // Leave one byte in between
  EXPECT_TRUE(B.setData(Addr(1996), as_bytes(gsl::make_span(Data1))));

  EXPECT_EQ(B.data(Addr(1996), Data1.size()), Data1);
  EXPECT_EQ(B.data(Addr(2000), Data2.size()), Data2);
  EXPECT_TRUE(empty(B.data(Addr(999), Data1.size())));
  EXPECT_TRUE(empty(B.data(Addr(1996), Data1.size() + 1)));
  EXPECT_TRUE(empty(B.data(Addr(1999), Data2.size())));
  EXPECT_TRUE(empty(B.data(Addr(2000), Data2.size() + 1)));
}

TEST(Unit_ByteMap, thirdRegionBetween) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};
  std::vector<std::byte> Data3 = {std::byte(7), std::byte(8), std::byte(9)};

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));
  EXPECT_TRUE(B.setData(Addr(2000), as_bytes(gsl::make_span(Data2))));
  EXPECT_TRUE(B.setData(Addr(1100), as_bytes(gsl::make_span(Data3))));

  EXPECT_EQ(B.data(Addr(1000), Data1.size()), Data1);
  EXPECT_EQ(B.data(Addr(2000), Data2.size()), Data2);
  EXPECT_EQ(B.data(Addr(1100), Data3.size()), Data3);
  EXPECT_TRUE(empty(B.data(Addr(1199), Data3.size())));
  EXPECT_TRUE(empty(B.data(Addr(1100), Data3.size() + 1)));
  EXPECT_TRUE(empty(B.data(Addr(1105), 1)));
}

TEST(Unit_ByteMap, extendRegionUp) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));
  EXPECT_TRUE(
      B.setData(Addr(1000 + Data1.size()), as_bytes(gsl::make_span(Data2))));

  auto Expected = Data1;
  Expected.insert(Expected.end(), Data2.begin(), Data2.end());
  EXPECT_EQ(B.data(Addr(1000), Expected.size()), Expected);
}

TEST(Unit_ByteMap, extendRegionDown) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};

  EXPECT_TRUE(
      B.setData(Addr(1000 + Data1.size()), as_bytes(gsl::make_span(Data2))));
  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));

  auto Expected = Data1;
  Expected.insert(Expected.end(), Data2.begin(), Data2.end());
  EXPECT_EQ(B.data(Addr(1000), Expected.size()), Expected);
}

TEST(Unit_ByteMap, mergeRegions) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};
  std::vector<std::byte> Data3 = {std::byte(4), std::byte(5), std::byte(6)};

  EXPECT_TRUE(B.setData(Addr(1000 + Data1.size() + Data2.size()),
                        as_bytes(gsl::make_span(Data3))));
  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));
  EXPECT_TRUE(
      B.setData(Addr(1000 + Data1.size()), as_bytes(gsl::make_span(Data2))));

  auto Expected = Data1;
  Expected.insert(Expected.end(), Data2.begin(), Data2.end());
  Expected.insert(Expected.end(), Data3.begin(), Data3.end());
  EXPECT_EQ(B.data(Addr(1000), Expected.size()), Expected);
}

TEST(Unit_ByteMap, overwriteExistingData) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5)};

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));
  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data2))));

  std::vector<std::byte> Expected1 = {std::byte(4), std::byte(5), std::byte(3)};
  EXPECT_EQ(B.data(Addr(1000), Expected1.size()), Expected1);

  EXPECT_TRUE(B.setData(Addr(1001), as_bytes(gsl::make_span(Data2))));

  std::vector<std::byte> Expected2 = {std::byte(4), std::byte(4), std::byte(5)};
  EXPECT_EQ(B.data(Addr(1000), Expected2.size()), Expected2);
}

TEST(Unit_ByteMap, overlappingRegionsAreInvalid) {
  ByteMap B;
  std::vector<std::byte> Data1 = {std::byte(1), std::byte(2), std::byte(3)};
  std::vector<std::byte> Data2 = {std::byte(4), std::byte(5), std::byte(6)};
  std::vector<std::byte> Big(1000);

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data1))));
  EXPECT_TRUE(B.setData(Addr(2000), as_bytes(gsl::make_span(Data2))));

  EXPECT_FALSE(B.setData(Addr(999), as_bytes(gsl::make_span(Data1))));
  EXPECT_FALSE(B.setData(Addr(1001), as_bytes(gsl::make_span(Data1))));
  EXPECT_FALSE(B.setData(Addr(1999), as_bytes(gsl::make_span(Data1))));
  EXPECT_FALSE(B.setData(Addr(2001), as_bytes(gsl::make_span(Data1))));
  EXPECT_FALSE(
      B.setData(Addr(1000) + Data1.size(), as_bytes(gsl::make_span(Big))));
}

TEST(Unit_ByteMap, getDataUnmapped) {
  ByteMap B;
  std::vector<std::byte> Data = {std::byte(1), std::byte(2), std::byte(3)};

  EXPECT_TRUE(B.setData(Addr(1000), as_bytes(gsl::make_span(Data))));

  EXPECT_TRUE(empty(B.data(Addr(900), 3)));
  EXPECT_TRUE(empty(B.data(Addr(999), 3)));
  EXPECT_TRUE(empty(B.data(Addr(999), 10)));
  EXPECT_TRUE(empty(B.data(Addr(1000), Data.size() + 1)));
  EXPECT_TRUE(empty(B.data(Addr(1000 + Data.size()), 1)));
}

TEST(Unit_ByteMap, protobufRoundTrip) {
  ByteMap Original;
  auto a = std::byte('a');
  auto b = std::byte('b');
  auto c = std::byte('c');
  EXPECT_TRUE(Original.setData(Addr(1), gsl::make_span(&a, 1)));
  EXPECT_TRUE(Original.setData(Addr(2), gsl::make_span(&b, 1)));
  EXPECT_TRUE(Original.setData(Addr(5000), gsl::make_span(&c, 1)));

  gtirb::ByteMap Result;
  proto::ByteMap Message;
  Original.toProtobuf(&Message);

  gtirb::Context Ctx;
  Result.fromProtobuf(Ctx, Message);

  EXPECT_EQ(Result.data(Addr(1), 1)[0], std::byte('a'));
  EXPECT_EQ(Result.data(Addr(2), 1)[0], std::byte('b'));
  EXPECT_EQ(Result.data(Addr(5000), 1)[0], std::byte('c'));
}
