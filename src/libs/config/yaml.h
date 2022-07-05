
/***************************************************************************
 *  yaml.h - Fawkes configuration stored in one or more YAML files
 *
 *  Created: Wed Aug 01 15:44:33 2012
 *  Copyright  2006-2012  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#ifndef __CONFIG_YAML_H_
#define __CONFIG_YAML_H_

#include <config/config.h>
#include <yaml-cpp/yaml.h>

#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace llsfrb {

class YamlConfigurationNode;

class YamlConfiguration : public Configuration
{
public:
	YamlConfiguration();
	YamlConfiguration(const char *sysconfdir, const char *userconfdir = NULL);
	virtual ~YamlConfiguration();

	virtual void copy(Configuration *copyconf);

	virtual void load(const char *file_path);

	virtual bool exists(const char *path);
	virtual bool is_float(const char *path);
	virtual bool is_uint(const char *path);
	virtual bool is_int(const char *path);
	virtual bool is_bool(const char *path);
	virtual bool is_string(const char *path);
	virtual bool is_list(const char *path);

	virtual bool is_default(const char *path);

	virtual std::string               get_type(const char *path);
	virtual float                     get_float(const char *path);
	virtual unsigned int              get_uint(const char *path);
	virtual int                       get_int(const char *path);
	virtual bool                      get_bool(const char *path);
	virtual std::string               get_string(const char *path);
	virtual std::vector<float>        get_floats(const char *path);
	virtual std::vector<unsigned int> get_uints(const char *path);
	virtual std::vector<int>          get_ints(const char *path);
	virtual std::vector<bool>         get_bools(const char *path);
	virtual std::vector<std::string>  get_strings(const char *path);
	virtual ValueIterator            *get_value(const char *path);
	virtual std::string               get_comment(const char *path);
	virtual std::string               get_default_comment(const char *path);

	virtual void set_float(const char *path, float f);
	virtual void set_uint(const char *path, unsigned int uint);
	virtual void set_int(const char *path, int i);
	virtual void set_bool(const char *path, bool b);
	virtual void set_string(const char *path, std::string &s);
	virtual void set_string(const char *path, const char *s);
	virtual void set_floats(const char *path, std::vector<float> &f);
	virtual void set_uints(const char *path, std::vector<unsigned int> &uint);
	virtual void set_ints(const char *path, std::vector<int> &i);
	virtual void set_bools(const char *path, std::vector<bool> &b);
	virtual void set_strings(const char *path, std::vector<std::string> &s);
	virtual void set_strings(const char *path, std::vector<const char *> &s);
	virtual void set_comment(const char *path, std::string &comment);
	virtual void set_comment(const char *path, const char *comment);

	virtual void erase(const char *path);

	virtual void set_default_float(const char *path, float f);
	virtual void set_default_uint(const char *path, unsigned int uint);
	virtual void set_default_int(const char *path, int i);
	virtual void set_default_bool(const char *path, bool b);
	virtual void set_default_string(const char *path, std::string &s);
	virtual void set_default_string(const char *path, const char *s);
	virtual void set_default_comment(const char *path, const char *comment);
	virtual void set_default_comment(const char *path, std::string &comment);

	virtual void erase_default(const char *path);

	ValueIterator *iterator();
	ValueIterator *search(const char *path);

	void lock();
	bool try_lock();
	void unlock();

	virtual void try_dump();

public:
	class YamlValueIterator : public Configuration::ValueIterator
	{
	public:
		YamlValueIterator();
		YamlValueIterator(std::map<std::string, std::shared_ptr<YamlConfigurationNode>> &nodes);

		virtual ~YamlValueIterator()
		{
		}
		virtual bool next();
		virtual bool valid() const;

		virtual const char *path() const;
		virtual const char *type() const;

		virtual bool   is_float() const;
		virtual bool   is_uint() const;
		virtual bool   is_int() const;
		virtual bool   is_bool() const;
		virtual bool   is_string() const;
		virtual bool   is_list() const;
		virtual size_t get_list_size() const;

		virtual float                     get_float() const;
		virtual unsigned int              get_uint() const;
		virtual int                       get_int() const;
		virtual bool                      get_bool() const;
		virtual std::string               get_string() const;
		virtual std::vector<float>        get_floats() const;
		virtual std::vector<unsigned int> get_uints() const;
		virtual std::vector<int>          get_ints() const;
		virtual std::vector<bool>         get_bools() const;
		virtual std::vector<std::string>  get_strings() const;
		virtual std::string               get_as_string() const;

		virtual std::string get_comment() const;

		virtual bool is_default() const;

	private:
		bool                                                                    first_;
		std::map<std::string, std::shared_ptr<YamlConfigurationNode>>           nodes_;
		std::map<std::string, std::shared_ptr<YamlConfigurationNode>>::iterator current_;
	};

private:
	/// @cond INTERNALS
	class LoadQueueEntry
	{
	public:
		LoadQueueEntry(std::string fn, bool im, bool id = false)
		: filename(fn), ignore_missing(im), is_dir(id)
		{
		}

		std::string filename;
		bool        ignore_missing;
		bool        is_dir;
	};
	/// @endcond

	std::shared_ptr<YamlConfigurationNode> query(const char *path) const;
	void
	read_meta_doc(YAML::Node &doc, std::queue<LoadQueueEntry> &load_queue, std::string &host_file);
	std::shared_ptr<YamlConfigurationNode> read_config_doc(const YAML::Node &doc);
	std::shared_ptr<YamlConfigurationNode> read_yaml_file(std::string                 filename,
	                                                      bool                        ignore_missing,
	                                                      std::queue<LoadQueueEntry> &load_queue,
	                                                      std::string                &host_file);
	void                                   read_yaml_config(std::string                             filename,
	                                                        std::string                            &host_file,
	                                                        std::shared_ptr<YamlConfigurationNode> &root,
	                                                        std::shared_ptr<YamlConfigurationNode> &host_root,
	                                                        std::list<std::string>                 &files,
	                                                        std::list<std::string>                 &dirs);
	void                                   write_host_file();

	std::string config_file_;
	std::string host_file_;

	std::shared_ptr<YamlConfigurationNode> root_;
	std::shared_ptr<YamlConfigurationNode> host_root_;

	bool           write_pending_;
	fawkes::Mutex *write_pending_mutex_;

private:
	fawkes::Mutex *mutex;

	typedef std::map<std::string, YAML::Node *> DocMap;
	mutable DocMap                              documents_;

	char *sysconfdir_;
	char *userconfdir_;
};

} // end namespace llsfrb

#endif
