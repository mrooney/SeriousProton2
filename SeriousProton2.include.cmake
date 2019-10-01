cmake_minimum_required(VERSION 3.6.0)

set(SERIOUS_PROTON2_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})

if(WIN32)
    set(CPACK_GENERATOR NSIS ZIP)
    find_package(Git)
    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} describe --always OUTPUT_VARIABLE GIT_VERSION WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_STRIP_TRAILING_WHITESPACE)
        set(CPACK_PACKAGE_VERSION "${GIT_VERSION}")
    endif()
    include(CPack)
endif()

if(CMAKE_HOST_WIN32)
    file(GLOB IMAGE_CONVERT_SEARCH_PATHS "C:/Program Files/ImageMagick-*")
    find_program(IMAGE_CONVERT magick PATHS ${IMAGE_CONVERT_SEARCH_PATHS})
else()
    find_program(IMAGE_CONVERT convert)
endif()

macro(serious_proton2_executable EXECUTABLE_NAME)
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH})
    if (EMSCRIPTEN)
        set(EMSCRIPTEN_FLAGS "-s USE_SDL=2 -s DISABLE_EXCEPTION_CATCHING=0 -s DEMANGLE_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
        #set(CMAKE_EXECUTABLE_SUFFIX ".html")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/resources@resources")
    else()
        find_package(SDL2 REQUIRED)
        if(NOT DEFINED SDL2_LIBRARIES)
            set(SDL2_LIBRARIES SDL2::SDL2)
        endif()
    endif()

    file(GLOB_RECURSE SP2_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/src/*.cpp" "${SERIOUS_PROTON2_BASE_DIR}/include/*.h")
    file(GLOB_RECURSE LUA_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/extlibs/lua/*.c" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/lua/*.h")
    file(GLOB_RECURSE JSON11_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/extlibs/json11/*.cpp" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/json11/*.hpp")
    file(GLOB_RECURSE MINIZ_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/extlibs/miniz/*.c" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/miniz/*.h")
    file(GLOB_RECURSE BOX2D_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/extlibs/Box2D/*.cpp" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/Box2D/*.h")
    file(GLOB_RECURSE BULLET_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/extlibs/bullet/*.cpp" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/bullet/*.h")
    file(GLOB_RECURSE FREETYPE_SOURCES "${SERIOUS_PROTON2_BASE_DIR}/extlibs/freetype-2.9/src/*.c" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/freetype-2.9/src/*.h")

    if(NOT WIN32)
        list(FILTER SP2_SOURCES EXCLUDE REGEX .*/win32/.*)
    endif()
    if(NOT UNIX)
        list(FILTER SP2_SOURCES EXCLUDE REGEX .*/unix/.*)
    endif()

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    endif()

    # Set our optimization flags.
    set(OPTIMIZER_FLAGS "")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        # On gcc, we want some general optimalizations that improve speed a lot.
        set(OPTIMIZER_FLAGS "${OPTIMIZER_FLAGS} -O3 -ffast-math")

        # If we are compiling for a rasberry pi, we want to aggressively optimize for the CPU we are running on.
        # Note that this check only works if we are compiling directly on the pi, as it is a dirty way of checkif if we are on the pi.
        if(EXISTS /opt/vc/include/bcm_host.h OR COMPILE_FOR_PI)
            set(OPTIMIZER_FLAGS "${OPTIMIZER_FLAGS} -mcpu=native -mfpu=neon-vfpv4 -mfloat-abi=hard -DRASBERRY_PI=1 -DNO_ASSERT=1")
        endif()
    elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(OPTIMIZER_FLAGS "${OPTIMIZER_FLAGS} -O3 -ffast-math")
    endif()

    set(WARNING_FLAGS -Wall)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(WARNING_FLAGS ${WARNING_FLAGS} -Wno-psabi -Wno-strict-aliasing -Werror=suggest-override)
    endif()

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OPTIMIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OPTIMIZER_FLAGS}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g1 ${OPTIMIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g1 ${OPTIMIZER_FLAGS}")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG=1")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1")

    set(CMAKE_CXX_STANDARD 11)

    add_library(box2d STATIC ${BOX2D_SOURCES})
    target_include_directories(box2d PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/extlibs")
    add_library(bullet STATIC ${BULLET_SOURCES})
    target_include_directories(bullet PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/extlibs/bullet")
    add_library(lua STATIC ${LUA_SOURCES})
    target_include_directories(lua PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/extlibs")
    add_library(json11 STATIC ${JSON11_SOURCES})
    target_include_directories(lua PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/extlibs")
    add_library(miniz STATIC ${MINIZ_SOURCES})
    target_include_directories(miniz PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/extlibs/miniz")
    add_library(sp2freetype STATIC ${FREETYPE_SOURCES})
    target_include_directories(sp2freetype PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/extlibs/freetype-2.9/include")
    target_compile_definitions(sp2freetype PRIVATE "-DFT2_BUILD_LIBRARY")

    if(SP2_ICON)
        if("${IMAGE_CONVERT}" STREQUAL "IMAGE_CONVERT-NOTFOUND")
            message(WARNING "ImageMagick not found, cannot build icons. Which is fine for test builds. But release builds are better with icons.")
        elseif(WIN32)
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/icon.ico"
                COMMAND "${IMAGE_CONVERT}" ARGS "${SP2_ICON}" -resize 256x256 -define icon:auto-resize="256,128,96,64,48,32,16" "${CMAKE_CURRENT_BINARY_DIR}/icon.ico"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                DEPENDS "${SP2_ICON}")
            file(GENERATE
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/win32.rc"
                CONTENT "id ICON \"icon.ico\"")
            set_source_files_properties(
                "${CMAKE_CURRENT_BINARY_DIR}/win32.rc"
                PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/icon.ico")
            list(APPEND SP2_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/win32.rc")
        elseif(ANDROID)
            file(MAKE_DIRECTORY
                "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi"
                "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-hdpi"
                "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xhdpi"
                "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxhdpi"
                "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxxhdpi")
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi/ic_launcher.png"
                COMMAND "${IMAGE_CONVERT}" ARGS "${SP2_ICON}" -resize 48x48 "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi/ic_launcher.png"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                DEPENDS "${SP2_ICON}")
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-hdpi/ic_launcher.png"
                COMMAND "${IMAGE_CONVERT}" ARGS "${SP2_ICON}" -resize 72x72 "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-hdpi/ic_launcher.png"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                DEPENDS "${SP2_ICON}")
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xhdpi/ic_launcher.png"
                COMMAND "${IMAGE_CONVERT}" ARGS "${SP2_ICON}" -resize 96x96 "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xhdpi/ic_launcher.png"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                DEPENDS "${SP2_ICON}")
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxhdpi/ic_launcher.png"
                COMMAND "${IMAGE_CONVERT}" ARGS "${SP2_ICON}" -resize 144x144 "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxhdpi/ic_launcher.png"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                DEPENDS "${SP2_ICON}")
            add_custom_command(
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxxhdpi/ic_launcher.png"
                COMMAND "${IMAGE_CONVERT}" ARGS "${SP2_ICON}" -resize 192x192 "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxxhdpi/ic_launcher.png"
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                DEPENDS "${SP2_ICON}")
            list(APPEND ANDROID_RESOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi/ic_launcher.png")
            list(APPEND ANDROID_RESOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-hdpi/ic_launcher.png")
            list(APPEND ANDROID_RESOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xhdpi/ic_launcher.png")
            list(APPEND ANDROID_RESOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxhdpi/ic_launcher.png")
            list(APPEND ANDROID_RESOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-xxxhdpi/ic_launcher.png")
        endif()
    else()
        if(ANDROID)
            message(WARNING "No icon specified, using SDL2 default icon. As android requires an icon.")
            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi")
            file(COPY "${SDL_SRC_PATH}/android-project/app/src/main/res/mipmap-mdpi/ic_launcher.png"
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi/")
            list(APPEND ANDROID_RESOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/android_resources/mipmap-mdpi/ic_launcher.png")
        endif()
    endif()

    set(SP2_TARGET_NAME ${EXECUTABLE_NAME})
    if(ANDROID)
        # For Android, we need a libmain.so, which is packed into an android APK, and loaded from java sources
        set(SP2_TARGET_NAME main)
        add_library(${SP2_TARGET_NAME} SHARED ${ARGN} ${SP2_SOURCES})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti")
        android_apk(${EXECUTABLE_NAME} resources)
    else()
        add_executable(${SP2_TARGET_NAME} ${ARGN} ${SP2_SOURCES})
        if (EMSCRIPTEN)
            configure_file("${SP2_DIR}/cmake/emscripten/template.html" "${CMAKE_CURRENT_BINARY_DIR}/${SP2_TARGET_NAME}.html")
        endif()
    endif()
    target_compile_options(${SP2_TARGET_NAME} PUBLIC ${WARNING_FLAGS})
    target_link_libraries(${SP2_TARGET_NAME} PUBLIC box2d bullet lua json11 miniz sp2freetype)
    target_include_directories(${SP2_TARGET_NAME} PUBLIC "${SERIOUS_PROTON2_BASE_DIR}/include" "${SERIOUS_PROTON2_BASE_DIR}/extlibs/bullet")
    target_link_libraries(${SP2_TARGET_NAME} PUBLIC ${SDL2_LIBRARIES})
    target_include_directories(${SP2_TARGET_NAME} PUBLIC ${SDL2_INCLUDE_DIRS})
    if(WIN32)
        target_link_libraries(${SP2_TARGET_NAME} PUBLIC dbghelp psapi ws2_32 iphlpapi ole32 strmiids crypt32)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_link_libraries(${SP2_TARGET_NAME} PUBLIC "-mconsole")
        endif()
    endif()
    if(UNIX)
        find_package(Threads)
        target_link_libraries(${SP2_TARGET_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    endif()

    if(WIN32)
        install(TARGETS ${SP2_TARGET_NAME} RUNTIME DESTINATION .)

        execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libwinpthread-1.dll OUTPUT_VARIABLE MINGW_PTHREAD_DLL OUTPUT_STRIP_TRAILING_WHITESPACE)
        install(FILES ${MINGW_STDCPP_DLL} ${MINGW_LIBGCC_DLL} ${MINGW_PTHREAD_DLL} DESTINATION .)
        install(FILES ${SDL2_PREFIX}/bin/SDL2.dll DESTINATION .)
    endif()
endmacro()

# Macro to build the android apk
macro(android_apk NAME ASSETS_FOLDER)
    configure_file("${SP2_DIR}/cmake/android/AndroidManifest.xml.in" "${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml")
    configure_file("${SP2_DIR}/cmake/android/SDLActivity.java.in" "${CMAKE_CURRENT_BINARY_DIR}/java_source/sp2/${NAME}/SDLActivity.java")
    # Generate the R.java file
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/java_source/sp2/${NAME}/R.java"
        COMMAND "${AAPT}" ARGS package -m -J "${CMAKE_CURRENT_BINARY_DIR}/java_source" -M "${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml" -S "${CMAKE_CURRENT_BINARY_DIR}/android_resources" -I "${ANDROID_PLATFORM_JAR}"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml" ${ANDROID_RESOURCE_FILES}
    )
    # Generate apk with resource files and manifest, but nothing else
    # Compile the java sources (TODO, warning about bootstrap classpath, rt.jar wrong version)
    file(GLOB JAVA_SOURCES "${SDL_SRC_PATH}/android-project/app/src/main/java/org/libsdl/app/*.java")
    list(APPEND JAVA_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/java_source/sp2/${NAME}/R.java")
    list(APPEND JAVA_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/java_source/sp2/${NAME}/SDLActivity.java")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/java_compiled/")
    add_custom_command(
        OUTPUT "java_compiled/sp2/Template/R.class"
        COMMAND ${Java_JAVAC_EXECUTABLE} ARGS -source 1.7 -target 1.7 -classpath "${ANDROID_PLATFORM_JAR}" ${JAVA_SOURCES} -d "${CMAKE_CURRENT_BINARY_DIR}/java_compiled/"
        DEPENDS ${JAVA_SOURCES}
    )
    # Convert sources into dex file
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/classes.dex"
        COMMAND "${DX}" ARGS "--dex" "--output=${CMAKE_CURRENT_BINARY_DIR}/apk_contents/classes.dex" "${CMAKE_CURRENT_BINARY_DIR}/java_compiled/"
        DEPENDS "java_compiled/sp2/Template/R.class"
    )

    file(COPY "${SDL_INSTALL_PATH}/lib/libSDL2.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/lib/${ANDROID_ABI}/")
    file(COPY "${SDL_INSTALL_PATH}/lib/libhidapi.so" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/lib/${ANDROID_ABI}/")
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/lib/${ANDROID_ABI}/libmain.so"
        COMMAND "${CMAKE_COMMAND}" ARGS -E copy "$<TARGET_FILE:main>" "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/lib/${ANDROID_ABI}/libmain.so"
        DEPENDS "$<TARGET_FILE:main>"
    )

    set(APK_ALIGNED "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.apk")
    set(APK "${APK_ALIGNED}.unaligned")
    file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/assets/")
    file(COPY "${ASSETS_FOLDER}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/assets/")
    file(GLOB_RECURSE ASSETS LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/" "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/assets/*")
    add_custom_command(
        OUTPUT "${APK}"
        COMMAND "${AAPT}" ARGS package -f -M "${CMAKE_CURRENT_BINARY_DIR}/AndroidManifest.xml" -S "${CMAKE_CURRENT_BINARY_DIR}/android_resources" -I "${ANDROID_PLATFORM_JAR}" -F "${APK}"
        COMMAND "${AAPT}" ARGS add "${APK}" classes.dex
        COMMAND "${AAPT}" ARGS add "${APK}" lib/${ANDROID_ABI}/libmain.so lib/${ANDROID_ABI}/libSDL2.so lib/${ANDROID_ABI}/libhidapi.so
        COMMAND "${AAPT}" ARGS add "${APK}" ${ASSETS}
        COMMAND "${Java_JARSIGNER_EXECUTABLE}" ARGS -verbose -sigalg SHA1withRSA -digestalg SHA1 -storepass "${ANDROID_SIGN_KEY_PASSWORD}" "${APK}" "${ANDROID_SIGN_KEY_NAME}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/classes.dex" "${CMAKE_CURRENT_BINARY_DIR}/apk_contents/lib/${ANDROID_ABI}/libmain.so"
    )
    add_custom_command(
        OUTPUT "${APK_ALIGNED}"
        COMMAND "${CMAKE_COMMAND}" -E remove -f "${APK_ALIGNED}"
        COMMAND "${ZIPALIGN}" ARGS 4 "${APK}" "${APK_ALIGNED}"
        DEPENDS "${APK}"
    )
    add_custom_target(apk ALL DEPENDS "${APK_ALIGNED}")
    add_custom_target(upload
        COMMAND ${ADB} install -r "${APK_ALIGNED}"
        COMMAND ${ADB} shell am start -n "sp2.${NAME}/.SDLActivity"
        DEPENDS "${APK_ALIGNED}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
endmacro()
