# CKSets

## Description

This is a C++ implementation of the KSETS Algorithm ([KSETS by uef-machine-learning](https://github.com/uef-machine-learning/K-sets_and_K-swaps/tree/main)).
Which is the companion code repository to the article by [Rezaei and Fränti (2023)](#inspiration)
## Quickstart

```
    mamba env create -f build_env.yml
    conda activate cksets_build
    pip install .
    py-cksets --help
    py-ksets --help # Original Authors code trimmed down for comparison sake
```

## Build with Cmake

```
    mamba env create -f build_env.yml
    conda activate cksets_build
    cmake  -B build   -DCMAKE_CXX_COMPILER=$(which g++)
    cd build 
    make -j 5 
    ./cksets --help
```

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.



## Gitlab Project Setup inspiration

### Collaborate with your team

- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/user/project/merge_requests/auto_merge/)

### Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

<details><summary>Readme Help</summary>

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

