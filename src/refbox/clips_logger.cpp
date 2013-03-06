
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

#include "clips_logger.h"

#include <logging/logger.h>
#include <clipsmm.h>

extern "C" {
#include <clips/clips.h>
}


namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/// @cond INTERNALS


class CLIPSContextMaintainer {
 public:
  CLIPSContextMaintainer(Logger *logger, Logger *trace_logger, const char *log_component_name)
  {
    this->logger = new CLIPSLogger(logger, trace_logger, log_component_name);
  }

  ~CLIPSContextMaintainer()
  {
    delete logger;
  }

 public:
  CLIPSLogger *logger;
};


CLIPSLogger::CLIPSLogger(Logger *logger, Logger *trace_logger, const char *component)
{
  logger_ = logger;
  trace_logger_ = trace_logger;
  if (component) {
    component_ = strdup(component);
  } else {
    component_ = NULL;
  }
}

CLIPSLogger::~CLIPSLogger()
{
  if (component_) {
    free(component_);
  }
}

void
CLIPSLogger::log(const char *logical_name, const char *str)
{
  if (strcmp(str, "\n") == 0) {
    if (strcmp(logical_name, "debug") == 0) {
      logger_->log_debug(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
    } else if (strcmp(logical_name, WTRACE) == 0) {
      trace_logger_->log_debug(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
    } else if (strcmp(logical_name, "warn") == 0 || strcmp(logical_name, WWARNING) == 0) {
      logger_->log_warn(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
    } else if (strcmp(logical_name, "error") == 0 || strcmp(logical_name, WERROR) == 0) {
      logger_->log_error(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
    } else {
      logger_->log_info(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
    }

    buffer_.clear();
  } else {
    buffer_ += str;
  }
}


static int
log_router_query(void *env, char *logical_name)
{
  if (strcmp(logical_name, "l") == 0) return TRUE;
  if (strcmp(logical_name, "info") == 0) return TRUE;
  if (strcmp(logical_name, "debug") == 0) return TRUE;
  if (strcmp(logical_name, "warn") == 0) return TRUE;
  if (strcmp(logical_name, "error") == 0) return TRUE;
  if (strcmp(logical_name, "stdout") == 0) return TRUE;
  if (strcmp(logical_name, WTRACE) == 0) return TRUE;
  if (strcmp(logical_name, WWARNING) == 0) return TRUE;
  if (strcmp(logical_name, WERROR) == 0) return TRUE;
  if (strcmp(logical_name, WDISPLAY) == 0) return TRUE;
  return FALSE;
}

static int
log_router_print(void *env, char *logical_name, char *str)
{
  void *rc = GetEnvironmentRouterContext(env);
  CLIPSLogger *logger = static_cast<CLIPSLogger *>(rc);
  logger->log(logical_name, str);
  return TRUE;
}

static int
log_router_exit(void *env, int exit_code)
{
  return TRUE;
}


void
init_clips_logger(void *env, Logger *logger, Logger *trace_logger)
{
  CLIPSContextMaintainer *cm =
    new CLIPSContextMaintainer(logger, trace_logger, "C");

  SetEnvironmentContext(env, cm);

  EnvAddRouterWithContext(env, (char *)"fawkeslog",
			  /* exclusive */ 50,
			  log_router_query,
			  log_router_print,
			  /* getc */   NULL,
			  /* ungetc */ NULL,
			  log_router_exit,
			  cm->logger);

}


void
finalize_clips_logger(void *env)
{
  CLIPSContextMaintainer *cm =
    static_cast<CLIPSContextMaintainer *>(GetEnvironmentContext(env));

  EnvDeleteRouter(env, (char *)"fawkeslog");
  SetEnvironmentContext(env, NULL);
  delete cm;
}

/// @endcond

} // end of namespace llsfrb
