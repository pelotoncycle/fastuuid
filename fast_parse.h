#pragma once

/* If you see an errror like this, your CPU doesn't support the right
  SSE and AVX instructions

  /usr/lib/gcc/x86_64-linux-gnu/5/include/smmintrin.h:631:1: error:
  inlining failed in call to always_inline ‘int _mm_cmpistri(__m128i,
  __m128i, int)’: target specific option mismatch
  _mm_cmpistri (__m128i __X, __m128i __Y, const int __M)
*/

#include <cstdint>
#include <immintrin.h>
#include <ios>
#include <ostream>
#include <sstream>
#include <string.h>

typedef __uint128_t uuid_t;

static const char hex_digits[] = "0123456789abcdef";


void dump_hex(char *hex, uuid_t value) {
  int offset = 32;
  while (offset) {
    offset -= 1;
    hex[offset] = hex_digits[(unsigned)(value & 0x0f)];
    value >>= 4;
  }
}

void dump_hex(char *hex,  __m256i values) {
  hex[16] = '-';
  hex[33] = '-';
  hex[50] = '-';
  for(int x=0; x<4; x++) {
    int offset = 16;
    auto value = values[x];
    while (offset) {
      offset -= 2;
      hex[15-offset] = hex_digits[(unsigned)(value & 0x0f)];
      value >>= 4;
      hex[15-offset-1] = hex_digits[(unsigned)(value & 0x0f)];
      value >>= 4;
    }
    hex += 17;
  }
}

std::ostream &operator<<(std::ostream &os, uuid_t value) {
  char hex[33];
  hex[32] = 0;
  dump_hex(hex, value);
  std::string output_string;
  return os << hex;
}

std::ostream &operator<<(std::ostream &os, __m256i value) {
  char hex[69];
  hex[64] = 0;
  dump_hex(hex, value);
  return os << hex;
}


uuid_t parse_hex(const char* const hex) {

  static const __m256i zero = _mm256_loadu_si256(reinterpret_cast<const __m256i*>("00000000000000000000000000000000"));
  static const __m256i nine = _mm256_loadu_si256(reinterpret_cast<const __m256i*>("99999999999999999999999999999999"));
  // 'W' is 'a' - 10
  static const __m256i sub_letter = _mm256_loadu_si256(reinterpret_cast<const __m256i*>("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"));

  __m256i h = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(hex));

  auto mask = h;
  auto x = mask;
  mask = _mm256_cmpgt_epi8(h, nine);
  auto low = _mm256_and_si256(mask, _mm256_sub_epi8(h, sub_letter));
  auto high =  _mm256_andnot_si256(mask, _mm256_sub_epi8(h, zero));
  auto merged = _mm256_or_si256(low, high);

  static const auto low_bit_shuffle = _mm256_set_epi8( 1,  3,  5,  7,  9, 11, 13, 15,
                                      -1, -1, -1, -1, -1, -1, -1, -1,
                                      1,  3,  5,  7,  9, 11, 13, 15,
                                      -1, -1, -1, -1, -1, -1, -1, -1);

  static const auto high_bit_shuffle = _mm256_set_epi8( 0,  2,  4,  6,  8, 10, 12, 14,
                                       -1, -1, -1, -1, -1, -1, -1, -1,
                                       0,  2,  4,  6,  8, 10, 12, 14,
                                       -1, -1, -1, -1, -1, -1, -1, -1);

  auto low_bits = _mm256_shuffle_epi8(merged, low_bit_shuffle);


  auto high_bits = _mm256_shuffle_epi8(merged, high_bit_shuffle);
  high_bits = _mm256_slli_epi32(high_bits, 4);  // * 16

  auto together = _mm256_or_si256(low_bits, high_bits);
  uuid_t retval = _mm256_extract_epi64(together, 1);
  retval <<= 64;
  return (((uuid_t)(_mm256_extract_epi64(together, 3))) & 0xffffffffffffffff )| retval;
};


void compose_hex(const uuid_t value, char *output) {
  //__m256i low_bits { (value >> 64) & 0xffffffffffffffff, (value >> 64) & 0xffffffffffffffff, (value >> 64) & 0xffffffffffffffff, (value >> 64) & 0xffffffffffffffff};
  int64_t l = value & 0xffffffffffffffff;
  int64_t h = (value >> 64) & 0xffffffffffffffff;
  __m256i bits { l, h, l, h};


  static const auto low_shuffle = _mm256_set_epi8(0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 8, -1, 9, -1, 10, -1, 11, -1, 12, -1, 13, -1, 14, -1, 15, -1);

  static const auto high_shuffle = _mm256_set_epi8(-1, 0, -1, 1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1, 7, -1, 8, -1, 9, -1, 10, -1, 11, -1, 12, -1, 13, -1, 14, -1, 15);

  auto low = _mm256_shuffle_epi8(bits, low_shuffle);
  auto high = _mm256_shuffle_epi8(bits, high_shuffle);


  static const auto low_mask = _mm256_set1_epi8(0x0f);
  low = _mm256_and_si256(low, low_mask);
  static const auto high_mask = _mm256_set1_epi8(0xf0);
  high = _mm256_and_si256(high, high_mask);
  high = _mm256_srli_epi32(high, 4);


  auto merged = _mm256_or_si256(low, high);
  auto mask = _mm256_cmpgt_epi8(merged, _mm256_set1_epi8(9));

  merged = _mm256_or_si256(
             _mm256_and_si256(mask, _mm256_add_epi8(merged, _mm256_set1_epi8('W'))),
             _mm256_andnot_si256(mask, _mm256_add_epi8(merged, _mm256_set1_epi8('0'))));
  memcpy(output, &merged, sizeof(merged));
}


template <char character_to_find>
inline int find_one(const char* const target, size_t length) {
  static const __m128i pattern = _mm_set1_epi8(character_to_find);
  __m128i s;
  size_t index = 0;
  int search = 16;

  while (search == 16 && index < length) {
    s = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target + index));
    search = _mm_cmpistri(pattern, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_EACH |_SIDD_LEAST_SIGNIFICANT);
    index += search;
  }
  return (index < length) ? index : length;  // Branch free max w/ Pentium Pro's cmov
}

template <char c0, char c1=0, char c2=0, char c3=4, char c4=0, char c5=0, char c6=0, char c7=0, char c8=0, char c9=0, char c10=0, char c11=0, char c12=0, char c13=0, char c14=0, char c15=0>
inline int find_many(const char* const target, size_t length) {
  static const  __m128i pattern = _mm_set_epi8(c15, c14, c13, c12, c11, c10, c9, c8, c7, c6, c5, c4, c3, c2, c1, c0);

  __m128i s;
  size_t index = 0;
  int search = 16;

  while (search == 16 && index < length) {
    s = _mm_loadu_si128(reinterpret_cast<const __m128i*>(target + index));
    search = _mm_cmpistri(pattern, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY |_SIDD_LEAST_SIGNIFICANT);
    index += search;
  }
  return (index < length) ? index : length;  // Branch free max w/ Pentium Pro's cmov
};

namespace std {
template<> struct hash<uuid_t> {
  typedef uuid_t arguement_type;
  typedef std::size_t result_type;
  result_type operator()(arguement_type const & uuid) const {
    std::size_t h1 = uuid & 0xffffffffffffffff;
    std::size_t h2 = (uuid >> 64) & 0xffffffffffffffff;
    return h1 ^ h2;
  };
};
};


