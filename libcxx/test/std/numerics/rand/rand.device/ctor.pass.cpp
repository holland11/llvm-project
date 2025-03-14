//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// See https://llvm.org/PR20183
// XFAIL: use_system_cxx_lib && target={{.+}}-apple-macosx10.{{9|10|11}}

// UNSUPPORTED: libcpp-has-no-random-device

// <random>

// class random_device;

// explicit random_device(const string& token = implementation-defined); // before C++20
// random_device() : random_device(implementation-defined) {}            // C++20
// explicit random_device(const string& token);                          // C++20

// For the following ctors, the standard states: "The semantics and default
// value of the token parameter are implementation-defined". Implementations
// therefore aren't required to accept any string, but the default shouldn't
// throw.

#include <random>
#include <system_error>
#include <cassert>

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include "test_macros.h"
#if TEST_STD_VER >= 11
#include "test_convertible.h"
#endif

void check_random_device_valid(const std::string &token) {
  std::random_device r(token);
}

void check_random_device_invalid(const std::string &token) {
#ifndef TEST_HAS_NO_EXCEPTIONS
  try {
    std::random_device r(token);
    LIBCPP_ASSERT(false);
  } catch (const std::system_error&) {
  }
#else
  ((void)token);
#endif
}

int main(int, char**) {
  {
    std::random_device r;
  }
  // Check the validity of various tokens
  {
    check_random_device_invalid("wrong file");
    check_random_device_invalid("/dev/whatever");
    check_random_device_valid("/dev/urandom");
#if defined(_LIBCPP_USING_DEV_RANDOM)
    check_random_device_valid("/dev/random");
#else
    check_random_device_invalid("/dev/random");
#endif
  }

#if !defined(_WIN32)
// Test that random_device(const string&) properly handles getting
// a file descriptor with the value '0'. Do this by closing the standard
// streams so that the descriptor '0' is available.
  {
    int ec;
    ec = close(STDIN_FILENO);
    assert(!ec);
    ec = close(STDOUT_FILENO);
    assert(!ec);
    ec = close(STDERR_FILENO);
    assert(!ec);
    std::random_device r;
  }
#endif // !defined(_WIN32)

#if TEST_STD_VER >= 11
  static_assert(test_convertible<std::random_device>(), "");
#endif

  return 0;
}
