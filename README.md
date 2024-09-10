## Introduction

`simplistic-fsm` is a lightweight C++ single header library for implementing finite state machines (FSM). It provides a straightforward API to define states and manage state transitions within a context. The library supports basic state management with optional thread safety. Ideal for projects needing a simple yet effective way to handle state-driven logic.

## Integrating `simplistic-fsm` Library

### Using CMake's `find_package`

If `simplistic-fsm` is installed on your system, you can integrate it with:
```cmake
cmake_minimum_required(VERSION 3.16)
project(my_project)

find_package(simplistic-fsm REQUIRED)

add_executable(my_executable main.cpp)
target_link_libraries(my_executable PRIVATE simplistic::fsm)
```
### Adding as a Subdirectory

To include the library directly in your project:

1.  Clone the `simplistic-fsm` repository.
    
2.  Update your `CMakeLists.txt`:
    
```cmake
cmake_minimum_required(VERSION 3.16)
project(my_project)

add_subdirectory(path/to/simplistic-fsm)

add_executable(my_executable main.cpp)
target_link_libraries(my_executable PRIVATE simplistic::fsm)
```
Replace `path/to/simplistic-fsm` with the relative path to the cloned repository.


## Getting Started

### Example: Basic State Machine

Create a minimal example to get started:

1.  **Include the Library**
```cpp
#include <simplistic/fsm.h>
```
2. **Define States**
```cpp
#include <iostream>

class StateA : public simplistic::fsm::IState {
public:
    void Handle(simplistic::fsm::IContext* ctx) override {
        std::cout << "State A\n";
        ctx->SetState(std::make_unique<StateB /*Impl must be visible*/ >());
    }
};

class StateB : public simplistic::fsm::IState {
public:
    void Handle(simplistic::fsm::IContext* ctx) override {
        std::cout << "State B\n";
        // Transition to State A again to demonstrate cycling
        ctx->SetState(std::make_unique<StateA>());
    }
};
```
3. **Setup Context**
```cpp
int main() {
    simplistic::fsm::Context context(std::make_unique<StateA>());

    for (int i = 0; i < 4; ++i) { // Loop to demonstrate state transitions
        context.Handle();
    }

    return 0;
}
```

### Build and Run
Ensure you link against `simplistic-fsm` 

## Contributions

Contributions are welcome! Please open issues or submit pull requests for improvements and bug fixes.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
