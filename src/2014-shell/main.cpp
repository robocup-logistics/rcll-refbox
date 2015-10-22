
/***************************************************************************
 *  main.cpp - LLSF RefBox shell - main program
 *
 *  Created: Fri Feb 15 10:07:47 2013
 *  Copyright  2013  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the authors nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "shell.h"

#include <locale.h>
#include <string>
#include <google/protobuf/message.h>
#include <cursesw.h>

int
main(int argc, char **argv)
{
  if (!setlocale(LC_CTYPE, "")) {
    fprintf(stderr, "Can't set the specified locale! "
	    "Check LANG, LC_CTYPE, LC_ALL.\n");
    return 1;
  }

  int rv = 0;

  initscr();
  int h, w;
  getmaxyx(stdscr, h, w);

  if (h < 31) {
    ::endwin();
    printf("A minimum of 31 lines is required in the terminal\n");
    return -1;
  }

  if (w < 87) {
    ::endwin();
    printf("A minimum of 87 columns is required in the terminal\n");
    return -1;
  }

  {
    NCursesWindow rootw(::stdscr);
    rootw.bkgd(' '|COLOR_PAIR(5));

    NCursesWindow::useColors();

    llsfrb_shell::LLSFRefBoxShell shell;
    rv = shell.run();
  }
  ::endwin();

  // Delete all global objects allocated by libprotobuf
  google::protobuf::ShutdownProtobufLibrary();

  // If we do not exit but return here, a segfault happens during protobuf
  // library cleanup, but only if we instantiate the network client in the
  // shell. It seems to be related to library global variable instantiation.
  // Very weird stuff!
  exit(rv);
}
