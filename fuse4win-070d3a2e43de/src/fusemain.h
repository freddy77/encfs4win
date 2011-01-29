#ifndef FUSEMAIN_H
#define FUSEMAIN_H

#include "dokan.h"
#include "fuse.h"
#include "utils.h"
#include <string>
#include <vector>
#include <memory>

#define CHECKED(arg) if (0);else {int __res=arg; if (__res<0) return __res;}
#define MAX_READ_SIZE (65536)

class impl_fuse_context;
struct impl_chain_link;

extern CRITICAL_SECTION fuse_mutex;

/*
	This class pushes the impl_fuse_context frame on a thread-local stack,
	this allows to have reentrant FUSE filesystem (if anyone really wants
	it one day...)
*/
class impl_chain_guard
{
	impl_chain_link* cur_link_;
public:
	impl_chain_guard(impl_fuse_context* ctx, int caller_pid);
	~impl_chain_guard();
};
impl_fuse_context* get_cur_impl();


/*
	FUSE filesystem context
*/
class impl_fuse_context
{
	friend class impl_chain_guard;

	struct fuse_operations ops_;
	fuse_conn_info conn_info_;
	void *user_data_;
	bool debug_;

	unsigned int filemask_;
	unsigned int dirmask_;
	const char *fsname_, *volname_;
public:
	impl_fuse_context(const struct fuse_operations *ops, void *user_data, 
		bool debug, unsigned int filemask, unsigned int dirmask,
		const char *fsname, const char *volname);

	bool debug() const {return debug_;}

	////////////////////////////////////Methods///////////////////////////////
	static int cast_from_longlong(LONGLONG src, FUSE_OFF_T *res);

	int do_open_dir(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo);

	int do_open_file(LPCWSTR FileName, DWORD Flags, PDOKAN_FILE_INFO DokanFileInfo);

	int do_create_file(LPCWSTR FileName, DWORD Disposition, DWORD Flags,
		PDOKAN_FILE_INFO DokanFileInfo);

	int convert_flags(DWORD Flags);

	int resolve_symlink(const std::string &name, std::string *res);
	int check_and_resolve(std::string *name);

	struct walk_data
	{
		impl_fuse_context *ctx;
		std::string dirname;
		PDOKAN_FILE_INFO DokanFileInfo;
		PFillFindData delegate;
		std::vector<std::string> getdir_data; //Used only in walk_directory_getdir()
	};
	static int walk_directory(void *buf, const char *name,
		const struct FUSE_STAT *stbuf, FUSE_OFF_T off);
	static int walk_directory_getdir(fuse_dirh_t hndl, const char *name, int type,ino_t ino);

	///////////////////////////////////Delegates//////////////////////////////
	int find_files(LPCWSTR file_name, PFillFindData fill_find_data,
		PDOKAN_FILE_INFO dokan_file_info);

	int open_directory(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info);

	int cleanup(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info);

	int create_directory(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info);

	int delete_directory(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info);

	int create_file(LPCWSTR file_name, DWORD access_mode, DWORD share_mode,
		DWORD creation_disposition, DWORD flags_and_attributes,
		PDOKAN_FILE_INFO dokan_file_info);

	int close_file(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info);

	int read_file(LPCWSTR file_name, LPVOID buffer, DWORD num_bytes_to_read,
		LPDWORD	read_bytes, LONGLONG offset, PDOKAN_FILE_INFO dokan_file_info);

	int write_file(LPCWSTR file_name, LPCVOID buffer, 
		DWORD num_bytes_to_write,LPDWORD num_bytes_written, 
		LONGLONG offset, PDOKAN_FILE_INFO dokan_file_info);

	int flush_file_buffers(LPCWSTR file_name, 
		PDOKAN_FILE_INFO dokan_file_info);

	int get_file_information(LPCWSTR file_name,
		LPBY_HANDLE_FILE_INFORMATION handle_file_information, 
		PDOKAN_FILE_INFO dokan_file_info);

	int delete_file(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info);

	int move_file(LPCWSTR file_name, LPCWSTR new_file_name, 
		BOOL replace_existing, PDOKAN_FILE_INFO dokan_file_info);

	int lock_file(LPCWSTR file_name, LONGLONG byte_offset, LONGLONG length,
		PDOKAN_FILE_INFO dokan_file_info);

	int unlock_file(LPCWSTR file_name, LONGLONG byte_offset, LONGLONG length,
		PDOKAN_FILE_INFO dokan_file_info);

	int set_end_of_file(LPCWSTR	file_name, LONGLONG byte_offset, 
		PDOKAN_FILE_INFO dokan_file_info);

	int set_file_attributes(LPCWSTR	file_name, DWORD file_attributes, 
		PDOKAN_FILE_INFO dokan_file_info);

	int helper_set_time_struct(const FILETIME* filetime, const time_t backup,
		time_t *dest);

	int set_file_time(PCWSTR file_name, const FILETIME* creation_time,
		const FILETIME* last_access_time, const FILETIME* last_write_time,
		PDOKAN_FILE_INFO dokan_file_info);

	int get_disk_free_space(PULONGLONG free_bytes_available, 
		PULONGLONG number_of_bytes, PULONGLONG number_of_free_bytes,
		PDOKAN_FILE_INFO dokan_file_info);

	int get_volume_information(LPWSTR volume_name_buffer,DWORD volume_name_size,
		LPWSTR file_system_name_buffer, DWORD file_system_name_size, 
		PDOKAN_FILE_INFO dokan_file_info);

	int unmount(PDOKAN_FILE_INFO DokanFileInfo);
};

class impl_file_handle
{	
	std::string name_;
	bool is_dir_;
	uint64_t fh_;
public:
	impl_file_handle(const std::string &name, bool is_dir, const fuse_file_info *finfo);

	bool is_dir() const {return is_dir_;}
	int close(const struct fuse_operations *ops);
	fuse_file_info make_finfo();
	const std::string& get_name() const {return name_;}
};

#endif //FUSEMAIN_H
