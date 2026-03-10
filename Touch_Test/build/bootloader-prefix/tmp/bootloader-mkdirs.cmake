# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/ESP-IDF/esp-idf/components/bootloader/subproject"
  "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader"
  "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix"
  "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix/tmp"
  "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix/src/bootloader-stamp"
  "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix/src"
  "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/ESP-IDF/LCD/LCD_test_2/Guition-ESP32-S3-4848S040/Touch Test/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
