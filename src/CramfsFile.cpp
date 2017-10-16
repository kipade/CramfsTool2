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
        bool exists = false;
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

static char outbuffer[PAGE_SIZE*2];
static z_stream stream;
#define ROMBUFFER_BITS	13
#define ROMBUFFERSIZE	(1 << ROMBUFFER_BITS)
#define ROMBUFFERMASK	(ROMBUFFERSIZE-1)
static char read_buffer[ROMBUFFERSIZE * 2];
static unsigned long read_buffer_block = ~0UL;
/*
static void *romfs_read(unsigned long offset)
{
	unsigned int block = offset >> ROMBUFFER_BITS;
	if (block != read_buffer_block) {
		read_buffer_block = block;
		lseek(fd, block << ROMBUFFER_BITS, SEEK_SET);
		read(fd, read_buffer, ROMBUFFERSIZE * 2);
	}
	return read_buffer + (offset & ROMBUFFERMASK);
}
*/

static int uncompress_block(void *src, int len)
{
	int err;

	stream.next_in = (Bytef*)src;
	stream.avail_in = len;

	stream.next_out = (unsigned char *) outbuffer;
	stream.avail_out = PAGE_SIZE*2;

	inflateReset(&stream);

	if (len > PAGE_SIZE*2) {
            return -1;
		//die(FSCK_UNCORRECTED, 0, "data block too large");
	}
	err = inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END) {
		//die(FSCK_UNCORRECTED, 0, "decompression error %p(%d): %s",
		//    src, len, zError(err));
		return -1;
	}
	return stream.total_out;
}


/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(uint8_t* buffer, size_t size, FILE *dest)
{
    #define CHUNK 16384

    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    //do {
    for(size_t i = 0; i < size; ++i)
    {
        strm.avail_in = CHUNK;//fread(in, 1, CHUNK, source);
        memcpy(in, buffer, CHUNK);
        /*if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
            */
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            if(ret != Z_STREAM_ERROR)  /* state not clobbered */
            {
                cout<<"zlib init failed."<<endl;
                return Z_DATA_ERROR;
            }
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } //while (ret != Z_STREAM_END);

    /* clean up and return */
    inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

static bool save_file(uint8_t* buffer, size_t len, int idx)
{
    char name[32];
    sprintf(name, "data_%d", idx);
    FILE *fo = fopen(name, "wb");
    fwrite(buffer, len, 1, fo);
    fclose(fo);
}

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


bool CramfsFile::ExtractFile(const char* path, uint8_t*& orig_data, size_t& size)
{
    CramfsInode* node = SearchFile(path);
    if(node != NULL)
    {
        size_t nblocks = (node->inode.size + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t* block_pointers = (uint32_t*)(base_buffer + node->inode.offset * 4);
        uint32_t data_start = node->inode.offset * 4 + 4 * nblocks;
        uint8_t* data_buffer = (uint8_t*)malloc(nblocks * PAGE_SIZE);
        FILE* fo = fopen("test.c", "wb");

        uint32_t prev_offset = data_start;
        uint32_t len;
        for(size_t i = 0; i < nblocks; ++i)
        {
            len = block_pointers[i] - prev_offset;
            save_file(base_buffer + prev_offset, len, i);
            //inf(base_buffer + prev_offset, len, fo);
            prev_offset += len;
        }
    }
    return false;
}
