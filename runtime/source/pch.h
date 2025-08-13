#pragma once

// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=msvc-170
#pragma warning(disable : 4996)

#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <regex>
#include <utility>
#include <memory>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <any>

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <unordered_map>
#include <unordered_set>


#include "core/core.h"

#ifdef PLATFORM_WINDOWS
#    include <Windows.h>
#endif
