// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'vcxproj' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef VCXPROJ_UTILS_H
#define VCXPROJ_UTILS_H

#include <algorithm>
#include <string>
#include <vector>

namespace vcxproj
{

inline std::vector<std::string> split(const std::string& text, char sep)
{
  std::vector<std::string> result;

  {
    auto it = text.begin();
    auto next = std::find(text.begin(), text.end(), sep);

    while (next != text.end())
    {
      result.push_back(std::string(it, next));
      it = next + 1;
      next = std::find(it, text.end(), sep);
    }

    result.push_back(std::string(it, next));
  }

  auto it = std::remove_if(result.begin(), result.end(), [](const std::string& str) -> bool {
    return str.empty();
    });

  result.erase(it, result.end());

  return result;
}

} // namespace vcxproj

#endif // VCXPROJ_UTILS_H
