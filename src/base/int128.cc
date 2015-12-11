// Copyright 2004 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// All Rights Reserved.
//

#include "base/int128.h"

#include <iomanip>
#include <iostream>  // NOLINT(readability/streams)
#include <sstream>

#include <glog/logging.h>

#include "base/integral_types.h"

const uint128_pod kuint128max = {
    static_cast<uint64>(GG_LONGLONG(0xFFFFFFFFFFFFFFFF)),
    static_cast<uint64>(GG_LONGLONG(0xFFFFFFFFFFFFFFFF))
};

// Returns the 0-based position of the last set bit (i.e., most significant bit)
// in the given uint64. The argument may not be 0.
//
// For example:
//   Given: 5 (decimal) == 101 (binary)
//   Returns: 2
#define STEP(T, n, pos, sh)                   \
  do {                                        \
    if ((n) >= (static_cast<T>(1) << (sh))) { \
      (n) = (n) >> (sh);                      \
      (pos) |= (sh);                          \
    }                                         \
  } while (0)
static inline int Fls64(uint64 n) {
  DCHECK_NE(0, n);
  int pos = 0;
  STEP(uint64, n, pos, 0x20);
  uint32 n32 = n;
  STEP(uint32, n32, pos, 0x10);
  STEP(uint32, n32, pos, 0x08);
  STEP(uint32, n32, pos, 0x04);
  return pos + ((GG_ULONGLONG(0x3333333322221100) >> (n32 << 2)) & 0x3);
}
#undef STEP

// Like Fls64() above, but returns the 0-based position of the last set bit
// (i.e., most significant bit) in the given uint128. The argument may not be 0.
static inline int Fls128(uint128 n) {
  if (uint64 hi = Uint128High64(n)) {
    return Fls64(hi) + 64;
  }
  return Fls64(Uint128Low64(n));
}

// Long division/modulo for uint128 implemented using the shift-subtract
// division algorithm adapted from:
// http://stackoverflow.com/questions/5386377/division-without-using
void uint128::DivModImpl(uint128 dividend, uint128 divisor,
                         uint128* quotient_ret, uint128* remainder_ret) {
  if (divisor == 0) {
    LOG(FATAL) << "Division or mod by zero: dividend.hi=" << dividend.hi_
               << ", lo=" << dividend.lo_;
  }

  if (divisor > dividend) {
    *quotient_ret = 0;
    *remainder_ret = dividend;
    return;
  }

  if (divisor == dividend) {
    *quotient_ret = 1;
    *remainder_ret = 0;
    return;
  }

  uint128 denominator = divisor;
  uint128 quotient = 0;

  // Left aligns the MSB of the denominator and the dividend.
  const int shift = Fls128(dividend) - Fls128(denominator);
  denominator <<= shift;

  // Uses shift-subtract algorithm to divide dividend by denominator. The
  // remainder will be left in dividend.
  for (int i = 0; i <= shift; ++i) {
    quotient <<= 1;
    if (dividend >= denominator) {
      dividend -= denominator;
      quotient |= 1;
    }
    denominator >>= 1;
  }

  *quotient_ret = quotient;
  *remainder_ret = dividend;
}

uint128& uint128::operator/=(const uint128& divisor) {
  uint128 quotient = 0;
  uint128 remainder = 0;
  DivModImpl(*this, divisor, &quotient, &remainder);
  *this = quotient;
  return *this;
}
uint128& uint128::operator%=(const uint128& divisor) {
  uint128 quotient = 0;
  uint128 remainder = 0;
  DivModImpl(*this, divisor, &quotient, &remainder);
  *this = remainder;
  return *this;
}

std::ostream& operator<<(std::ostream& o, const uint128& b) {
  std::ios_base::fmtflags flags = o.flags();

  // Select a divisor which is the largest power of the base < 2^64.
  uint128 div;
  std::streamsize div_base_log;
  switch (flags & std::ios::basefield) {
    case std::ios::hex:
      div = GG_ULONGLONG(0x1000000000000000);  // 16^15
      div_base_log = 15;
      break;
    case std::ios::oct:
      div = GG_ULONGLONG(01000000000000000000000);  // 8^21
      div_base_log = 21;
      break;
    default:  // std::ios::dec
      div = GG_ULONGLONG(10000000000000000000);  // 10^19
      div_base_log = 19;
      break;
  }

  // Now piece together the uint128 representation from three chunks of
  // the original value, each less than "div" and therefore representable
  // as a uint64.
  std::ostringstream os;
  std::ios_base::fmtflags copy_mask =
      std::ios::basefield | std::ios::showbase | std::ios::uppercase;
  os.setf(flags & copy_mask, copy_mask);
  uint128 high = b;
  uint128 low;
  uint128::DivModImpl(high, div, &high, &low);
  uint128 mid;
  uint128::DivModImpl(high, div, &high, &mid);
  if (high.lo_ != 0) {
    os << high.lo_;
    os << std::noshowbase << std::setfill('0') << std::setw(div_base_log);
    os << mid.lo_;
    os << std::setw(div_base_log);
  } else if (mid.lo_ != 0) {
    os << mid.lo_;
    os << std::noshowbase << std::setfill('0') << std::setw(div_base_log);
  }
  os << low.lo_;
  std::string rep = os.str();

  // Add the requisite padding.
  std::streamsize width = o.width(0);
  if (width > rep.size()) {
    if ((flags & std::ios::adjustfield) == std::ios::left) {
      rep.append(width - rep.size(), o.fill());
    } else {
      rep.insert(0, width - rep.size(), o.fill());
    }
  }

  // Stream the final representation in a single "<<" call.
  return o << rep;
}