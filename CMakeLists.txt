cmake_minimum_required(VERSION 3.14)
project(MyProjectName)

# 添加子目录
add_subdirectory(external/astcenc)

# 设定你的项目的源文件
add_executable(MyExecutable main.cpp)

# 添加依赖确保在构建MyExecutable之前构建astcenc-native-static
add_dependencies(MyExecutable astcenc-native-static)

add_dependencies(MyExecutable astcenc-native)


# 链接 astcenc 库
target_link_libraries(MyExecutable astcenc-native-static)

target_link_libraries(MyExecutable astcenc-native)


# ISPC 库的头文件路径
target_include_directories(MyExecutable PRIVATE ${CMAKE_SOURCE_DIR}/external/ISPC/builds)
# ISPC 库的导入库.lib文件路径
target_link_directories(MyExecutable PRIVATE ${CMAKE_SOURCE_DIR}/external/ISPC/builds/windows)
# 链接 ISPC 库
target_link_libraries(MyExecutable ispc_texcomp)
# 将 DLL 文件复制到输出目录（可选）
add_custom_command(TARGET MyExecutable POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/external/ISPC/builds/windows/ispc_texcomp.dll"
    $<TARGET_FILE_DIR:MyExecutable>)


#如果 astcenc-native-static 的 CMake 配置中包含了像下面的代码：
#target_include_directories(astcenc-native-static PUBLIC include)
#这表示任何链接了 astcenc-native-static 的目标（例如您的 MyExecutable）都将自动获得 include 目录作为头文件搜索路径。
#这样，当您在 MyExecutable 中 #include 相关头文件时，编译器能够找到这些文件，而您无需在 MyExecutable 的 CMake 配置中显式添加 target_include_directories

# 指定头文件搜索路径
# target_include_directories(MyExecutable PRIVATE external/astcenc/Source)