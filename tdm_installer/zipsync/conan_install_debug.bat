conan install . -if build -s build_type=Debug -s BLAKE2:build_type=Release --build=outdated
conan build . -bf build
