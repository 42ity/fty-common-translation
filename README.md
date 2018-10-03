# fty-common-translation

This is a library providing :
* A translation system based on idea of gettext

## How to build

To build the `fty-common-translation` project run:

```bash
./autogen.sh
./configure
make
make check # to run self-test
```

## How to use Translation System

TBD

## How to compile and test using 42ITy standards

### project.xml

Add this block in the `project.xml` file :

````
    <use project = "fty-common-translation" libname = "libfty_common_translation" header="fty_common_translation.h"
        repository = "https://github.com/42ity/fty-common-translation.git"
        release = "master"
        test = "fty_common_translation_selftest" >

        <use project = "fty-common-mlm" libname = "libfty_common_mlm" header="fty_common_mlm.h"
            repository = "https://github.com/42ity/fty-common-mlm.git"
            test = "fty_commmon_mlm_selftest"
            />
    </use>
````

NOTE: The `header` value must be changed from `fty_common_translation.h` to
`fty-common-translation/fty_common_ctranslation.h` for a C project.

NOTE: In this `use` section, remove the dependecy already needed directly
by the particular agent/library whose project you are configuring, to
simplify maintenance of the configuration later.

### How to pass Travis CI checks

In recent zproject revisions, dependency code that can be compiled from
source or taken from packages (the 42ITy project does not publish any at
the moment) is defined in a separate list, so it suffices to comment away
the reference to this list:

````
# NOTE: Our forks are checked out and built without pkg dependencies in use
pkg_deps_prereqs: &pkg_deps_prereqs
#    - *pkg_deps_prereqs_source
    - *pkg_deps_prereqs_distro
````

Alternately, to avoid any errors, you can add these two lines in the
`before_install` section of the `.travis.yml` file, to remove these
packages if added into the build system (e.g. by some other dependencies):

````
before_install:
- sudo apt-get autoremove
````

It is not recommended to do this right away (before the problem bites in
practice) because calls to packaging have considerable overheads in the
run-times of the tests.
