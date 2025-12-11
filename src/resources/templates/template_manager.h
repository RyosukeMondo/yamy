#pragma once

#include <string>
#include <vector>

namespace yamy
{
namespace resources
{
namespace templates
{
class TemplateManager
{
public:
  TemplateManager() = default;
  ~TemplateManager() = default;

  TemplateManager(const TemplateManager&) = delete;
  TemplateManager& operator=(const TemplateManager&) = delete;

  std::vector<std::string> listTemplates() const;
  bool createFromTemplate(const std::string& templateName,
                          const std::string& targetPath) const;
};
}
}
}
