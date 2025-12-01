FetchContent_Declare(
    filewatch
    GIT_REPOSITORY https://github.com/Okaetsu/filewatch.git # fork of ThomasMonkman's filewatch with cmake_minimum_required set to 3.22.
    GIT_TAG 19e7fcaa3ded615a753cd0a4cf040e373441e53c
    SOURCE_SUBDIR folderThatDoesntExist # need to do this hack to make it work since it's a header only library, should be a folder that doesn't exist.
)

FetchContent_MakeAvailable(filewatch)