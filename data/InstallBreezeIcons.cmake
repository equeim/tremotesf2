# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

cmake_policy(SET CMP0011 NEW)
cmake_policy(SET CMP0009 NEW)
cmake_policy(SET CMP0057 NEW)

set(breeze_icons_destination "${CMAKE_INSTALL_PREFIX}/${TREMOTESF_EXTERNAL_RESOURCES_PATH}/icons/breeze")

file(INSTALL "${breeze_extracted_path}/icons/index.theme" DESTINATION "${breeze_icons_destination}")

# Keep in sync with QIcon::fromTheme() calls in source code
set(bundled_icon_files
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
    preferences-system-time.svg
    system-shutdown.svg
    view-refresh.svg
    view-refresh.svg
    view-statistics.svg
    window-close.svg
)

set(files_to_install "")
file(GLOB_RECURSE all_icon_files LIST_DIRECTORIES OFF "${breeze_extracted_path}/icons/**/*.svg")
foreach (icon_path IN LISTS all_icon_files)
    cmake_path(GET icon_path FILENAME icon_filename)
    if (icon_filename IN_LIST bundled_icon_files)
        cmake_path(GET icon_path PARENT_PATH icon_dir)
        cmake_path(RELATIVE_PATH icon_dir BASE_DIRECTORY "${breeze_extracted_path}/icons" OUTPUT_VARIABLE relative_icon_dir)
        set(destination "${breeze_icons_destination}/${relative_icon_dir}")
        if (IS_SYMLINK "${icon_path}")
            file(READ_SYMLINK "${icon_path}" original_icon_filename)
            set(original_icon_path "${icon_dir}/${original_icon_filename}")
            if (WIN32 OR CMAKE_HOST_WIN32)
                # Copy original file with new name
                file(INSTALL "${original_icon_path}" DESTINATION "${destination}" RENAME "${icon_filename}")
            else()
                # Copy original file and symlink
                file(INSTALL "${original_icon_path}" DESTINATION "${destination}")
                file(INSTALL "${icon_path}" DESTINATION "${destination}")
            endif()
        else()
            file(INSTALL "${icon_path}" DESTINATION "${destination}")
        endif()
    endif()
endforeach()

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
                # Copy original directory with new name
                cmake_path(GET size_dir FILENAME size)
                file(INSTALL "${installed_original_size_dir}" DESTINATION "${destination}" RENAME "${size}")
            else()
                # Copy symlink
                file(INSTALL "${size_dir}" DESTINATION "${destination}")
            endif()
        endif()
    endif()
endforeach()
