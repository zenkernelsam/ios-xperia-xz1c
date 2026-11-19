/* Exact image/anchors/empty slots must all match before writes are considered. */
#include "../../gpu/sweetgl/QuartzCoreMatrixLayout.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    uint8_t uuid[16];uint32_t prologue[4],table[6],cases[6];
    memcpy(uuid,sweet_ca_uuid,sizeof(uuid));memcpy(prologue,sweet_ca_prologue,sizeof(prologue));
    memcpy(table,sweet_ca_table_anchor,sizeof(table));memcpy(cases,sweet_ca_cases_anchor,sizeof(cases));
#define MATCH(identity,ic,matrix,mc,vibrant,vc) sweet_ca_matrix_layout_matches(uuid,prologue,table,cases,identity,ic,matrix,mc,vibrant,vc)
    assert(MATCH(sweet_ca_identity_expression,0,NULL,0xde,NULL,0xde));
    for(unsigned i=0;i<16;++i){uuid[i]^=1;assert(!MATCH(sweet_ca_identity_expression,0,NULL,0xde,NULL,0xde));uuid[i]^=1;}
    for(unsigned i=0;i<4;++i){prologue[i]^=1;assert(!MATCH(sweet_ca_identity_expression,0,NULL,0xde,NULL,0xde));prologue[i]^=1;}
    for(unsigned i=0;i<6;++i){table[i]^=1;assert(!MATCH(sweet_ca_identity_expression,0,NULL,0xde,NULL,0xde));table[i]^=1;
        cases[i]^=1;assert(!MATCH(sweet_ca_identity_expression,0,NULL,0xde,NULL,0xde));cases[i]^=1;}
    assert(!MATCH("wrong shader expression",0,NULL,0xde,NULL,0xde));
    assert(!MATCH(sweet_ca_identity_expression,1,NULL,0xde,NULL,0xde));
    assert(!MATCH(sweet_ca_identity_expression,0,"existing",0xde,NULL,0xde));
    assert(!MATCH(sweet_ca_identity_expression,0,NULL,0,NULL,0xde));
    assert(!MATCH(sweet_ca_identity_expression,0,NULL,0xde,"existing",0xde));
    assert(!MATCH(sweet_ca_identity_expression,0,NULL,0xde,NULL,0));
    puts("CA matrix layout PASS: UUID/anchors/expression/nonempty/wrong cases rejected");
    return 0;
}
