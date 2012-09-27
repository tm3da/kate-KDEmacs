project(ktexteditor_kdemacs)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

set(ktexteditor_kdemacs_SRCS kdemacsplugin.cpp)

kde4_add_plugin(ktexteditor_kdemacs ${ktexteditor_kdemacs_SRCS})

target_link_libraries(ktexteditor_kdemacs ${KDE4_KDEUI_LIBS} ${KDE4_KTEXTEDITOR_LIBS})

install(TARGETS ktexteditor_kdemacs DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES kdemacsui.rc DESTINATION ${DATA_INSTALL_DIR}/ktexteditor_kdemacs)

install(FILES ktexteditor_kdemacs.desktop DESTINATION ${SERVICES_INSTALL_DIR})
 
