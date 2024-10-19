target("HelloWindow")
  set_kind("binary")
  
  add_files("Source/**.cpp")
  
  for _, ext in ipairs({".hpp", ".inl"}) do
    add_headerfiles("Include/**" .. ext)
  end

  add_includedirs("Include")
  
  add_deps("Framework")