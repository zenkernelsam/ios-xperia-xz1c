/* SPDX-License-Identifier: BSD-2-Clause
 * Exact local iOS16.7.16/20H392 QuartzCore layout. Unknown images are refused. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define SWEET_CA_IMAGE UINT64_C(0x188284000)
#define SWEET_CA_SHADER UINT64_C(0x1884b30d8)
#define SWEET_CA_TABLE UINT64_C(0x1d5b48a10)
#define SWEET_CA_CASES UINT64_C(0x18855e1e4)
#define SWEET_CA_ANCHOR_TABLE UINT64_C(0x1884b3838)
#define SWEET_CA_ANCHOR_CASES UINT64_C(0x1884b3e78)
#define SWEET_CA_UUID_TEXT "9e40b2eb260c3af1ae45a1a551a8c1b9"
static const uint8_t sweet_ca_uuid[16]={0x9e,0x40,0xb2,0xeb,0x26,0x0c,0x3a,0xf1,0xae,0x45,0xa1,0xa5,0x51,0xa8,0xc1,0xb9};
static const uint32_t sweet_ca_prologue[4]={0xa9ba6ffc,0xa90167fa,0xa9025ff8,0xa90357f6};
static const uint32_t sweet_ca_table_anchor[6]={0xb026b4a9,0x91284129,0xf8767920,0x5100aac9,0x71000d3f,0x1a882508};
static const uint32_t sweet_ca_cases_anchor[6]={0xf0000549,0x91079129,0x1000008a,0x3868692b,0x8b0b094a,0xd61f0140};
static const char sweet_ca_identity_expression[]="colorMatrix4x4Bias(s0)";
static inline bool sweet_ca_matrix_core_matches(const uint8_t uuid[16],const void *prologue,
    const void *table_anchor,const void *cases_anchor,const char *identity,uint8_t identity_case)
{
    return uuid && prologue && table_anchor && cases_anchor && identity &&
        !memcmp(uuid,sweet_ca_uuid,16) && !memcmp(prologue,sweet_ca_prologue,sizeof(sweet_ca_prologue)) &&
        !memcmp(table_anchor,sweet_ca_table_anchor,sizeof(sweet_ca_table_anchor)) &&
        !memcmp(cases_anchor,sweet_ca_cases_anchor,sizeof(sweet_ca_cases_anchor)) &&
        !memcmp(identity,sweet_ca_identity_expression,sizeof(sweet_ca_identity_expression)) &&
        identity_case==0;
}
static inline bool sweet_ca_matrix_layout_matches(const uint8_t uuid[16],const void *prologue,
    const void *table_anchor,const void *cases_anchor,const char *identity,uint8_t identity_case,
    const char *matrix4,uint8_t matrix4_case,const char *vibrant,uint8_t vibrant_case)
{
    return sweet_ca_matrix_core_matches(uuid,prologue,table_anchor,cases_anchor,identity,identity_case) &&
        !matrix4 && matrix4_case==0xde && !vibrant && vibrant_case==0xde;
}
/* Current slot identity is checked without writing or re-installing anything. */
static inline bool sweet_ca_matrix_installed_matches(bool prior_install_checked,
    const uint8_t uuid[16],const void *prologue,const void *table_anchor,const void *cases_anchor,
    const char *identity,uint8_t identity_case,const char *matrix4,uint8_t matrix4_case,
    const char *vibrant,uint8_t vibrant_case,const char *expected_matrix4,const char *expected_vibrant)
{
    return prior_install_checked && expected_matrix4 && expected_vibrant &&
        sweet_ca_matrix_core_matches(uuid,prologue,table_anchor,cases_anchor,identity,identity_case) &&
        matrix4==expected_matrix4 && matrix4_case==0 && vibrant==expected_vibrant && vibrant_case==0;
}
