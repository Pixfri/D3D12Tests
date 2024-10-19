set_xmakever("2.9.3")

set_project("D3D12Tests")

add_rules("mode.debug", "mode.release")
set_languages("cxx20")
set_allowedplats("windows")

option("override_runtime", {description = "Override VS runtime to MD in release and MDd in debug.", default = true})
option("usepch", {description = "Use the precompiled header to speedup compilation speeds.", default = true})

add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate")

if is_mode("release") then
  set_fpmodels("fast")
  set_optimize("fastest")
  set_symbols("debug", "hidden")
else
  add_defines("D3D12TESTS_DEBUG")
end

set_encodings("utf-8")
set_exceptions("cxx")
set_languages("cxx20")
set_rundir(".")
set_targetdir("./bin/$(plat)_$(arch)_$(mode)")
set_warnings("allextra")

if is_plat("windows") then
  if has_config("override_runtime") then
    set_runtimes(is_mode("debug") and "MDd" or "MD")
  end

  add_defines("UNICODE", "_UNICODE")

  add_syslinks("User32", "dxgi", "d3d12", "d3dcompiler", "shell32")
end

add_cxflags("-Wno-missing-field-initializers -Werror=vla", {tools = {"clang", "gcc"}})

add_requires("directx-headers", "directxtk12", "directxshadercompiler", "directxmath")

target("Framework")
  set_kind("static")
  
  add_files("Source/**.cpp")
  
  add_includedirs("Include", {public = true})
  for _, ext in ipairs({".hpp", ".inl"}) do
    add_headerfiles("Include/**" .. ext)
  end

  if has_config("usepch") then
    set_pcxxheader("Include/Framework/pch.hpp")
  end
  
  add_rpathdirs("$ORIGIN")

  add_packages("directx-headers", "directxtk12", "directxshadercompiler", "directxmath", {public = true})

rule("cp_tests_resources")
  after_build(function (target)
    os.cp("Tests/" .. target:name() .. "/Resources", "bin/$(plat)_$(arch)_$(mode)/" .. target:name())
  end)

includes("Tests/**/xmake.lua")

includes("xmake/**.lua")