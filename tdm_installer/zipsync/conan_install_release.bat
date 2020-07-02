conan install . -if build -s build_type=RelWithDebInfo --build=outdated
conan build . -bf build
