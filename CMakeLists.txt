cmake_minimum_required(VERSION 3.14)

project(QtNativeWebView LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(PROJECT_SOURCES
    include/QNativeWebView_global.h include/private/qnativewebview_p.h
    include/qnativewebview.h src/qnativewebview.cpp)

if(WIN32)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindWebView2.cmake")
  include_directories("${WEBVIEW2_INCLUDE_DIR}")
  set(WebViewLibs ${WEBVIEW2_LIBRARY})
  list(APPEND PROJECT_SOURCES include/private/qwebview2webview.h
       src/qwebview2webview.cpp)
endif()

if(LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  pkg_search_module(WEBKIT2GTK REQUIRED webkit2gtk-4.1 webkit2gtk-4.0)
  include_directories(${GTK3_INCLUDE_DIRS} ${WEBKIT2GTK_INCLUDE_DIRS})
  set(WebViewLibs ${WEBKIT2GTK_LIBRARIES})
  list(APPEND PROJECT_SOURCES include/private/qlinuxwebview.h
       src/qlinuxwebview.cpp)
endif()

if(APPLE)
  set(WebViewLibs "-framework WebKit" "-framework AppKit"
                  "-framework Foundation")
  list(APPEND PROJECT_SOURCES include/private/qdarwinwebview.h
       src/qdarwinwebview.mm)
endif()

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})

target_link_libraries(
  ${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::WidgetsPrivate
                          Qt${QT_VERSION_MAJOR}::Widgets ${WebViewLibs})

target_compile_definitions(${PROJECT_NAME} PRIVATE QNATIVEWEBVIEW_LIBRARY)

if(NOT DEFINED NO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
