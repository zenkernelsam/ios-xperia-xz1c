/* SPDX-License-Identifier: BSD-2-Clause */
static const char sweet_aegir_scope_id[] = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
static const char sweet_aegir_scope_component[] = "sweet-aegir-1234567890abcdef1234567890abcdef";
#include "../../gpu/compat/AegirSnapshotCacheScope.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    @autoreleasepool {
        assert(argc==2);
        NSString *root=[NSString stringWithUTF8String:argv[1]];
        assert(![[NSFileManager defaultManager] fileExistsAtPath:root]);
        assert([[NSFileManager defaultManager] createDirectoryAtPath:root withIntermediateDirectories:NO attributes:nil error:NULL]);
        NSURL *original=[NSURL fileURLWithPath:root isDirectory:YES];
        NSURL *other=[NSURL fileURLWithPath:@"/preserved/second" isDirectory:YES];
        NSArray *urls=@[original,other];
        int result=117;errno=73;
        NSArray *scoped=aegir_scoped_cache_urls(urls,root,sweet_aegir_scope_component,&result);
        assert(!result && errno==73 && scoped!=urls && scoped[1]==other && urls[0]==original);
        NSString *expected=[root stringByAppendingPathComponent:[NSString stringWithUTF8String:sweet_aegir_scope_component]];
        assert([[scoped[0] path] isEqualToString:expected]);
        NSString *old=[root stringByAppendingPathComponent:@"old-original-snapshot.png"];
        assert([@"old original pixels fixture" writeToFile:old atomically:YES encoding:NSUTF8StringEncoding error:NULL]);
        errno=73;result=117;NSArray *again=aegir_scoped_cache_urls(urls,root,sweet_aegir_scope_component,&result);
        assert(!result && errno==73 && [[again[0] path] isEqualToString:expected]);
        assert([[NSString stringWithContentsOfFile:old encoding:NSUTF8StringEncoding error:NULL] isEqualToString:@"old original pixels fixture"]);
        for (NSString *wrong in @[@"/foreign/cache", @"../relative"]) {
            result=117;errno=73;assert(aegir_scoped_cache_urls(urls,wrong,sweet_aegir_scope_component,&result)==urls);
            assert(result<0 && errno==73);
        }
        result=117;assert(aegir_scoped_cache_urls(urls,root,"../bad",&result)==urls && result<0);
        assert(chmod(expected.fileSystemRepresentation,0755)==0);
        result=117;assert(aegir_scoped_cache_urls(urls,root,sweet_aegir_scope_component,&result)==urls && result<0);
        assert(chmod(expected.fileSystemRepresentation,0700)==0);
        const char *link_component="sweet-aegir-ffffffffffffffffffffffffffffffff";
        NSString *link=[root stringByAppendingPathComponent:[NSString stringWithUTF8String:link_component]];
        assert(symlink(root.fileSystemRepresentation,link.fileSystemRepresentation)==0);
        result=117;assert(aegir_scoped_cache_urls(urls,root,link_component,&result)==urls && result<0);

        struct {
            struct mach_header_64 header;
            struct uuid_command uuid;
        } image={0};
        image.header.magic=MH_MAGIC_64;image.header.ncmds=1;image.header.sizeofcmds=sizeof(image.uuid);
        image.uuid.cmd=LC_UUID;image.uuid.cmdsize=sizeof(image.uuid);
        memcpy(image.uuid.uuid,aegir_original_uuid,16);
        Dl_info info={.dli_fname="/root/AegirPoster.appex/AegirPoster",.dli_fbase=&image};
        const void *caller=(const void *)((uintptr_t)&image+0x63b0);
        assert(aegir_scope_identity(caller,&info));
        assert(!aegir_scope_identity((const void *)((uintptr_t)caller+4),&info));
        info.dli_fname="/root/foreign";assert(!aegir_scope_identity(caller,&info));
        info.dli_fname="/root/AegirPoster.appex/AegirPoster";
        image.uuid.uuid[0]^=1;assert(!aegir_scope_identity(caller,&info));image.uuid.uuid[0]^=1;
        image.uuid.cmdsize=8;assert(!aegir_scope_identity(caller,&info));
        puts("Aegir snapshot namespace PASS: original caller/UUID, stable scoped cache, old files/other URLs preserved, wrong root/mode/symlink refused");
    }
    return 0;
}
