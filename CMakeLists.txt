cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
#------------------------------------------------------------------------------#
#
#   JS8Call Build:
#
#     1) mkdir build
#     2) cd build
#     3) cmake ..
#     4) make
#
#     For subsequent builds, just skip directly to step 4.
#
#------------------------------------------------------------------------------#

#------------------------------------------------------------------------------#
# Policies; add to this list as required:
#
#  - CMP0167: Require a Boost 1.70+ installation, with cmake support.
#  - CMP0177: DESTINATION paths are normalized.
#------------------------------------------------------------------------------#

foreach(policy CMP0167 CMP0177)
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

#------------------------------------------------------------------------------#
# Configuration defaults that we allow to be overridden on the cmake
# command line.
#------------------------------------------------------------------------------#

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/../js8libs)
endif()

#------------------------------------------------------------------------------#
# Configuration defaults that we don't allow to be overridden.
#------------------------------------------------------------------------------#

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)

#------------------------------------------------------------------------------#
# Project
#------------------------------------------------------------------------------#

project(
  JS8Call
  VERSION   2.5.0
  LANGUAGES C CXX
)

set(PROJECT_VENDOR "Jordan Sherer, KN4CRD")
set(PROJECT_CONTACT "Jordan Sherer <kn4crd@gmail.com>")
set(PROJECT_COPYRIGHT "Copyright (C) 2001-2018 by Joe Taylor, K1JT, (C) 2018 by Jordan Sherer, KN4CRD, (C) of contributed work is owned by individual contributors")
set(PROJECT_HOMEPAGE https://groups.io/g/js8call)
set(PROJECT_SUMMARY_DESCRIPTION "${PROJECT_NAME} - Digital Modes for Weak Signal Communications in Amateur Radio.")
set(PROJECT_DESCRIPTION "${PROJECT_SUMMARY_DESCRIPTION}
 ${PROJECT_NAME} is a computer program designed to facilitate amateur
 radio communication using very weak signals.")

#------------------------------------------------------------------------------#
# Add our own CMake modules.
#------------------------------------------------------------------------------#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Modules")

#------------------------------------------------------------------------------#
# CMake Script Includes
#------------------------------------------------------------------------------#

include(CheckTypeSize)
include(CheckSymbolExists)
include(CMakeDependentOption)
include(GenerateExportHeader)
include(GNUInstallDirs)
include(VersionCompute)

#------------------------------------------------------------------------------#
# Options and features. Some of these directly affect compilation by being
# define in wsjtx_config.h.in, which makes them available to the C/C++
# preprocessor.
#------------------------------------------------------------------------------#

option(WSJT_QDEBUG_TO_FILE     "Redirect Qt debugging messages to a trace file.")
option(WSJT_HAMLIB_TRACE       "Debugging option that turns on minimal Hamlib internal diagnostics.")
option(WSJT_RIG_NONE_CAN_SPLIT "Allow split operation with \"None\" as rig.")

cmake_dependent_option(
  WSJT_HAMLIB_VERBOSE_TRACE
  "Debugging option that turns on full Hamlib internal diagnostics."
  OFF
  WSJT_HAMLIB_TRACE
  OFF)

cmake_dependent_option(
  WSJT_QDEBUG_IN_RELEASE
  "Leave Qt debugging statements in Release configuration."
  OFF
  "NOT (CMAKE_BUILD_TYPE STREQUAL Debug)"
  OFF)

cmake_dependent_option(
  WSJT_ENABLE_EXPERIMENTAL_FEATURES
  "Enable features not fully ready for public releases."
  ON
  "CMAKE_BUILD_TYPE STREQUAL Debug"
  OFF)

#------------------------------------------------------------------------------#
# Ensure we have required library dependencies; we'll need headers from the
# Boost and FFTW3 libraries.
#------------------------------------------------------------------------------#

find_package(Boost 1.77 REQUIRED)
find_package(FFTW3      REQUIRED COMPONENTS single threads)
find_package(Hamlib     REQUIRED)
find_package(Qt6    6.5 REQUIRED COMPONENTS Multimedia Network SerialPort Widgets)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${FFTW3_INCLUDE_DIRS})

#------------------------------------------------------------------------------#
# OSX requires an icon file resource.
#------------------------------------------------------------------------------#

if (APPLE)
  set(ICON_FILE ${CMAKE_PROJECT_NAME}.icns)
  set(ICON_SRCS
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_16x16.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_16x16@2x.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_32x32.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_32x32@2x.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_128x128.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_128x128@2x.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_256x256.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_256x256@2x.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_512x512.png
    icons/Darwin/${CMAKE_PROJECT_NAME}.iconset/icon_512x512@2x.png
  )
  add_custom_command(
    OUTPUT ${ICON_FILE}
    COMMAND iconutil -c icns --output "${CMAKE_BINARY_DIR}/${ICON_FILE}" "${CMAKE_SOURCE_DIR}/icons/Darwin/${CMAKE_PROJECT_NAME}.iconset"
    DEPENDS ${ICON_SRCS}
    COMMENT "Building Icons"
  )
  set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif (APPLE)

#------------------------------------------------------------------------------#
# Create a target that's the same as our project name; that'll be our
# created executable.
#------------------------------------------------------------------------------#

set(TARGET ${PROJECT_NAME})

qt_add_executable(${TARGET} MACOSX_BUNDLE WIN32 ${ICON_FILE})

#------------------------------------------------------------------------------#
# OSX requires a number of settings in the plist file.
#------------------------------------------------------------------------------#

set_target_properties(
  ${TARGET} PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST             "${CMAKE_CURRENT_SOURCE_DIR}/Darwin/Info.plist.in"
  MACOSX_BUNDLE_INFO_STRING            "JS8 mode providing weak signal communications for Amateur Radio"
  MACOSX_BUNDLE_ICON_FILE              "${ICON_FILE}"
  MACOSX_BUNDLE_BUNDLE_VERSION         "${wsjtx_VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING   "${wsjtx_SHORT_VERSION}"
  MACOSX_BUNDLE_BUNDLE_NAME            "${PROJECT_NAME}"
  MACOSX_BUNDLE_BUNDLE_EXECUTABLE_NAME "${PROJECT_NAME}"
  MACOSX_BUNDLE_COPYRIGHT              "${PROJECT_COPYRIGHT}"
  MACOSX_BUNDLE_GUI_IDENTIFIER          "org.kn4crd.js8call"
)

#------------------------------------------------------------------------------#
# Set CMAKE_BUILD to 1 globally for use by things that need to know
# they're being compiled under cmake, then preprocess and precompile
# the configuration header.
#------------------------------------------------------------------------------#

target_compile_definitions(${TARGET} PRIVATE CMAKE_BUILD)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/JS8_Include/wsjtx_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/wsjtx_config.h"
)

target_precompile_headers(
  ${TARGET} PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/wsjtx_config.h
)

#------------------------------------------------------------------------------#
# Source files common to all platforms.
#------------------------------------------------------------------------------#

target_sources(
  ${TARGET} PRIVATE
  vendor/sqlite3/sqlite3.c
  JS8_Audio/AudioDevice.cpp
  JS8_Audio/BWFFile.cpp
  JS8_Audio/NotificationAudio.cpp
  JS8_Audio/soundin.cpp
  JS8_Audio/soundout.cpp
  JS8_Deprecated/DisplayManual.cpp
  JS8_jsc/jsc.cpp
  JS8_jsc/jsc_checker.cpp
  JS8_jsc/jsc_list.cpp
  JS8_jsc/jsc_map.cpp
  JS8_Logbook/adif.cpp
  JS8_Logbook/countrydat.cpp
  JS8_Logbook/countriesworked.cpp
  JS8_Logbook/logbook.cpp
  JS8_Main/APRSISClient.cpp
  JS8_Main/AttenuationSlider.cpp
  JS8_Main/Bands.cpp
  JS8_Main/CallsignValidator.cpp
  JS8_Main/CandidateKeyFilter.cpp
  JS8_Main/DriftingDateTime.cpp
  JS8_Main/Flatten.cpp
  JS8_Main/ForeignKeyDelegate.cpp
  JS8_Main/FrequencyLineEdit.cpp
  JS8_Main/FrequencyList.cpp
  JS8_Main/Geodesic.cpp
  JS8_Main/HelpTextWindow.cpp
  JS8_Main/IARURegions.cpp
  JS8_Main/Inbox.cpp
  JS8_Main/JS8MessageBox.cpp
  JS8_Main/Message.cpp
  JS8_Main/MessageClient.cpp
  JS8_Main/MessageError.cpp
  JS8_Main/MessageServer.cpp
  JS8_Main/MetaDataRegistry.cpp
  JS8_Main/MultiSettings.cpp
  JS8_Main/RDP.cpp
  JS8_Main/StationList.cpp
  JS8_Main/TxLoop.cpp
  JS8_Main/WF.cpp
  JS8_Main/fileutils.cpp
  JS8_Main/main.cpp
  JS8_Main/Modes.cpp
  JS8_Main/plotter.cpp
  JS8_Main/ProcessThread.cpp
  JS8_Main/qDateTimeExperiment.cpp
  JS8_Main/qt_helpers.cpp
  JS8_Main/Radio.cpp
  JS8_Main/RadioMetaType.cpp
  JS8_Main/revision_utils.cpp
  JS8_Main/SelfDestructMessageBox.cpp
  JS8_Main/SignalMeter.cpp
  JS8_Main/TraceFile.cpp
  JS8_Main/TransmitTextEdit.cpp
  JS8_Main/TwoPhaseSignal.cpp
  JS8_Main/varicode.cpp
  JS8_Mainwindow/buildQueryMenu.cpp
  JS8_Mainwindow/checkVersion.cpp
  JS8_Mainwindow/dataSink.cpp
  JS8_Mainwindow/displayBandActivity.cpp
  JS8_Mainwindow/displayCallActivity.cpp
  JS8_Mainwindow/initializeDummyData.cpp
  JS8_Mainwindow/initializeGroupMessage.cpp
  JS8_Mainwindow/networkMessage.cpp
  JS8_Mainwindow/processCommandActivity.cpp
  JS8_Mainwindow/processDecodeEvent.cpp
  JS8_Mainwindow/processRxActivity.cpp
  JS8_Mode/Detector.cpp
  JS8_Mode/JS8.cpp
  JS8_Mode/JS8Submode.cpp
  JS8_Mode/Modulator.cpp
  JS8_Mode/decodedtext.cpp
  JS8_Mode/kalman.cpp
  JS8_Network/NetworkServerLookup.cpp
  JS8_Network/PSKReporter.cpp
  JS8_Network/SpotClient.cpp
  JS8_Network/TCPClient.cpp
  JS8_Transceiver/DXLabSuiteCommanderTransceiver.cpp
  JS8_Transceiver/EmulateSplitTransceiver.cpp
  JS8_Transceiver/HamlibTransceiver.cpp
  JS8_Transceiver/HRDTransceiver.cpp
  JS8_Transceiver/PollingTransceiver.cpp
  JS8_Transceiver/Transceiver.cpp
  JS8_Transceiver/TransceiverBase.cpp
  JS8_Transceiver/TransceiverFactory.cpp
  JS8_UDP/NetworkMessage.cpp
  JS8_UDP/WSJTXMessageClient.cpp
  JS8_UDP/WSJTXMessageMapper.cpp
  JS8_UI/aprsinboundrelay.cpp
  JS8_UI/Configuration.cpp
  JS8_UI/about.cpp
  JS8_UI/logqso.cpp
  JS8_UI/mainwindow.cpp
  JS8_UI/messagereplydialog.cpp
  JS8_UI/messagewindow.cpp
  JS8_UI/widegraph.cpp
  JS8_Widgets/CheckableItemComboBox.cpp
  JS8_Widgets/LazyFillComboBox.cpp
)

#------------------------------------------------------------------------------#
# Link libraries common to all platforms.
#------------------------------------------------------------------------------#

target_link_libraries(
  ${TARGET} PRIVATE
  ${FFTW3_LIBRARIES}
  Hamlib::Hamlib
  Qt::Multimedia
  Qt::Network
  Qt::SerialPort
  Qt::Widgets
)

#------------------------------------------------------------------------------#
# Resources for country data and eclipse dates, used by the log book and the
# PSK reporter, respectively.
#------------------------------------------------------------------------------#

qt_add_resources(
  ${TARGET} "data"
  FILES
  JS8_Include/cty.dat
  JS8_Include/eclipse.txt
)

#------------------------------------------------------------------------------#
# Up and down arrow image resources, used in the main window by the
# frequency controls.
#------------------------------------------------------------------------------#

qt_add_resources(
  ${TARGET} "images"
  PREFIX   "/images"
  FILES
  artwork/up.png
  artwork/down.png
)

#------------------------------------------------------------------------------#
# Waterfall palette resources.
#------------------------------------------------------------------------------#

qt_add_resources(
  ${TARGET} "palettes"
  BASE      "Palettes"
  PREFIX   "/Palettes"
  FILES
  Palettes/Banana.pal
  Palettes/Blue1.pal
  Palettes/Blue2.pal
  Palettes/Blue3.pal
  Palettes/Brown.pal
  Palettes/Cyan1.pal
  Palettes/Cyan2.pal
  Palettes/Cyan3.pal
  Palettes/Default.pal
  Palettes/Digipan.pal
  Palettes/Fldigi.pal
  Palettes/Gray1.pal
  Palettes/Gray2.pal
  Palettes/Green1.pal
  Palettes/Green2.pal
  Palettes/Jungle.pal
  Palettes/Linrad.pal
  Palettes/Negative.pal
  Palettes/Orange.pal
  Palettes/Pink.pal
  Palettes/Rainbow.pal
  Palettes/Scope.pal
  Palettes/Sunburst.pal
  Palettes/VK4BDJ.pal
  Palettes/YL2KF.pal
  Palettes/Yellow1.pal
  Palettes/Yellow2.pal
  Palettes/ZL1FZ.pal
)

#------------------------------------------------------------------------------#
# Compiler setup. OSX will by default choose the correct compiler flags;
# on other platforms we'll probably need to expand on this by platform.
#------------------------------------------------------------------------------#

set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wextra")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fexceptions -frtti")

#------------------------------------------------------------------------------#
# In order to execute the macdeployqt and windeployqt commands, we'll need
# to know where they live, which is the same location as the qmake binary.
#------------------------------------------------------------------------------#

get_target_property(QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)

#------------------------------------------------------------------------------#
# On OSX, all we have to do at this point is deploy in overwrite mode to
# have all the needed library dependencies injected into the target.
#------------------------------------------------------------------------------#

if (APPLE)
  find_program(MACDEPLOYQT macdeployqt HINTS "${QT_BIN_DIR}")
  add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND "${MACDEPLOYQT}"
      "$<TARGET_FILE_DIR:${TARGET}>/../.."
      -always-overwrite
    COMMENT "Running macdeployqt..."
  )
endif (APPLE)

#------------------------------------------------------------------------------#
# On Windows, we want to conditionally enable OmniRig, as frankly, it's at
# present got a foot in the grave and another on a banana peel, we we must
# be prepared to eliminate the integration if nothing changes for the better.
#------------------------------------------------------------------------------#

if (WIN32)

  option(ENABLE_OMNIRIG "Enable OmniRig COM integration" OFF)

  if (ENABLE_OMNIRIG)
    include(QtAxMacros)
    find_package(Qt6 REQUIRED COMPONENTS AxContainer AxServer)
    find_program(DUMPCPP dumpcpp HINTS "${QT_BIN_DIR}")
    execute_process(
      COMMAND "${DUMPCPP}" -getfile {4FE359C5-A58F-459D-BE95-CA559FB4F270}
      OUTPUT_VARIABLE AXSERVER
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    string(STRIP "${AXSERVER}" AXSERVER)
    if (AXSERVER)
      string(REPLACE "\"" "" AXSERVER "${AXSERVER}")
      file(TO_CMAKE_PATH "${AXSERVER}" AXSERVERSRCS)

      wrap_ax_server(GENAXSRCS "${AXSERVERSRCS}")

      target_compile_definitions(${TARGET}
        PRIVATE ENABLE_OMNIRIG=$<BOOL:${ENABLE_OMNIRIG}>
      )

      target_sources(${TARGET} PRIVATE
        JS8_Transceiver/OmniRigTransceiver.cpp
        ${GENAXSRCS}
      )

      target_link_libraries(${TARGET} PRIVATE
        Qt::AxContainer
        Qt::AxServer
      )
    else()
      message(STATUS "OmniRig not installed; skipping OmniRig integration.")
    endif()
  endif()

  # Windows icons require an RC file.

  target_sources(${TARGET} PRIVATE JS8_Include/wsjtx.rc)

  # Find the windeployqt command and run it against the generated binary,
  # which should handle all the gnarly bits.

  find_program(WINDEPLOYQT NAMES windeployqt HINTS "${QT_BIN_DIR}")
  add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND "${WINDEPLOYQT}" "$<TARGET_FILE_DIR:${TARGET}>"
    --libdir JS8Call
    --plugindir JS8Call
    --no-translations
    COMMENT "Running windeployqt..."
  )
endif (WIN32)

#------------------------------------------------------------------------------#
