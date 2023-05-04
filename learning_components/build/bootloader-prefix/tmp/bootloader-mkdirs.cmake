# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/cold/esp/esp-idf/components/bootloader/subproject"
  "/home/cold/esp/learning_components/build/bootloader"
  "/home/cold/esp/learning_components/build/bootloader-prefix"
  "/home/cold/esp/learning_components/build/bootloader-prefix/tmp"
  "/home/cold/esp/learning_components/build/bootloader-prefix/src/bootloader-stamp"
  "/home/cold/esp/learning_components/build/bootloader-prefix/src"
  "/home/cold/esp/learning_components/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/cold/esp/learning_components/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/cold/esp/learning_components/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
