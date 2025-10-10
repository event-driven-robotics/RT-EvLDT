# LEDGE library
C++ LEDGE library including the following functions
- `core`: core functions for LEDGE, such as initialization, visualization, etc.
- `detection`: functions for detecting line segments
- `tracking`: functions for tracking line segments
- `manager`: functions for managing line segments

# Build LEDGE library
Terminal 1 (in docker container)
```
cd /app/LEDGE/ledge
mkdir -p build
cd build
cmake ..
make
sudo make install
```
- Build not only static library but also .cmake files to find LEDGE library by using cmake command
- LEDGE library can be installed into your local or docker environment

# Link LEDGE library to your repository
Use cmake and add the following commands into your `CMakeLists.txt`
```
find_package(ledge REQUIRED)
target_link_libraries(your_project_name ledge::ledge)
```
- [Reference](../example/CMakeLists.txt)