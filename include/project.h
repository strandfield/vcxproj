// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'vcxproj' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef VCXPROJ_PROJECT_H
#define VCXPROJ_PROJECT_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace vcxproj
{

typedef std::string uuid_t;

enum class CppStandard
{
  Cpp14,
  Cpp17,
  Cpp20,
  CppLatest,
};

struct ProjectConfiguration 
{
  enum Platform
  {
    Win32,
    x64,
    Unknown,
  };

  std::string name;
  std::string configuration;
  std::string platformStr;
  Platform platform = Unknown;
};

struct ItemDefinitionGroup
{
  std::string condition;
  std::string preprocessorDefinitions;
  std::string additionalIncludeDirectories;
  CppStandard cppstd = CppStandard::CppLatest;
};

struct Project
{
  uuid_t id;
  std::string name;
  std::string filepath;
  std::vector<uuid_t> dependencies;
  std::vector<ProjectConfiguration> projectConfigurationList;
  std::vector<std::string> compileList;
  std::vector<std::string> includeList;
  std::vector<ItemDefinitionGroup> itemDefinitionGroupList;
};


bool evaluate(std::string& s, const std::map<std::string, std::string>& variables, bool readenv = true);

void load_project(Project* project, const std::filesystem::path& filepath, std::map<std::string, std::string>& variables);

} // namespace vcxproj

#endif // VCXPROJ_PROJECT_H
