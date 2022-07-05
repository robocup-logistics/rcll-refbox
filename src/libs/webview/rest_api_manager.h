
/***************************************************************************
 *  rest_api_manager.h - Webview REST API manager
 *
 *  Created: Fri Mar 16 16:42:36 2018
 *  Copyright  2006-2018  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#ifndef _LIBS_WEBVIEW_REST_API_MANAGER_H_
#define _LIBS_WEBVIEW_REST_API_MANAGER_H_

#include <core/threading/mutex.h>

#include <map>
#include <string>

namespace fawkes {

class Mutex;
class WebviewRestApi;

class WebviewRestApiManager
{
public:
	WebviewRestApiManager();
	~WebviewRestApiManager();

	void register_api(WebviewRestApi *api);
	void unregister_api(WebviewRestApi *api);

	WebviewRestApi *get_api(std::string &name);
	Mutex          &mutex();

private:
	Mutex                                   mutex_;
	std::map<std::string, WebviewRestApi *> apis_;
};

} // end namespace fawkes

#endif
