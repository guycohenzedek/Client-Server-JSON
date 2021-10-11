================================
======== JSON-C parser =========
================================

* Home page: https://github.com/json-c/json-c

================
* Description: *
================
JSON-C implements a reference counting object model that allows you to easily construct JSON objects in C, 
output them as JSON formatted strings and parse JSON formatted strings back into the C representation of JSON objects. 
It aims to conform to RFC 7159.

========================
* install source code: *
========================

1. Get source code into directory named json-c:
   $ git clone https://github.com/json-c/json-c.git

2. Create new directory beside json-c:
   $ mkdir json-c-build

3. Enter to the new directory:
   $ cd json-c-build

4. Generating files and configuration by cmake:
   $ cmake ../json-c

# Note! A. stay in json-c-build directory for use "make" options
		B. cmake just once !
5. Complie the project:
   $ make

6. Install the project on the computer(/usr/local/lib, /usr/local/include):
   $ sudo make install

=====================
* run json-c tests: *
=====================
* Run automatic tests supplied by the author:
   $ make test

================================
* rebuild and Install project: *
================================
* Complie the project:
   $ make
   $ sudo make install

==================
* clean project: *
==================
* Clean objects and executables from the project:
   $ make clean

==========================
* uninstall source code: *
==========================
* Uninstall json-c library from the computer:
   $ sudo make uninstall

=================
* using json-c: *
=================

* #include <json-c/json.h>

* Add "-ljson-c" to compilation, for example: "$ gcc test.c -o test -ljson-c"
