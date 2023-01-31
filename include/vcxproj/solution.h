// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'vcxproj' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef VCXPROJ_SOLUTION_H
#define VCXPROJ_SOLUTION_H

#include "project.h"

namespace vcxproj
{

typedef std::string version_t;

struct Solution
{
  std::string name;
  std::filesystem::path filepath;
  version_t version;
  std::vector<std::string> configurations;
  std::vector<Project> projects;
};

vcxproj::Solution load_solution(const std::filesystem::path& filepath);

} // namespace vcxproj

#endif // VCXPROJ_SOLUTION_H
