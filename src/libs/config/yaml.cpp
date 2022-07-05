
/***************************************************************************
 *  yaml.cpp - Fawkes configuration stored in one or more YAML files
 *
 *  Created: Wed Aug 01 16:46:13 2012
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

#include "yaml.h"

#include "yaml_node.h"

#include <core/exceptions/software.h>
#include <core/threading/mutex.h>
// include <logging/liblogger.h>
#ifdef HAVE_FAM
#	include <utils/system/fam_thread.h>
#endif

#include <sys/stat.h>
#include <utils/misc/string_split.h>
#include <yaml-cpp/exceptions.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <queue>
#include <regex>
#include <unistd.h>
namespace llsfrb {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

#define YAML_FILE_REGEX "^[a-zA-Z0-9_-]+\\.yaml$"

/** @class YamlConfiguration::YamlValueIterator <config/yaml.h>
 * Iterator for YAML config trees.
 * This iterator is used by YamlConfiguration as a result value
 * for queries. Its use is opaque and knowledge of
 * Configuration::ValueIterator will suffice for interaction.
 * @author Tim Niemueller
 */

/** Constructor.
 * Creates an iterator representing the invalid iterator.
 */
YamlConfiguration::YamlValueIterator::YamlValueIterator() : first_(true)
{
	current_ = nodes_.end();
}

/** Initializing constructor.
 * @param nodes nodes to iterate over
 */
YamlConfiguration::YamlValueIterator::YamlValueIterator(
  std::map<std::string, std::shared_ptr<YamlConfigurationNode>> &nodes)
: first_(true), nodes_(nodes)
{
	current_ = nodes_.begin();
}

bool
YamlConfiguration::YamlValueIterator::next()
{
	if (first_) {
		first_ = false;
	} else {
		++current_;
	}
	return (current_ != nodes_.end());
}

bool
YamlConfiguration::YamlValueIterator::valid() const
{
	return (current_ != nodes_.end());
}

const char *
YamlConfiguration::YamlValueIterator::path() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get path of invalid iterator");
	}
	return current_->first.c_str();
}

const char *
YamlConfiguration::YamlValueIterator::type() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get type of invalid iterator");
	}
	return YamlConfigurationNode::Type::to_string(current_->second->get_type());
}

bool
YamlConfiguration::YamlValueIterator::is_float() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	return (current_->second->is_type<float>());
}

bool
YamlConfiguration::YamlValueIterator::is_uint() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	return (current_->second->is_type<unsigned int>());
}

bool
YamlConfiguration::YamlValueIterator::is_int() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	return (current_->second->is_type<int>());
}

bool
YamlConfiguration::YamlValueIterator::is_bool() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	return (current_->second->is_type<bool>());
}

bool
YamlConfiguration::YamlValueIterator::is_string() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	return (current_->second->is_type<std::string>());
}

bool
YamlConfiguration::YamlValueIterator::is_list() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	return current_->second->get_type() == YamlConfigurationNode::Type::SEQUENCE;
}

size_t
YamlConfiguration::YamlValueIterator::get_list_size() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot check type on invalid iterator");
	}
	if (current_->second->get_type() != YamlConfigurationNode::Type::SEQUENCE) {
		throw fawkes::Exception("YamlValueIterator: cannot get list size of non-list value");
	}
	return current_->second->get_list_size();
}

float
YamlConfiguration::YamlValueIterator::get_float() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_value<float>();
}

unsigned int
YamlConfiguration::YamlValueIterator::get_uint() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_value<unsigned int>();
}

int
YamlConfiguration::YamlValueIterator::get_int() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_value<int>();
}

bool
YamlConfiguration::YamlValueIterator::get_bool() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_value<bool>();
}

std::string
YamlConfiguration::YamlValueIterator::get_string() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_value<std::string>();
}

std::string
YamlConfiguration::YamlValueIterator::get_as_string() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	if (current_->second->get_type() == YamlConfigurationNode::Type::SEQUENCE) {
		return current_->second->get_list_as_string();
	} else {
		return current_->second->get_value<std::string>();
	}
}

std::vector<float>
YamlConfiguration::YamlValueIterator::get_floats() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_list<float>();
}

std::vector<unsigned int>
YamlConfiguration::YamlValueIterator::get_uints() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_list<unsigned int>();
}

std::vector<int>
YamlConfiguration::YamlValueIterator::get_ints() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_list<int>();
}

std::vector<bool>
YamlConfiguration::YamlValueIterator::get_bools() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_list<bool>();
}

std::vector<std::string>
YamlConfiguration::YamlValueIterator::get_strings() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->get_list<std::string>();
}

std::string
YamlConfiguration::YamlValueIterator::get_comment() const
{
	throw fawkes::NotImplementedException("YamlConfig: comments are not available");
}

bool
YamlConfiguration::YamlValueIterator::is_default() const
{
	if (current_ == nodes_.end()) {
		throw fawkes::Exception("YamlValueIterator: cannot get value of invalid iterator");
	}
	return current_->second->is_default();
}

/** @class YamlConfiguration <config/yaml.h>
 * Configuration store using YAML documents.
 * @author Tim Niemueller
 */

/** Constructor. */
YamlConfiguration::YamlConfiguration()
{
	mutex                = new fawkes::Mutex();
	write_pending_       = false;
	write_pending_mutex_ = new fawkes::Mutex();

	sysconfdir_  = NULL;
	userconfdir_ = NULL;
}

/** Constructor.
 * @param sysconfdir system configuration directory, will be searched for
 * default configuration file, and system will try to create host-specific
 * database if writable
 * @param userconfdir user configuration directory, will be searched preferably
 * for default configuration file, and will be used to create host-specific
 * database if sysconfdir is not writable. This directory will be created
 * if it does not exist during load().
 */
YamlConfiguration::YamlConfiguration(const char *sysconfdir, const char *userconfdir)
{
	mutex                = new fawkes::Mutex();
	write_pending_       = false;
	write_pending_mutex_ = new fawkes::Mutex();

	sysconfdir_ = strdup(sysconfdir);

	if (userconfdir != NULL) {
		userconfdir_ = strdup(userconfdir);
	} else {
		const char *homedir = getenv("HOME");
		if (homedir == NULL) {
			userconfdir_ = strdup(sysconfdir);
		} else {
			if (asprintf(&userconfdir_, "%s/%s", homedir, USERDIR) == -1) {
				userconfdir_ = strdup(sysconfdir);
			}
		}
	}

	root_ = std::make_shared<YamlConfigurationNode>();
}

/** Destructor. */
YamlConfiguration::~YamlConfiguration()
{
	if (write_pending_) {
		write_host_file();
	}

	if (sysconfdir_)
		free(sysconfdir_);
	if (userconfdir_)
		free(userconfdir_);
	delete mutex;
	delete write_pending_mutex_;
}

void
YamlConfiguration::load(const char *file_path)
{
	if (file_path == NULL) {
		file_path = "config.yaml";
	}

	std::string filename;
	if (file_path[0] == '/') {
		filename = file_path;
	} else {
		const char *try_paths[]   = {userconfdir_, sysconfdir_};
		int         try_paths_len = 2;

		for (int i = 0; i < try_paths_len; ++i) {
			char *path;
			if (asprintf(&path, "%s/%s", try_paths[i], file_path) != -1) {
				if (access(path, R_OK) == 0) {
					filename = path;
					free(path);
					break;
				}
				free(path);
			}
		}
		if (filename == "") {
			throw fawkes::Exception("YamlConfig: cannot find configuration file %s/%s or %s/%s",
			                        userconfdir_,
			                        file_path,
			                        sysconfdir_,
			                        file_path);
		}
	}

	config_file_ = filename;

	host_file_ = "";
	std::list<std::string> files, dirs;
	read_yaml_config(filename, host_file_, root_, host_root_, files, dirs);

	//root_->print();
}

std::shared_ptr<YamlConfigurationNode>
YamlConfiguration::read_yaml_file(std::string                 filename,
                                  bool                        ignore_missing,
                                  std::queue<LoadQueueEntry> &load_queue,
                                  std::string                &host_file)
{
	if (access(filename.c_str(), R_OK) == -1) {
		if (ignore_missing) {
			return NULL;
		}
		throw fawkes::Exception(errno, "YamlConfig: cannot access file %s", filename.c_str());
	}

	std::vector<YAML::Node> docs;
	bool                    have_doc1 = false, have_doc2 = false;

	try {
		docs      = YAML::LoadAllFromFile(filename);
		have_doc1 = docs.size() > 0;
		have_doc2 = docs.size() > 1;
	} catch (YAML::ParserException &e) {
		throw CouldNotOpenConfigException("Failed to parse %s line %i column %i: %s",
		                                  filename.c_str(),
		                                  e.mark.line,
		                                  e.mark.column,
		                                  e.msg.c_str());
	}

	std::shared_ptr<YamlConfigurationNode> sub_root;

	if (!have_doc1) {
		//throw fawkes::Exception("YamlConfig: file %s contains no document", filename.c_str());
		// empty -> ignore
	} else if (have_doc1 && have_doc2) {
		// we have a meta info and a config document
		read_meta_doc(docs[0], load_queue, host_file);
		sub_root = read_config_doc(docs[1]);

	} else {
		// only one, assume this to be the config document
		sub_root = read_config_doc(docs[0]);
	}

	return sub_root;
}

void
YamlConfiguration::read_yaml_config(std::string                             filename,
                                    std::string                            &host_file,
                                    std::shared_ptr<YamlConfigurationNode> &root,
                                    std::shared_ptr<YamlConfigurationNode> &host_root,
                                    std::list<std::string>                 &files,
                                    std::list<std::string>                 &dirs)
{
	std::queue<LoadQueueEntry> load_queue;
	load_queue.push(LoadQueueEntry(filename, false));

	while (!load_queue.empty()) {
		LoadQueueEntry &qe = load_queue.front();

		if (qe.is_dir) {
			dirs.push_back(qe.filename);
		} else {
			//LibLogger::log_debug("YamlConfiguration",
			//                     "Reading YAML file '%s' (ignore missing: %s)",
			//                     qe.filename.c_str(), qe.ignore_missing ? "yes" : "no");

			std::shared_ptr<YamlConfigurationNode> sub_root =
			  read_yaml_file(qe.filename, qe.ignore_missing, load_queue, host_file);

			if (sub_root) {
				files.push_back(qe.filename);
				*root += sub_root;
			}
		}

		load_queue.pop();
	}

	if (host_file != "") {
		//LibLogger::log_debug("YamlConfiguration",
		//			 "Reading Host YAML file '%s'", host_file.c_str());
		std::queue<LoadQueueEntry> host_load_queue;
		host_root = read_yaml_file(host_file, true, host_load_queue, host_file);
		if (!host_load_queue.empty()) {
			throw CouldNotOpenConfigException("YamlConfig: includes are not allowed "
			                                  "in host document");
		}
		if (host_root) {
			*root += host_root;
			files.push_back(host_file);
		} else {
			host_root = std::make_shared<YamlConfigurationNode>();
		}
	} else {
		host_root = std::make_shared<YamlConfigurationNode>();
	}
}

/** Create absolute config path.
 * If the @p path starts with / it is considered to be absolute. Otherwise
 * it is prefixed with the config directory.
 * @param path path
 * @return absolute path
 */
static std::string
abs_cfg_path(const std::string &path)
{
	if (path[0] == '/') {
		return path;
	} else {
		return std::string(CONFDIR) + "/" + path;
	}
}

/** Replace $host in string with hostname
 * @param prelim preliminary filename (potentially with $host)
 * @return filename with $host replaced with hostname
 */
static std::string
insert_hostname(std::string prelim)
{
	const std::string to_replace = "$host";
	static char      *hostname   = NULL;
	if (hostname == NULL) {
		hostname = new char[256];
		gethostname(hostname, 256);
	}
	size_t repl_position = prelim.find(to_replace);
	if (repl_position == std::string::npos) {
		return prelim;
	} else {
		return prelim.replace(repl_position, to_replace.length(), std::string(hostname));
	}
}

void
YamlConfiguration::read_meta_doc(YAML::Node                 &doc,
                                 std::queue<LoadQueueEntry> &load_queue,
                                 std::string                &host_file)
{
	try {
		const YAML::Node &includes = doc["include"];
		for (YAML::const_iterator it = includes.begin(); it != includes.end(); ++it) {
			std::string include        = insert_hostname(it->as<std::string>());
			bool        ignore_missing = false;
			if (it->Tag() == "tag:fawkesrobotics.org,cfg/ignore-missing") {
				ignore_missing = true;
			}

			if (it->Tag() == "tag:fawkesrobotics.org,cfg/host-specific") {
				if (host_file != "") {
					throw fawkes::Exception("YamlConfig: Only one host-specific file can be specified");
				}

				host_file = abs_cfg_path(insert_hostname(it->Scalar()));
				continue;
			}

			if (include.empty()) {
				throw fawkes::Exception("YamlConfig: invalid empty include");
			}

			if (include[include.size() - 1] == '/') {
				// this should be a directory
				std::string dirname = abs_cfg_path(include);
				struct stat dir_stat;
				if ((stat(dirname.c_str(), &dir_stat) != 0)) {
					if (ignore_missing)
						continue;
					throw fawkes::Exception(errno,
					                        "YamlConfig: Failed to stat directory %s",
					                        dirname.c_str());
				}

				if (!S_ISDIR(dir_stat.st_mode)) {
					throw fawkes::Exception("YamlConfig: %s is not a directory", dirname.c_str());
				}

				DIR *d = opendir(dirname.c_str());
				if (!d) {
					throw fawkes::Exception(errno,
					                        "YamlConfig: failed to open directory %s",
					                        dirname.c_str());
				}

				load_queue.push(LoadQueueEntry(dirname, ignore_missing, true));

				std::list<std::string> files;

				std::regex yaml_regex{YAML_REGEX, std::regex_constants::extended};

				struct dirent *dent;
				while ((dent = readdir(d)) != NULL) {
					if (regex_search(dent->d_name, yaml_regex)) {
						std::string dn = dent->d_name;
						files.push_back(dirname + dn);
					}
				}
				closedir(d);

				files.sort();
				for (std::list<std::string>::iterator f = files.begin(); f != files.end(); ++f) {
					load_queue.push(LoadQueueEntry(*f, ignore_missing));
				}

			} else {
				load_queue.push(LoadQueueEntry(abs_cfg_path(include), ignore_missing));
			}
		}
	} catch (YAML::KeyNotFound &e) {
		//ignored, no includes
	}
}

std::shared_ptr<YamlConfigurationNode>
YamlConfiguration::read_config_doc(const YAML::Node &doc)
{
	return YamlConfigurationNode::create(doc);
}

void
YamlConfiguration::write_host_file()
{
	if (host_file_ == "") {
		throw fawkes::Exception("YamlConfig: no host config file specified");
	}
	if (mutex->try_lock()) {
		try {
			host_root_->emit(host_file_);
			mutex->unlock();
		} catch (...) {
			write_pending_mutex_->unlock();
			mutex->unlock();
			throw;
		}
	} else {
		write_pending_mutex_->lock();
		write_pending_ = true;
		write_pending_mutex_->unlock();
	}
}

void
YamlConfiguration::copy(Configuration *copyconf)
{
	throw fawkes::NotImplementedException("YamlConfig does not support copying of a configuration");
}

bool
YamlConfiguration::exists(const char *path)
{
	try {
		std::shared_ptr<YamlConfigurationNode> n = root_->find(path);
		return !n->has_children();
	} catch (fawkes::Exception &e) {
		return false;
	}
}

std::string
YamlConfiguration::get_type(const char *path)
{
	std::shared_ptr<YamlConfigurationNode> n = root_->find(path);
	if (n->has_children()) {
		throw ConfigEntryNotFoundException(path);
	}

	return YamlConfigurationNode::Type::to_string(n->get_type());
}

std::string
YamlConfiguration::get_comment(const char *path)
{
	return "";
}

/** Retrieve value casted to given type T.
 * @param root root node of the tree to search
 * @param path path to query
 * @return value casted as desired
 * @throw YAML::ScalarInvalid thrown if value does not exist or is of
 * a different type.
 */
template <typename T>
static inline T
get_value_as(std::shared_ptr<YamlConfigurationNode> root, const char *path)
{
	std::shared_ptr<YamlConfigurationNode> n = root->find(path);
	if (n->has_children()) {
		throw ConfigEntryNotFoundException(path);
	}
	return n->get_value<T>();
}

/** Retrieve value casted to given type T.
 * @param root root node of the tree to search
 * @param path path to query
 * @return value casted as desired
 * @throw YAML::ScalarInvalid thrown if value does not exist or is of
 * a different type.
 */
template <typename T>
static inline std::vector<T>
get_list(std::shared_ptr<YamlConfigurationNode> root, const char *path)
{
	std::shared_ptr<YamlConfigurationNode> n = root->find(path);
	if (n->has_children()) {
		throw ConfigEntryNotFoundException(path);
	}
	return n->get_list<T>();
}

float
YamlConfiguration::get_float(const char *path)
{
	return get_value_as<float>(root_, path);
}

unsigned int
YamlConfiguration::get_uint(const char *path)
{
	return get_value_as<unsigned int>(root_, path);
}

int
YamlConfiguration::get_int(const char *path)
{
	return get_value_as<int>(root_, path);
}

bool
YamlConfiguration::get_bool(const char *path)
{
	return get_value_as<bool>(root_, path);
}

std::string
YamlConfiguration::get_string(const char *path)
{
	return get_value_as<std::string>(root_, path);
}

std::vector<float>
YamlConfiguration::get_floats(const char *path)
{
	return get_list<float>(root_, path);
}

std::vector<unsigned int>
YamlConfiguration::get_uints(const char *path)
{
	return get_list<unsigned int>(root_, path);
}

std::vector<int>
YamlConfiguration::get_ints(const char *path)
{
	return get_list<int>(root_, path);
}

std::vector<bool>
YamlConfiguration::get_bools(const char *path)
{
	return get_list<bool>(root_, path);
}

std::vector<std::string>
YamlConfiguration::get_strings(const char *path)
{
	return get_list<std::string>(root_, path);
}

/** Check if value is of given type T.
 * @param root root node of the tree to search
 * @param path path to query
 * @return true if value is of desired type, false otherwise
 */
template <typename T>
static inline bool
is_type(std::shared_ptr<YamlConfigurationNode> root, const char *path)
{
	std::shared_ptr<YamlConfigurationNode> n = root->find(path);
	if (n->has_children()) {
		throw ConfigEntryNotFoundException(path);
	}
	return n->is_type<T>();
}

bool
YamlConfiguration::is_float(const char *path)
{
	return is_type<float>(root_, path);
}

bool
YamlConfiguration::is_uint(const char *path)
{
	std::shared_ptr<YamlConfigurationNode> n = root_->find(path);
	if (n->has_children()) {
		throw ConfigEntryNotFoundException(path);
	}

	if (!n->is_type<unsigned int>())
		return false;

	int v = n->get_value<int>();
	return (v >= 0);
}

bool
YamlConfiguration::is_int(const char *path)
{
	return is_type<int>(root_, path);
}

bool
YamlConfiguration::is_bool(const char *path)
{
	return is_type<bool>(root_, path);
}

bool
YamlConfiguration::is_string(const char *path)
{
	return is_type<std::string>(root_, path);
}

bool
YamlConfiguration::is_list(const char *path)
{
	std::shared_ptr<YamlConfigurationNode> n = root_->find(path);
	if (n->has_children()) {
		throw ConfigEntryNotFoundException(path);
	}
	return (n->get_type() == YamlConfigurationNode::Type::SEQUENCE);
}

std::string
YamlConfiguration::get_default_comment(const char *path)
{
	return "";
}

bool
YamlConfiguration::is_default(const char *path)
{
	return false;
}

Configuration::ValueIterator *
YamlConfiguration::get_value(const char *path)
{
	try {
		std::shared_ptr<YamlConfigurationNode> n = root_->find(path);
		if (n->has_children()) {
			return new YamlValueIterator();
		}
		std::map<std::string, std::shared_ptr<YamlConfigurationNode>> nodes;
		nodes[path] = n;
		return new YamlValueIterator(nodes);
	} catch (ConfigEntryNotFoundException &e) {
		return new YamlValueIterator();
	}
}

void
YamlConfiguration::set_float(const char *path, float f)
{
	root_->set_value(path, f);
	host_root_->set_value(path, f);
	write_host_file();
}

void
YamlConfiguration::set_uint(const char *path, unsigned int uint)
{
	root_->set_value(path, uint);
	host_root_->set_value(path, uint);
	write_host_file();
}

void
YamlConfiguration::set_int(const char *path, int i)
{
	root_->set_value(path, i);
	host_root_->set_value(path, i);
	write_host_file();
}

void
YamlConfiguration::set_bool(const char *path, bool b)
{
	root_->set_value(path, b);
	host_root_->set_value(path, b);
	write_host_file();
}

void
YamlConfiguration::set_string(const char *path, const char *s)
{
	root_->set_value(path, std::string(s));
	host_root_->set_value(path, std::string(s));
	write_host_file();
}

void
YamlConfiguration::set_string(const char *path, std::string &s)
{
	set_string(path, s.c_str());
}

void
YamlConfiguration::set_floats(const char *path, std::vector<float> &f)
{
	root_->set_list(path, f);
	host_root_->set_list(path, f);
	write_host_file();
}

void
YamlConfiguration::set_uints(const char *path, std::vector<unsigned int> &u)
{
	root_->set_list(path, u);
	host_root_->set_list(path, u);
	write_host_file();
}

void
YamlConfiguration::set_ints(const char *path, std::vector<int> &i)
{
	root_->set_list(path, i);
	host_root_->set_list(path, i);
	write_host_file();
}

void
YamlConfiguration::set_bools(const char *path, std::vector<bool> &b)
{
	root_->set_list(path, b);
	host_root_->set_list(path, b);
	write_host_file();
}

void
YamlConfiguration::set_strings(const char *path, std::vector<std::string> &s)
{
	root_->set_list(path, s);
	host_root_->set_list(path, s);
	write_host_file();
}

void
YamlConfiguration::set_strings(const char *path, std::vector<const char *> &s)
{
	root_->set_list(path, s);
	host_root_->set_list(path, s);
	write_host_file();
}

void
YamlConfiguration::set_comment(const char *path, const char *comment)
{
}

void
YamlConfiguration::set_comment(const char *path, std::string &comment)
{
}

void
YamlConfiguration::erase(const char *path)
{
	host_root_->erase(path);
	root_->erase(path);
	write_host_file();
}

void
YamlConfiguration::set_default_float(const char *path, float f)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

void
YamlConfiguration::set_default_uint(const char *path, unsigned int uint)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

void
YamlConfiguration::set_default_int(const char *path, int i)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

void
YamlConfiguration::set_default_bool(const char *path, bool b)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

void
YamlConfiguration::set_default_string(const char *path, const char *s)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

void
YamlConfiguration::set_default_string(const char *path, std::string &s)
{
	set_default_string(path, s.c_str());
}

void
YamlConfiguration::set_default_comment(const char *path, const char *comment)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

void
YamlConfiguration::set_default_comment(const char *path, std::string &comment)
{
	set_default_comment(path, comment.c_str());
}

void
YamlConfiguration::erase_default(const char *path)
{
	throw fawkes::NotImplementedException("YamlConfiguration does not support default values");
}

/** Lock the config.
 * No further changes or queries can be executed on the configuration and will block until
 * the config is unlocked.
 */
void
YamlConfiguration::lock()
{
	mutex->lock();
}

/** Try to lock the config.
 * @see Configuration::lock()
 * @return true, if the lock has been aquired, false otherwise
 */
bool
YamlConfiguration::try_lock()
{
	return mutex->try_lock();
}

/** Unlock the config.
 * Modifications and queries are possible again.
 */
void
YamlConfiguration::unlock()
{
	write_pending_mutex_->lock();
	if (write_pending_) {
		host_root_->emit(host_file_);
		write_pending_ = false;
	}
	write_pending_mutex_->unlock();
	mutex->unlock();
}

void
YamlConfiguration::try_dump()
{
}

Configuration::ValueIterator *
YamlConfiguration::iterator()
{
	std::map<std::string, std::shared_ptr<YamlConfigurationNode>> nodes;
	root_->enum_leafs(nodes);
	return new YamlValueIterator(nodes);
}

Configuration::ValueIterator *
YamlConfiguration::search(const char *path)
{
	std::string            tmp_path = path;
	std::string::size_type tl       = tmp_path.length();
	if ((tl > 0) && (tmp_path[tl - 1] == '/')) {
		tmp_path.resize(tl - 1);
	}
	try {
		std::shared_ptr<YamlConfigurationNode>                        n = root_->find(tmp_path.c_str());
		std::map<std::string, std::shared_ptr<YamlConfigurationNode>> nodes;
		n->enum_leafs(nodes, tmp_path);
		return new YamlValueIterator(nodes);
	} catch (fawkes::Exception &e) {
		return new YamlValueIterator();
	}
}

/** Query node for a specific path.
 * @param path path to retrieve node for
 * @return node representing requested path query result, if the path only
 * consists of collection and path name returns the whole document.
 */
std::shared_ptr<YamlConfigurationNode>
YamlConfiguration::query(const char *path) const
{
	std::queue<std::string> pel_q = fawkes::str_split_to_queue(path);
	return root_->find(pel_q);
}

} // end namespace llsfrb
