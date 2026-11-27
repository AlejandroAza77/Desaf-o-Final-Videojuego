cmake_minimum_required(VERSION 3.16)

project(DesafioFinal VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    elementodebatalla.cpp
    elementodebatalla.h
    armas.cpp
    armas.h
    objetodelescenario.cpp
    objetodelescenario.h
    germanquintero.cpp
    germanquintero.h
    agenteinteligente.cpp
    agenteinteligente.h
    enemigorealista.cpp
    enemigorealista.h
    niveldebatalla.cpp
    niveldebatalla.h
    nivelrioteatinos.cpp
    nivelrioteatinos.h
    nivelpuenteboyaca.cpp
    nivelpuenteboyaca.h
    escenadebatalla.cpp
    escenadebatalla.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(DesafioFinal
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(DesafioFinal SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(DesafioFinal
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(DesafioFinal PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)

file(GLOB_RECURSE SONIDOS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "sonidos/*.mp3" "sonidos/*.wav")
file(GLOB_RECURSE SPRITES_PNG CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "sprites/*.png" "sprites/*.jpg")
if(SPRITES_PNG AND ${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # BIG_RESOURCES: los sprites pesan ~42 MB; sin esto rcc genera un .cpp enorme y MinGW se queda sin memoria.
    qt_add_resources(DesafioFinal "sprites" BIG_RESOURCES PREFIX "/" FILES ${SPRITES_PNG} ${SONIDOS})  # imagenes (:/sprites/...) y sonidos (:/sonidos/...)
elseif(NOT SPRITES_PNG)
    message(WARNING "No hay sprites en ${CMAKE_CURRENT_SOURCE_DIR}/sprites: el juego se vera sin imagenes.")
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.DesafioFinal)
endif()
set_target_properties(DesafioFinal PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS DesafioFinal
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(DesafioFinal)
endif()