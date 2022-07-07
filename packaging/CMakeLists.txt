# CPack
include(InstallRequiredSystemLibraries) # what does this do?

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
# Remember WINDOWS PATHS!
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Pleep\\\\Intercession")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-installer")
set(CPACK_PACKAGE_VENDOR "Pleep Inc.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/packaging/License.txt")
# Cannot use ${PROJECT_NAME} here, it won't resolve
set(CPACK_PACKAGE_VERSION_MAJOR ${INTERCESSION_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${INTERCESSION_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${INTERCESSION_VERSION_PATCH})

# NSIS specific
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/packaging/installer.ico")
set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} Version ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
#set(CPACK_NSIS_WELCOME_TITLE "Hey there u little nerd")    # defeault "Welcome to CPACK_PACKAGE_NAME Setup"
set(CPACK_NSIS_MUI_FINISHPAGE_RUN ${PROJECT_NAME})
#set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Desktop shortcut??
# Modify_path shows the gui option, but create_desktop_links acutally creates it
# however if the user disables "create start menu links" there is no link to make the desktop
# create desktop icon + no path + create shortcuts = success
# create desktop icon + add path + create shortcuts = Cannot modify path error
# create desktop icon + no path + no shortcuts = Cannot modify path error
#set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME};${PROJECT_NAME}-link")
#set(CPACK_CREATE_DESKTOP_LINKS "${PROJECT_NAME}")
#set(CPACK_NSIS_MODIFY_PATH ON)  # doesn't create shortcut, and throws PATH too long to append

# Shortcut in install directory and desktop (no user option because of above)
# Only CLIENT will get a desktop shortcut
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    CreateShortCut \\\"$INSTDIR\\\\${SERVER_NAME}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${SERVER_NAME}.exe\\\"
    CreateShortCut \\\"$INSTDIR\\\\${CLIENT_NAME}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${CLIENT_NAME}.exe\\\"
    CreateShortCut \\\"$DESKTOP\\\\${CLIENT_NAME}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${CLIENT_NAME}.exe\\\"
")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    Delete \\\"$INSTDIR\\\\${SERVER_NAME}.lnk\\\"
    Delete \\\"$INSTDIR\\\\${CLIENT_NAME}.lnk\\\"
    Delete \\\"$DESKTOP\\\\${CLIENT_NAME}.lnk\\\"
")

# These didn't seem to be used by NSIS
#set(CPACK_PACKAGE_VENDOR "Pleep inc.")
#set(CPACK_PACKAGE_DESCRIPTION "This is the description")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "This is the summary")
#set(CPACK_PACKAGE_HOMEPAGE_URL "www.pleep.com/game")
#set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\pleep.ico")
#set(CPACK_RESOURCE_FILE_README  "${CMAKE_SOURCE_DIR}/README.md")
#set(CPACK_NSIS_URL_INFO_ABOUT "www.pleep.com/game")
#set(CPACK_NSIS_HELP_LINK "www.pleep.com/faq")
#set(CPACK_NSIS_CONTACT "www.pleep.com/contact")
#set(CPACK_NSIS_MENU_LINKS "www.pleep.com" "Homepage for ${CPACK_PACKAGE_NAME}")
#set(CPACK_NSIS_DISPLAY_NAME "Pleep game name")

include(CPack)
