/*
* Copyright (c) 2017 Digi International Inc.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
* REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
* INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
* LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
* OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
* PERFORMANCE OF THIS SOFTWARE.
*
* Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
* =======================================================================
*/

#include "ccimp/ccimp_filesystem.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#if (defined UNIT_TEST)
#define ccimp_fs_error_desc                 ccimp_fs_error_desc_real
#define ccimp_fs_session_error              ccimp_fs_session_error_real
#define ccimp_fs_file_open                  ccimp_fs_file_open_real
#define ccimp_fs_file_read                  ccimp_fs_file_read_real
#define ccimp_fs_file_write                 ccimp_fs_file_write_real
#define ccimp_fs_file_close                 ccimp_fs_file_close_real
#define ccimp_fs_file_seek                  ccimp_fs_file_seek_real
#define ccimp_fs_file_truncate              ccimp_fs_file_truncate_real
#define ccimp_fs_file_remove                ccimp_fs_file_remove_real
#define ccimp_fs_dir_open                   ccimp_fs_dir_open_real
#define ccimp_fs_dir_read_entry             ccimp_fs_dir_read_entry_real
#define ccimp_fs_dir_entry_status           ccimp_fs_dir_entry_status_real
#define ccimp_fs_dir_close                  ccimp_fs_dir_close_real
#define ccimp_fs_hash_alg                   ccimp_fs_hash_alg_real
#define ccimp_fs_hash_file                  ccimp_fs_hash_file_real
#endif

#define MIN_VALUE(a, b)    ((a) < (b) ? (a) : (b))

typedef struct
{
   DIR         * dirp;
   struct dirent dir_entry;
} dir_data_t;

static int app_convert_file_open_mode(int const oflag)
{
#if (CCIMP_FILE_O_RDONLY == O_RDONLY) && (CCIMP_FILE_O_WRONLY == O_WRONLY) && (CCIMP_FILE_O_RDWR == O_RDWR) && \
    (CCIMP_FILE_O_CREAT == O_CREAT)   && (CCIMP_FILE_O_APPEND == O_APPEND) && (CCIMP_FILE_O_TRUNC == O_TRUNC)

    return oflag;
#else
    int result = 0;

    if (oflag & CCIMP_FILE_O_WRONLY) result |= O_WRONLY;
    if (oflag & CCIMP_FILE_O_RDWR)   result |= O_RDWR;
    if (oflag & CCIMP_FILE_O_APPEND) result |= O_APPEND;
    if (oflag & CCIMP_FILE_O_CREAT)  result |= O_CREAT;
    if (oflag & CCIMP_FILE_O_TRUNC)  result |= O_TRUNC;

    if ((oflag & (CCIMP_FILE_O_WRONLY | CCIMP_FILE_O_RDWR)) == 0)
        result |= O_RDONLY;

    return result;
#endif
}

ccimp_status_t ccimp_fs_file_open(ccimp_fs_file_open_t * const file_open_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int const oflag = app_convert_file_open_mode(file_open_data->flags);
    int const fd = open(file_open_data->path, oflag, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH); /* 0664 = Owner RW + Group RW + Others R */

    if (fd < 0)
    {
        file_open_data->errnum = errno;
        status = CCIMP_STATUS_ERROR;
        goto done;
    }

    file_open_data->handle = fd;
done:
    return status;
}

ccimp_status_t ccimp_fs_file_read(ccimp_fs_file_read_t * const file_read_data)
{
   ccimp_status_t status = CCIMP_STATUS_OK;
   int result = read(file_read_data->handle, file_read_data->buffer, file_read_data->bytes_available);
   if (result >= 0)
   {
       file_read_data->bytes_used = result;
   }
   else
   {
       if (errno == EAGAIN)
       {
             status = CCIMP_STATUS_BUSY;
       }
       else
       {
             file_read_data->errnum = errno;
             status = CCIMP_STATUS_ERROR;
       }
   }
  return status;
}

ccimp_status_t ccimp_fs_file_write(ccimp_fs_file_write_t * const file_write_data)
{
   ccimp_status_t status = CCIMP_STATUS_OK;
   int result = write(file_write_data->handle, file_write_data->buffer, file_write_data->bytes_available);
   if (result >= 0)
   {
       file_write_data->bytes_used = result;
   }
   else
   {
       if (errno == EAGAIN)
       {
             status = CCIMP_STATUS_BUSY;
       }
       else
       {
             file_write_data->errnum = errno;
             status = CCIMP_STATUS_ERROR;
       }
   }

   return status;
}

ccimp_status_t ccimp_fs_file_close(ccimp_fs_file_close_t * const file_close_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int result = close(file_close_data->handle);

    if (result < 0)
    {
        file_close_data->errnum = errno;
        status = CCIMP_STATUS_ERROR;
    }

    return status;
}

ccimp_status_t ccimp_fs_file_seek(ccimp_fs_file_seek_t * const file_seek_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int origin;
    off_t offset;
    switch (file_seek_data->origin)
    {
       case CCIMP_SEEK_SET:
          origin = SEEK_SET;
          break;
       case CCIMP_SEEK_END:
          origin = SEEK_END;
          break;
       case CCIMP_SEEK_CUR:
          origin = SEEK_CUR;
          break;
   }
   offset = lseek(file_seek_data->handle, file_seek_data->requested_offset, origin);
   file_seek_data->resulting_offset = (ccimp_file_offset_t) offset;
   if (offset < 0)
   {
       if (errno == EAGAIN)
       {
           status = CCIMP_STATUS_BUSY;
       }
       else
       {
           file_seek_data->errnum = errno;
           status = CCIMP_STATUS_ERROR;
       }
   }

    return status;
}

ccimp_status_t ccimp_fs_file_truncate(ccimp_fs_file_truncate_t * const file_truncate_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int result = ftruncate(file_truncate_data->handle, file_truncate_data->length_in_bytes);
    if (result < 0)
    {
        if (errno == EAGAIN)
        {
            status = CCIMP_STATUS_BUSY;
        }
        else
        {
            file_truncate_data->errnum = errno;
            status = CCIMP_STATUS_ERROR;
        }
    }
    return status;
}

ccimp_status_t ccimp_fs_file_remove(ccimp_fs_file_remove_t * const file_remove_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    int result = unlink(file_remove_data->path);
    if (result < 0)
    {
        if (errno == EAGAIN)
        {
            status = CCIMP_STATUS_BUSY;
        }
        else
        {
            file_remove_data->errnum = errno;
            status = CCIMP_STATUS_ERROR;
        }
    }
    return status;
}

ccimp_status_t ccimp_fs_dir_open(ccimp_fs_dir_open_t * const dir_open_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    DIR * dirp;

    dirp = opendir(dir_open_data->path);
    if (dirp != NULL)
    {
        dir_data_t * dir_data = malloc(sizeof *dir_data);
        if (dir_data != NULL)
        {
            dir_open_data->handle = dir_data;
            dir_data->dirp = dirp;
        }
        else
        {
            closedir(dirp);
            dir_open_data->errnum = ENOMEM;
            status = CCIMP_STATUS_ERROR;
        }
    }
    else
    {
       if (errno == EAGAIN)
       {
             status = CCIMP_STATUS_BUSY;
       }
       else
       {
             dir_open_data->errnum = errno;
             status = CCIMP_STATUS_ERROR;
       }
    }
    return status;
}

ccimp_status_t ccimp_fs_dir_read_entry(ccimp_fs_dir_read_entry_t * const dir_read_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    dir_data_t * dir_data = (dir_data_t *)dir_read_data->handle;
    struct dirent * p_dirent = NULL;
    int error;
    /* This sample does not skip "." and ".." */
    error = readdir_r(dir_data->dirp, &dir_data->dir_entry, &p_dirent);
    if (error != 0)
    {
       if (error == EAGAIN)
       {
             status = CCIMP_STATUS_BUSY;
       }
       else
       {
             dir_read_data->errnum = error;
             status = CCIMP_STATUS_ERROR;
       }
    }
    else if (p_dirent == NULL)
    {
        /* Finished with the directory */
        dir_read_data->entry_name[0] = '\0';
    }
    else
    {
        /* Valid entry, copy the name */
        size_t name_len = strlen(p_dirent->d_name);
        if(name_len < dir_read_data->bytes_available)
        {
            memcpy(dir_read_data->entry_name, p_dirent->d_name, name_len + 1);
        }
        else
        {
            dir_read_data->errnum = ENAMETOOLONG;
            status = CCIMP_STATUS_ERROR;
        }
    }
    return status;
}

ccimp_status_t ccimp_fs_dir_entry_status(ccimp_fs_dir_entry_status_t * const dir_entry_status_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    struct stat statbuf;
    int const result = stat(dir_entry_status_data->path, &statbuf);

    if (result != 0)
    {
        if (errno == EAGAIN)
        {
              status = CCIMP_STATUS_BUSY;
        }
        else
        {
            dir_entry_status_data->status.type  = CCIMP_FS_DIR_ENTRY_UNKNOWN;
            dir_entry_status_data->status.file_size = 0;
            dir_entry_status_data->status.last_modified = 0;
            dir_entry_status_data->errnum = errno;
            status = CCIMP_STATUS_ERROR;
        }
        goto done;
    }

    dir_entry_status_data->status.last_modified = (uint32_t)statbuf.st_mtim.tv_sec;
    if (S_ISDIR(statbuf.st_mode))
    {
       dir_entry_status_data->status.type = CCIMP_FS_DIR_ENTRY_DIR;
    }
    else if (S_ISREG(statbuf.st_mode))
    {
       dir_entry_status_data->status.type = CCIMP_FS_DIR_ENTRY_FILE;
       dir_entry_status_data->status.file_size = (ccimp_file_offset_t) statbuf.st_size;
    }
    else
    {
        dir_entry_status_data->status.type  = CCIMP_FS_DIR_ENTRY_UNKNOWN;
    }

done:
    return status;
}

ccimp_status_t ccimp_fs_dir_close(ccimp_fs_dir_close_t * const dir_close_data)
{
    dir_data_t const * const dir_data = (dir_data_t const * const)dir_close_data->handle;
    closedir(dir_data->dirp);
    free((void *)dir_data);

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_fs_hash_alg(ccimp_fs_get_hash_alg_t * const hash_status_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;

    switch (hash_status_data->hash_alg.requested)
    {
        case CCIMP_FS_HASH_NONE:
        case CCIMP_FS_HASH_MD5:
        case CCIMP_FS_HASH_CRC32:
            hash_status_data->hash_alg.actual = hash_status_data->hash_alg.requested;
            break;
        case CCIMP_FS_HASH_BEST:
            hash_status_data->hash_alg.actual = CCIMP_FS_HASH_MD5;
            break;
    }

    return status;
}

static ccimp_status_t app_calc_md5(char const * const path, uint8_t * hash_value, size_t const hash_value_len);
static ccimp_status_t app_calc_crc32(char const * const path, uint8_t * hash_value, size_t const hash_value_len);

ccimp_status_t ccimp_fs_hash_file(ccimp_fs_hash_file_t * const file_hash_data)
{
    ccimp_status_t status = CCIMP_STATUS_OK;
    switch (file_hash_data->hash_algorithm)
    {
         case CCIMP_FS_HASH_CRC32:
             status = app_calc_crc32(file_hash_data->path, file_hash_data->hash_value, file_hash_data->bytes_requested);
             break;
         case CCIMP_FS_HASH_MD5:
             status = app_calc_md5(file_hash_data->path, file_hash_data->hash_value, file_hash_data->bytes_requested);
             break;
         case CCIMP_FS_HASH_NONE:
         case CCIMP_FS_HASH_BEST:
             break;
    }
    return status;
}

ccimp_status_t ccimp_fs_error_desc(ccimp_fs_error_desc_t * const error_desc_data)
{
    ccimp_fs_errnum_t errnum = error_desc_data->errnum;

    error_desc_data->bytes_used = 0;

    if (errnum != 0)
    {
        char * err_str = strerror(errnum);

        error_desc_data->bytes_used = MIN_VALUE(strlen(err_str), error_desc_data->bytes_available);
        memcpy(error_desc_data->error_string, err_str, error_desc_data->bytes_used);
    }

    switch(errnum)
    {
        case EACCES:
        case EPERM:
        case EROFS:
            error_desc_data->error_status = CCIMP_FS_ERROR_PERMISSION_DENIED;
            break;

        case ENOMEM:
        case ENAMETOOLONG:
            error_desc_data->error_status = CCIMP_FS_ERROR_INSUFFICIENT_MEMORY;
            break;

        case ENOENT:
        case ENODEV:
        case EBADF:
            error_desc_data->error_status = CCIMP_FS_ERROR_PATH_NOT_FOUND;
            break;

        case EINVAL:
        case ENOSYS:
        case ENOTDIR:
        case EISDIR:
            error_desc_data->error_status = CCIMP_FS_ERROR_INVALID_PARAMETER;
            break;

        case ENOSPC:
            error_desc_data->error_status = CCIMP_FS_ERROR_INSUFFICIENT_SPACE;
            break;

        default:
            error_desc_data->error_status = CCIMP_FS_ERROR_UNKNOWN;
            break;
    }

    return CCIMP_STATUS_OK;
}

ccimp_status_t ccimp_fs_session_error(ccimp_fs_session_error_t * const session_error_data)
{
    printf("Session error %d\n", session_error_data->session_error);
    if (session_error_data->imp_context != NULL)
    {
        free(session_error_data->imp_context);
    }

    return CCIMP_STATUS_OK;
}


#include <openssl/md5.h>

#define APP_MD5_BUFFER_SIZE 1024


static ccimp_status_t app_calc_md5(char const * const path, uint8_t * const hash_value, size_t const hash_value_len)
{
    ccimp_status_t status;
    MD5_CTX md5;
    char buf[APP_MD5_BUFFER_SIZE];
    int ret;
    ccapi_bool_t done;
    int const fd = open(path, O_RDONLY);

    assert(fd > 0);

    MD5_Init(&md5);

    done = CCAPI_FALSE;
    do {
        ret = read(fd, buf, sizeof buf);

        if (ret > 0)
        {
            MD5_Update(&md5, buf, ret);
        }
        else if (ret == 0)
        {
            done = CCAPI_TRUE;
        }
        else if (ret == -1)
        {
            if (errno != EAGAIN)
            {
                done = CCAPI_TRUE;
            }
        }
    } while (!done);

    close (fd);

    if (ret == 0)
    {
        MD5_Final(hash_value, &md5);
        status = CCIMP_STATUS_OK;
    }
    else
    {
        memset(hash_value, 0, hash_value_len);
        status = CCIMP_STATUS_ERROR;
    }

    return status;
}


extern int crc32file(char const * const name, uint32_t * const crc, long * const charcnt);

static ccimp_status_t app_calc_crc32(char const * const path, uint8_t * hash_value, size_t const hash_value_len)
{
    ccimp_status_t status;
    uint32_t crc32;
    long char_count;
    size_t i;
    int const retval = crc32file(path, &crc32, &char_count);

    if (retval < 0)
    {
        status = CCIMP_STATUS_ERROR;
        goto done;
    }

    for (i = 0; i < hash_value_len; i++)
    {
        uint32_t const mask = 0xFF000000 >> (8 * i);
        uint8_t const shift =  8 * (hash_value_len - (i + 1));

        hash_value[i] = (crc32 & mask) >> shift;
    }
    status = CCIMP_STATUS_OK;

done:
    return status;
}
