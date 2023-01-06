/* -*- Mode: C++; c-file-style: "bsd"; comment-column: 40 -*- */
/*
   Copyright (C) 2000 Daniel Christian, T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

//
// fopen-path.cc
//
// Modified version of fopen, that searches a path.
// Technically this is a c++ file, but it should work in C too.
// The functions use a C calling convention for compatibility.

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "value.h"
#include "fopen-path.h"
#include "symbol.h"
#include "ui.h"

// should be command line

static std::vector<std::string> searchPath;

class CSourceSearchPath : public String
{
public:
  CSourceSearchPath()
    : String("SourcePath", nullptr, "Search path for source files")
  {
  }
  void set(const char *cP, int len = 0) override
  {
    (void)len;
    set_search_path(cP);
  }
  void set(Value *pValue) override
  {
    String *pString = dynamic_cast<String*>(pValue);
    if (pString) {
      set_search_path(pString->getVal());
    }
  }

  std::string toString() override
  {
    std::string str_path;
    for (const auto &path : searchPath) {
      str_path += path;
      str_path += ':';
    }
    // Remove the final ':' character
    if (!str_path.empty()) {
      str_path.erase(str_path.size() - 1, 1);
    }
    return str_path;
  }
};


///
/// InitSourceSearchSymbol
/// Used to initialize the CSourceSearchPath during startup.
/// The symbol table will delete the CSourceSearchPath
/// object so CSourceSearchPath cannot be static.
void InitSourceSearchAsSymbol()
{
  // The symbol table will delete the CSourceSearchPath object
  globalSymbolTable().addSymbol(new CSourceSearchPath());
}


// Given a colon separated path, setup searchPath
// Ensure the path ends with a '/'
void set_search_path(const char *path)
{
  searchPath.clear();
  if (!path || !*path) {		// clear the path
    if (verbose)
      std::cout << "Clearing Search directory.\n";

    return;
  }

  std::string path_str = path;

  for (std::string::size_type start = 0; ; ) {
    auto end = path_str.find(':', start);

    if (end == std::string::npos) {
      if (path_str.back() == '/')
        searchPath.push_back(path_str.substr(start));
      else
        searchPath.push_back(path_str.substr(start) + '/');
      if (verbose)
        std::cout << "Search directory: " << searchPath.back() << '\n';
      break;
    } else if (start != end) {
      if (path_str[end - 1] == '/')
        searchPath.push_back(path_str.substr(start, end - start));
      else
        searchPath.push_back(path_str.substr(start, end - start) + '/');
      if (verbose)
        std::cout << "Search directory: " << searchPath.back() << '\n';
    }
    start = end + 1;
  }
}


//-----------------------------------------------------------
// Try to open a file on a series of paths.  First try as an absolute path.
// This tries to keep as much of the original file path as possible.
// for example: fopen_path ("src/pic/foo.inc", "r"), will try
//	src/pic/foo.inc,
//	PATH1/src/pic/foo.inc, PATH1/pic/foo.inc, PATH1/foo.inc
//	PATH2/src/pic/foo.inc, PATH2/pic/foo.inc, PATH2/foo.inc
//	...
// It also converts back slashes to forward slashes (for MPLAB compatibility)

FILE *fopen_path(const char *filename, const char *perms)
{
  long maxpath;

#if defined(_WIN32)
  maxpath = MAX_PATH;
#else
  if ((maxpath = pathconf(filename, _PC_PATH_MAX)) < 0)
    return nullptr;
#endif

  if (strlen(filename) >= (unsigned int)maxpath) {
    errno = ENAMETOOLONG;
    return nullptr;
  }

  std::string nameBuff = filename;
  // convert DOS slash to Unix slash
  std::replace(nameBuff.begin(), nameBuff.end(), '\\', '/');

  FILE *fp = fopen(nameBuff.c_str(), perms);		// first try absolute path
  if (fp) {
    if (verbose)
      printf("Found %s as %s\n", filename, nameBuff.c_str());
    return fp;
  }

  // check along each directory
  for (const auto &ii : searchPath) {
    // check each subdir in path
    const char *fileStr = filename;			// filename walker
    for ( ; fileStr && *fileStr; fileStr = strpbrk(fileStr + 1, "/\\")) {
      nameBuff = ii + fileStr;

      if (nameBuff.size() < (unsigned int)maxpath) {
        // convert DOS slash to Unix slash
        std::replace(nameBuff.begin(), nameBuff.end(), '\\', '/');

        if (verbose) {
          printf("Trying to open %s\n", nameBuff.c_str());
        }
        fp = fopen(nameBuff.c_str(), perms);	// try it
        if (fp) {
          if (verbose)
            printf("Found %s as %s\n", filename, nameBuff.c_str());
          return fp;
        }
      }
    }
  }

  if (verbose) {
    printf("Failed to open %s in path: ", filename);
    for (const auto &ii : searchPath) {
      printf("%s ", ii.c_str());
    }
    printf("\n");
  }

  return nullptr;
}
