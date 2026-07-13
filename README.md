# CKSets

## Description

This is a C++ implementation of the KSETS Algorithm ([KSETS by uef-machine-learning](https://github.com/uef-machine-learning/K-sets_and_K-swaps/tree/main)).
Which is the companion code repository to the article by [Rezaei and Fränti (2023)](#inspiration)
## Quickstart

```
    mamba env create -f build_env.yml
    conda activate cksets_build
    pip install .
    py-cksets --help # python wrapped c++ code 
    py-ksets --help # Original Authors code trimmed down for comparison sake
```

## Build with Cmake

```
    mamba env create -f build_env.yml
    conda activate cksets_build
    cmake  -B build   -DCMAKE_CXX_COMPILER=$(which g++)
    cmake --build build -j 5
    ./cksets --help
```

## Usage


### Include in Project by using CPM and CMake

```CMake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MYLIB_CKSETS_VERSION 0.2.1 "")
CPMDeclarePackage(
    cksets
    NAME cksets
    GIT_REPOSITORY https://git.imp.fu-berlin.de/rwk53376/cksets.git
    GIT_TAG ${MYLIB_CKSETS_VERSION}
    SYSTEM TRUE
    EXCLUDE_FROM_ALL TRUE
    OPTIONS "BUILD_PYTHON_BINDINGS OFF"
)

CPMGetPackage (cksets)
add_executable(my_amazing_executable src/my_amazing_executable.cpp)
target_link_libraries(my_amazing_executable PRIVATE ksets::ksets)
```

### Quickstart 

`ksets::cluster()` accepts any forward range of token sequences. Each inner sequence represents one set/document, and the returned labels contain the assigned cluster for each input sequence.

`ksets::token_t` is the library's token type (currently an unsigned integer). Each sequence should contain the identifiers of the elements in a set

```cpp
// src/my_amazing_executable.cpp
#include <iostream>
#include <vector>

#include <ksets/ksets.hpp>

int main()
{
    std::vector<std::vector<ksets::token_t>> data{
        {1, 2, 3, 4, 5},
        {6, 2, 5, 4},
        {2, 3, 4, 8},
        {2, 1, 4, 7},
        {1, 2, 3, 4}
    };

    ksets::ClusterOptions opts;
    opts.clusters = 4;

    ksets::ClusterResult result = ksets::cluster(data, opts);

    std::cout << "Objective: " << result.objective
              << "\nIterations: " << result.iterations
              << "\nLabels:\n";

    for (auto label : result.labels)
        std::cout << label << '\n';

    return 0;
}
```

### ToDos: 
 - [ ] Explain API of ksets library
 - [ ] Show how to run the example test runs from the ksets authors 




***

<details><summary>Readme Help</summary>


### Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)



### Editing this README



#### Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.



#### Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.




#### Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

#### Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

#### Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

#### Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

#### License
For open source projects, say how it is licensed.

#### Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.

</details>


## References

<a name="inspiration"></a>
Mohammad Rezaei, Pasi Fränti,
K-sets and k-swaps algorithms for clustering sets,
Pattern Recognition, Volume 139, 2023, 109454,ISSN 0031-3203,
https://doi.org/10.1016/j.patcog.2023.109454.

