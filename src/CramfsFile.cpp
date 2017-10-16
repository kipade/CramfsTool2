#include "CramfsFile.h"
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
using namespace std;
#include <zlib.h>
#include <fcntl.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE (4096)
#endif // PAGE_SIZE

#ifndef CEIL_4
#define CEIL_4(x)   ((((x) + 3) >> 2) << 2)
#endif // CEIL_4

const uint32_t CramfsFile::Front_Padding_Size = 512;

CramfsFile::CramfsFile(const char* path) : file_path(path), is_ok(false), front_padding(NULL)
    , base_buffer(NULL)
{
    Open(path);
}

CramfsFile::~CramfsFile()
{
    //dtor
}

bool CramfsFile::Open(const char* path)
{
    if(path != NULL)
    {
        if(access(path, 0) == 0)
        {
            file_path = path;
            return Parse();
        }
    }
    return false;
}

bool CramfsFile::Parse()
{
    FILE* fi = fopen(file_path.c_str(), "rb");
    if(fi != NULL)
    {
        bool ret = false;
        fseek(fi, 0, SEEK_END);
        size_t size = ftell(fi);
        fseek(fi, 0, SEEK_SET);

        uint8_t* buffer = (uint8_t*)malloc(size);
        if(buffer != NULL)
        {
            if(fread(buffer, size, 1, fi) > 0)
            {
                base_buffer = buffer;
                ret = Parse(buffer, size);
            }
        }
        fclose(fi);
        return ret;
    }
    return false;
}

bool CramfsFile::Parse(uint8_t* buffer, size_t size)
{
    is_ok = false;
    if(buffer != NULL && size > sizeof(struct cramfs_super))
    {
        super = *(struct cramfs_super*)buffer;
        if(super.magic != CRAMFS_MAGIC)
        {
            super = *(struct cramfs_super*)(buffer + CramfsFile::Front_Padding_Size);
            if(super.magic != CRAMFS_MAGIC)
            {
                return false;
            }
            front_padding = (uint8_t*)malloc(CramfsFile::Front_Padding_Size);
            if(front_padding == NULL)
            {
                return false;
            }
            memcpy(front_padding, buffer, CramfsFile::Front_Padding_Size);
        }
        if(CheckSuper(size) != true)
        {
            cout<<"Check super failed"<<endl;
            return false;
        }

        //处理inodes
        uint8_t* inodes_start = buffer + ((uint8_t*)&super.root - (uint8_t*)&super);//.offset * 4;
        if(front_padding != NULL)
        {
            inodes_start += CramfsFile::Front_Padding_Size;
        }
        root.parent = NULL;
        if(ParseInodes(inodes_start, root) != true)
        {
            cout<<"Parse inodes failed."<<endl;
            return false;
        }
        is_ok = true;
        return true;
    }

    return false;
}

bool CramfsFile::CheckSuper(size_t size)
{
    if(super.flags & ~CRAMFS_SUPPORTED_FLAGS)
    {
        cout<<"Unspported filesystem"<<endl;
        return false;
    }
    if(super.size < PAGE_SIZE)
    {
        cout<<"Super block size:"<<super.size<<" is tool small."<<endl;
        return false;
    }
    if (super.flags & CRAMFS_FLAG_FSID_VERSION_2)
    {
        if (super.fsid.files == 0)
        {
            cout<<"Zero file count."<<endl;
            return false;
        }
        if(super.size > size)
        {
            cout<<"Super size:"<<super.size<<" is larger then file size:"<<size<<endl;
            return false;
        }
        else if(super.size < size)
        {
            cout<<"File size is extends the filesystem size"<<endl;
        }
    }
    return true;
}

bool CramfsFile::ParseInodes(uint8_t* buffer, CramfsInode& node)
{
    struct cramfs_inode* pnode = (struct cramfs_inode*)buffer;
    node.inode = *pnode;
    node.name = (char*)malloc(pnode->namelen * 4 + 1);
    if(node.name == NULL)
    {
        return false;
    }
    memcpy(node.name, pnode + 1, pnode->namelen * 4);
    node.name[pnode->namelen*4] = 0;
    //cout<<node.name<<endl;

    if(S_ISDIR(node.inode.mode))
    {
        //struct cramfs_inode* child_node = (struct cramfs_inode*)(buffer + sizeof(struct cramfs_inode) + CEIL_4(pnode->namelen*4));
        struct cramfs_inode* child_node = (struct cramfs_inode*)(base_buffer + (pnode->offset << 2));
        int32_t block_size = node.inode.size;

        while(block_size > 0)
        {
            CramfsInode* child = new CramfsInode;
            child->name = NULL;
            child->parent = &node;
            if(ParseInodes((uint8_t*)child_node, *child) != true)
            {
                return false;
            }
            block_size -= sizeof(struct cramfs_inode) + child_node->namelen * 4;
            node.children.push_back(child);
            child_node = (struct cramfs_inode*)((uint8_t*)(child_node + 1) + CEIL_4(child_node->namelen * 4));
        }
    }
    else
    {
        PrintPath(node);
    }

    return true;
}

void CramfsFile::PrintPath(const CramfsInode& node) const
{
    string path = node.name;
    CramfsInode* parent = node.parent;

    while(parent != NULL)
    {
        path = string(parent->name).append("/").append(path);
        parent = parent->parent;
    }
    cout<<path<<endl;
}

CramfsFile::CramfsInode* CramfsFile::SearchFile(const char* fullpath) const
{
    if(is_ok && fullpath != NULL && fullpath[0] == '/')
    {
        CramfsInode* pnode = (CramfsInode*)&root;
        const char* name_start = fullpath + 1;
        const char* next_split = strchr(name_start, '/');
        char name[256];
        int namelen;

        if(next_split == NULL)
        {
            next_split = fullpath + strlen(fullpath);
        }

        while(next_split != NULL)
        {
            namelen = next_split - name_start;
            strncpy(name, name_start, namelen);
            name[namelen] = 0;
            vector<struct Cramfs_Inode*>::const_iterator it = pnode->children.begin();
            vector<struct Cramfs_Inode*>::const_iterator end = pnode->children.end();
            while(it != end)
            {
                if(strcmp(name, (*it)->name) == 0)
                {
                    pnode = *it;
                    break;
                }
                it++;
            }
            if(it == end)
            {
                return NULL;
            }
            else if(*next_split == '\0')
            {
                return pnode;
            }
            name_start = next_split + 1;
            next_split = strchr(name_start, '/');
            if(next_split == NULL)
            {
                next_split = name_start + strlen(name_start);
                if(name_start == next_split)
                {
                    break;
                }
            }
        }
        return pnode;
    }
    return NULL;
}

bool CramfsFile::Exists(const char* path) const
{
    CramfsInode* node = SearchFile(path);
    if(node != NULL)
    {
        return true;
    }
    return false;
}

#define ROMBUFFER_BITS	13
#define ROMBUFFERSIZE	(1 << ROMBUFFER_BITS)
#define ROMBUFFERMASK	(ROMBUFFERSIZE-1)


/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

int inf2(uint8_t* buffer, size_t len, uint8_t* dst, size_t avail)
{
    int ret;
    unsigned have;
    z_stream strm;


    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;
    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = len;

        if (strm.avail_in == 0)
            break;
        strm.next_in = buffer;
        uint32_t left = avail;
        uint8_t* out = dst;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = left;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = left - strm.avail_out;
            left += have;
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


bool CramfsFile::ExtractFile(const char* path, uint8_t*& orig_data, size_t& size)
{
    CramfsInode* node = SearchFile(path);
    if(node != NULL)
    {
        size_t nblocks = (node->inode.size + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t* block_pointers = (uint32_t*)(base_buffer + node->inode.offset * 4);
        uint32_t data_start = node->inode.offset * 4 + 4 * nblocks;
        uint8_t* orig_data = (uint8_t*)malloc(node->inode.size);
        if(orig_data == NULL)
        {
            cout<<"Allocate memory while extract file for ["<<path<<"] failed, size["<<node->inode.size<<"]."<<endl;
            return false;
        }

        uint32_t prev_offset = data_start;
        uint32_t len;

        uint8_t* out = orig_data;
        size_t i;
        int ret;
        for(i = 0; i < nblocks; ++i)
        {
            len = block_pointers[i] - prev_offset;
            ret = inf2(base_buffer + prev_offset, len, out, PAGE_SIZE);
            if(ret != Z_OK)
            {
                break;
            }
            prev_offset += len;
            out += PAGE_SIZE;
        }
        if(ret != Z_OK)
        {
            free(orig_data);
            orig_data = NULL;
            return false;
        }
        size = node->inode.size;
        FILE* fo = fopen("tmp", "wb");
        fwrite(orig_data, node->inode.size, 1, fo);
        fclose(fo);
        return true;
    }
    return false;
}
