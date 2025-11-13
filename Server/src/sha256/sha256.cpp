#include <iostream>
#include <vector>
#include <bit>
#include <algorithm>

#include "sha256.h"

typedef unsigned uint;

std::string SHA256( const char *Msg, uint64_t length )
{
  std::vector<uint> Bulk;
  auto ToBin = []( uint x )
  {
    std::vector<uint> Res;

    while (x > 0)
    {
      Res.push_back(x % 2);
      x /= 2;
    }
    while (Res.size() < 8)
      Res.push_back(0);
    std::reverse(Res.begin(), Res.end());

    return Res;
  };

  auto ToBin64 = []( uint64_t x )
  { 
    std::vector<uint> Res;

    while (x > 0)
    {
      Res.push_back(x % 2);
      x /= 2;
    }
    while (Res.size() < 64)
      Res.push_back(0);
    std::reverse(Res.begin(), Res.end());

    return Res;
  };

  /*
   * STEP 1 - PREPROCESSING
   */

  // Convert message to binary
  while (*Msg != 0)
  {
    auto tmp = ToBin(*Msg++);

    Bulk.insert(Bulk.end(), tmp.begin(), tmp.end());
  }

  // Append a single 1
  Bulk.push_back(1);

  // Pad with 0's
  while (Bulk.size() % 512 != 448)
    Bulk.push_back(0);

  // Add message length in bits
  auto bitlen = ToBin64(length * 8);
  Bulk.insert(Bulk.end(), bitlen.begin(), bitlen.end());

  /*
   * STEP 2 - INITIALIZE CONSTANTS
   */
  uint
    h0 = 0x6A09E667,
    h1 = 0xBB67AE85,
    h2 = 0x3C6EF372,
    h3 = 0xA54FF53A,
    h4 = 0x510E527F,
    h5 = 0x9B05688C,
    h6 = 0x1F83D9AB,
    h7 = 0x5BE0CD19;
  uint K[64] = 
  {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
  };

  /*
   * STEP 3 - SHUNK LOOP
   */
  for (uint i = 0; i < (uint)Bulk.size(); i += 512)
  {
    std::vector<uint> SubBulk;

    SubBulk.insert(SubBulk.end(), Bulk.begin() + i, Bulk.begin() + i + 512);

    // Create words array
    std::vector<uint> Words(64);

    for (uint j = 0; j < 512; j += 32)
    {
      std::string BitWord;

      for (uint k = 0; k < 32; k++)
        BitWord += ('0' + SubBulk[(i + j + k) % SubBulk.size()]);
      Words[j / 32] = strtoul(BitWord.c_str(), nullptr, 2);
    }

    // Generate additional 48 words
    for (uint j = 16; j < 64; j++)
    {
      uint s0 = (std::rotr(Words[j - 15], 7)) ^ (std::rotr(Words[j - 15], 18)) ^ (Words[j - 15] >> 3);
      uint s1 = (std::rotr(Words[j - 2], 17)) ^ (std::rotr(Words[j - 2], 19)) ^ (Words[j - 2] >> 10);

      Words[j] = Words[j - 16] + s0 + Words[j - 7] + s1;
    }

    // Initialize variables
    uint
      a = h0,
      b = h1,
      c = h2,
      d = h3,
      e = h4,
      f = h5,
      g = h6,
      h = h7;

    // Main cycle
    for (uint j = 0; j < 64; j++)
    {
      uint
        E0 = (std::rotr(a, 2)) ^ (std::rotr(a, 13)) ^ (std::rotr(a, 22)),
        Ma = (a & b) ^ (a & c) ^ (b & c),
        t2 = E0 + Ma,
        E1 = (std::rotr(e, 6)) ^ (std::rotr(e, 11)) ^ (std::rotr(e, 25)),
        Ch = (e & f) ^ ((~e) & g),
        t1 = h + E1 + Ch + K[j] + Words[j];

      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

    // Add variables to hash
    h0 = h0 + a;
    h1 = h1 + b;
    h2 = h2 + c;
    h3 = h3 + d;
    h4 = h4 + e;
    h5 = h5 + f;
    h6 = h6 + g;
    h7 = h7 + h;
  }

  std::string res;
  char resBuf[65];

  sprintf(resBuf, "%08X%08X%08X%08X%08X%08X%08X%08X", h0, h1, h2, h3, h4, h5, h6, h7);
  res = resBuf;

  return res;
}

std::string SHA224( const char *Msg, uint64_t length )
{
  std::vector<uint> Bulk;
  auto ToBin = []( uint x )
  {
    std::vector<uint> Res;

    while (x > 0)
    {
      Res.push_back(x % 2);
      x /= 2;
    }
    while (Res.size() < 8)
      Res.push_back(0);
    std::reverse(Res.begin(), Res.end());

    return Res;
  };

  auto ToBin64 = []( uint64_t x )
  { 
    std::vector<uint> Res;

    while (x > 0)
    {
      Res.push_back(x % 2);
      x /= 2;
    }
    while (Res.size() < 64)
      Res.push_back(0);
    std::reverse(Res.begin(), Res.end());

    return Res;
  };

  /*
   * STEP 1 - PREPROCESSING
   */

  // Convert message to binary
  while (*Msg != 0)
  {
    auto tmp = ToBin(*Msg++);

    Bulk.insert(Bulk.end(), tmp.begin(), tmp.end());
  }

  // Append a single 1
  Bulk.push_back(1);

  // Pad with 0's
  while (Bulk.size() % 512 != 448)
    Bulk.push_back(0);

  // Add message length in bits
  auto bitlen = ToBin64(length * 8);
  Bulk.insert(Bulk.end(), bitlen.begin(), bitlen.end());

  /*
   * STEP 2 - INITIALIZE CONSTANTS
   */
  uint
    h0 = 0xC1059ED8,
    h1 = 0x367CD507,
    h2 = 0x3070DD17,
    h3 = 0xF70E5939,
    h4 = 0xFFC00B31,
    h5 = 0x68581511,
    h6 = 0x64F98FA7,
    h7 = 0xBEFA4FA4;
  uint K[64] = 
  {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
  };

  /*
   * STEP 3 - SHUNK LOOP
   */
  for (uint i = 0; i < (uint)Bulk.size(); i += 512)
  {
    std::vector<uint> SubBulk;

    SubBulk.insert(SubBulk.end(), Bulk.begin() + i, Bulk.begin() + i + 512);

    // Create words array
    std::vector<uint> Words(64);

    for (uint j = 0; j < 512; j += 32)
    {
      std::string BitWord;

      for (uint k = 0; k < 32; k++)
        BitWord += ('0' + SubBulk[i + j + k]);
      Words[j / 32] = strtoul(BitWord.c_str(), nullptr, 2);
    }

    // Generate additional 48 words
    for (uint j = 16; j < 64; j++)
    {
      uint s0 = (std::rotr(Words[j - 15], 7)) ^ (std::rotr(Words[j - 15], 18)) ^ (Words[j - 15] >> 3);
      uint s1 = (std::rotr(Words[j - 2], 17)) ^ (std::rotr(Words[j - 2], 19)) ^ (Words[j - 2] >> 10);

      Words[j] = Words[j - 16] + s0 + Words[j - 7] + s1;
    }

    // Initialize variables
    uint
      a = h0,
      b = h1,
      c = h2,
      d = h3,
      e = h4,
      f = h5,
      g = h6,
      h = h7;

    // Main cycle
    for (uint j = 0; j < 64; j++)
    {
      uint
        E0 = (std::rotr(a, 2)) ^ (std::rotr(a, 13)) ^ (std::rotr(a, 22)),
        Ma = (a & b) ^ (a & c) ^ (b & c),
        t2 = E0 + Ma,
        E1 = (std::rotr(e, 6)) ^ (std::rotr(e, 11)) ^ (std::rotr(e, 25)),
        Ch = (e & f) ^ ((~e) & g),
        t1 = h + E1 + Ch + K[j] + Words[j];

      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

    // Add variables to hash
    h0 = h0 + a;
    h1 = h1 + b;
    h2 = h2 + c;
    h3 = h3 + d;
    h4 = h4 + e;
    h5 = h5 + f;
    h6 = h6 + g;
    h7 = h7 + h;
  }

  std::string res;
  char resBuf[57];

  sprintf(resBuf, "%08X%08X%08X%08X%08X%08X%08X", h0, h1, h2, h3, h4, h5, h6);
  res = resBuf;

  return res;
}

std::string SHA256( const std::string &msg )
{
  return SHA256(msg.c_str(), msg.length());
}

std::string SHA224( const std::string &msg )
{
  return SHA224(msg.c_str(), msg.length());
}