#pragma once

#include <assert.h>
#include <wchar.h>
#include <cmath>
#include <math.h>
#include <algorithm>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#include <array>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <limits>
#include <wchar.h>
#include <ctype.h>


// C++11 countof definition taken from http://www.g-truc.net/post-0708.html
template<typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept {
  return N;
}

using namespace std;
