cmake_minimum_required(VERSION 3.20)

project(KoboDeluxe
  VERSION 1.0.0
  DESCRIPTION "Kobo Deluxe - an enhanced SDL port of XKobo (modern macOS port)"
  LANGUAGES C CXX
)

# --------------------------------------------------------------------------
# Global settings
# --------------------------------------------------------------------------
set(CMAKE_C_EXTENSIONS ON)      # gnu11
set(CMAKE_CXX_EXTENSIONS ON)    # gnu++14
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")

# No personal info in the bundle identifier (final pick pending owner approval)
set(KOBO_BUNDLE_ID "org.kobodluxe.KoboDeluxe" CACHE STRING
    "Bundle identifier for Kobo Deluxe.app")

# --------------------------------------------------------------------------
# Dependencies (Homebrew)
# --------------------------------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH
  "/opt/homebrew/opt/sdl2"
  "/opt/homebrew/opt/sdl2_image"
)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

# --------------------------------------------------------------------------
# Sources (shared by the .app and the CLI target)
# --------------------------------------------------------------------------
set(KOBO_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/KoboDeluxe-0.5.1)

set(KOBO_SOURCES
  # Top-level game code
  ${KOBO_SRC_ROOT}/cfgform.cpp
  ${KOBO_SRC_ROOT}/cfgparse.cpp
  ${KOBO_SRC_ROOT}/dashboard.cpp
  ${KOBO_SRC_ROOT}/enemies.cpp
  ${KOBO_SRC_ROOT}/enemy.cpp
  ${KOBO_SRC_ROOT}/filemap.cpp
  ${KOBO_SRC_ROOT}/form.cpp
  ${KOBO_SRC_ROOT}/game.cpp
  ${KOBO_SRC_ROOT}/gamectl.cpp
  ${KOBO_SRC_ROOT}/gamestate.cpp
  ${KOBO_SRC_ROOT}/kobo.cpp
  ${KOBO_SRC_ROOT}/manage.cpp
  ${KOBO_SRC_ROOT}/map.cpp
  ${KOBO_SRC_ROOT}/myship.cpp
  ${KOBO_SRC_ROOT}/options.cpp
  ${KOBO_SRC_ROOT}/pfile.cpp
  ${KOBO_SRC_ROOT}/prefs.cpp
  ${KOBO_SRC_ROOT}/radar.cpp
  ${KOBO_SRC_ROOT}/random.cpp
  ${KOBO_SRC_ROOT}/scenes.cpp
  ${KOBO_SRC_ROOT}/score.cpp
  ${KOBO_SRC_ROOT}/screen.cpp
  ${KOBO_SRC_ROOT}/sound.cpp
  ${KOBO_SRC_ROOT}/states.cpp
  ${KOBO_SRC_ROOT}/logger.c
  # Graphics engine
  ${KOBO_SRC_ROOT}/graphics/cs.c
  ${KOBO_SRC_ROOT}/graphics/display.cpp
  ${KOBO_SRC_ROOT}/graphics/filters.c
  ${KOBO_SRC_ROOT}/graphics/gfxengine.cpp
  ${KOBO_SRC_ROOT}/graphics/glSDL.c
  ${KOBO_SRC_ROOT}/graphics/region.c
  ${KOBO_SRC_ROOT}/graphics/sofont.cpp
  ${KOBO_SRC_ROOT}/graphics/sprite.c
  ${KOBO_SRC_ROOT}/graphics/toolkit.cpp
  ${KOBO_SRC_ROOT}/graphics/vidmodes.c
  ${KOBO_SRC_ROOT}/graphics/window.cpp
  # EEL script engine
  ${KOBO_SRC_ROOT}/eel/e_builtin.c
  ${KOBO_SRC_ROOT}/eel/e_getargs.c
  ${KOBO_SRC_ROOT}/eel/e_lexer.c
  ${KOBO_SRC_ROOT}/eel/e_path.c
  ${KOBO_SRC_ROOT}/eel/e_register.c
  ${KOBO_SRC_ROOT}/eel/e_script.c
  ${KOBO_SRC_ROOT}/eel/e_symtab.c
  ${KOBO_SRC_ROOT}/eel/e_util.c
  ${KOBO_SRC_ROOT}/eel/eel.c
  # a_midisynth sound engine
  ${KOBO_SRC_ROOT}/sound/a_agw.c
  ${KOBO_SRC_ROOT}/sound/a_bus.c
  ${KOBO_SRC_ROOT}/sound/a_channel.c
  ${KOBO_SRC_ROOT}/sound/a_commands.c
  ${KOBO_SRC_ROOT}/sound/a_control.c
  ${KOBO_SRC_ROOT}/sound/a_delay.c
  ${KOBO_SRC_ROOT}/sound/a_events.c
  ${KOBO_SRC_ROOT}/sound/a_filters.c
  ${KOBO_SRC_ROOT}/sound/a_globals.c
  ${KOBO_SRC_ROOT}/sound/a_limiter.c
  ${KOBO_SRC_ROOT}/sound/a_midi.c
  ${KOBO_SRC_ROOT}/sound/a_midicon.c
  ${KOBO_SRC_ROOT}/sound/a_midifile.c
  ${KOBO_SRC_ROOT}/sound/a_midisock.c
  ${KOBO_SRC_ROOT}/sound/a_patch.c
  ${KOBO_SRC_ROOT}/sound/a_pitch.c
  ${KOBO_SRC_ROOT}/sound/a_plugin.c
  ${KOBO_SRC_ROOT}/sound/a_sequencer.c
  ${KOBO_SRC_ROOT}/sound/a_struct.c
  ${KOBO_SRC_ROOT}/sound/a_voice.c
  ${KOBO_SRC_ROOT}/sound/a_wave.c
  ${KOBO_SRC_ROOT}/sound/a_wca.c
  ${KOBO_SRC_ROOT}/sound/audio.c
  ${KOBO_SRC_ROOT}/sound/audiodev.c
  ${KOBO_SRC_ROOT}/sound/sfifo.c
)

set(KOBO_INCLUDES
  ${KOBO_SRC_ROOT}
  ${KOBO_SRC_ROOT}/graphics
  ${KOBO_SRC_ROOT}/sound
  ${KOBO_SRC_ROOT}/eel
  ${KOBO_SRC_ROOT}/data/sfx
)

# --------------------------------------------------------------------------
# Targets
# --------------------------------------------------------------------------
function(kobo_add_target name)
  add_executable(${name} ${KOBO_SOURCES})
  target_include_directories(${name} PRIVATE ${KOBO_INCLUDES})
  target_link_libraries(${name} PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    "-framework CoreFoundation"
    "-framework Accelerate"
    "-framework Cocoa"
  )
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${name} PRIVATE DEBUG=1)
  endif()
endfunction()

# --- Kobo Deluxe.app -------------------------------------------------------
kobo_add_target(KoboDeluxe)
target_compile_definitions(KoboDeluxe PRIVATE USE_RESOURCE_WORKING_DIR=1)

set_target_properties(KoboDeluxe PROPERTIES
  OUTPUT_NAME "Kobo Deluxe"
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Kobo Deluxe/Info.plist.in"
  MACOSX_BUNDLE_BUNDLE_NAME "Kobo Deluxe"
  MACOSX_BUNDLE_GUI_IDENTIFIER "${KOBO_BUNDLE_ID}"
  MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
  MACOSX_BUNDLE_ICON_FILE "Icon.icns"
  MACOSX_BUNDLE_COPYRIGHT "Kobo Deluxe (c) 1995-96 Akira Higuchi, (c) 2001-2007 David Olofson, (c) 2005 Erik Auerswald"
)

set(KOBO_APP_RESOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/Icon.icns
  ${CMAKE_CURRENT_SOURCE_DIR}/Kobo Deluxe/en.lproj/Credits.rtf
  ${CMAKE_CURRENT_SOURCE_DIR}/Kobo Deluxe/en.lproj/InfoPlist.strings
)
target_sources(KoboDeluxe PRIVATE ${KOBO_APP_RESOURCES})
set_source_files_properties(${KOBO_APP_RESOURCES}
  PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

# Compile MainMenu.xib -> MainMenu.nib (matches the old Xcode project behavior)
set(MAINMENU_NIB ${CMAKE_CURRENT_BINARY_DIR}/MainMenu.nib)
add_custom_command(OUTPUT ${MAINMENU_NIB}
  COMMAND xcrun ibtool --compile ${MAINMENU_NIB}
          "${CMAKE_CURRENT_SOURCE_DIR}/Kobo Deluxe/en.lproj/MainMenu.xib"
  DEPENDS "Kobo Deluxe/en.lproj/MainMenu.xib"
  COMMENT "Compiling MainMenu.xib"
  VERBATIM)
add_custom_target(MainMenuNib DEPENDS ${MAINMENU_NIB})
add_dependencies(KoboDeluxe MainMenuNib)

add_custom_command(TARGET KoboDeluxe POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${MAINMENU_NIB}
          $<TARGET_BUNDLE_DIR:KoboDeluxe>/Contents/Resources/MainMenu.nib
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${KOBO_SRC_ROOT}/data
          $<TARGET_BUNDLE_DIR:KoboDeluxe>/Contents/Resources/data
  COMMENT "Copying MainMenu.nib and game data into Kobo Deluxe.app"
  VERBATIM)

# --- kobodl (command line tool) --------------------------------------------
kobo_add_target(kobodl)
target_compile_definitions(kobodl PRIVATE USE_RESOURCE_WORKING_DIR=0)

add_custom_command(TARGET kobodl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${KOBO_SRC_ROOT}/data
          $<TARGET_FILE_DIR:kobodl>/data
  COMMENT "Copying game data next to kobodl"
  VERBATIM)
