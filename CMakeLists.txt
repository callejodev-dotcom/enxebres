cmake_minimum_required(VERSION 3.22)

# En Mac: binario universal (Apple Silicon + Intel), desde macOS 11. En Linux/Windows non fai nada.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo")
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas de Mac")
endif()

project(BarulloEnxebres VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: cartafol irmán ../JUCE, ou o que se indique con -DJUCE_DIR=...
if(NOT JUCE_DIR)
  set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
endif()
get_filename_component(JUCE_DIR "${JUCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
  add_subdirectory("${JUCE_DIR}" JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

# Núcleo común de Escoita: a versión fixada ou, se existe, a carpeta irmá ../escoita-nucleo
# (para traballar no núcleo e probalo nos plugins á vez); tamén -DESCOITA_NUCLEO=<carpeta>
set(ESCOITA_NUCLEO_VERSION "v0.4.0")
if(NOT ESCOITA_NUCLEO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../escoita-nucleo/escoita_nucleo")
  set(ESCOITA_NUCLEO "${CMAKE_CURRENT_SOURCE_DIR}/../escoita-nucleo")
endif()
if(ESCOITA_NUCLEO)
  add_subdirectory("${ESCOITA_NUCLEO}" escoita-nucleo)
else()
  include(FetchContent)
  FetchContent_Declare(escoita_nucleo GIT_REPOSITORY https://github.com/callejodev-dotcom/escoita-nucleo.git
                       GIT_TAG ${ESCOITA_NUCLEO_VERSION} GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(escoita_nucleo)
endif()

# Os motores doutros plugins, nunha versión fixada (non se copian): o filtro e o chorus do
# Juno-106 de Barullo Xuno e a reverb de Barullo Verb. Se están ao carón (../barullo-xuno e
# ../barulloverb) úsanse eses; se non, descárganse. Tamén -DENXEBRES_MOTORES=<cartafol cos dous>.
set(ENXEBRES_XUNO_VERSION "v0.1.0")
set(ENXEBRES_VERB_VERSION "v0.1.0")
get_filename_component(ENXEBRES_IRMANS "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
if(NOT ENXEBRES_MOTORES AND EXISTS "${ENXEBRES_IRMANS}/barullo-xuno/Source/xuno/Board.h" AND EXISTS "${ENXEBRES_IRMANS}/barulloverb/Source/Espazo.h")
  set(ENXEBRES_MOTORES "${ENXEBRES_IRMANS}")
endif()
if(NOT ENXEBRES_MOTORES)
  set(ENXEBRES_MOTORES "${CMAKE_BINARY_DIR}/motores")
  include(FetchContent)
  # SOURCE_SUBDIR sen CMakeLists: só se descargan as fontes (non se compilan os plugins)
  FetchContent_Declare(barullo_xuno GIT_REPOSITORY https://github.com/callejodev-dotcom/barullo-xuno.git
                       GIT_TAG ${ENXEBRES_XUNO_VERSION} GIT_SHALLOW TRUE SOURCE_DIR "${ENXEBRES_MOTORES}/barullo-xuno" SOURCE_SUBDIR sen-cmake)
  FetchContent_Declare(barulloverb GIT_REPOSITORY https://github.com/callejodev-dotcom/barulloverb.git
                       GIT_TAG ${ENXEBRES_VERB_VERSION} GIT_SHALLOW TRUE SOURCE_DIR "${ENXEBRES_MOTORES}/barulloverb" SOURCE_SUBDIR sen-cmake)
  FetchContent_MakeAvailable(barullo_xuno barulloverb)
endif()
message(STATUS "Barullo Enxebres: motores en ${ENXEBRES_MOTORES}")

# Formatos por sistema (sen aplicación independente: son efectos para o DAW)
if(APPLE)
  set(FORMATS AU VST3)
elseif(WIN32)
  set(FORMATS VST3)
else()
  set(FORMATS VST3 LV2)
endif()

option(BARULLO_CLAP "Compilar tamén o formato CLAP" ON)
if(BARULLO_CLAP)
  include(FetchContent)
  FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d   # main (2026): a 0.26.0 aínda non vai con JUCE 8
    GIT_SUBMODULES_RECURSE TRUE)
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)   # FetchContent_Populate + add_subdirectory(... EXCLUDE_FROM_ALL) ata CMake 3.28
  endif()
  FetchContent_GetProperties(clap-juce-extensions)
  if(NOT clap-juce-extensions_POPULATED)
    FetchContent_Populate(clap-juce-extensions)
    add_subdirectory(${clap-juce-extensions_SOURCE_DIR} ${clap-juce-extensions_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
endif()

# tipografías Montserrat e Inter, as da familia (licenza OFL, ver Resources/Montserrat-OFL.txt e Inter-OFL.txt)
juce_add_binary_data(BEFonts SOURCES
  Resources/Inter-Medium.ttf
  Resources/Montserrat-SemiBold.ttf
  Resources/Montserrat-Bold.ttf)

# O código propio: o mesmo en todos os plugins e na proba (cada plugin escolle a súa ferramenta)
set(ENXEBRES_SOURCES
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

set(ENXEBRES_DEFS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)

# Un plugin: número (Ferramentas.h), destino, nome, código, categoría VST3 e característica CLAP.
# Os nomes, sen tiles: JUCE 8 escríbeos mal no VST3 (o DAW non o carga) e o codesign de Apple
# rompe con eles nos ficheiros.
function(enxebre NUMERO DESTINO NOME CODIGO VST3CAT CLAPCAT)
  string(TOLOWER "${DESTINO}" ID)
  juce_add_plugin(${DESTINO}
    COMPANY_NAME "Escoita"
    COMPANY_WEBSITE "https://www.escoita.link"
    COMPANY_EMAIL "ola@escoita.link"
    BUNDLE_ID "link.escoita.${ID}"
    PLUGIN_MANUFACTURER_CODE Esco
    PLUGIN_CODE ${CODIGO}
    PRODUCT_NAME "${NOME}"
    DESCRIPTION "Barullo Enxebres: ${NOME}"
    FORMATS ${FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx ${VST3CAT}
    AU_MAIN_TYPE kAudioUnitType_Effect
    LV2URI "https://www.escoita.link/enxebres/${ID}"
    LV2_SHARED_LIBRARY_NAME "${DESTINO}"
    COPY_PLUGIN_AFTER_BUILD FALSE)
  if(BARULLO_CLAP)
    clap_juce_extensions_plugin(TARGET ${DESTINO}
      CLAP_ID "link.escoita.${ID}"
      CLAP_FEATURES audio-effect ${CLAPCAT} stereo
      CLAP_MANUAL_URL "https://www.escoita.link"
      CLAP_SUPPORT_URL "https://www.escoita.link")
  endif()
  target_sources(${DESTINO} PRIVATE ${ENXEBRES_SOURCES})
  target_include_directories(${DESTINO} PRIVATE Source "${ENXEBRES_MOTORES}")
  if(MSVC)
    target_compile_options(${DESTINO} PRIVATE /utf-8 /bigobj)
  endif()
  target_compile_definitions(${DESTINO} PUBLIC ${ENXEBRES_DEFS} ENXEBRE_FERRAMENTA=${NUMERO})
  target_link_libraries(${DESTINO}
    PRIVATE BEFonts juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
  escoita_produto(${DESTINO} NOME "${NOME}" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/enxebres)
endfunction()

enxebre(0 BarulloAmbiente  "Barullo Ambiente"  Bamb Reverb     reverb)
enxebre(1 BarulloSaturador "Barullo Saturador" Bsat Distortion distortion)
enxebre(2 BarulloBrillo    "Barullo Brillo"    Bbri EQ         equalizer)
enxebre(3 BarulloGraves    "Barullo Graves"    Bgra EQ         equalizer)
enxebre(4 BarulloVolume    "Barullo Volume"    Bvol Dynamics   limiter)
enxebre(5 BarulloCompresor "Barullo Compresor" Bcom Dynamics   compressor)
enxebre(6 BarulloSidechain "Barullo Sidechain" Bsdc Dynamics   tremolo)
enxebre(7 BarulloFiltro    "Barullo Filtro"    Bfil Filter     filter)
enxebre(8 BarulloChorus    "Barullo Chorus"    Bcho Modulation chorus)

# Avisos como erros no código propio (non no de JUCE)
option(BARULLO_WERROR "Avisos como erros en Source/ e tools/" OFF)
set(BARULLO_OWN_SOURCES ${ENXEBRES_SOURCES} tools/RenderTest.cpp)
if(BARULLO_WERROR)
  if(MSVC)
    set_source_files_properties(${BARULLO_OWN_SOURCES} PROPERTIES COMPILE_OPTIONS "/WX")
  else()
    set_source_files_properties(${BARULLO_OWN_SOURCES} PROPERTIES COMPILE_OPTIONS "-Werror")
  endif()
endif()

option(BARULLO_SANITIZE "Compilar a proba con AddressSanitizer e UBSan" OFF)

# Proba sen interface: as nove ferramentas; garda un WAV
option(BARULLO_TESTS "Compilar a proba de render" OFF)
if(BARULLO_TESTS)
  juce_add_console_app(BERenderTest PRODUCT_NAME "BERenderTest")
  target_sources(BERenderTest PRIVATE tools/RenderTest.cpp ${ENXEBRES_SOURCES})
  target_include_directories(BERenderTest PRIVATE Source "${ENXEBRES_MOTORES}")
  if(MSVC)
    target_compile_options(BERenderTest PRIVATE /utf-8 /bigobj)
    target_link_options(BERenderTest PRIVATE /STACK:16777216)
  endif()
  target_compile_definitions(BERenderTest PRIVATE ${ENXEBRES_DEFS} BARULLO_NO_PLUGIN_ENTRY=1)
  target_link_libraries(BERenderTest PRIVATE BEFonts juce::juce_audio_utils juce::juce_audio_processors juce::juce_audio_formats juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
  escoita_produto(BERenderTest NOME "Barullo Enxebres" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/enxebres PROBA)
  if(BARULLO_SANITIZE AND NOT MSVC)
    target_compile_options(BERenderTest PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
    target_link_options(BERenderTest PRIVATE -fsanitize=address,undefined)
  endif()
endif()
