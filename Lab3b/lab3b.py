import csv

#helper function to find inode of indirect block
def find_inode(entry):
    #search indirect
    for i in marked_inodes:
        if marked_inodes[i][22] == entry:
            return i

    #search doubly indirect
    for i in marked_inodes:
        indirect = marked_inodes[i][23]
        for j, info in indirect_blocks:
            if j == indirect:
                if info[1] == entry:
                    return i

    #search triply indirect
    for i in marked_inodes:
        indirect = marked_inodes[24]
        for j, info in indirect_blocks:
            if j == indirect:
                double_indirect = info[1]
                for k, info in indirect_blocks:
                    if k == double_indirect:
                        if info[1] == entry:
                            return i
    return 0
# helper function to find parent node of indirect block
def findparent(entry):
    for i in dict_entries:
        if i[2] == entry:
            return i[0]

# read superblock info
with open('super.csv', newline='') as superfile:
    super_raw = csv.reader(superfile, delimiter=' ', quotechar='|')
    for row in super_raw:
        super_block = row[0].split(',')

num_inodes = int(super_block[1])
inodes_per_group = int(super_block[6])
first_block = int(super_block[8])
num_blocks = int(super_block[2])

group_info = []
# read group info
with open('group.csv', newline='') as groupfile:
    group_raw = csv.reader(groupfile, delimiter=' ', quotechar='|')
    for row in group_raw:
       group_info.append(row[0].split(','))


# gather block bitmap blocks

bitmap_blocks = [x[5] for x in group_info]
inode_blocks = [x[4] for x in group_info]


# read marked free blocks into set
free_bitmap = set()
free_inodemap = set()

with open('bitmap.csv', newline='') as bitmapfile:
    bitmap_raw = csv.reader(bitmapfile, delimiter=' ', quotechar='|')
    for row in bitmap_raw:
        read = row[0].split(',')

        if read[0] in bitmap_blocks:
            free_bitmap.add(int(read[1]))
        elif read[0] in inode_blocks:
            free_inodemap.add(int(read[1]))
        else:
            print(row[0].split(','))


multiple_referenced = {}
marked_inodes = {}
# read blocks used by inodes
used_bitmap = {}
with open('inode.csv', newline='') as inodefile:
    inode_raw = csv.reader(inodefile, delimiter=' ', quotechar='|')
    for row in inode_raw:
        read = row[0].split(',')
        marked_inodes.update({int(read[0]):read[1:]})
        entry = 0
        for block in read[11:]:
            block = int(block, 16)
            if block != 0:
                if block in used_bitmap:
                    if block not in multiple_referenced:
                        multiple_referenced.update({block: [[used_bitmap[block][0], used_bitmap[block][1],used_bitmap[block][2]]]})
                        multiple_referenced[block].append([read[0], entry, 'inode'])
                    else:
                        multiple_referenced[block].append([read[0], entry, 'inode'])



                else:
                    used_bitmap.update({block:[read[0], entry, 'inode']})
            entry += 1


# read blocks used by indirect blocks
indirect_blocks = {}
with open('indirect.csv', newline='') as indirectfile:
    indirect_raw = csv.reader(indirectfile, delimiter=' ', quotechar='|')
    for row in indirect_raw:
        read = row[0].split(',')
        if read[0] not in indirect_blocks:
            indirect_blocks.update({read[0]:[read[1:]]})
        else:
            indirect_blocks[read[0]].append(read[1:])
        block = int(read[2], 16)
        if block != 0:
            parent = find_inode(read[0])
            if block in used_bitmap:
                if block not in multiple_referenced:
                    multiple_referenced.update({block: [[used_bitmap[block][0], used_bitmap[block][1], used_bitmap[block][2], used_bitmap[block][3]]]})
                    multiple_referenced[block].append([read[0], entry, 'indirect', parent])
                else:
                    multiple_referenced[block].append([read[0], entry, 'indirect', parent])
            else:
                used_bitmap.update({block: [read[0], read[1], 'indirect', parent]})

# read inodes used in dictionaries
# read dictionary entries
used_inodes = {}
dict_entries = []
with open('directory.csv', newline='') as directoryfile:
    directory_raw = csv.reader(directoryfile, delimiter=' ', quotechar='|')
    for row in directory_raw:
        read = row[0].split(',')

        inode = int(read[4])
        if inode not in used_inodes:
            used_inodes.update({inode: [[read[0], read[1]]]})
        else:
            used_inodes[inode].append([read[0], read[1]])

        dict_entries.append([read[0], read[1], read[4], read[5]])


# detect unallocated blocks
unallocated_blocks = {}
for block, info in used_bitmap.items():
    if block in free_bitmap:
        if block not in unallocated_blocks:
            unallocated_blocks.update({block: [info]})
        else:
            unallocated_blocks[block].append(info)



# detected unallocated inodes
unallocated_inodes = {}
for inode, info in used_inodes.items():
    if inode not in marked_inodes:
        if inode not in unallocated_inodes:
            unallocated_inodes.update({inode: [info]})
        else:
            unallocated_inodes[inode].append(info)


# detect missing inodes
missing_inodes = []
for i in range(11, num_inodes):
    if i not in free_inodemap and i not in used_inodes:
        group_of_inode = int(i/inodes_per_group)
        free_list = group_info[group_of_inode][4]
        missing_inodes.append([i, free_list])

wrong_linkcount = []
# detect incorrect link count
for i in used_inodes:
    if int(marked_inodes[i][4]) != len(used_inodes[i]):
        wrong_linkcount.append([i, marked_inodes[i][4], len(used_inodes[i])])





# detect incorrect directory entries
incorrect_entries = []
for entry in dict_entries:

    if entry[3] == '"."':
        if entry[0] != entry[2]:
            incorrect_entries.append([entry[0], entry[3], entry[2], entry[0]])

    if entry[3] == '".."':
        parent = findparent(entry[0])
        if parent!= entry[2]:
            incorrect_entries.append([entry[0], entry[3], entry[2], parent])


# detect invalid block pointers
incorrect_blocks = []
incorrect_indirect_blocks = []


for entry, info in indirect_blocks.items():

    for i in info:
        block_num = int(i[1],16)
        if block_num < first_block or block_num > num_blocks:
            parent = find_inode(entry)
            incorrect_indirect_blocks.append([block_num, parent, entry, i[0]])

for entry, info in marked_inodes.items():
    for i in range(1, int(info[9])+1):
            if i<13:
                block_num = int(info[9+i],16)
                if block_num<first_block or block_num>num_blocks:
                    incorrect_blocks.append([block_num, entry, i-1])



# TIME TO PRINT OUTPUTS
output = open('lab3b_check.txt', 'w')

# unallocated blocks
for i, info in unallocated_blocks.items():
    info.sort(key = lambda x: int(x[0]))
    line = "UNALLOCATED BLOCK < {} > REFERENCED BY".format(i)
    for j in info:
        if j[2] == 'inode':
            line += " INODE < {} > ENTRY < {} >".format(j[0], j[1])
        if j[2] == 'indirect':
            line += " INODE < {} > INDIRECT BLOCK < {} > ENTRY < {} >".format(j[0],j[3],j[1])
    line += "\n"
    output.write(line)


# multiple referenced
for i, info in multiple_referenced.items():
    info.sort(key = lambda x: int(x[0]))
    line = "MULTIPLY REFERENCED BLOCK < {} > BY".format(i)
    for j in info:
        if j[2] == 'inode':
            line += " INODE < {} > ENTRY < {} >".format(j[0], j[1])
        if j[2] == 'indirect':
            line += " INODE < {} > INDIRECT BLOCK < {} > ENTRY < {} >".format(j[0],j[3],j[1])
    line += "\n"
    output.write(line)

# unallocated inodes
for i, info in unallocated_inodes:
    info.sort(key=lambda x: int(x[0]))
    line = "UNALLOCATED INODE < {} > REFERENCED BY".format(i)
    for j in info:
        line += " DIRECTORY < {} > ENTRY < {} >".format(j[0],j[1])
    line += "\n"
    output.write(line)

# missing inode
for i in missing_inodes:
    line = "MISSING INODE < {} > SHOULD BE IN FREE LIST < {} >\n".format(i[0], i[1])
    output.write(line)

# incorrect link count
for i in wrong_linkcount:
    line = "LINKCOUNT < {} > IS < {} > SHOULD BE < {} >\n".format(i[0],i[1],i[2])
    output.write(line)

# incorrect directory entry
for i in incorrect_entries:
    line = "INCORRECT ENTRY IN < {} > NAME < {} > LINK TO < {} > SHOULD BE < {} >\n".format(i[0],i[1][1:-1],i[2],i[3])
    output.write(line)

# invalid block pointer
for i in incorrect_blocks:
    line = "INVALID BLOCK < {} > IN INODE < {} > ENTRY < {} >\n".format(i[0],i[1],i[2])
    output.write(line)
for i in incorrect_indirect_blocks:
    line = "INVALID BLOCK < {} > IN INODE < {} > INDIRECT BLOCK < {} > ENTRY < {} >\n".format(i[0],i[1],i[2],i[3])
    output.write(line)

