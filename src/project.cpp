// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'vcxproj' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "vcxproj/project.h"

#include <tinyxml2.h>

#include <filesystem>
#include <fstream>

#include <cassert>

inline bool streq(const char* a, const char* b)
{
  return std::strcmp(a, b) == 0;
}

inline bool elementis(const tinyxml2::XMLElement& e, const char* name)
{
  return streq(e.Name(), name);
}

class XmlChildElementIterator
{
public:
  const tinyxml2::XMLElement* child = nullptr;

public:
  explicit XmlChildElementIterator(const tinyxml2::XMLElement* c)
    : child(c)
  {

  }

  XmlChildElementIterator(const XmlChildElementIterator&) = default;

  const tinyxml2::XMLElement& operator*() const
  {
    return *child;
  }

  XmlChildElementIterator& operator++()
  {
    child = child->NextSiblingElement();
    return *this;
  }

  XmlChildElementIterator operator++(int)
  {
    XmlChildElementIterator copy{ *this };
    ++(*this);
    return copy;
  }

  XmlChildElementIterator& operator=(const XmlChildElementIterator&) = default;
};

bool operator!=(const XmlChildElementIterator& lhs, const XmlChildElementIterator& rhs)
{
  return lhs.child != rhs.child;
}

class XmlChildElements
{
private:
  const tinyxml2::XMLElement* element = nullptr;

public:
  explicit XmlChildElements(const tinyxml2::XMLElement& e)
    : element(&e)
  {

  }

  XmlChildElementIterator begin() const
  {
    return XmlChildElementIterator(element->FirstChildElement());
  }

  XmlChildElementIterator end() const
  {
    return XmlChildElementIterator(nullptr);
  }
};

XmlChildElements children(const tinyxml2::XMLElement& e)
{
  return XmlChildElements(e);
}

template<typename T>
T from_xml(const tinyxml2::XMLElement*);

template<>
vcxproj::ProjectConfiguration from_xml(const tinyxml2::XMLElement* xml)
{
  vcxproj::ProjectConfiguration ret;

  const char* a = xml->Attribute("Include");
  if (a)
    ret.name = a;

  for (const tinyxml2::XMLElement& e : children(*xml))
  {
    if (!e.GetText())
      continue;

    if (elementis(e, "Configuration"))
    {
      ret.configuration = e.GetText();
    }
    else if (elementis(e, "Platform"))
    {
      ret.platformStr = e.GetText();

      if (ret.platformStr == "Win32")
        ret.platform = vcxproj::ProjectConfiguration::Win32;
      else if (ret.platformStr == "x64")
        ret.platform = vcxproj::ProjectConfiguration::x64;
      else
        ret.platform = vcxproj::ProjectConfiguration::Unknown;
    }
  }

  return ret;
}

template<>
vcxproj::ItemDefinitionGroup from_xml(const tinyxml2::XMLElement* xml)
{
  vcxproj::ItemDefinitionGroup ret;

  const char* condAttr = xml->Attribute("Condition");

  if (condAttr)
    ret.condition = condAttr;

  for (const tinyxml2::XMLElement& clcompile : children(*xml))
  {
    if (!elementis(clcompile, "ClCompile"))
      continue;

    for (const tinyxml2::XMLElement& e : children(clcompile))
    {
      if (!e.GetText())
        continue;

      if (elementis(e, "PreprocessorDefinitions"))
      {
        ret.preprocessorDefinitions = e.GetText();
      }
      else if (elementis(e, "AdditionalIncludeDirectories"))
      {
        ret.additionalIncludeDirectories += e.GetText();
      }
      else if (elementis(e, "LanguageStandard"))
      {
        if (streq(e.GetText(), "stdcpp14"))
          ret.cppstd = vcxproj::CppStandard::Cpp14;
        else if (streq(e.GetText(), "stdcpp17"))
          ret.cppstd = vcxproj::CppStandard::Cpp17;
        else if (streq(e.GetText(), "stdcpp20"))
          ret.cppstd = vcxproj::CppStandard::Cpp20;
        else if (streq(e.GetText(), "stdcpplatest"))
          ret.cppstd = vcxproj::CppStandard::CppLatest;
      }
    }
  }

  return ret;
}

class ProjectParser
{
public:
  vcxproj::Project* project;
  std::map<std::string, std::string>& variables;

public:

  ProjectParser(vcxproj::Project* p, std::map<std::string, std::string>& vars)
    : project(p),
      variables(vars)
  {

  }

  void operator()(const tinyxml2::XMLElement* root)
  {
    for (const tinyxml2::XMLElement& node : children(*root)) 
    {
      if (elementis(node, "ItemGroup")) 
      {
        const char* labelAttribute = node.Attribute("Label");

        if (labelAttribute && streq(labelAttribute, "ProjectConfigurations")) 
        {
          for (const tinyxml2::XMLElement& cfg : children(node)) 
          {
            if (elementis(cfg, "ProjectConfiguration")) 
            {
              auto pconf = from_xml<vcxproj::ProjectConfiguration>(&cfg);

              if (pconf.platform != vcxproj::ProjectConfiguration::Unknown)
                project->projectConfigurationList.push_back(std::move(pconf));
            }
          }
        }
        else 
        {
          for (const tinyxml2::XMLElement& e : children(node)) 
          {
            if (elementis(e, "ClCompile")) 
            {
              const char* include = e.Attribute("Include");

              if (include)
                project->compileList.emplace_back(include);
            }
            else if (elementis(e, "ClInclude"))
            {
              const char* include = e.Attribute("Include");

              if (include)
                project->includeList.emplace_back(include);
            }
          }
        }
      }
      else if (elementis(node, "ItemDefinitionGroup")) 
      {
        project->itemDefinitionGroupList.push_back(from_xml<vcxproj::ItemDefinitionGroup>(&node));
      }
    }
  }
};

namespace vcxproj
{

bool evaluate(std::string& s, const std::map<std::string, std::string>& variables, bool readenv)
{
  size_t start = 0;

  while ((start = s.find("$(", start)) != std::string::npos)
  {
    size_t end = s.find(')', start);

    if (end == std::string::npos)
      return false;

    std::string var = s.substr(start + 2, end - (start + 2));

    auto it = variables.find(var);

    if (it != variables.end())
    {
      s.replace(start, end - start + 1, it->second);
    }
    else
    {
      if (!readenv)
        return false;

      const char* env = std::getenv(var.c_str());

      if (!env)
        return false;

      s.replace(start, end + 1 - start, std::string(env));
    }
  }

  return s.find("$(") == std::string::npos;
}

void load_project(Project* project, const std::filesystem::path& filepath, std::map<std::string, std::string>& variables)
{
  std::string fp = filepath.u8string();
  
  ProjectParser parser{ project, variables };

  tinyxml2::XMLDocument doc;

  if (doc.LoadFile(fp.c_str()) != tinyxml2::XML_SUCCESS)
    return;

  const tinyxml2::XMLElement* rootnode = doc.FirstChildElement();

  if (rootnode == nullptr)
    return;

  parser(rootnode);
}

} // namespace vcxproj
