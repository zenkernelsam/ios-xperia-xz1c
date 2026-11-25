/* SPDX-License-Identifier: BSD-2-Clause
 * Use a GPU-source namespace only at the original Aegir SnapshotManager caller. */
#pragma once
#import <Foundation/Foundation.h>
#include <objc/runtime.h>
#include <dlfcn.h>
#include <mach-o/loader.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

typedef void (*SweetAegirScopeEmit)(const char *, ...);
static SweetAegirScopeEmit aegir_scope_emit;
static IMP aegir_scope_original;
static atomic_uint aegir_scope_calls;
static const uint8_t aegir_original_uuid[16] = {
    0xfc,0x55,0x40,0xca,0x94,0x40,0x3f,0x16,0xb3,0x65,0xdc,0x94,0xe0,0x70,0x1f,0xda
};

static bool aegir_scope_identity(const void *caller, const Dl_info *info)
{
    if (!caller || !info || !info->dli_fbase || !info->dli_fname ||
        (uintptr_t)caller - (uintptr_t)info->dli_fbase != 0x63b0 ||
        !strstr(info->dli_fname, "/AegirPoster.appex/AegirPoster")) return false;
    const struct mach_header_64 *mh = info->dli_fbase;
    if (mh->magic != MH_MAGIC_64 || mh->ncmds > 128 || mh->sizeofcmds > 65536) return false;
    const uint8_t *at = (const uint8_t *)(mh+1), *end = at + mh->sizeofcmds;
    unsigned matched = 0;
    for (uint32_t i = 0; i < mh->ncmds; ++i) {
        if (end-at < (ptrdiff_t)sizeof(struct load_command)) return false;
        const struct load_command *command = (const void *)at;
        if (command->cmdsize < sizeof(*command) || command->cmdsize > (uintptr_t)(end-at)) return false;
        if (command->cmd == LC_UUID) {
            if (command->cmdsize != sizeof(struct uuid_command) ||
                memcmp(((const struct uuid_command *)command)->uuid, aegir_original_uuid, 16)) return false;
            ++matched;
        }
        at += command->cmdsize;
    }
    return at == end && matched == 1;
}
static bool aegir_scope_caller(const void *caller)
{
    Dl_info info = {0};
    return caller && dladdr(caller,&info) && aegir_scope_identity(caller,&info);
}
static NSArray *aegir_scoped_cache_urls(NSArray *urls, NSString *allowed_root, const char *component, int *status)
{
    int incoming = errno; int result = -EINVAL; NSArray *answer = urls;
    @try {
        if (![urls isKindOfClass:[NSArray class]] || !urls.count || !component ||
            strncmp(component,"sweet-aegir-",12) || strlen(component)!=44) return urls;
        for (unsigned i=12; i<44; ++i) if (!((component[i]>='0' && component[i]<='9') ||
            (component[i]>='a' && component[i]<='f'))) return urls;
        id first = urls[0];
        if (![first isKindOfClass:[NSURL class]] || ![(NSURL *)first isFileURL] ||
            ![[(NSURL *)first path] isEqualToString:allowed_root]) return urls;
        NSString *path = [allowed_root stringByAppendingPathComponent:[NSString stringWithUTF8String:component]];
        const char *bytes = path.fileSystemRepresentation;
        if (!bytes || strlen(bytes)>512) return urls;
        bool created = mkdir(bytes,0700)==0;
        if (!created && errno!=EEXIST) { result=-errno; return urls; }
        struct stat before;
        if (lstat(bytes,&before) || !S_ISDIR(before.st_mode) || before.st_uid!=geteuid() ||
            (before.st_mode&07777)!=0700) return urls;
        int fd=open(bytes,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
        if (fd<0) { result=-errno; return urls; }
        struct stat actual; bool valid=!fstat(fd,&actual) && S_ISDIR(actual.st_mode) &&
            before.st_dev==actual.st_dev && before.st_ino==actual.st_ino;
        close(fd); if (!valid) return urls;
        NSMutableArray *scoped = [[urls mutableCopy] autorelease];
        scoped[0]=[NSURL fileURLWithPath:path isDirectory:YES];
        answer=scoped; result=0;
    } @finally {
        if (status) *status=result;
        errno=incoming;
    }
    return answer;
}
static NSArray *aegir_scope_urls(id receiver, SEL selector, NSSearchPathDirectory directory, NSSearchPathDomainMask domain)
{
    const void *caller = __builtin_return_address(0);
    NSArray *urls = ((NSArray *(*)(id,SEL,NSSearchPathDirectory,NSSearchPathDomainMask))aegir_scope_original)(
        receiver,selector,directory,domain);
    int outgoing = errno;
    if (directory!=NSCachesDirectory || domain!=NSUserDomainMask || !aegir_scope_caller(caller)) {
        errno=outgoing; return urls;
    }
    unsigned call=atomic_fetch_add_explicit(&aegir_scope_calls,1,memory_order_relaxed)+1;
    int result=-EINVAL; NSArray *answer=urls;
    @try {
        NSString *path=[urls isKindOfClass:[NSArray class]] && urls.count &&
            [urls[0] isKindOfClass:[NSURL class]] ? [(NSURL *)urls[0] path] : nil;
        NSString *root=[path isEqualToString:@"/var/mobile/Library/Caches"] ? @"/var/mobile/Library/Caches" :
            [path isEqualToString:@"/private/var/mobile/Library/Caches"] ? @"/private/var/mobile/Library/Caches" : nil;
        if (root) answer=aegir_scoped_cache_urls(urls,root,sweet_aegir_scope_component,&result);
        if (call<=8) {
            aegir_scope_emit("SWEET ASCOPE CALL pid=%d call=%u result=%d original_caller=1 old_files_preserved=1",getpid(),call,result);
            if (!result) aegir_scope_emit("SWEET ASCOPE ROOT pid=%d call=%u path=%s mode=0700",
                getpid(),call,[answer[0] fileSystemRepresentation]);
        }
    } @finally { errno=outgoing; }
    return answer;
}
static bool aegir_snapshot_cache_scope_observe(Class owner, SweetAegirScopeEmit emit)
{
    int incoming=errno; aegir_scope_emit=emit;
    SEL selector=sel_registerName("URLsForDirectory:inDomains:");
    bool valid=owner && !strcmp(class_getName(owner),"NSFileManager") && !class_isMetaClass(owner);
    Method method=valid?class_getInstanceMethod(owner,selector):NULL;
    unsigned total=0;Method *list=valid?class_copyMethodList(owner,&total):NULL;bool owned=false;
    for(unsigned i=0;i<total;++i)if(method_getName(list[i])==selector)owned=true;
    free(list);char types[24]={0};
    if(method)method_getReturnType(method,types,sizeof(types));
    valid=method && owned && method_getNumberOfArguments(method)==4 && !strcmp(types,@encode(id));
    for(unsigned i=0;valid && i<2;++i){method_getArgumentType(method,i+2,types,sizeof(types));valid=!strcmp(types,@encode(NSUInteger));}
    IMP prior=valid?method_getImplementation(method):NULL;
    valid=valid && prior && prior!=(IMP)aegir_scope_urls && !aegir_scope_original;
    if(valid){aegir_scope_original=prior;method_setImplementation(method,(IMP)aegir_scope_urls);}
    if(emit) {
        emit("SWEET ASCOPE ID pid=%d value=%s",getpid(),sweet_aegir_scope_id);
        emit("SWEET ASCOPE CONFIG pid=%d revision=1 installed=%u caller=00000000000063b0 old_files_preserved=1 other_calls_original=1",getpid(),valid);
    }
    errno=incoming;return valid;
}
