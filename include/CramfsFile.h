#ifndef CRAMFSFILE_H
#define CRAMFSFILE_H

/* CramfsFile.h
 * cramfs文件解析/生成类
 *
 * 说明：为了保证其独立通用性，使用标准库替代wx库
 *
 * 作者: 知遇(qq:178041876)
 * 日期: 2017/10/15
 * 版权: GPL
 */

#include <stdint.h>
#include <linux/cramfs_fs.h>
#include <string>
#include <vector>
using namespace std;

typedef uint32_t u32;


class CramfsFile
{
    typedef struct Cramfs_Inode
    {
        struct cramfs_inode inode;
        char* name;
        struct Cramfs_Inode* parent;
        vector<struct Cramfs_Inode*> children;
    }CramfsInode;
    static const uint32_t Front_Padding_Size;
public:
    CramfsFile(const char* path);
    virtual ~CramfsFile();

    //open a cramfs binary file
    bool Open(const char* path);

    //测试文件是否存在
    bool Exists(const char* path) const;

    //解压文件
    bool ExtractFile(const char* path, uint8_t*& orig_data, size_t& size);

protected:
    bool Parse();
    bool Parse(uint8_t* buffer, size_t size);
    bool CheckSuper(size_t size);
    bool ParseInodes(uint8_t* buffer, CramfsInode& node);
    CramfsInode* SearchFile(const char* path) const;
    void PrintPath(const CramfsInode& node) const;
    void do_uncompress(char *path, int fd, unsigned long offset, unsigned long size);

private:
    string file_path;
    bool is_ok;
    uint8_t* front_padding;
    uint8_t* base_buffer;
    struct cramfs_super super;
    CramfsInode root;
};

#endif // CRAMFSFILE_H
