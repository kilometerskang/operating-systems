#!/usr/bin/env python3

import sys
import csv

block_count = 0
inode_count = 0
block_size = 0
inode_size = 0
first_avail_inode = 0
first_avail_block = 0

#stores the inode numbers of free inodes
free_inodes = set()

#dictionary of printed INODE lines
#key = inode number, value = [i_mode, link_count]
#can navigate using allocated_inodes[inode_num][index_in_value_list]
allocated_inodes = {}

#dictionary of link counts associated with each inode
inode_linkcount = {}

#stores the inode numbers of free blocks
free_blocks = set()

#dictionary of blocks
allocated_blocks = {}

#class structure to store information for printed DIRENT lines
class dirEnt:
    def __init__(self, par, inum, name):
        self.parent_inode_num = par
        self.inode_num = inum
        self.name = name

#stores the list of dirent class objects
dirent_list = []

#dictionary of inodes' parents, referenced by name
#key: inode number, value: parent inode number
inode_named_parents = {}

#dictionary of inodes' parents, referenced by '..'
#key: inode number, value: parent inode number   
inode_ref_parents = {}

#global variable to keep track of whether fs is corrupted
isCorrupted = False

def print_error(msg):
    global isCorrupted
    isCorrupted = True
    print(msg)

def read_superblock(line):
    global block_count, inode_count, block_size, inode_size, first_avail_inode 
    block_count = int(line[1])
    inode_count = int(line[2])
    block_size = int(line[3])
    inode_size = int(line[4])
    first_avail_inode = int(line[-1])
    
def read_file(filename):
    try:
        with open(filename) as csvfile:
            filereader = csv.reader(csvfile)
            for line in filereader:
                entry_type = line[0]
                num = line[1]

                if entry_type == 'SUPERBLOCK':
                    read_superblock(line)
                elif entry_type == 'GROUP':
                    global first_avail_block
                    first_avail_block = int(line[-1]) + ((inode_size * inode_count) / block_size)
                elif entry_type == 'BFREE':
                    free_blocks.add(int(num))
                elif entry_type == 'IFREE':
                    free_inodes.add(int(num))
                elif entry_type == 'INODE':
                    allocated_inodes[int(num)] = [ int(line[3]), int(line[6]) ]
                    read_block_pointers(line[12:], int(num))
                elif entry_type == 'DIRENT':
                    direntry = dirEnt(int(num), int(line[3]), str(line[6])) 
                    dirent_list.append(direntry)
                elif entry_type == 'INDIRECT':
                    blocks = [int(line[-1])]
                    read_block_pointers(blocks, int(line[1]))
    except:
        sys.stderr.write('ERROR: failure to open csv file.\n')
        sys.exit(1)

def get_level_msg(level):
    if level == 0:
        level_msg = ""
    elif level == 1:
        level_msg = " INDIRECT"
    elif level == 2:
        level_msg = " DOUBLE INDIRECT"
    elif level == 3:
        level_msg = " TRIPLE INDIRECT"
    return level_msg

def read_block_pointers(blocks, inode):
    for i in range(len(blocks)):
        blocks[i] = int(blocks[i])
        if blocks[i] == 0:
            continue
        offset = 0

        # Handle indirect blocks.
        level = 0
        if i == 12:
            offset = 12
            level = 1
        elif i == 13:
            offset = 268
            level = 2
        elif i == 14:
            offset = 65804
            level = 3

        level_msg = get_level_msg(level)
        
        if blocks[i] < 0 or blocks[i] > block_count:
            print_error('INVALID{} BLOCK {} IN INODE {} AT OFFSET {}'.format(level_msg, blocks[i], inode, offset))
        elif blocks[i] < first_avail_block:
            print_error('RESERVED{} BLOCK {} IN INODE {} AT OFFSET {}'.format(level_msg, blocks[i], inode, offset))
        elif blocks[i] not in allocated_blocks:
            allocated_blocks[blocks[i]] = [ (level, inode, offset) ]
        else:
            allocated_blocks[blocks[i]].append( (level, inode, offset) )

def find_duplicate_blocks():
    for block in allocated_blocks:
        if len(allocated_blocks[block]) > 1:
            for inode in allocated_blocks[block]:
                print_error('DUPLICATE{} BLOCK {} IN INODE {} AT OFFSET {}'.format(get_level_msg(inode[0]), block, inode[1], inode[2]))

def scan_blocks():
    for block in range(0, block_count):
        if block in allocated_blocks and block in free_blocks:
            print_error('ALLOCATED BLOCK {} ON FREELIST'.format(block))
        if (block >= first_avail_block) and block not in allocated_blocks and block not in free_blocks:
            print_error('UNREFERENCED BLOCK {}'.format(block))

def scan_inodes():
    for inode in range(1, inode_count+1):
        if inode in allocated_inodes and inode in free_inodes and allocated_inodes[inode][0] != 0:
            print_error('ALLOCATED INODE {} ON FREELIST'.format(inode))
        if inode >= first_avail_inode and inode not in allocated_inodes and inode not in free_inodes:
            print_error('UNALLOCATED INODE {} NOT ON FREELIST'.format(inode))

def scan_dir_entries():
    for dir in dirent_list:
        inum = dir.inode_num
        
        if dir.name == "'..'":
            inode_ref_parents[dir.parent_inode_num] = inum
        else:
            if dir.name != "'.'":
                inode_named_parents[inum] = dir.parent_inode_num

        #add link count to associated inode
        if inum not in inode_linkcount:
            inode_linkcount[inum] = 1
        else:
            inode_linkcount[inum] += 1

        #print unallocated inodes
        if inum in free_inodes and inum >= first_avail_inode:
            print_error('DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}'.format(dir.parent_inode_num, dir.name, inum))

        #print invalid inodes
        if inum < 1 or inum > inode_count:
            print_error('DIRECTORY INODE {} NAME {} INVALID INODE {}'.format(dir.parent_inode_num, dir.name, inum))

        #check '.' links
        if dir.name == "'.'" and inum != dir.parent_inode_num:
            print_error('DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(dir.parent_inode_num, dir.name, inum, dir.parent_inode_num))

def check_linkcounts():
    for inode in allocated_inodes:
        if inode in inode_linkcount:
            links = inode_linkcount[inode]
        else:
            links = 0
        
        linkcount = allocated_inodes[inode][1]
        if links != linkcount:
            print_error('INODE {} HAS {} LINKS BUT LINKCOUNT IS {}'.format(inode, links, linkcount))

def check_parents():
    for inum in inode_ref_parents:
        parent_inum = inode_ref_parents[inum] 

        if inum == 2 and parent_inum != 2:
            print_error("DIRECTORY INODE 2 NAME '..' LINK TO INODE {} SHOULD BE 2".format(parent_inum))

        if inum != 2 and inode_named_parents[inum] != parent_inum:
            print_error("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(inum, parent_inum, inode_named_parents[inum]))

def main():
    if len(sys.argv) != 2:
        sys.stderr.write('ERROR: program requires one filename argument.\n')
        sys.exit(1)

    read_file(sys.argv[1])
    scan_inodes()
    scan_blocks()
    find_duplicate_blocks()
    scan_dir_entries()
    check_linkcounts()
    check_parents()

    if isCorrupted:
        sys.exit(2)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()
