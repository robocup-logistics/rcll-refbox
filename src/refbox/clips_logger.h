
/***************************************************************************
 *  clips_logger.h - LLSF RefBox CLIPS logging utilities
 *
 *  Created: Tue Mar 05 00:52:29 2013
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

#ifndef __LLSF_REFBOX_CLIPS_LOGGER_H_
#define __LLSF_REFBOX_CLIPS_LOGGER_H_

#include <string>

namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class Logger;

/// @cond INTERNALS

class CLIPSLogger
{
 public:
  CLIPSLogger(Logger *logger, Logger *trace_logger, const char *component = 0);
  ~CLIPSLogger();

  void log(const char *logical_name, const char *str);

 private:
  Logger *logger_;
  Logger *trace_logger_;
  char *component_;
  std::string buffer_;
};

extern void
init_clips_logger(void *env, Logger *logger, Logger *trace_logger);

extern void
finalize_clips_logger(void *env);

/// @endcond

} // end of namespace llsfrb

#endif
