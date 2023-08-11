# C++ Lua RTTR

Embedding Lua in a C++ application using the [Run Time Type Reflection (RTTR)][rttr] C++ reflection library.

This example is based on the [Embedding Lua in C++][davepoo:lua] YouTube tutorial by [Dave Poo][davepoo].

# Prerequisites&nbsp;:vertical_traffic_light:

- [Git][git]
- [CMake][cmake]
- C++ compiler
    - Windows: [MSVC][msvc] (Bundled with [Visual Studio][msvs])
    - MacOS: [Clang][clang] (Bundled with [Xcode][xcode])
    - Linux: [GCC][gcc]

# Getting started&nbsp;:runner:

### Getting the code&nbsp;:octocat:

- Clone the repository: `git clone https://github.com/taardal/cpp-lua-rttr`

### Running the app&nbsp;:rocket:

**RTTR**

- Build the RTTR library with CMake by following the instructions in the file `lib/rttr-0.9.6/INSTALL.md`
  - See also: [RTTR docs: Building & Installing][rttr:install]

**App**

- Generate project files: `cmake -S . -B build`
- Build executable from generated files: `cmake --build build`
- Install executable: `cmake --install build` 
- Run the executable: `./app`

### Known problems&nbsp;:warning:

The build fails with the error below in the following environment:

```
MacOS 13.4
Xcode 14.3.1
Clang 14.0.3
```

```
In file included from /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.3.sdk/usr/include/c++/v1/__functional/boyer_moore_searcher.h:26:
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.3.sdk/usr/include/c++/v1/vector:540:52: error: arithmetic on a pointer to an incomplete type 'rttr::argument'
        {return static_cast<size_type>(__end_cap() - this->__begin_);}
```

There is a working binary in the `bin` folder that was built in the following environment:

```
MacOS 12.6
Xcode 14.1
Clang 14.0
```

# Resources&nbsp;:books:

- [Embedding Lua in C++][davepoo:lua] (YouTube @ [Dave Poo][davepoo])


[clang]: http://clang.org/
[cmake]: https://cmake.org/
[davepoo]: https://www.youtube.com/@DavePoo
[davepoo:lua]: https://www.youtube.com/watch?v=xrLQ0OXfjaI&list=PLLwK93hM93Z3nhfJyRRWGRXHaXgNX0Itk&ab_channel=DavePoo
[gcc]: https://gcc.gnu.org/
[git]: https://git-scm.com
[msvc]: https://visualstudio.microsoft.com/vs/features/cplusplus/
[rttr]: https://www.rttr.org/
[rttr:install]: https://www.rttr.org/doc/rttr-0-9-6/building_install_page.html
[msvs]: https://visualstudio.microsoft.com/
[xcode]: https://developer.apple.com/xcode/
