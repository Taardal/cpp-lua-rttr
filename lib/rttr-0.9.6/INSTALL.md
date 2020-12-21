# Installation ([v 0.9.6][rttr.install])

1. [Download][rttr.download] and unzip RTTR
```
unzip <zip-file> <project>/lib
```
2. Create build-folder for RTTR:
```
cd <project>/lib/<rttr-version>
mkdir build
```
3. Run cmake-command to generate build files
```
cmake \
-S . \
-B build \
-D BUILD_UNIT_TESTS=OFF \
-D BUILD_BENCHMARKS=OFF \
-D BUILD_EXAMPLES=OFF \
-D BUILD_DOCUMENTATION=OFF
```
4. Run command to build RTTR from generated build files
```
cmake \
--build build \
--target install
```
5. Fix include directories path
```
//Expected
<project>/lib/rttr-<version>/build/install/indclude/rttr/registration.h

//Generated
<project>/lib/rttr-<version>/build/install/include/rttr/Users/<user>/<project>/lib/rttr-<version>/src/rttr/registration.h
```
6. Add RTTR to project `CMakeLists.txt`
```
// In <project>/CMakeLists.txt
set(RTTR_DIR lib/rttr-<version>/build/install/share/rttr/cmake)
find_package(RTTR CONFIG REQUIRED Core)
target_link_libraries(${PROJECT_NAME} RTTR::Core)
```

[rttr.download]: https://www.rttr.org/download
[rttr.install]: https://www.rttr.org/doc/rttr-0-9-6/building_install_page.html