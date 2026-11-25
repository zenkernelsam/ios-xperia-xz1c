#include "../../gpu/diagnostics/LoadedSnapshotPNGExport.h"
#include <assert.h>
#include <stdio.h>

int main(int argc,char **argv)
{
    @autoreleasepool {
        assert(argc==2);NSString *root=[NSString stringWithUTF8String:argv[1]];
        assert(mkdir(root.fileSystemRepresentation,0700)==0);
        NSString *file=[root stringByAppendingPathComponent:@"aegir-(fixture).png"];
        uint8_t bytes[120]={137,80,78,71,13,10,26,10};for(unsigned i=8;i<sizeof(bytes);++i)bytes[i]=(uint8_t)i;
        NSData *data=[NSData dataWithBytes:bytes length:sizeof(bytes)];assert([data writeToFile:file atomically:NO]);
        int status=117;errno=73;NSData *actual=loaded_snapshot_file(file.fileSystemRepresentation,root.fileSystemRepresentation,&status);
        assert(!status && errno==73 && [actual isEqualToData:data]);
        NSString *link=[root stringByAppendingPathComponent:@"link.png"];assert(symlink(file.fileSystemRepresentation,link.fileSystemRepresentation)==0);
        assert(!loaded_snapshot_file(link.fileSystemRepresentation,root.fileSystemRepresentation,&status) && status<0);
        assert(!loaded_snapshot_file(file.fileSystemRepresentation,"/foreign",&status));
        assert(loaded_snapshot_path("/var/mobile/Library/Caches/aegir-(preview).png"));
        assert(loaded_snapshot_path("/private/var/mobile/Library/Caches/sweet-aegir-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/aegir-(preview).png"));
        assert(!loaded_snapshot_path("/private/var/mobile/Library/Caches/foreign.png"));
        assert(!loaded_snapshot_path("/var/mobile/Library/Caches/../aegir-.png"));
        assert(!loaded_snapshot_path("/var/mobile/Library/Caches/sweet-aegir-z/aegir-.png"));
        puts("loaded snapshot PNG PASS: actual original file bytes preserved, exact cache paths, size/signature/no-follow refusal");
    }
    return 0;
}
