/* SPDX-License-Identifier: BSD-2-Clause
 * Use the existing C logger unchanged; preserve the caller's errno. */
#include "../sweetgl/LogWriteAll.h"

int sweet_geo_log_write_remaining(int fd,const void *bytes,size_t count,SweetLogWriter writer)
{
    int saved=errno;
    int result=sweet_log_write_remaining(fd,bytes,count,writer);
    errno=saved;
    return result;
}
