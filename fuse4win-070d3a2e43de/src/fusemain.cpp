#include <windows.h>
#include <errno.h>
#include <sys/utime.h>

#include "fusemain.h"
#include "utils.h"

///////////////////////////////////////////////////////////////////////////////////////
////// FUSE frames chain
///////////////////////////////////////////////////////////////////////////////////////
struct impl_chain_link
{
	impl_chain_link *prev_link_;
	fuse_context call_ctx_;
	impl_fuse_context* ctx_;
};

#ifdef _MSC_VER
__declspec(thread) impl_chain_link * cur_impl_chain_link=NULL;
#else
static __thread impl_chain_link * cur_impl_chain_link=NULL;
#endif

impl_chain_guard::impl_chain_guard(impl_fuse_context* ctx, int caller_pid)
{
	fuse_context call_ctx={0};
	call_ctx.pid=caller_pid;
	call_ctx.private_data=ctx->user_data_;
	call_ctx.fuse=(struct fuse*)(void*)ctx; //Hack, really...

	std::auto_ptr<impl_chain_link> link(new impl_chain_link());
	link->ctx_=ctx;
	link->call_ctx_=call_ctx;
	link->prev_link_=cur_impl_chain_link;

	//Push current context on the chain stack.
	//Note, this is thread-safe since we work with a thread-local variable
	cur_link_=link.get();
	cur_impl_chain_link=link.release();
}

impl_chain_guard::~impl_chain_guard()
{
	if (cur_link_!=cur_impl_chain_link)
		abort();//"FUSE frames stack is damaged!"
	cur_impl_chain_link=cur_link_->prev_link_;

	delete cur_link_;
}

impl_fuse_context* get_cur_impl()
{
	if (cur_impl_chain_link==NULL)
		abort();//"Can't find FUSE frame!"
	return cur_impl_chain_link->ctx_;
}

struct fuse_context *fuse_get_context(void)
{
	if (cur_impl_chain_link==NULL)
		return NULL;
	return &cur_impl_chain_link->call_ctx_;
}

///////////////////////////////////////////////////////////////////////////////////////
////// FUSE bridge
///////////////////////////////////////////////////////////////////////////////////////
impl_fuse_context::impl_fuse_context(const struct fuse_operations *ops, 
									 void *user_data, bool debug, 
									 unsigned int filemask, unsigned int dirmask,
									 const char *fsname, const char *volname) 
{
	this->ops_=*ops;
	this->debug_=debug;
	this->filemask_=filemask;
	this->dirmask_=dirmask;
	this->fsname_=fsname;
	this->volname_=volname;
	user_data_=user_data; //Use current user data

	//Reset connection info
	memset(&conn_info_,0,sizeof(fuse_conn_info));
	conn_info_.max_write=ULONG_MAX;
	conn_info_.max_readahead=ULONG_MAX;
	conn_info_.proto_major=FUSE_MAJOR_VERSION;
	conn_info_.proto_minor=FUSE_MINOR_VERSION;

	if (ops_.init)
	{
		//Create a special FUSE frame
		impl_chain_guard guard(this,-1);

		//Run constructor and replace private data
		user_data_=ops_.init(&conn_info_);
	}
}

int impl_fuse_context::cast_from_longlong(LONGLONG src, FUSE_OFF_T *res)
{
#ifndef WIDE_OFF_T
	if (src>LONG_MAX || src<LONG_MIN)
		return -E2BIG;
#endif
	*res=(FUSE_OFF_T)src;
	return 0;
}

int impl_fuse_context::do_open_dir(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo)
{
	if (ops_.opendir)
	{
		fuse_file_info finfo={0};
		std::string fname=unixify(wchar_to_utf8_cstr(FileName));
		CHECKED(ops_.opendir(fname.c_str(),&finfo));

		DokanFileInfo->Context=reinterpret_cast<ULONG64>(new impl_file_handle(fname,true,&finfo));
		return 0;
	}

	DokanFileInfo->Context=0;
	return 0;
}

int impl_fuse_context::do_open_file(LPCWSTR FileName, DWORD Flags,
									PDOKAN_FILE_INFO DokanFileInfo)
{
	if (!ops_.open) return -EINVAL;
	std::string fname=unixify(wchar_to_utf8_cstr(FileName));
	CHECKED(check_and_resolve(&fname));

	fuse_file_info finfo={0};
	//if ((ShareMode & FILE_SHARE_READ) || (ShareMode & FILE_SHARE_DELETE))
	//TODO: add sharing support?	
	finfo.flags=convert_flags(Flags);

	CHECKED(ops_.open(fname.c_str(),&finfo));
	DokanFileInfo->Context=reinterpret_cast<ULONG64>(new impl_file_handle(fname,false,&finfo));
	return 0;
}

int impl_fuse_context::do_create_file(LPCWSTR FileName, DWORD Disposition, DWORD Flags,
									PDOKAN_FILE_INFO DokanFileInfo)
{
	std::string fname=unixify(wchar_to_utf8_cstr(FileName));

	//Create file?
	if (Disposition!=CREATE_NEW && Disposition!=CREATE_ALWAYS && Disposition!=OPEN_ALWAYS)		
		return -ENOENT; //No, we're trying to open an existing file!

	if (!ops_.create)
	{
		//Use mknod+open.
		if (!ops_.mknod || !ops_.open) return -EINVAL;
		CHECKED(ops_.mknod(fname.c_str(),filemask_,0));
		return do_open_file(FileName,Flags, DokanFileInfo);
	}

	fuse_file_info finfo={0};
	finfo.flags=O_CREAT | O_EXCL | convert_flags(Flags); //TODO: these flags should be OK for new files?	
	
	CHECKED(ops_.create(fname.c_str(),filemask_,&finfo));
	DokanFileInfo->Context=reinterpret_cast<ULONG64>(new impl_file_handle(fname,false,&finfo));
	return 0;
}

int impl_fuse_context::convert_flags(DWORD Flags)
{
	bool read=(Flags & GENERIC_READ) || (Flags & GENERIC_EXECUTE);
	bool write=(Flags & GENERIC_WRITE);
	if (read && !write)
		return O_RDONLY;
	if (!read && write)
		return O_WRONLY;
	return O_RDWR;
}

int impl_fuse_context::resolve_symlink(const std::string &name, std::string *res)
{
	if (!ops_.readlink)
		return -EINVAL;

	char buf[MAX_PATH*2]={0};
	CHECKED(ops_.readlink(name.c_str(),buf,MAX_PATH*2));
	if (buf[0]=='/')
		*res=buf;
	else
	{
		//TODO: add full path normalization here
		*res=extract_dir_name(name)+buf;
	}

	return 0;
}

int impl_fuse_context::check_and_resolve(std::string *name)
{
	if (!ops_.getattr)
		return -EINVAL;

	struct FUSE_STAT stat={0};
	CHECKED(ops_.getattr(name->c_str(),&stat));
	if (S_ISLNK(stat.st_mode))
		CHECKED(resolve_symlink(*name, name));

	return 0;
}

int impl_fuse_context::walk_directory(void *buf, const char *name, 
									  const struct FUSE_STAT *stbuf, FUSE_OFF_T off)
{
	walk_data *wd=(walk_data*)buf;
	WIN32_FIND_DATAW find_data={0};	

	utf8_to_wchar_buf(name,find_data.cFileName,MAX_PATH);
	GetShortPathNameW(find_data.cFileName,find_data.cAlternateFileName,MAX_PATH);

	struct FUSE_STAT stat={0};

	if (stbuf!=NULL)
		stat=*stbuf;		
	else
	{
		//No stat buffer - use 'getattr'.
		//TODO: fill directory params here!!!
		if (strcmp(name,".")==0 || strcmp(name,"..")==0) //Special entries
			stat.st_mode|=S_IFDIR;
		else
			CHECKED(wd->ctx->ops_.getattr((wd->dirname+name).c_str(),&stat));
	}

	if (S_ISLNK(stat.st_mode))
	{
		std::string resolved;
		CHECKED(wd->ctx->resolve_symlink(wd->dirname+name,&resolved));		
		CHECKED(wd->ctx->ops_.getattr(resolved.c_str(),&stat));		
	}
	
	convertStatlikeBuf(&stat,name,&find_data);
	return wd->delegate(&find_data,wd->DokanFileInfo);
}

int impl_fuse_context::walk_directory_getdir(fuse_dirh_t hndl, const char *name, int type,ino_t ino)
{
	walk_data *wd=(walk_data*)hndl;
	wd->getdir_data.push_back(name); //Add this name to list
	return 0; //Get more entries
}

int impl_fuse_context::find_files(LPCWSTR file_name, PFillFindData fill_find_data,
			   PDOKAN_FILE_INFO dokan_file_info)
{
	if ((!ops_.readdir&&!ops_.getdir) || !ops_.getattr)
		return -EINVAL;

	std::string fname=unixify(wchar_to_utf8_cstr(file_name));
	CHECKED(check_and_resolve(&fname));

	walk_data wd;
	wd.ctx=this;
	wd.dirname=fname;
	if (*fname.rbegin()!='/')
		wd.dirname.append("/");
	wd.delegate=fill_find_data;
	wd.DokanFileInfo=dokan_file_info;
	
	if (ops_.readdir)
	{
		impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);
		if (hndl!=NULL)
		{
			fuse_file_info finfo(hndl->make_finfo());
			return ops_.readdir(fname.c_str(),&wd,&walk_directory,0,&finfo);
		}
		else
			return ops_.readdir(fname.c_str(),&wd,&walk_directory,0,NULL);
	} else
	{
		CHECKED(ops_.getdir(fname.c_str(),(fuse_dirh_t)&wd,&walk_directory_getdir));
		//Convert returned data The getdir_data array will be filled during getdir() call.
		//We emulate FUSE behavior and do not pass information directly to Dokan 
		//in walk_directory_getdir callback. This can cause excessive network traffic
		//in sshfs because it populates stat buffer cache AFTER calling our callback.
		//See: cache.c file, function cache_dirfill() in SSHFS 2.2
		for(std::vector<std::string>::const_iterator f=wd.getdir_data.begin();
			f!=wd.getdir_data.end();++f)
			CHECKED(walk_directory(&wd,f->c_str(),0,0));
	}

	return 0;
}

int impl_fuse_context::open_directory(LPCWSTR file_name, 
									  PDOKAN_FILE_INFO dokan_file_info)
{
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));	

	if (ops_.opendir)
		return do_open_dir(file_name,dokan_file_info);

	//We don't have opendir(), so the most we can do is make sure
	//that the target is indeed a directory
	struct FUSE_STAT st={0};
	CHECKED(ops_.getattr(fname.c_str(),&st));
	if (S_ISLNK(st.st_mode))
	{
		std::string resolved;
		CHECKED(resolve_symlink(fname,&resolved));	
		CHECKED(ops_.getattr(resolved.c_str(),&st));
	}

	//Not a directory
	if ((st.st_mode&S_IFDIR)!=S_IFDIR)
		return -ENOTDIR;
	
	dokan_file_info->Context=(ULONG64)NULL; //Do not want to attach anything
	return 0; //Use readdir here?
}

int impl_fuse_context::cleanup(LPCWSTR file_name, PDOKAN_FILE_INFO dokan_file_info)
{
	//TODO:
	//There's a subtle race condition possible here. 'Cleanup' is called when the
	//system closes the last handle from user space. However, there might still
	//be outstanding handles from kernel-space. So when userspace tries to
	//make CreateFile call - it might get error because the file is still locked
	//by the kernel space. 
	
	//The one way to solve this is to keep a table of files 'still in flight'
	//and block until the file is closed. We're not doing this yet.
	return 0;
}

int impl_fuse_context::create_directory(LPCWSTR file_name, 
										PDOKAN_FILE_INFO dokan_file_info)
{
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));

	if (!ops_.mkdir)
		return -EINVAL;

	return ops_.mkdir(fname.c_str(),dirmask_);
}

int impl_fuse_context::delete_directory(LPCWSTR file_name, 
										PDOKAN_FILE_INFO dokan_file_info)
{
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));	

	if (!ops_.rmdir || !ops_.getattr)
		return -EINVAL;

	//Make sure directory is NOT opened
	//TODO: potential race here - Unix filesystems typically allow
	//to delete open files and directories.
	impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);
	if (hndl)
		return -EBUSY;

	//A special case: symlinks are deleted by unlink, not rmdir
	struct FUSE_STAT stbuf={0};
	CHECKED(ops_.getattr(fname.c_str(),&stbuf));
	if (S_ISLNK(stbuf.st_mode) && ops_.unlink)
		return ops_.unlink(fname.c_str());

	//Ok, try to rmdir it.
	return ops_.rmdir(fname.c_str());
}

int impl_fuse_context::create_file(LPCWSTR file_name, DWORD access_mode, 
								   DWORD share_mode, DWORD creation_disposition, 
								   DWORD flags_and_attributes, 
								   PDOKAN_FILE_INFO dokan_file_info)
{
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));
	dokan_file_info->Context=0;

	if (!ops_.getattr)
		return -EINVAL;

	struct FUSE_STAT stbuf={0};
	//Check if the target file/directory exists
	if (ops_.getattr(fname.c_str(),&stbuf)<0)
	{
		//Nope.		
		if (dokan_file_info->IsDirectory) 
			return -EINVAL; //We can't create directories using CreateFile
		return do_create_file(file_name, creation_disposition, access_mode,
			dokan_file_info);
	} else
	{		
		if (S_ISLNK(stbuf.st_mode))
		{
			//Get link's target
			CHECKED(resolve_symlink(fname,&fname));
			CHECKED(ops_.getattr(fname.c_str(),&stbuf));
		}

		if ((stbuf.st_mode&S_IFDIR)==S_IFDIR)
		{
			//Existing directory
			//TODO: add access control
			dokan_file_info->IsDirectory=TRUE;
			return do_open_dir(file_name,dokan_file_info);
		} else
		{
			//Existing file
			//Check if we'll be able to truncate or delete the opened file
			//TODO: race condition here?
			if (creation_disposition==CREATE_ALWAYS)
			{
				if (!ops_.unlink) return -EINVAL;
				CHECKED(ops_.unlink(fname.c_str())); //Delete file
				//And create it!
				return do_create_file(file_name,creation_disposition,
					access_mode,dokan_file_info);
			} else if (creation_disposition==TRUNCATE_EXISTING)
			{
				if (!ops_.truncate) return -EINVAL;
				CHECKED(ops_.truncate(fname.c_str(),0));
			} else if (creation_disposition==CREATE_NEW)
			{
				return -EEXIST;
			}

			return do_open_file(file_name,access_mode,dokan_file_info);
		}
	}
}

int impl_fuse_context::close_file(LPCWSTR file_name, 
								  PDOKAN_FILE_INFO dokan_file_info)
{
	impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);

	int flush_err=0;
	if (hndl)
	{
		flush_err=hndl->close(&ops_);
		delete hndl;
	}
	dokan_file_info->Context=0;

	return flush_err;
}

int impl_fuse_context::read_file(LPCWSTR /*file_name*/, LPVOID buffer, DWORD num_bytes_to_read,
			  LPDWORD read_bytes, LONGLONG offset, PDOKAN_FILE_INFO dokan_file_info)
{
	//Please note, that we ifnore file_name here, because it might
	//have been retargeted by a symlink.
	if (!ops_.read) return -EINVAL;

	*read_bytes=0; //Conform to ReadFile semantics	

	impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);
	if (!hndl)
		return -EINVAL;
	if (hndl->is_dir())
		return -EACCES;
	
	FUSE_OFF_T off;
	CHECKED(cast_from_longlong(offset,&off));
	fuse_file_info finfo(hndl->make_finfo());

	DWORD total_read=0;
	while(total_read<num_bytes_to_read)
	{
		DWORD to_read=num_bytes_to_read-total_read;
		if (to_read>MAX_READ_SIZE) to_read=MAX_READ_SIZE;

		int res=ops_.read(hndl->get_name().c_str(),(char*)buffer,to_read, off,&finfo);
		if (res<0) return res; //Error
		if (res==0) break; //End of file reached

		total_read+=res;
		off+=res;
		buffer=(char*)buffer+res;
	}
	//OK!
	*read_bytes=total_read;
	return 0;
}

int impl_fuse_context::write_file(LPCWSTR /*file_name*/, LPCVOID buffer, 
			   DWORD num_bytes_to_write,LPDWORD num_bytes_written, 
			   LONGLONG offset, PDOKAN_FILE_INFO dokan_file_info)
{	
	//Please note, that we ifnore file_name here, because it might
	//have been retargeted by a symlink.

	*num_bytes_written=0; //Conform to ReadFile semantics

	if (!ops_.write) return -EINVAL;

	impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);
	if (!hndl) 
		return -EINVAL;
	if (hndl->is_dir())
		return -EACCES;

	//Clip the maximum write size
	if (num_bytes_to_write>conn_info_.max_write)
		num_bytes_to_write=conn_info_.max_write;

	FUSE_OFF_T off;
	CHECKED(cast_from_longlong(offset,&off));

	fuse_file_info finfo(hndl->make_finfo());
	int res=ops_.write(hndl->get_name().c_str(),(const char*)buffer,
		num_bytes_to_write, off,&finfo);
	if (res<0) return res; //Error

	//OK!
	*num_bytes_written=res;
	return 0;
}

int impl_fuse_context::flush_file_buffers(LPCWSTR /*file_name*/, 
					   PDOKAN_FILE_INFO dokan_file_info)
{
	//Please note, that we ifnore file_name here, because it might
	//have been retargeted by a symlink.
	impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);
	if (!hndl) 
		return -EINVAL;

	if (hndl->is_dir())
	{
		if (!ops_.fsyncdir) return -EINVAL;
		fuse_file_info finfo(hndl->make_finfo());
		return ops_.fsyncdir(hndl->get_name().c_str(),0,&finfo);
	}
	else
	{
		if (!ops_.fsync) return -EINVAL;
		fuse_file_info finfo(hndl->make_finfo());
		return ops_.fsync(hndl->get_name().c_str(),0,&finfo);
	}
}

int impl_fuse_context::get_file_information(LPCWSTR file_name,
						 LPBY_HANDLE_FILE_INFORMATION handle_file_information, 
						 PDOKAN_FILE_INFO dokan_file_info)
{
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));

	if (!ops_.getattr)
		return -EINVAL;

	struct FUSE_STAT st={0};
	CHECKED(ops_.getattr(fname.c_str(),&st));
	if (S_ISLNK(st.st_mode))
	{
		std::string resolved;
		CHECKED(resolve_symlink(fname.c_str(),&resolved));
		CHECKED(ops_.getattr(resolved.c_str(),&st));
	}

	handle_file_information->nNumberOfLinks=st.st_nlink;
	if ((st.st_mode&S_IFDIR)==S_IFDIR)
		dokan_file_info->IsDirectory=TRUE;
	convertStatlikeBuf(&st,fname,handle_file_information);

	return 0;
}

int impl_fuse_context::delete_file(LPCWSTR file_name, 
								   PDOKAN_FILE_INFO dokan_file_info)
{
	if (!ops_.unlink)
		return -EINVAL;

	//Note: we do not try to resolve symlink target
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));
	return ops_.unlink(fname.c_str());
}

int impl_fuse_context::move_file(LPCWSTR file_name, LPCWSTR new_file_name, 
			  BOOL replace_existing, PDOKAN_FILE_INFO dokan_file_info)
{
	if (!ops_.rename || !ops_.getattr) return -EINVAL;

	std::string name=unixify(wchar_to_utf8_cstr(file_name));
	std::string new_name=unixify(wchar_to_utf8_cstr(new_file_name));
	
	struct FUSE_STAT stbuf={0};
	if (ops_.getattr(new_name.c_str(),&stbuf)!=-ENOENT)
	{
		//Cannot delete directory
		if ((stbuf.st_mode&S_IFDIR)!=0) return -EISDIR;
		if (!ops_.unlink) return -EINVAL;
		CHECKED(ops_.unlink(new_name.c_str()));
	}

	return ops_.rename(name.c_str(),new_name.c_str());
}

int impl_fuse_context::lock_file(LPCWSTR file_name, LONGLONG byte_offset, LONGLONG length,
			  PDOKAN_FILE_INFO dokan_file_info)
{
	//Not implemented yet. There's some mismatch between UNIX and Windows locking semantics.
	return -EINVAL;
}

int impl_fuse_context::unlock_file(LPCWSTR file_name, LONGLONG byte_offset, LONGLONG length,
								 PDOKAN_FILE_INFO dokan_file_info)
{
	//Not implemented yet. There's some mismatch between UNIX and Windows locking semantics.
	return -EINVAL;
}

int impl_fuse_context::set_end_of_file(LPCWSTR	file_name, LONGLONG byte_offset, 
					PDOKAN_FILE_INFO dokan_file_info)
{
	FUSE_OFF_T off;
	CHECKED(cast_from_longlong(byte_offset,&off));
	std::string fname=unixify(wchar_to_utf8_cstr(file_name));
	CHECKED(check_and_resolve(&fname));

	impl_file_handle *hndl=reinterpret_cast<impl_file_handle*>(dokan_file_info->Context);
	if (hndl && ops_.ftruncate)
	{
		fuse_file_info finfo(hndl->make_finfo());
		return ops_.ftruncate(hndl->get_name().c_str(),off,&finfo);
	}

	if (!ops_.truncate)
		return -EINVAL;
	return ops_.truncate(fname.c_str(),off);
}

int impl_fuse_context::set_file_attributes(LPCWSTR file_name, DWORD file_attributes, 
						PDOKAN_FILE_INFO dokan_file_info)
{
	//This method is unlikely to be implemented since we do not support 
	//advanced properties
	//TODO: maybe use extended properties of underlying FS?

	//Just return 'success' since returning -EINVAL interferes with modification time 
	//setting from FAR Manager.
	return 0;
}

int impl_fuse_context::helper_set_time_struct(const FILETIME* filetime, const time_t backup,
									   time_t *dest)
{
	if (is_filetime_set(filetime))
		*dest=filetimeToUnixTime(filetime);
	else if (backup!=0)
		*dest=backup;
	else
		return -EINVAL;

	return 0;
}

int impl_fuse_context::set_file_time(PCWSTR file_name, const FILETIME* creation_time,
				  const FILETIME* last_access_time, const FILETIME* last_write_time,
				  PDOKAN_FILE_INFO dokan_file_info)
{
	if (!ops_.utimens && !ops_.utime) return -EINVAL;
	if (!ops_.getattr)
		return -EINVAL;

	std::string fname=unixify(wchar_to_utf8_cstr(file_name));
	CHECKED(check_and_resolve(&fname));

	struct FUSE_STAT st={0};
	CHECKED(ops_.getattr(fname.c_str(),&st));

	if (ops_.utimens)
	{
		struct timespec tv[2]={0};
		//TODO: support nanosecond resolution
		//Access time
		CHECKED(helper_set_time_struct(last_access_time,st.st_atime,&(tv[0].tv_sec)));		
		//Modification time
		CHECKED(helper_set_time_struct(last_write_time,st.st_mtime,&(tv[1].tv_sec)));

		return ops_.utimens(fname.c_str(),tv);
	} else
	{
		struct utimbuf ut={0};
		//Access time
		CHECKED(helper_set_time_struct(last_access_time,st.st_atime,&(ut.actime)));
		//Modification time
		CHECKED(helper_set_time_struct(last_write_time,st.st_mtime,&(ut.modtime)));
		
		return ops_.utime(fname.c_str(),&ut);
	}
}

int impl_fuse_context::get_disk_free_space(PULONGLONG free_bytes_available, 
						PULONGLONG number_of_bytes, PULONGLONG number_of_free_bytes,
						PDOKAN_FILE_INFO dokan_file_info)
{
	if (!ops_.statfs) return -EINVAL;
	
	struct statvfs vfs={0};
	//Why do we need path argument??
	CHECKED(ops_.statfs("",&vfs));

	if (free_bytes_available!=NULL)
		*free_bytes_available=uint64_t(vfs.f_bsize)*vfs.f_bavail;
	if (number_of_free_bytes!=NULL)
		*number_of_free_bytes=uint64_t(vfs.f_bsize)*vfs.f_bfree;
	if (number_of_bytes!=NULL)
		*number_of_bytes=uint64_t(vfs.f_bsize)*vfs.f_blocks;

	return 0;
}

int impl_fuse_context::get_volume_information(LPWSTR volume_name_buffer,DWORD volume_name_size,
						   LPWSTR file_system_name_buffer, DWORD file_system_name_size,
						   PDOKAN_FILE_INFO dokan_file_info)
{
	if(volname_)
		utf8_to_wchar_buf(volname_,volume_name_buffer,volume_name_size);
	else
		utf8_to_wchar_buf(DEFAULT_FUSE_VOLUME_NAME,volume_name_buffer,volume_name_size);

	if(fsname_)
		utf8_to_wchar_buf(fsname_,file_system_name_buffer,file_system_name_size);
	else
		utf8_to_wchar_buf(DEFAULT_FUSE_FILESYSTEM_NAME,file_system_name_buffer,
			file_system_name_size);

	return 0;
}

int impl_fuse_context::unmount(PDOKAN_FILE_INFO	DokanFileInfo)
{
	if (ops_.destroy)
		ops_.destroy(user_data_); //Ignoring result
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
////// File handle
///////////////////////////////////////////////////////////////////////////////////////
impl_file_handle::impl_file_handle(const std::string &name, bool is_dir,
				 const fuse_file_info *finfo) : name_(name), is_dir_(is_dir), 
				 fh_(finfo->fh)
{
}

int impl_file_handle::close(const struct fuse_operations *ops)
{
	int flush_err=0;
	if (is_dir_)
	{
		if (ops->releasedir)
		{
			fuse_file_info finfo(make_finfo());
			ops->releasedir(name_.c_str(),&finfo);
		}
	} else
	{
		if (ops->flush)
		{
			fuse_file_info finfo(make_finfo());
			finfo.flush=1;
			flush_err=ops->flush(name_.c_str(),&finfo);
		}
		if (ops->release) //Ignoring result.
		{
			fuse_file_info finfo(make_finfo());
			ops->release(name_.c_str(),&finfo); //Set open() flags here?
		}
	}
	return flush_err;
}

fuse_file_info impl_file_handle::make_finfo()
{
	fuse_file_info res={0};
	res.fh=fh_;
	return res;
}
