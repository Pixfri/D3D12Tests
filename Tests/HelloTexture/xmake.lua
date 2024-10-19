target("HelloTexture")
  set_kind("binary")
  
  add_rules("cp_tests_resources")

  add_files("Source/**.cpp")
  
  for _, ext in ipairs({".hpp", ".inl"}) do
    add_headerfiles("Include/**" .. ext)
  end

  add_headerfiles("Resources/**") -- A trick to make them show up in visual studio solutions, but to not copy them at build time.

  add_includedirs("Include")
  
  add_deps("Framework")