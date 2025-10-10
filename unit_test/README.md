# Unit tests for LEDGE functions
These unit tests is to confirm that functions in LEDGE library work correctly by comparing to pre-defined ground truth data. All unit tests can be executed automatically in Github Actions. However, if you want to execute them manually, you can follow the procedure.

## Build all unit tests
```
cd /app/LEDGE/unit_test
mkdir -p build
cd build
cmake ..
make
```

## Run all unit tests
```
cd /app/LEDGE/unit_test/build
./test_ledge_core && ./test_ledge_manager && ./test_ledge_tracking
```