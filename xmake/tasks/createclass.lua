local headerTemplate, inlineTemplate, sourceTemplate, headerTestTemplate, inlineTestTemplate, sourceTestTemplate

task("create-class")

set_menu({
  usage = "xmake create-class [options] name",
  description = "Task to help class creation.",
  options = {
      {nil, "nocpp", "k", nil, "Set this to create a header-only class."},
      {nil, "name", "v", nil, "Class name" },
      {nil, "test", "k", nil, "Set this to create the class in a test."},
      {nil, "testname", "v", nil, "If the test flag is set, provide the test name with this."}
  } 
})

on_run(function()
  import("core.base.option")

  local classPath = option.get("name")
  if not classPath then
    os.raise("Missing class name")
  end

  local className = path.basename(classPath)

  local files = {
  }

  if not option.get("test") then
    table.insert(files, { TargetPath = path.join("Include/Framework", classPath) .. ".hpp", Template = headerTemplate })
    table.insert(files, { TargetPath = path.join("Include/Framework", classPath) .. ".inl", Template = inlineTemplate })
  else
    local testName = option.get("testname")
    if not testName then
      os.raise("Test flag set, but no test name provided.")
    end
    
    table.insert(files, { TargetPath = path.join("Tests/" .. testName .. "/Include/" .. testName, classPath) .. ".hpp", Template = headerTestTemplate })
    table.insert(files, { TargetPath = path.join("Tests/" .. testName .. "/Include/" .. testName, classPath) .. ".inl", Template = inlineTestTemplate })  
  end

  if not option.get("nocpp") then
    if not option.get("test") then
      table.insert(files, {TargetPath = path.join("Source", "Framework", classPath) .. ".cpp", Template = sourceTemplate})
    else
      local testName = option.get("testname")
      if not testName then
        os.raise("Test flag set, but no test name provided.")
      end

      table.insert(files, { TargetPath = path.join("Tests/" .. testName .. "/Source/" .. testName, classPath) .. ".cpp", Template = sourceTestTemplate })
    end
  end

  local replacements

  if option.get("test") then
    local testName = option.get("testname")
    if not testName then
      os.raise("Test flag set, but no test name provided.")
    end

    replacements = {
      CLASS_NAME = className,
      CLASS_PATH = classPath,
      COPYRIGHT = os.date("%Y") .. [[ Jean "Pixfri" Letessier ]],
      HEADER_GUARD = "D3D12TESTS_" .. testName:upper() .. "_" .. classPath:gsub("[/\\]", "_"):upper() .. "_HPP",
      TEST_NAME = testName
    }
  else
    replacements = {
      CLASS_NAME = className,
      CLASS_PATH = classPath,
      COPYRIGHT = os.date("%Y") .. [[ Jean "Pixfri" Letessier ]],
      HEADER_GUARD = "D3D12TESTS_" .. classPath:gsub("[/\\]", "_"):upper() .. "_HPP"
    } 
  end

  for _, file in pairs(files) do
    local content = file.Template:gsub("%%([%u_]+)%%", function (kw)
      local r = replacements[kw]
      if not r then
        os.raise("Missing replacement for " .. kw)
      end

      return r
    end)

    io.writefile(file.TargetPath, content)
  end
end)

headerTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef %HEADER_GUARD%
#define %HEADER_GUARD%

#include "D3D12Tests/pch.hpp"

namespace D3D12Tests {
    class %CLASS_NAME% {
    public:
        %CLASS_NAME%() = default;
        ~%CLASS_NAME%() = default;

        %CLASS_NAME%(const %CLASS_NAME%&) = delete;
        %CLASS_NAME%(%CLASS_NAME%&&) = delete;

        %CLASS_NAME%& operator=(const %CLASS_NAME%&) = delete;
        %CLASS_NAME%& operator=(%CLASS_NAME%&&) = delete;
    
    private:
    };
}

#include "D3D12Tests/%CLASS_PATH%.inl"

#endif // %HEADER_GUARD%
]]

inlineTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

namespace D3D12Tests {
    
}
]]

sourceTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "D3D12Tests/%CLASS_PATH%.hpp"

namespace D3D12Tests {
    
}
]]

headerTestTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef %HEADER_GUARD%
#define %HEADER_GUARD%

#include "Framework/pch.hpp"

namespace %TEST_NAME% {
    class %CLASS_NAME% {
    public:
        %CLASS_NAME%() = default;
        ~%CLASS_NAME%() = default;

        %CLASS_NAME%(const %CLASS_NAME%&) = delete;
        %CLASS_NAME%(%CLASS_NAME%&&) = delete;

        %CLASS_NAME%& operator=(const %CLASS_NAME%&) = delete;
        %CLASS_NAME%& operator=(%CLASS_NAME%&&) = delete;
    
    private:
    };
}

#include "%TEST_NAME%/%CLASS_PATH%.inl"

#endif // %HEADER_GUARD%
]]

inlineTestTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

namespace %TEST_NAME% {
    
}
]]

sourceTestTemplate = [[
// Copyright (C) %COPYRIGHT%
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "%TEST_NAME%/%CLASS_PATH%.hpp"

namespace %TEST_NAME% {
    
}
]]
