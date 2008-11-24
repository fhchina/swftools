/* pool.c

   Routines for handling Flash2 AVM2 ABC contantpool entries.

   Extension module for the rfxswf library.
   Part of the swftools package.

   Copyright (c) 2008 Matthias Kramm <kramm@quiss.org>
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <assert.h>
#include "pool.h"


// ----------------------------- float ----------------------------------

void* float_clone(const void*_v) {
    if(_v==0) 
        return 0;
    const double*v1=_v;
    double*v2 = malloc(sizeof(double));
    *v2 = *v1;
    return v2;
}
unsigned int float_hash(const void*_v) {
    if(!_v)
        return 0;
    const unsigned char*b=_v;
    unsigned int h=0;
    int t;
    for(t=0;t<8;t++)
        h = crc32_add_byte(h, b[t]);
    return h;
}
void float_destroy(void*_v) {
    double*v = (double*)_v;
    if(v)
        free(v);
}
char float_equals(const void*_v1, const void*_v2) {
    const double*v1=_v1;
    const double*v2=_v2;
    if(!v1 || !v2) 
        return v1==v2;
    return *v1==*v2;
}

type_t float_type = {
    dup: float_clone,
    hash: float_hash,
    free: float_destroy,
    equals: float_equals
};

// ----------------------------- uint ----------------------------------

unsigned int undefined_uint = 0;

void*uint_clone(const void*_v) {
    if(!_v)
        return 0;
    const unsigned int*v1=_v;
    unsigned int*v2 = malloc(sizeof(unsigned int));
    *v2 = *v1;
    return v2;
}
unsigned int uint_hash(const void*_v) {
    if(!_v)
        return 0;
    const unsigned int*v=_v;
    return *v;
}
void uint_destroy(void*_v) {
    unsigned int*v = (unsigned int*)_v;
    if(v)
        free(v);
}
char uint_equals(const void*_v1, const void*_v2) {
    const unsigned int*v1=_v1;
    const unsigned int*v2=_v2;
    if(!v1 || !v2)
        return v1==v2;
    return *v1==*v2;
}

type_t uint_type = {
    dup: (dup_func)uint_clone,
    hash: (hash_func)uint_hash,
    free: (free_func)uint_destroy,
    equals: (equals_func)uint_equals
};

// ----------------------------- namespace ----------------------------------

unsigned int namespace_hash(namespace_t*n)
{
    if(!n)
        return 0;
    unsigned int hash = 0;
    hash = crc32_add_byte(hash, n->access);
    hash = crc32_add_string(hash, n->name);
    return hash;
}

unsigned char namespace_equals(const namespace_t*n1, const namespace_t*n2)
{
    if(!n1 || !n2)
        return n1==n2;
    if(n1->access != n2->access)
        return 0;
    if(!(n1->name) != !(n2->name))
        return 0;
    if(n1->name && n2->name && strcmp(n1->name, n2->name))
        return 0;
    return 1;
}

char*escape_string(const char*str)
{
    if(!str)
        return strdup("NULL");
    int len=0;
    unsigned const char*s=str;
    while(*s) {
        if(*s<10) {
            len+=2; // \d
        } else if(*s<32) {
            len+=3; // \dd
        } else if(*s<127) {
            len++;
        } else {
            len+=4; // \xhh
        }
        s++;
    }
    char*newstr = malloc(len+1);
    char*dest = newstr;
    s=str;
    while(*s) {
        if(*s<9) {
            dest+=sprintf(dest, "\\%d", *s);
        } else if(*s<32) {
            if(*s==13)
                dest+=sprintf(dest, "\\r");
            else if(*s==10) 
                dest+=sprintf(dest, "\\n");
            else if(*s==9) 
                dest+=sprintf(dest, "\\t");
            else 
                dest+=sprintf(dest, "\\%2o", *s);
        } else if(*s<127) {
            *dest++=*s;
        } else {
            dest+=sprintf(dest, "\\x%02x", *s);
        }
        s++;
    }
    *dest = 0;
    return newstr;
}

char* namespace_to_string(namespace_t*ns)
{
    if(!ns)
        return strdup("NULL");
    char*access = 0;
    U8 type = ns->access;
    access = access2str(type);
    char*s = escape_string(ns->name);
    char*string = (char*)malloc(strlen(access)+strlen(s)+3);
    int l = sprintf(string, "[%s]%s", access, s);
    free(s);
    return string;
}

namespace_t* namespace_clone(namespace_t*other)
{
    if(!other)
        return 0;
    NEW(namespace_t,n);
    n->access = other->access;
    n->name = other->name?strdup(other->name):0;
    return n;
}

namespace_t* namespace_fromstring(const char*name)
{
    namespace_t*ns = malloc(sizeof(namespace_t));
    memset(ns, 0, sizeof(namespace_t));
    if(name[0] == '[') {
        U8 access;
        char*n = strdup(name);
        char*bracket = strchr(n, ']');
        if(bracket) {
            *bracket = 0;
            char*a = n+1;
            name += (bracket-n)+1;
            if(!strcmp(a, "")) access=0x16;
            else if(!strcmp(a, "undefined")) access=0x08; // public??
            else if(!strcmp(a, "package")) access=0x16;
            else if(!strcmp(a, "packageinternal")) access=0x17;
            else if(!strcmp(a, "protected")) access=0x18;
            else if(!strcmp(a, "explicit")) access=0x19;
            else if(!strcmp(a, "staticprotected")) access=0x1a;
            else if(!strcmp(a, "private")) access=0x05;
            else {
                fprintf(stderr, "Undefined access level: [%s]\n", a);
                free(n);
                return 0;
            }
        }
        ns->access = access;
        ns->name = strdup(name);
        free(n);
        return ns;
    } else {
        ns->access = 0x16;
        ns->name = strdup(name);
        return ns;
    }
}

namespace_t* namespace_new(U8 access, const char*name)
{
    namespace_t*ns = malloc(sizeof(namespace_t));
    ns->access = access;
    /* not sure what namespaces with empty strings are good for, but they *do* exist */
    ns->name = name?strdup(name):0;
    return ns;
}
namespace_t* namespace_new_undefined(const char*name) {
    return namespace_new(0x08, name); // public?
}
namespace_t* namespace_new_package(const char*name) {
    return namespace_new(0x16 , name);
}
namespace_t* namespace_new_packageinternal(const char*name) {
    return namespace_new(0x17, name);
}
namespace_t* namespace_new_protected(const char*name) {
    return namespace_new(0x18, name);
}
namespace_t* namespace_new_explicit(const char*name) {
    return namespace_new(0x19, name);
}
namespace_t* namespace_new_staticprotected(const char*name) {
    return namespace_new(0x1a, name);
}
namespace_t* namespace_new_private(const char*name) {
    return namespace_new(0x05, name);
}

void namespace_destroy(namespace_t*n)
{
    if(n) {
        free(n->name);n->name=0;
        n->access=0x00;
        free(n);
    }
}

type_t namespace_type = {
    dup: (dup_func)namespace_clone,
    hash: (hash_func)namespace_hash,
    free: (free_func)namespace_destroy,
    equals: (equals_func)namespace_equals
};

// ---------------------------namespace sets --------------------------------

unsigned int namespace_set_hash(namespace_set_t*set)
{
    if(!set)
        return 0;
    namespace_list_t*l = set->namespaces;
    unsigned int hash = 0;
    while(l) {
        hash = crc32_add_byte(hash, l->namespace->access);
        hash = crc32_add_string(hash, l->namespace->name);
        l = l->next;
    }
    return hash;
}

int namespace_set_equals(namespace_set_t*m1, namespace_set_t*m2)
{
    if(!m1 || !m2)
        return m1==m2;
    namespace_list_t*l1 = m1->namespaces;
    namespace_list_t*l2 = m2->namespaces;
    while(l1 && l2) {
        if(l1->namespace->access != l2->namespace->access)
            return 0;
        if(!(l1->namespace->name) != !(l2->namespace->name))
            return 0;
        if(l1->namespace->name && l2->namespace->name && strcmp(l1->namespace->name, l2->namespace->name))
            return 0;
        l1 = l1->next;
        l2 = l2->next;
    }
    if(l1||l2)
        return 0;
    return 1;
}

namespace_set_t* namespace_set_clone(namespace_set_t*other)
{
    if(!other)
        return 0;
    NEW(namespace_set_t,set);
    set->namespaces = list_new();
    namespace_list_t*l = other->namespaces;
    while(l) {
        list_append(set->namespaces, namespace_clone(l->namespace));
        l = l->next;
    }
    return set;
}
namespace_set_t* namespace_set_new()
{
    NEW(namespace_set_t,set);
    set->namespaces = list_new();
    return set;
}
char* namespace_set_to_string(namespace_set_t*set)
{
    if(!set)
        return strdup("NULL");
    /* TODO: is the order of the namespaces important (does it
       change the lookup order?). E.g. flex freely shuffles namespaces
       around.
       If the order is not important, we can optimize constant pools by sorting
       the namespaces.
    */
    int l = 0;
    namespace_list_t*lns = set->namespaces;
    while(lns) {
        char*s = namespace_to_string(lns->namespace);
        l += strlen(s)+1;
        free(s);
        lns = lns->next;
    }
    char*desc = malloc(l+16);
    strcpy(desc, "{");
    lns = set->namespaces;
    while(lns) {
        char*s = namespace_to_string(lns->namespace);
        strcat(desc, s);
        free(s);
        lns = lns->next;
        if(lns)
            strcat(desc, ",");
    }
    strcat(desc, "}");
    return desc;
}

void namespace_set_destroy(namespace_set_t*set)
{
    if(set) {
        namespace_list_t*l = set->namespaces;
        while(l) {
            namespace_destroy(l->namespace);l->namespace=0;
            l = l->next;
        }
        list_free(set->namespaces);
        free(set);
    }
}

type_t namespace_set_type = {
    dup: (dup_func)namespace_set_clone,
    hash: (hash_func)namespace_set_hash,
    free: (free_func)namespace_set_destroy,
    equals: (equals_func)namespace_set_equals
};

// ----------------------------- multiname ----------------------------------

unsigned int multiname_hash(multiname_t*m)
{
    if(!m)
        return 0;
    unsigned int hash = crc32_add_byte(0, m->type);
    if(m->name) {
        hash = crc32_add_string(hash, m->name);
    }
    if(m->ns) {
        hash = crc32_add_byte(hash, m->ns->access);
        hash = crc32_add_string(hash, m->ns->name);
    }
    if(m->namespace_set) {
        namespace_list_t*l = m->namespace_set->namespaces;
        while(l) {
            hash = crc32_add_byte(hash, l->namespace->access);
            hash = crc32_add_string(hash, l->namespace->name);
            l = l->next;
        }
    }
    return hash;
}

int multiname_equals(multiname_t*m1, multiname_t*m2)
{
    if(!m1 || !m2)
        return m1==m2;
    if(m1->type!=m2->type)
        return 0;

    if((!m1->name) != (!m2->name))
        return 0;
    if((!m1->ns) != (!m2->ns))
        return 0;
    if((!m1->namespace_set) != (!m2->namespace_set))
        return 0;

    if(m1->name && m2->name && strcmp(m1->name,m2->name))
        return 0;
    if(m1->ns && m2->ns) {
        if(!namespace_equals(m1->ns, m2->ns))
            return 0;
    }
    if(m1->namespace_set && m2->namespace_set) {
        if(!namespace_set_equals(m1->namespace_set, m2->namespace_set))
            return 0;
    }
    return 1;
}

multiname_t* multiname_new(namespace_t*ns, const char*name)
{
    NEW(multiname_t,m);
    m->type = QNAME;
    m->ns = namespace_clone(ns);
    m->name = strdup(name);
    return m;
}

multiname_t* multiname_clone(multiname_t*other)
{
    if(!other)
        return 0;
    NEW(multiname_t,m);
    m->type = other->type;
    if(other->ns)
        m->ns = namespace_clone(other->ns);
    if(other->namespace_set)
        m->namespace_set = namespace_set_clone(other->namespace_set);
    if(other->name)
        m->name = strdup(other->name);
    return m;
}


char* access2str(int type)
{
    if(type==0x08) return "access08";
    else if(type==0x16) return "package";
    else if(type==0x17) return "packageinternal";
    else if(type==0x18) return "protected";
    else if(type==0x19) return "explicit";
    else if(type==0x1A) return "staticprotected";
    else if(type==0x05) return "private";
    else {
        fprintf(stderr, "Undefined access type %02x\n", type);
        return "undefined";
    }
}


char multiname_late_namespace(multiname_t*m)
{
    if(!m)
        return 0;
    return (m->type==RTQNAME || m->type==RTQNAMEA ||
            m->type==RTQNAMEL || m->type==RTQNAMELA);
}

char multiname_late_name(multiname_t*m)
{
    if(!m)
        return 0;
    return m->type==RTQNAMEL || m->type==RTQNAMELA ||
           m->type==MULTINAMEL || m->type==MULTINAMELA;
}

char* multiname_to_string(multiname_t*m)
{
    char*mname = 0;
    if(!m)
        return strdup("NULL");
    if(m->type==0xff)
        return strdup("--<MULTINAME 0xff>--");

    char*name = m->name?escape_string(m->name):strdup("*");
    int namelen = strlen(name);

    if(m->type==QNAME || m->type==QNAMEA) {
        char*nsname = escape_string(m->ns->name);
        mname = malloc(strlen(nsname)+namelen+32);
        strcpy(mname, "<q");
        if(m->type == QNAMEA)
            strcat(mname, ",attr");
        strcat(mname, ">[");
        strcat(mname,access2str(m->ns->access));
        strcat(mname, "]");
        strcat(mname, nsname);
        free(nsname);
        strcat(mname, "::");
        strcat(mname, name);
    } else if(m->type==RTQNAME || m->type==RTQNAMEA) {
        mname = malloc(namelen+32);
        strcpy(mname, "<rt");
        if(m->type == RTQNAMEA) 
            strcat(mname, ",attr");
        strcat(mname, ">");
        strcat(mname, name);
    } else if(m->type==RTQNAMEL) {
        mname = strdup("<rt,l>");
    } else if(m->type==RTQNAMELA) {
        mname = strdup("<rt,l,attr>");
    } else if(m->type==MULTINAME || m->type==MULTINAMEA) {
        char*s = namespace_set_to_string(m->namespace_set);
        mname = malloc(strlen(s)+namelen+16);
        if(m->type == MULTINAME)
            strcpy(mname,"<multi>");
        else //MULTINAMEA
            strcpy(mname,"<multi,attr>");
        strcat(mname, s);
        strcat(mname, "::");
        strcat(mname, name);
        free(s);
    } else if(m->type==MULTINAMEL || m->type==MULTINAMELA) {
        char*s = namespace_set_to_string(m->namespace_set);
        mname = malloc(strlen(s)+16);
        if(m->type == MULTINAMEL)
            strcpy(mname,"<l,multi>");
        else //MULTINAMELA
            strcpy(mname,"<l,multi,attr>");
        strcat(mname,s);
        free(s);
    } else {
        fprintf(stderr, "Invalid multiname type: %02x\n", m->type);
    }
    free(name);
    return mname;
}

multiname_t* multiname_fromstring(const char*name2)
{
    if(!name2)
        return 0;
    char*n = strdup(name2);
    char*p = strstr(n, "::");
    char*namespace=0,*name=0;
    if(!p) {
        if(strchr(n, ':')) {
            fprintf(stderr, "Error: single ':' in name\n");
        }
	namespace = "";
	name = n;
    } else {
	*p = 0;
	namespace = n;
	name = p+2;
        if(strchr(namespace, ':')) {
            fprintf(stderr, "Error: single ':' in namespace\n");
        }
        if(strchr(name, ':')) {
            fprintf(stderr, "Error: single ':' in qualified name\n");
        }
    }

    multiname_t*m = malloc(sizeof(multiname_t));
    memset(m, 0, sizeof(multiname_t));
    m->type = QNAME;
    m->namespace_set = 0;
    m->ns = namespace_fromstring(namespace);
    m->name = name?strdup(name):0;
    free(n);
    return m;
}

void multiname_destroy(multiname_t*m)
{
    if(m) {
        if(m->name) {
            free((void*)m->name);m->name = 0;
        }
        if(m->ns) {
            namespace_destroy(m->ns);m->ns = 0;
        }
        if(m->namespace_set) {
            namespace_set_destroy(m->namespace_set);m->namespace_set = 0;
        }
        free(m);
    }
}

type_t multiname_type = {
    dup: (dup_func)multiname_clone,
    hash: (hash_func)multiname_hash,
    free: (free_func)multiname_destroy,
    equals: (equals_func)multiname_equals
};

// ------------------------------- pool -------------------------------------

int pool_register_uint(pool_t*p, unsigned int i)
{
    int pos = array_append_if_new(p->x_uints, &i, 0);
    assert(pos!=0);
    return pos;
}
int pool_register_int(pool_t*p, int i)
{
    int pos = array_append_if_new(p->x_ints, &i, 0);
    assert(pos!=0);
    return pos;
}
int pool_register_float(pool_t*p, double d)
{
    int pos = array_append_if_new(p->x_floats, &d, 0);
    assert(pos!=0);
    return pos;
}
int pool_register_string(pool_t*pool, const char*s)
{
    if(!s) return 0;
    int pos = array_append_if_new(pool->x_strings, s, 0);
    assert(pos!=0);
    return pos;
}
int pool_register_namespace(pool_t*pool, namespace_t*ns)
{
    if(!ns) return 0;
    int pos = array_append_if_new(pool->x_namespaces, ns, 0);
    assert(pos!=0);
    return pos;
}
int pool_register_namespace_set(pool_t*pool, namespace_set_t*set)
{
    if(!set) return 0;
    int pos = array_append_if_new(pool->x_namespace_sets, set, 0);
    assert(pos!=0);
    return pos;
}
int pool_register_multiname(pool_t*pool, multiname_t*n)
{
    if(!n) return 0;
    int pos = array_append_if_new(pool->x_multinames, n, 0);
    if(pos==0) {
        *(int*)0=0xdead;
    }
    assert(pos!=0);
    return pos;
}
int pool_register_multiname2(pool_t*pool, char*name)
{
    if(!name) return 0;
    multiname_t*n = multiname_fromstring(name);
    int pos = array_append_if_new(pool->x_multinames, n, 0);
    multiname_destroy(n);
    assert(pos!=0);
    return pos;
}


int pool_find_uint(pool_t*pool, unsigned int x)
{
    int i = array_find(pool->x_uints, &x);
    if(i<=0) {
        fprintf(stderr, "Couldn't find uint \"%d\" in constant pool\n", x);
        return 0;
    }
    return i;
}
int pool_find_int(pool_t*pool, int x)
{
    int i = array_find(pool->x_ints, &x);
    if(i<=0) {
        fprintf(stderr, "Couldn't find int \"%d\" in constant pool\n", x);
        return 0;
    }
    return i;
}
int pool_find_float(pool_t*pool, double x)
{
    int i = array_find(pool->x_ints, &x);
    if(i<=0) {
        fprintf(stderr, "Couldn't find int \"%d\" in constant pool\n", x);
        return 0;
    }
    return i;
}
int pool_find_namespace(pool_t*pool, namespace_t*ns)
{
    if(!ns)
        return 0;
    int i = array_find(pool->x_namespaces, ns);
    if(i<=0) {
        char*s = namespace_to_string(ns);
        fprintf(stderr, "Couldn't find namespace \"%s\" %08x in constant pool\n", s, ns);
        free(s);
        return 0;
    }
    return i;
}
int pool_find_namespace_set(pool_t*pool, namespace_set_t*set)
{
    if(!set)
        return 0;
    int i = array_find(pool->x_namespace_sets, set);
    if(i<=0) {
        char*s = namespace_set_to_string(set);
        fprintf(stderr, "Couldn't find namespace_set \"%s\" in constant pool\n", s);
        free(s);
        return 0;
    }
    return i;
}
int pool_find_string(pool_t*pool, const char*s)
{
    if(!s)
        return 0;
    int i = array_find(pool->x_strings, s);
    if(i<=0) {
        fprintf(stderr, "Couldn't find string \"%s\" in constant pool\n", s);
        return 0;
    }
    return i;
}
int pool_find_multiname(pool_t*pool, multiname_t*name)
{
    if(!name)
        return 0;
    int i = array_find(pool->x_multinames, name);
    if(i<=0) {
        char*s = multiname_to_string(name);
        fprintf(stderr, "Couldn't find multiname \"%s\" in constant pool\n", s);
        free(s);
        return 0;
    }
    return i;
}

int pool_lookup_int(pool_t*pool, int i)
{
    if(!i) return 0;
    return *(int*)array_getkey(pool->x_ints, i);
}
unsigned int pool_lookup_uint(pool_t*pool, int i)
{
    if(!i) return 0;
    return *(unsigned int*)array_getkey(pool->x_uints, i);
}
double pool_lookup_float(pool_t*pool, int i)
{
    if(!i) return __builtin_nan("");
    return *(double*)array_getkey(pool->x_floats, i);
}
char*pool_lookup_string(pool_t*pool, int i)
{
    return (char*)array_getkey(pool->x_strings, i);
}
namespace_t*pool_lookup_namespace(pool_t*pool, int i)
{
    return (namespace_t*)array_getkey(pool->x_namespaces, i);
}
namespace_set_t*pool_lookup_namespace_set(pool_t*pool, int i)
{
    return (namespace_set_t*)array_getkey(pool->x_namespace_sets, i);
}
multiname_t*pool_lookup_multiname(pool_t*pool, int i)
{
    return (multiname_t*)array_getkey(pool->x_multinames, i);
}

pool_t*pool_new()
{
    NEW(pool_t, p);

    p->x_ints = array_new2(&uint_type);
    p->x_uints = array_new2(&uint_type);
    p->x_floats = array_new2(&float_type);
    p->x_strings = array_new2(&charptr_type);
    p->x_namespaces = array_new2(&namespace_type);
    p->x_namespace_sets = array_new2(&namespace_set_type);
    p->x_multinames = array_new2(&multiname_type);

    /* add a zero-index entry in each list */
  
    array_append(p->x_ints, 0, 0);
    array_append(p->x_uints, 0, 0);
    array_append(p->x_floats, 0, 0);
    array_append(p->x_strings, 0, 0);
    array_append(p->x_namespaces, 0, 0);
    array_append(p->x_namespace_sets, 0, 0);
    array_append(p->x_multinames, 0, 0);
    return p;
}

#define DEBUG if(0)
//#define DEBUG

void pool_read(pool_t*pool, TAG*tag)
{
    int num_ints = swf_GetU30(tag);
    DEBUG printf("%d ints\n", num_ints);
    int t;
    for(t=1;t<num_ints;t++) {
        S32 v = swf_GetS30(tag);
        DEBUG printf("int %d) %d\n", t, v);
        array_append(pool->x_ints, &v, 0);
    }

    int num_uints = swf_GetU30(tag);
    DEBUG printf("%d uints\n", num_uints);
    for(t=1;t<num_uints;t++) {
        U32 v = swf_GetU30(tag);
        DEBUG printf("uint %d) %d\n", t, v);
        array_append(pool->x_uints, &v, 0);
    }
    
    int num_floats = swf_GetU30(tag);
    DEBUG printf("%d floats\n", num_floats);
    for(t=1;t<num_floats;t++) {
        double d = swf_GetD64(tag);
        DEBUG printf("float %d) %f\n", t, d);
        array_append(pool->x_floats, &d, 0);
    }
    
    int num_strings = swf_GetU30(tag);
    DEBUG printf("%d strings\n", num_strings);
    for(t=1;t<num_strings;t++) {
	int len = swf_GetU30(tag);
	char*s = malloc(len+1);
	swf_GetBlock(tag, s, len);
	s[len] = 0;
	array_append(pool->x_strings, s, 0);
        free(s);
	DEBUG printf("%d) \"%s\"\n", t, pool->x_strings->d[t].name);
    }
    int num_namespaces = swf_GetU30(tag);
    DEBUG printf("%d namespaces\n", num_namespaces);
    for(t=1;t<num_namespaces;t++) {
	U8 type = swf_GetU8(tag);
	int namenr = swf_GetU30(tag);
	const char*name = ""; 
        if(namenr) //spec page 22: "a value of zero denotes an empty string"
            name = array_getkey(pool->x_strings, namenr);
        namespace_t*ns = namespace_new(type, name);
	array_append(pool->x_namespaces, ns, 0);
	DEBUG printf("%d) %02x \"%s\"\n", t, type, namespace_to_string(ns));
        namespace_destroy(ns);
    }
    int num_sets = swf_GetU30(tag);
    DEBUG printf("%d namespace sets\n", num_sets);
    for(t=1;t<num_sets;t++) {
        int count = swf_GetU30(tag);
        int s;
        
        NEW(namespace_set_t, nsset);
        for(s=0;s<count;s++) {
            int nsnr = swf_GetU30(tag);
            if(!nsnr)
                fprintf(stderr, "Zero entry in namespace set\n");
            namespace_t*ns = (namespace_t*)array_getkey(pool->x_namespaces, nsnr);
            list_append(nsset->namespaces, namespace_clone(ns));
        }
        array_append(pool->x_namespace_sets, nsset, 0);
        DEBUG printf("set %d) %s\n", t, namespace_set_to_string(nsset));
        namespace_set_destroy(nsset);
    }

    int num_multinames = swf_GetU30(tag);
    DEBUG printf("%d multinames\n", num_multinames);
    for(t=1;t<num_multinames;t++) {
        multiname_t m;
        memset(&m, 0, sizeof(multiname_t));
	m.type = swf_GetU8(tag);
	if(m.type==0x07 || m.type==0x0d) {
	    int namespace_index = swf_GetU30(tag);
            m.ns = (namespace_t*)array_getkey(pool->x_namespaces, namespace_index);
            int name_index = swf_GetU30(tag);
            if(name_index) // 0 = '*' (any)
	        m.name = array_getkey(pool->x_strings, name_index);
	} else if(m.type==0x0f || m.type==0x10) {
            int name_index = swf_GetU30(tag);
            if(name_index) // 0 = '*' (any name)
	        m.name = array_getkey(pool->x_strings, name_index);
	} else if(m.type==0x11 || m.type==0x12) {
	} else if(m.type==0x09 || m.type==0x0e) {
            int name_index = swf_GetU30(tag);
            int namespace_set_index = swf_GetU30(tag);
            if(name_index)
	        m.name = array_getkey(pool->x_strings, name_index);
	    m.namespace_set = (namespace_set_t*)array_getkey(pool->x_namespace_sets, namespace_set_index);
        } else if(m.type==0x1b || m.type==0x1c) {
            int namespace_set_index = swf_GetU30(tag);
	    m.namespace_set = (namespace_set_t*)array_getkey(pool->x_namespace_sets, namespace_set_index);
	} else {
	    printf("can't parse type %d multinames yet\n", m.type);
	}
        DEBUG printf("multiname %d) %s\n", t, multiname_to_string(&m));
	array_append(pool->x_multinames, &m, 0);
    }
    printf("%d ints\n", num_ints);
    printf("%d uints\n", num_uints);
    printf("%d strings\n", num_strings);
    printf("%d namespaces\n", num_namespaces);
    printf("%d namespace sets\n", num_sets);
    printf("%d multinames\n", num_multinames);
} 

void pool_write(pool_t*pool, TAG*tag)
{
    int t;
    
    /* make sure that all namespaces used by multinames / namespace sets
       and all strings used by namespaces exist */

    for(t=1;t<pool->x_multinames->num;t++) {
        multiname_t*m = (multiname_t*)array_getkey(pool->x_multinames, t);
        if(m->ns) {
            pool_register_namespace(pool, m->ns);
        }
        if(m->namespace_set) {
            pool_register_namespace_set(pool, m->namespace_set);
        }
        if(m->name) {
            pool_register_string(pool, m->name);
        }
    }
    for(t=1;t<pool->x_namespace_sets->num;t++) {
        namespace_set_t*set = (namespace_set_t*)array_getkey(pool->x_namespace_sets, t);
        namespace_list_t*i = set->namespaces;
        while(i) {
            pool_register_namespace(pool, i->namespace);
            i = i->next;
        }
    }
    for(t=1;t<pool->x_namespaces->num;t++) {
	namespace_t*ns= (namespace_t*)array_getkey(pool->x_namespaces, t);
        if(ns->name && ns->name[0])
            array_append_if_new(pool->x_strings, ns->name, 0);
    }

    /* write data */
    swf_SetU30(tag, pool->x_ints->num>1?pool->x_ints->num:0);
    for(t=1;t<pool->x_ints->num;t++) {
        S32 val = *(int*)array_getkey(pool->x_ints, t);
        swf_SetS30(tag, val);
    }
    swf_SetU30(tag, pool->x_uints->num>1?pool->x_uints->num:0);
    for(t=1;t<pool->x_uints->num;t++) {
        swf_SetU30(tag, *(unsigned int*)array_getkey(pool->x_uints, t));
    }
    swf_SetU30(tag, pool->x_floats->num>1?pool->x_floats->num:0);
    for(t=1;t<pool->x_floats->num;t++) {
        array_getvalue(pool->x_floats, t);
        swf_SetD64(tag, 0.0); // fixme
    }
    swf_SetU30(tag, pool->x_strings->num>1?pool->x_strings->num:0);
    for(t=1;t<pool->x_strings->num;t++) {
	swf_SetU30String(tag, array_getkey(pool->x_strings, t));
    }
    swf_SetU30(tag, pool->x_namespaces->num>1?pool->x_namespaces->num:0);
    for(t=1;t<pool->x_namespaces->num;t++) {
	namespace_t*ns= (namespace_t*)array_getkey(pool->x_namespaces, t);
	swf_SetU8(tag, ns->access);
	const char*name = ns->name;
	int i = 0;
        if(name && name[0])
            i = pool_find_string(pool, name);
	swf_SetU30(tag, i);
    }
    swf_SetU30(tag, pool->x_namespace_sets->num>1?pool->x_namespace_sets->num:0);
    for(t=1;t<pool->x_namespace_sets->num;t++) {
        namespace_set_t*set = (namespace_set_t*)array_getkey(pool->x_namespace_sets, t);
        namespace_list_t*i = set->namespaces; 
        int len = list_length(i);
        swf_SetU30(tag, len);
        while(i) {
            int index = pool_find_namespace(pool, i->namespace);
            swf_SetU30(tag, index);
            i = i->next;
        }
    }

    swf_SetU30(tag, pool->x_multinames->num>1?pool->x_multinames->num:0);
    for(t=1;t<pool->x_multinames->num;t++) {
	multiname_t*m = (multiname_t*)array_getkey(pool->x_multinames, t);
	swf_SetU8(tag, m->type);

        if(m->ns) {
            assert(m->type==0x07 || m->type==0x0d);
	    int i = pool_find_namespace(pool, m->ns);
            if(i<0) fprintf(stderr, "internal error: unregistered namespace %02x %s %s\n", m->ns->access, access2str(m->ns->access), m->ns->name);
	    swf_SetU30(tag, i);
        } else {
            assert(m->type!=0x07 && m->type!=0x0d);
        }
        if(m->name) {
            assert(m->type==0x09 || m->type==0x0e || m->type==0x07 || m->type==0x0d || m->type==0x0f || m->type==0x10);
	    int i = pool_find_string(pool, m->name);
            if(i<0) fprintf(stderr, "internal error: unregistered name\n");
	    swf_SetU30(tag, i);
        } else {
            assert(m->type!=0x09 && m->type!=0x0e && m->type!=0x07 && m->type!=0x0d && m->type!=0x0f && m->type!=0x10);
        }
        if(m->namespace_set) {
            assert(m->type==0x09 || m->type==0x0e || m->type==0x1c || m->type==0x1b);
	    int i = pool_find_namespace_set(pool, m->namespace_set);
            if(i<0) fprintf(stderr, "internal error: unregistered namespace set\n");
	    swf_SetU30(tag, i);
        } else {
            assert(m->type!=0x09 && m->type!=0x0e && m->type!=0x1c && m->type!=0x1b);
        }
    }
}


void pool_destroy(pool_t*pool)
{
    int t;
    array_free(pool->x_ints);
    array_free(pool->x_uints);
    array_free(pool->x_floats);
    array_free(pool->x_strings);
    array_free(pool->x_namespaces);
    array_free(pool->x_namespace_sets);
    array_free(pool->x_multinames);
    free(pool);
}
