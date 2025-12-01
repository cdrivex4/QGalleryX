# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\appSamsungGalleryTest_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\appSamsungGalleryTest_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\appSamsungGallery_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\appSamsungGallery_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\tst_imagemodel_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\tst_imagemodel_autogen.dir\\ParseCache.txt"
  "appSamsungGalleryTest_autogen"
  "appSamsungGallery_autogen"
  "tst_imagemodel_autogen"
  )
endif()
