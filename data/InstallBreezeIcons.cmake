# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

cmake_policy(SET CMP0011 NEW)
cmake_policy(SET CMP0009 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0177 NEW)

set(breeze_icons_destination "${CMAKE_INSTALL_PREFIX}/${TREMOTESF_EXTERNAL_RESOURCES_PATH}/icons/breeze")

file(INSTALL "${breeze_extracted_path}/icons/breeze.theme.in" DESTINATION "${breeze_icons_destination}")
file(RENAME "${breeze_icons_destination}/breeze.theme.in" "${breeze_icons_destination}/index.theme")
file(READ "${breeze_extracted_path}/icons/commonthemeinfo.theme.in" commonthemeinfo)
file(APPEND "${breeze_icons_destination}/index.theme" "${commonthemeinfo}")

# Keep in sync with QIcon::fromTheme() calls in source code
set(icon_files_to_bundle
    application-exit.svg
    applications-utilities.svg
    configure.svg
    dialog-cancel.svg
    dialog-error.svg
    document-open.svg
    document-preview.svg
    document-properties.svg
    download.svg
    edit-copy.svg
    edit-delete.svg
    edit-delete.svg
    edit-rename.svg
    edit-select-all.svg
    edit-select-invert.svg
    folder-download.svg
    go-bottom.svg
    go-down.svg
    go-jump.svg
    go-top.svg
    go-up.svg
    help-about.svg
    insert-link.svg
    list-add.svg
    list-remove.svg
    mark-location.svg
    media-playback-pause.svg
    media-playback-start.svg
    network-connect.svg
    network-disconnect.svg
    network-server.svg
    network-server.svg
    preferences-system.svg
    preferences-system-network.svg
    preferences-desktop.svg
    preferences-desktop-notification.svg
    preferences-system-time.svg
    system-shutdown.svg
    tag.svg
    view-refresh.svg
    view-refresh.svg
    view-statistics.svg
    window-close.svg

    audio-x-generic.svg
    image-x-generic.svg
    package-x-generic.svg
    text-x-generic.svg
    video-x-generic.svg
    application-epub+zip.svg
    application-pdf.svg
    application-vnd.efi.iso.svg
    application-x-cue.svg
    application-x-executable.svg
    application-x-fictionbook+xml.svg
    application-x-msdownload.svg
    application-x-subrip.svg
    image-vnd.djvu.svg
    text-x-script.svg
    text-x-ssa.svg
    x-office-document.svg
    x-office-presentation.svg
    x-office-spreadsheet.svg
)

# Replacement for file(REAL_PATH) that doesn't work on Windows
function(resolve_symlink_recursively path output_variable)
    while (IS_SYMLINK "${path}")
        message("Resolving symlink ${path}")
        cmake_path(GET path PARENT_PATH parent)
        file(READ_SYMLINK "${path}" target)
        cmake_path(ABSOLUTE_PATH target BASE_DIRECTORY "${parent}" OUTPUT_VARIABLE path)
        message("Resolved to ${path}")
    endwhile()
    set("${output_variable}" "${path}" PARENT_SCOPE)
endfunction()

set(files_to_install "")
file(GLOB_RECURSE all_icon_files LIST_DIRECTORIES OFF "${breeze_extracted_path}/icons/**/*.svg")
foreach (icon_path IN LISTS all_icon_files)
    cmake_path(GET icon_path FILENAME icon_filename)
    if (icon_filename IN_LIST icon_files_to_bundle)
        cmake_path(GET icon_path PARENT_PATH icon_dir)
        cmake_path(RELATIVE_PATH icon_dir BASE_DIRECTORY "${breeze_extracted_path}/icons" OUTPUT_VARIABLE relative_icon_dir)
        set(destination "${breeze_icons_destination}/${relative_icon_dir}")
        if (IS_SYMLINK "${icon_path}")
            if (WIN32 OR CMAKE_HOST_WIN32)
                # Copy original file with linked name
                resolve_symlink_recursively("${icon_path}" original_icon_path)
                file(INSTALL "${original_icon_path}" DESTINATION "${destination}" RENAME "${icon_filename}")
            else()
                # Copy the whole symlink chain
                file(INSTALL "${icon_path}" DESTINATION "${destination}" FOLLOW_SYMLINK_CHAIN)
            endif()
        else()
            file(INSTALL "${icon_path}" DESTINATION "${destination}")
        endif()
        list(REMOVE_ITEM icon_files_to_bundle "${icon_filename}")
        if (NOT icon_files_to_bundle)
            break()
        endif()
    endif()
endforeach()

if (icon_files_to_bundle)
    message(FATAL_ERROR "Did not find icons: ${icon_files_to_bundle}")
endif()

file(GLOB size_dirs LIST_DIRECTORIES ON "${breeze_extracted_path}/icons/*/*")
foreach (size_dir IN LISTS size_dirs)
    if (IS_SYMLINK "${size_dir}")
        cmake_path(GET size_dir PARENT_PATH category_dir)
        cmake_path(GET category_dir FILENAME category)
        set(destination "${breeze_icons_destination}/${category}")
        file(READ_SYMLINK "${size_dir}" original_size)
        set(installed_original_size_dir "${destination}/${original_size}")
        if (EXISTS "${installed_original_size_dir}")
            if (WIN32 OR CMAKE_HOST_WIN32)
                # Copy original directory with linked name
                cmake_path(GET size_dir FILENAME size)
                file(INSTALL "${installed_original_size_dir}" DESTINATION "${destination}" RENAME "${size}")
            else()
                # Copy symlink
                file(INSTALL "${size_dir}" DESTINATION "${destination}")
            endif()
        endif()
    endif()
endforeach()
