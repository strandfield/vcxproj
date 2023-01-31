// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'vcxproj' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "vcxproj/solution.h"

#include <filesystem>
#include <fstream>

#include <cassert>
#include <stdexcept>
#include <sstream>

namespace
{

inline bool starts_with(const std::string& str, char c) noexcept
{
  return !str.empty() && str.front() == c;
}

inline bool starts_with(const std::string& str, std::string_view text) noexcept
{
  return str.size() >= text.size()
    && std::strncmp(str.data(), text.data(), text.size()) == 0;
}

inline bool starts_with(const std::string& str, const char* text) noexcept
{
  return starts_with(str, std::string_view(text));
}

inline bool starts_with(const std::string& str, const std::string& text) noexcept
{
  return str.size() >= text.size()
    && std::strncmp(str.data(), text.data(), text.size()) == 0;
}

inline bool ends_with(const std::string& str, char c) noexcept
{
  return !str.empty() && str.back() == c;
}

inline bool ends_with(const std::string& str, std::string_view text) noexcept
{
  return str.size() >= text.size()
    && std::strncmp(str.data() + (str.size() - text.size()), text.data(), text.size()) == 0;
}

inline bool ends_with(const std::string& str, const char* text) noexcept
{
  return ends_with(str, std::string_view(text));
}

inline bool ends_with(const std::string& str, const std::string& text) noexcept
{
  return str.size() >= text.size()
    && std::strncmp(str.data() + (str.size() - text.size()), text.data(), text.size()) == 0;
}

inline std::string trimmed(std::string str)
{
  size_t r = 0;
  size_t w = 0;

  // Remove whitespaces from the end
  while (str.length() > 0 && std::isspace(str.back()))
    str.pop_back();

  // Skip whitespaces from the beginning
  while (r < str.length() && std::isspace(str[r]))
    ++r;

  while (r < str.length())
    str[w++] = str[r++];

  str.resize(w);
  return str;
}

inline std::string simplified(std::string str)
{
  size_t r = 0;
  size_t w = 0;

  // Remove whitespaces from the end
  while (str.length() > 0 && std::isspace(str.back()))
    str.pop_back();

  // Skip whitespaces from the beginning
  while (r < str.length() && std::isspace(str[r]))
    ++r;

  do
  {
    while (r < str.length() && !std::isspace(str[r]))
      str[w++] = str[r++];

    if (r < str.length())
    {
      str[w++] = ' ';

      // Skip whitespaces in the middle
      while (r < str.length() && std::isspace(str[r]))
        ++r;
    }
  } while (r < str.length());

  str.resize(w);
  return str;
}

inline bool contains(const std::string& text, const std::string& str)
{
  return text.find(str) != std::string::npos;
}

std::vector<std::string> split(const std::string& text, char sep)
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

std::string read_line(std::istream& stream)
{
  std::string str;
  std::getline(stream, str);
  return str;
}

std::string read_line_simplified(std::istream& stream)
{
  return simplified(read_line(stream));
}

} // namespace

class SolutionParser
{
public:
  std::istream& stream;
  vcxproj::Solution& solution;

public:
  SolutionParser(std::istream& stream_, vcxproj::Solution& sol)
    : stream(stream_),
      solution(sol)
  {

  }

  static std::string unquoted(std::string str)
  {
    if (ends_with(str, '"'))
    {
      str.pop_back();
      str.erase(0, 1);
    }

    return str;
  }

  static std::string unbraced(std::string str)
  {
    if (ends_with(str, '}'))
    {
      str.pop_back();
      str.erase(0, 1);
    }

    return str;
  }

  void readVersion(std::string line)
  {
    std::string str = line.substr(std::string("Microsoft Visual Studio Solution File, Format Version ").length());
    solution.version = str;
  }

  void readProject(std::string line, std::istream& stream)
  {
    size_t index = line.find("=");
    line = simplified(line.substr(index + 1));
    std::vector<std::string> items = split(line, ',');

    vcxproj::Project project;

    project.name = unquoted(items.at(0));
    project.filepath = unquoted(trimmed(items.at(1)));
    project.id = unbraced(unquoted(trimmed(items.at(2))));

    solution.projects.push_back(std::move(project));

    while (!stream.eof())
    {
      line = simplified(read_line(stream));

      if (line == "EndProject")
        return;

      if (starts_with(line, "ProjectSection"))
        readProjectSection(line, stream, solution.projects.back());
    }
  }

  void readProjectSection(std::string line, std::istream& stream, vcxproj::Project& project)
  {
    if (starts_with(line, "ProjectSection(ProjectDependencies)"))
    {
      while (!stream.eof())
      {
        line = read_line_simplified(stream);

        if (line == "EndProjectSection")
          return;

        // e.g. {17A02C77-346E-3F08-A2A3-5AF377AC0452} = {17A02C77-346E-3F08-A2A3-5AF377AC0452}
        size_t equal_sign = line.find("=");
        line.erase(equal_sign);
        line = trimmed(line);
        vcxproj::uuid_t dependency_id = unbraced(line);

        project.dependencies.push_back(dependency_id);
      }
    }
    else
    {
      while (!stream.eof())
      {
        line = read_line_simplified(stream);

        if (line == "EndProjectSection")
          return;
      }
    }
  }

  void readGlobal(std::string line, std::istream& stream)
  {
    while (!stream.eof())
    {
      line = read_line_simplified(stream);

      if (line == "EndGlobal")
        return;

      if (starts_with(line, "GlobalSection("))
        readGlobalSection(line, stream);
    }
  }

  void readGlobalSection(std::string line, std::istream& stream)
  {
    static const std::string EndGlobalSection = "EndGlobalSection";

    if (starts_with(line, "GlobalSection(SolutionConfigurationPlatforms)"))
    {
      while (!stream.eof())
      {
        line = read_line_simplified(stream);

        if (line == EndGlobalSection)
          return;

        // e.g. Debug|x64 = Debug|x64
        size_t equal_sign = line.find("=");
        line.erase(equal_sign, line.size());
        while (ends_with(line, ' '))
          line.pop_back();

        solution.configurations.push_back(line);
      }
    }
    else
    {
      while (!stream.eof())
      {
        line = read_line_simplified(stream);

        if (line == EndGlobalSection)
          return;
      }
    }
  }

  void operator()()
  {
    read_line(stream);

    while (!stream.eof())
    {
      std::string line = read_line(stream);

      if (starts_with(line, "Microsoft Visual Studio Solution File, Format Version"))
        readVersion(line);
      else if (starts_with(line, "Project("))
        readProject(line, stream);
      else if (starts_with(line, "Global"))
        readGlobal(line, stream);
      else
        continue; // @TODO: decide what to do here
    }
  }
};

namespace vcxproj
{

vcxproj::Solution load_solution(const std::filesystem::path& filepath)
{
  std::ifstream file{ filepath.u8string() };

  if (!file.good())
    throw std::runtime_error("Could not open file");

  vcxproj::Solution solution;
  solution.filepath = filepath;
  solution.name = filepath.stem().string();

  SolutionParser parser{ file, solution };
  parser();

  for (auto& pro : solution.projects)
  {
    std::filesystem::path path{ solution.filepath };
    path = path.parent_path();
    path = path / pro.filepath;

    std::map<std::string, std::string> variables;
    variables["SolutionDir"] = solution.filepath.parent_path().u8string();

    load_project(&pro, path, variables);
  }
    
  return solution;
}

} // namespace vcxproj
