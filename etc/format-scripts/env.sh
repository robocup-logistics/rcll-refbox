#! /bin/bash
#
#  Copyright (c) 2020 Till Hofmann
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  Read the full text in the LICENSE.GPL file in the doc directory.

DIFF=diff
if type -p gdiff >/dev/null; then
  DIFF=gdiff
fi

if ! type -p $DIFF >/dev/null ; then
  echo "Cannot find $DIFF!" >&2;
  exit 1;
fi

if ! type -p paste >/dev/null ; then
  echo "Cannot find paste!" >&2;
  exit 1;
fi

if ! type -p xargs >/dev/null ; then
  echo "Cannot find xargs!" >&2;
  exit 1;
fi

if ! type -p sort >/dev/null ; then
  echo "Cannot find sort!" >&2;
  exit 1;
fi

if ! type -p clang-format >/dev/null ; then
  echo "Cannot find clang-format!" >&2;
  exit 1;
fi
