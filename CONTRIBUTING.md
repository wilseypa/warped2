# How to contribute

Due to the nature of academic projects, contributors join and leave
frequently. To keep the code base usable for future contributors, follow these
guidelines when writing code.

## Getting Started

* Make sure you have a [GitHub account](https://github.com/signup/free).
* Read the materials in the Resources section of this document. 
* If applicable, submit an issue on GitHub describing the issue are you working on.

## Making Changes

* Create a topic branch from where you want your base to work.
	* You usually want to branch off of the master branch.
    * All work should be done in a topic branch: don't commit directly to
      master. Especially don't commit broken or incomplete features to master.
* Commit changes to your topic branch in logical units.
	* The first line of the commit message should be a descriptive summary of the commit. 
	* The first line should not be longer the 50 columns
    * If the commit is not small, follow the description by a single blank
      line and begin a description on the third line.
	* The description lines should not exceed 72 columns.
* Add tests for any code that you write. See the Testing section below for
  more details. Tests are extremely important to ensure that not only is you
  code correct, but more importantly that future code changes don't break your
  code.
* Once you've written tests for your code and they and all existing tests
  pass, you are ready to merge your branch.
    * If your changes do not interfere with any other contributors' work, then
      you can merge your branch into master.
    * If your changes do affect other contributors, open a pull request
      detailing your changes.  Once other contributors have signed off on your
      request, you can merge your branch into master. See the Scott Schacon
      article for details on this process. An additional advantage of using
      pull requests is that [TravisCI will run a build on your changes
      ](http://about.travis-ci.org/blog/2012-09-04-pull-requests-just-got-even-more-awesome/) 
      when you open one. ([See here for instructions on using GitHub pull requests](https://help.github.com/articles/using-pull-requests))
* Warped uses TravisCI for Integration Testing. Any time a commit is pushed to
  master, or a pull request is opened, Travis will build and test the changes.
  A badge in the README displays the status of the most recent build. Use pull
  requests to avoid merging changes that break the build. Again, **never merge
  or commit changes to master that break the Travis build.**

## Testing

You must write thorough tests for any code that you commit to the project.
Unit tests for WARPED are written using the [Catch](http://catch-test.net/)
framework. Instructions for building and running the tests can be found in the
`test` directory. The Ken Beck book listed in the resources is recommended
reading. A tutorial for writing tests with Catch can be found
[here](https://github.com/philsquared/Catch/blob/master/docs/tutorial.md).

## Style Guide

As with most style guides, the most important rule is to remain consistent
with the surrounding code. If a file differs from acceptable style, it is more
important that your code follow the conventions within the file than to have
one function styled differently than the rest of the file. Having said that,
if a file does differ from the accepted style, fixing it is a good idea. Do
not commit style changes at the same time as functional changes! This makes it
very difficult to review your changes, and creates unnecessary merge conflicts
if other people are working on the same code.

The formatting style is enforced using [Artistic
Style](astyle.sourceforge.ne). There is a `.astylerc` file in the root of the
repository which specifies the style used in the project's code. There are
AStyle plugins for most popular editors, or you can run it from the command
line with the a command like the following:

    astyle --options=.astylerc /src/your-file.cpp

Non-formatting style generally follows the [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml). 
There are some guidelines there that are specific to Google. Here are some of
the ways that WARPED style differs from the Google style:

* All C++11 features are allowed and encouraged.
* Using output parameters is discouraged. Simply return by value. If multiple return values are necessary, return a std::pair or std::tuple.
* Do not include a license header at the top of each file. Do include a
  comment at the top of each file describing the files usage and purpose.

Most importantly, remember that you are not writing code for yourself to read.
You are writing code that dozens of contributors in the future will have to
read. Comment your code liberally. Especially important are usage comments:
every file, class, and function that you write needs to have a comment
describing its usage and purpose.

## Memory semantics

In general, prefer to allocate memory on the stack using local or class variables instead of pointers. The heap should only be used when necessary (e.g. to enable polymorphism).

C++11 Smart pointers allow you to avoid manual memory management. Read the [Rule of Zero](http://flamingdangerzone.com/cxx11/2012/08/15/rule-of-zero.html) for an explanation on some modern memory management techniques. Smart pointers should be used instead of storing raw pointers whenever possible. There are many tutorials on the usage of smart pointers, but a quick summary of the various pointer types and when to use them:

<table>
  <tr>
    <th>Name</th>
    <th>C++</th>
    <th>When to Use</th>
  </tr>
  <tr>
    <td>Unique Pointer</td>
    <td>std::unique_ptr&lt;T&gt;</td>
    <td>The default choice. Unique pointers enforce single ownership of the memory.</td>
  </tr>
  <tr>
    <td>Reference</td>
    <td>T&amp;</td>
    <td>A non-owning reference. Use to refer to memory owned by a unique pointer when the memory cannot be null.</td>
  </tr>
  <tr>
    <td>Raw Pointer</td>
    <td>T*</td>
    <td>A non-owning pointer. Use to refer to memory owned by a unique pointer when the memory can be null. Do not allocate into a raw pointer.</td>
  </tr>
  <tr>
    <td>Shared Pointer</td>
    <td>std::shared_ptr&lt;T&gt;</td>
    <td>You probably don't need this. Use shared ownership only when an object has uncertain lifetime (which is very rare)</td>
  </tr>
  <tr>
    <td>Weak Pointer</td>
    <td>std::weak_ptr&lt;T&gt;</td>
    <td>A non-owning pointer to a shared pointer.</td>
  </tr>
</table>

Most importantly, there is never a reason to allocate into a raw pointer, and there is almost never a reason to use `new` at all (see [GotW #102](http://herbsutter.com/gotw/_102/)). [`std::make_unique`](http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique) should be used instead of `new` whenever possible. `std::make_unique` is a C++14, however. Until C++14 becomes standard, use the `warped::make_unique` implementation in `utility/memory.hpp`.

##Releases

Warped uses [Semantic Versioning](http://semver.org/) to label it's releases. Not every commit to master should become a release, but when you want to create a new release, follow these steps:

* As always, make sure that all code builds and passes all tests. Perform some manual functional testing in addition to the automated unit tests.
* Increment the version number in the `AC_INIT` macro at the top of `configure.ac`.
* Configure the source tree as usual, then run `make && make dist && make distcheck`.
	* If you pass any flags to configure when building WARPED, you need to specify these flags with the `DISTCHECK_CONFIGURE_FLAGS` variable.
	* For example: `make distcheck DISTCHECK_CONFIGURE_FLAGS="--with-mpiheader=/usr/include/mpich --with-mpi=/usr/lib/mpich2"`
* If distcheck succeeds, create an annotated Git tag with the same version number as the one that now exists in the `AC_INIT` macro. (see [this section of the Git book](http://git-scm.com/book/en/Git-Basics-Tagging) for instructions on Git tags)
* Push your tag to GitHub
* Add the source tarball created by `make dist` to the release on GitHub. ([instructions here](https://github.com/blog/1547-release-your-software))

## Resources
* The [Git book](http://git-scm.com/book) is indispensable if you haven't used Git before. 

* [This article](http://scottchacon.com/2011/08/31/github-flow.html) by Scott 
  Chacon, an engineer at GitHub, is a very good description of how GitHub uses
  their own service, and is the model for how we organize our workflow.

* The [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml). 
  Although some of the guidelines are specific to Google themselves, the
  majority are very useful.
  
* *Test-Driven Development: By Example* by Kent Beck. This is a very good 
  introduction to Unit Testing and test driven development. If you are a
  University of Cincinnati student, the UC Library has both [physical and
  electronic copies available](http://uclid.uc.edu/record=b4376056~S39).
