/* Independent host recipe output, never a device proof or GPU substitute. */
#include <stdbool.h>
#include <stdio.h>
#include "../../gpu/diagnostics/MultisampleProbeValues.h"
int main(void)
{
    const unsigned slots[]={0,8,9,10,11};unsigned char bytes[3008];unsigned checked=0,guards=0;
    for(unsigned phase=0;phase<MULTISAMPLE_PHASES;++phase){unsigned samples=multisample_probe_samples(phase);
        for(unsigned i=0;i<5;++i){unsigned slot=slots[i],size=multisample_probe_bytes(samples,slot);multisample_probe_expected(phase,slot,bytes);
            printf("phase=%u slot=%u samples=%u bytes=%u hash=%016llx\n",phase,slot,samples,size,(unsigned long long)multisample_probe_hash(bytes,size));
            checked+=size;guards+=size-17*9*(slot==9?1:4)*(slot>=10?1:samples);}}
    printf("Author MSAA recipe END checked_bytes=%u guard_bytes=%u host_only=1\n",checked,guards);
    return checked!=MULTISAMPLE_CHECKED_BYTES || guards!=MULTISAMPLE_GUARD_BYTES;
}
