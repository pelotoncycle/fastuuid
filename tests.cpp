#include <algorithm>
#include <iostream>
#include <iostream>
#include <istream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "gtest/gtest.h"

#include "fast_parse.h"


namespace {

  
class ParseHexTest : public ::testing::Test {
};


TEST_F(ParseHexTest, Parse) {
  EXPECT_EQ(1, parse_hex("00000000000000000000000000000001"));
  EXPECT_EQ(15, parse_hex("0000000000000000000000000000000f"));
  EXPECT_EQ(16, parse_hex("00000000000000000000000000000010"));

  // We can't just say (_uint128_t); gcc does not support compile
  // time constant literals larger than long long, which is in 2017
  // still 64 bits :-D
  // https://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html
  //
  // Also stream >> _int128_t doesn't work either.
  //
  // I'm tempted to "build" it by oring together two 64 bit
  // integers, but this is just cleaner.

  __uint128_t expected = 0;

  for (auto c : std::string("146288173456218371156967821625524084837"))
    expected = expected * 10 + (c - '0');

  // And EXPECT_EQ doesn't work correctly with __int128_t.  Its so
  // unloved.
  auto actual = parse_hex("6e0e13cef90a46a086cf6865401de065");

  EXPECT_EQ(expected, actual);  // Fix this later

  char reversed[33];
  reversed[32] = 0;
  compose_hex(actual, reversed);

  std::string restored_text {reversed};
  std::string actual_text {"6e0e13cef90a46a086cf6865401de065"};
  EXPECT_EQ(actual_text,restored_text);
}

class FindTest : public ::testing::Test {
};

TEST_F(FindTest, FindOne) {
  EXPECT_EQ(0, find_one<'a'>("abcdef", 6));
  EXPECT_EQ(2, find_one<'c'>("abcdef", 6));
  EXPECT_EQ(4, find_one<'e'>("abcdef", 6));
  EXPECT_EQ(6, find_one<'q'>("abcdef", 6));

  EXPECT_EQ(5, find_one<'f'>("abcdef", 6));
  EXPECT_EQ(3, find_one<'f'>("abcdef", 3));
}

TEST_F(FindTest, FindMany) {
  EXPECT_EQ(1, (find_many<'e', 'b'>("abcdef", 6)));
  EXPECT_EQ(1, (find_many<'b', 'e'>("abcdef", 6)));
  EXPECT_EQ(4, (find_many<'f', 'e'>("abcdef", 4)));
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}



