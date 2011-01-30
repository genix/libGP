Licence
------------------------------------------------------------------------------

This readme is part of libGP C++ library.

Copyright (c) 2011 Craig Furness

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

About
------------------------------------------------------------------------------

This library is licensed under the MIT License. It is fairly free to use. The
license info is atop every file in the library.

The library implements a framework and algorithms for implementing and testing
genetic programs.

Its basic tenets are:

* Ease of use
  (complexity under the hood, but using is trivial)

* C++ implementation.
  (for my ease of use with existing projects in C++ such as games)

* To allow conventional C++ functions as nodes in the GP
  (ease of hooking up to existing projects)

* To not require any 3rd party libraries, just standard C++

Overview
------------------------------------------------------------------------------

FOLDERS
-------

The library includes four folders: src, include, auxiliary, and examples.

src       : All the .cpp files you need for the framework itself.
include   : All necessary .h files for the framework itself.
auxiliary : Some supporting functionality (serialisation, visualisation)
examples  : A few .cpp files demonstrating the use of the library.

The examples are well documented, and written to be guides as a manual would
be. So the use of the library is not documented here.

SUPPORT TOOLS
-------------

For visualization, the auxiliary support code contains functions to save
in a format readable by a program called Dotty. All the examples save
out .dot files for viewing.

Go to: http://www.graphviz.org/ to get the latest package including Dotty.

Additional code under development generates html/jscript to produce graphs
using the Google Charts API.

FEATURES
--------

My implementation goals for the library have been:

* Uses C++ functions as 'nodes' in the genetic programs.
* Supports any parameter types for those functions.
* Can be modded to support more parameters per function if needed.
* Aggressively limits the maximum number of nodes any individual can have.


Compiling
------------------------------------------------------------------------------

I have not run this with GCC, but with Microsoft's compiler. There are some
funky pointer casts which may cause GCC to throw its hands up. I hope to get
around to testing and resolving these (if any).

You'll want to put the 'include' and 'src' folders into a project and compile
as a library. Then when you compile the examples, you'll need to add the files
in the 'auxiliary' folder for compilation too, and link with the aforementioned
library.

Notes & Known Issues
------------------------------------------------------------------------------

Im just going to list some key ones off the top of my head:

* Untested with GCC or non-Microsoft compilers.

* Code is littered with TODO comments. This is because the project has evolved
as a personal project and undergone a number of minor refactorings. It never
had a formal way of tracking tasks.

* The support for member functions as nodes in a GP is still under development.
Currently the library has compile errors if 'const' members are used.

* All 'support' code (serialisation, visualisation, stats tracking) is fairly
rudimentary. Im not sure Ill ever take these much further than current since
they are not the focus of the library.

Contact
------------------------------------------------------------------------------

This library is provided pretty much as-is. I'll work on it when I have the
interest to. Right now its in a place where I can effectively use it so
development has stopped.

However if you have any questions, you can reach me at:
genix79@gmail.com
