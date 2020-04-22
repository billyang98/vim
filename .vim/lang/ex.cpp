#include "bobfs.h"
#include "libk.h"
#include "heap.h"

uint8_t zero_1024[BobFS::BLOCK_SIZE];

//////////
// Node //
//////////

Node::Node(StrongPtr<BobFS> fs, uint32_t inumber) : fs(fs) {
    this->inumber = inumber;
    this->offset = fs->inodeBase + inumber * SIZE;
//    Debug::printf("*** node %d offset %d\n", inumber, offset);

//    Debug::printf("*** ---NODE #%d ***\n*** type %d\n*** links %d\n*** size: %d\n*** direct %d\n*** indirect %d\n***--- *** \n ", inumber, getType(), getLinks(), getSize(), getDirect(), getIndirect());
}


uint16_t Node::getType(void) {
    uint16_t type = 0;
    fs->device->readAll(offset, &type, 2);
    return type; 
}

bool Node::isDirectory(void) {
    return getType() == DIR_TYPE;
}

bool Node::isFile(void) {
    return getType() == FILE_TYPE;
}

bool streq(const char* a, const char* b) {
    int i = 0;

    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

//return [inode number, offset start, name length] if found, -1 otherwise
int32_t* Node::searchBlock(StrongPtr<BobFS> fs, const char* name,  uint32_t dirSize) {
    int32_t foundNode = -1;
    uint32_t offset = 0;
    uint32_t startNode = 0;
    uint32_t nodeNameLength = 0;
    while(offset < dirSize && foundNode == -1) {
        uint32_t inum = 0; 
        readAll(offset, &inum, 4);
        offset += 4;
        uint32_t nameSize = 0;
        readAll(offset, &nameSize, 4);
        offset += 4;
        char* readName = new char[nameSize + 1];
        readAll(offset, readName, nameSize);
        readName[nameSize] = 0;
        if(streq(name, readName)) {
            foundNode = inum;
            startNode = offset - 8;  
            nodeNameLength = nameSize;
        }
        delete[] readName;
        offset += nameSize;

    }
//    Debug::printf("searching for %s\n", name);
    int32_t* returnValues = (int32_t*) malloc(sizeof(int32_t) * 3);
    returnValues[0] = foundNode;
    returnValues[1] = startNode;
    returnValues[2] = nodeNameLength;
    return returnValues;
     
}

StrongPtr<Node> Node::findNode(const char* name) {
    uint32_t dirSize = getSize();
    int32_t* values = searchBlock(fs, name, dirSize);
    int32_t inum = values[0];
    free(values);
    if(inum >= 0) {
        return StrongPtr<Node>(new Node(fs, (uint32_t) inum));
    }
    return StrongPtr<Node>(nullptr);
}

uint16_t Node::getLinks(void) {
    uint16_t links = 0;
    fs->device->readAll(offset + 2, &links, 2);
    return links; 
}

uint32_t Node::getSize(void) {
    uint32_t size = 0;
    fs->device->readAll(offset + 4, &size, 4);
    return size; 
}

uint32_t Node::getDirect(void) {
    uint32_t direct = 0;
    fs->device->readAll(offset + 8, &direct, 4);
    return direct; 
}

uint32_t Node::getIndirect(void) {
    uint32_t indirect = 0;
    fs->device->readAll(offset + 12, &indirect, 4);
    return indirect; 
}

int32_t Node::write(uint32_t offset, const void* buffer, uint32_t n) {
    if(n + offset % BobFS::BLOCK_SIZE > BobFS::BLOCK_SIZE) {
        n = BobFS::BLOCK_SIZE - offset % BobFS::BLOCK_SIZE;
    }
    uint32_t direct = getDirect();
    if(offset < BobFS::BLOCK_SIZE) {
        if(direct == 0) {
            direct = getFreeBlock();
            fs->device->writeAll(direct * BobFS::BLOCK_SIZE, zero_1024, BobFS::BLOCK_SIZE);
            setDirect(direct);
            fs->dataBitmap->setIndex(direct, 1);
        } 
        fs->device->writeAll(direct * BobFS::BLOCK_SIZE + offset, buffer, n);
    } else { // gotta do some indirection
        // haven't done any indirection yet
        uint32_t indirect = getIndirect();
        if(indirect == 0) {
            indirect = getFreeBlock();
            fs->device->writeAll(indirect * BobFS::BLOCK_SIZE, zero_1024, BobFS::BLOCK_SIZE);
            setIndirect(indirect);
            fs->dataBitmap->setIndex(indirect, 1);
        }
        uint32_t indirectIndex = (offset / BobFS::BLOCK_SIZE) - 1;
        uint32_t newOffset = offset % BobFS::BLOCK_SIZE;
        uint32_t* indirectBlock = new uint32_t[BobFS::BLOCK_SIZE / 4];
        fs->device->readAll(indirect * BobFS::BLOCK_SIZE, indirectBlock, BobFS::BLOCK_SIZE);
        // need a new block for a new indirection
        uint32_t secondIndirect = indirectBlock[indirectIndex];
        if(secondIndirect == 0) {
            secondIndirect = getFreeBlock();
            fs->device->writeAll(secondIndirect * BobFS::BLOCK_SIZE, zero_1024, 
                    BobFS::BLOCK_SIZE);
            fs->device->writeAll(indirect * BobFS::BLOCK_SIZE + indirectIndex * 4, 
                    &secondIndirect, 4);
            fs->dataBitmap->setIndex(secondIndirect, 1);
        }
        fs->device->writeAll(secondIndirect * BobFS::BLOCK_SIZE + newOffset, buffer, n);
        delete[] indirectBlock;
    }
    uint32_t size = getSize();
    uint32_t newSize = offset + n > size ? offset + n : size; 
    setSize(newSize);
    return n;
}

int32_t Node::writeAll(uint32_t offset, const void* buffer_, uint32_t n) {

    int32_t total = 0;
    char* buffer = (char*) buffer_;

    while (n > 0) {
        int32_t cnt = write(offset,buffer,n);
        if (cnt <= 0) return total;

        total += cnt;
        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
    return total;
}

int32_t Node::read(uint32_t offset, void* buffer, uint32_t n) {
//    Debug::printf("*** reading here? %d\n", getDirect() * BobFS::BLOCK_SIZE + offset);
    // fix the number of bytes to read if we'll go over a block
    if(n + offset % BobFS::BLOCK_SIZE > BobFS::BLOCK_SIZE) {
        n = BobFS::BLOCK_SIZE - offset % BobFS::BLOCK_SIZE;
    }
    if(offset < BobFS::BLOCK_SIZE) {
        uint32_t direct = getDirect();
        if(direct == 0) {
            for(uint32_t i = 0; i < n; i++) {
                ((char*) buffer)[i] = 0;
            }
        } else {
            fs->device->readAll(getDirect() * BobFS::BLOCK_SIZE + offset, buffer, n);
        }
    } else { // gotta do some indirection
        offset -= BobFS::BLOCK_SIZE;
        uint32_t indirectIndex = offset / BobFS::BLOCK_SIZE;
        uint32_t newOffset = offset % BobFS::BLOCK_SIZE;
        uint32_t* indirectBlock = new uint32_t[BobFS::BLOCK_SIZE / 4];
        fs->device->readAll(getIndirect() * BobFS::BLOCK_SIZE, indirectBlock, BobFS::BLOCK_SIZE);
        uint32_t secondIndirect = indirectBlock[indirectIndex];
        if(secondIndirect == 0) {
            for(uint32_t i = 0; i < n; i++) {
                ((char*) buffer)[i] = 0;
            }
        } else {
            fs->device->readAll(indirectBlock[indirectIndex] * BobFS::BLOCK_SIZE + newOffset, buffer, n);
        }
        delete[] indirectBlock;
    }
    return n;
}

int32_t Node::readAll(uint32_t offset, void* buffer_, uint32_t n) {

    int32_t total = 0;
    char* buffer = (char*) buffer_;

    uint32_t size = getSize();
    // fix the number of bytes read if we are reading more than the size
    if(n + offset > size) {
        n = size - offset;
    }  

    if(offset >= size) {
        return -1;
    }
    while (n > 0) {
        int32_t cnt = read(offset,buffer,n);
        if (cnt <= 0) return total;

        total += cnt;
        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
    return total;
}

uint8_t getTaken(uint8_t* block, uint32_t index) {
    uint8_t temp = block[index/8];
    return (temp >> (index % 8)) & 1;
}

uint32_t Node::getFreeINode() {
    uint8_t* inodes = new uint8_t[BobFS::BLOCK_SIZE];
    fs->device->readAll(fs->inodeBitmapBase, inodes, BobFS::BLOCK_SIZE);
    for(uint32_t i = 0; i < BobFS::BLOCK_SIZE * 8; i++) {
        if(((inodes[i / 8] >> (i % 8)) & 1) == 0) {
//            Debug::printf("new inode %d\n", i);
            delete[] inodes;
            return i;
        }
    }
    Debug::panic("*** no more free data blocks\n");
    return 0;
}

uint32_t Node::getFreeBlock() {
    uint8_t* blocks = new uint8_t[BobFS::BLOCK_SIZE];
    fs->device->readAll(fs->dataBitmapBase, blocks, BobFS::BLOCK_SIZE);
    for(uint32_t i = fs->dataBase / BobFS::BLOCK_SIZE; 
            i < BobFS::BLOCK_SIZE * 8;
            i++) {
        if(((blocks[i / 8] >> (i % 8)) & 1) == 0) {
//            Debug::printf("new block %d\n", i);
            delete[] blocks;
            return i;
        }
    }
    Debug::panic("*** no more free data blocks\n");
    return 0;
}

void Node::setType(uint16_t type) {
    fs->device->writeAll(this->offset, &type, 2);
}

void Node::setLinks(uint16_t type) {
    fs->device->writeAll(this->offset + 2, &type, 2);
}

void Node::setSize(uint32_t type) {
    fs->device->writeAll(this->offset + 4, &type, 4);
}

void Node::setDirect(uint32_t type) {
    fs->device->writeAll(this->offset + 8, &type, 4);
}

void Node::setIndirect(uint32_t type) {
    fs->device->writeAll(this->offset + 12, &type, 4);
}

void Node::newEntryDir(const char* name, uint32_t inumber) {
    uint32_t size = getSize();
    writeAll(size, &inumber, 4);
    uint32_t nameLen = strlen(name);
    writeAll(size + 4, &nameLen, 4);
    writeAll(size + 8, name, nameLen);
}

void Node::linkNode(const char* name, StrongPtr<Node> node) {
    // write into the directory
    newEntryDir(name, node->inumber);
    // write into the new inode
    node->setLinks(node->getLinks() + 1);
}

StrongPtr<Node> Node::newNode(const char* name, uint32_t type) {    
    uint32_t newINode = getFreeINode();
    fs->inodeBitmap->setIndex(newINode, 1);
    // write into the directory
    newEntryDir(name, newINode);
    // write into the new inode
    StrongPtr<Node> node(new Node(fs, newINode));
    node->setType(type);
    node->setLinks(1);
    node->setSize(0);
    node->setDirect(0);
    node->setIndirect(0);
    return node;
}

StrongPtr<Node> Node::newFile(const char* name) {
    return newNode(name, FILE_TYPE);
}

StrongPtr<Node> Node::newDirectory(const char* name) {
    return newNode(name, DIR_TYPE);
}

void Node::freeBlocks(uint32_t size) {
    uint32_t direct = getDirect();
    if(direct != 0) {
        fs->dataBitmap->setIndex(direct, 0);
    }
    uint32_t indirect = getIndirect();
    if(indirect != 0) {
        uint32_t* indirectBlock = new uint32_t[BobFS::BLOCK_SIZE / 4];
        fs->device->readAll(indirect * BobFS::BLOCK_SIZE, indirectBlock, BobFS::BLOCK_SIZE);
        for(uint32_t i = 0; i < size / BobFS::BLOCK_SIZE; i ++) {
            if(indirectBlock[i] != 0) {
                fs->dataBitmap->setIndex(indirectBlock[i], 0);
            }
        }
        fs->dataBitmap->setIndex(indirect, 0);
        delete[] indirectBlock;
    }
}

void Node::freeBlock(uint32_t i) {
    if(i == 0) {
        fs->dataBitmap->setIndex(getDirect(), 0);
    } else {
        i--;
        uint32_t indirect = getIndirect();
        uint32_t* indirectBlock = new uint32_t[BobFS::BLOCK_SIZE / 4];
        fs->device->readAll(indirect * BobFS::BLOCK_SIZE, indirectBlock, BobFS::BLOCK_SIZE);
        fs->dataBitmap->setIndex(indirectBlock[i], 0);
        delete[] indirectBlock;
    }
}

void Node::removeEntry(uint32_t size, uint32_t base, uint32_t nameLength) {
    uint32_t numShiftBytes = (size - base - nameLength - 8);
    char* buffer = new char[numShiftBytes];    
    readAll(base + 8 + nameLength, buffer, numShiftBytes);
    writeAll(base, buffer, numShiftBytes);
    uint32_t newSize = size - 8 - nameLength;
    for(uint32_t endBlock = size / BobFS::BLOCK_SIZE; endBlock < newSize / BobFS::BLOCK_SIZE; endBlock--) {
        // free up these blocks in block bit map
        freeBlock(endBlock);
    }
    setSize(newSize);
    delete[] buffer; 
}

long Node::fixINode(uint32_t inum) {
    StrongPtr<Node> inode(new Node(fs, (uint32_t) inum));
//    Debug::printf("*** 0 ----node removing link %d\n", inum);
    uint32_t type = inode->getType();
    uint32_t nLinks = inode->getLinks();
    long count = 0;
    if(nLinks > 1) {
        inode->setLinks(nLinks - 1);
    } else {
        // delete inode
        count = 1;
        uint32_t size = inode->getSize();
        if(type == DIR_TYPE) {
            uint32_t offset = 0;
            while(offset < size) {
                uint32_t inum = 0; 
                inode->readAll(offset, &inum, 4);
                offset += 4;
//                Debug::printf("*** 0 ----node %d deleted\n", inum);
                uint32_t nameSize = 0;
                inode->readAll(offset, &nameSize, 4);
                offset += 4 + nameSize;
                count += fixINode(inum);
            }
        }
        fs->inodeBitmap->setIndex(inum, 0);
        if(size > 0) {
            inode->freeBlocks(size);
        }
    }
    return count;
}

long Node::unlink(const char* name) {
    if(getType() != DIR_TYPE) {
        return -1;
    }
    uint32_t size = getSize();
    int32_t* values = searchBlock(fs, name, size);
    int32_t inum = values[0];
    uint32_t base = (uint32_t) values[1]; 
    uint32_t nameLength = (uint32_t) values[2];
    free(values);
    if(inum < 0) { // invalid name, not found
        return 0;
    }
    // remove entry from directory
    removeEntry(size, base, nameLength);
    // clean up inode stuff
    long count = fixINode(inum);
    return count;
}

void Node::dump(const char* name) {
    uint32_t type = getType();
    switch (type) {
    case DIR_TYPE:
        Debug::printf("*** 0 directory:%s(%d)\n",name,getLinks());
        {
            uint32_t sz = getSize();
            uint32_t offset = 0;

            while (offset < sz) {
                uint32_t ichild = 0;
                readAll(offset,&ichild,4);
//                Debug::printf("*** looking at inum  %d\n", ichild);
                offset += 4;
                uint32_t len = 0;
                readAll(offset,&len,4);
//                Debug::printf("*** name size  %d\n", len);
                offset += 4;
//                Debug::printf("dumping %s", name);
                char* ptr = (char*) malloc(len+1);
                readAll(offset,ptr,len);
                offset += len;
                ptr[len] = 0;              
//                Debug::printf("*** file %s \n", ptr);
                
                StrongPtr<Node> child = Node::get(fs,ichild);
                child->dump(ptr);
                free(ptr);
            }
        }
        break;
    case FILE_TYPE:
        Debug::printf("*** 0 file:%s(%d,%d)\n",name,getLinks(),getSize());
        break;
    default:
         Debug::panic("unknown i-node type %d\n",type);
    }
}


///////////
// BobFS //
///////////

bool checkBobFS(uint8_t* superBlock) {
    if(superBlock[0] != 'B') return false;
    if(superBlock[1] != 'O') return false;
    if(superBlock[2] != 'B') return false;
    if(superBlock[3] != 'F') return false;
    if(superBlock[4] != 'S') return false;
    if(superBlock[5] != '4') return false;
    if(superBlock[6] != '3') return false;
    if(superBlock[7] != '9') return false;
    return true;
}

BobFS::BobFS(StrongPtr<Ide> device) {
    this->device = device;
    this->superBlock = (uint8_t*) malloc(sizeof(uint8_t) * 12);
    this->device->readAll(0, superBlock, 12); 
    if(!checkBobFS(superBlock)) {
//        Debug::panic("Not BobFS\n");
    }
    dataBitmap = new Bitmap(device, dataBitmapBase); 
    inodeBitmap = new Bitmap(device, inodeBitmapBase);
}

BobFS::~BobFS() {
    free(superBlock);
}

StrongPtr<Node> BobFS::root(StrongPtr<BobFS> fs) {
    uint32_t rootNodeNum = ((uint32_t*) fs->superBlock)[2];
    return StrongPtr<Node>(new Node(fs, rootNodeNum));
}

StrongPtr<BobFS> BobFS::mount(StrongPtr<Ide> device) {
    StrongPtr<BobFS> fs { new BobFS(device) };
    return fs;
}

StrongPtr<BobFS> BobFS::mkfs(StrongPtr<Ide> device) {
    device->writeAll(0, zero_1024, BLOCK_SIZE);
    device->writeAll(0, "BOBFS439", 8);
    uint32_t rootNodeNum = 0;

    device->writeAll(8, &rootNodeNum, 4);

    StrongPtr<BobFS> fs { new BobFS(device) };

    // zero out the bitmaps first
    device->writeAll(fs->dataBitmapBase, zero_1024, BLOCK_SIZE);
    device->writeAll(fs->inodeBitmapBase, zero_1024, BLOCK_SIZE);
    //set the blocks to be taken for super block, 2 bitmaps, and inodes
    uint32_t dataBlockNum = 1 + 1 + 1 + 8 * Node::SIZE;
    for(uint32_t i = 0; i < dataBlockNum; i++) {
        fs->dataBitmap->setIndex(i, 1);
    }

    // make the root directory 
    fs->inodeBitmap->setIndex(0, 1);
    StrongPtr<Node> node(root(fs));
    node->setType(Node::DIR_TYPE); // file type 1
    node->setLinks(1); // num links
    node->setSize(0); // size 
    node->setDirect(0);
    node->setIndirect(0);
    return fs;
}
