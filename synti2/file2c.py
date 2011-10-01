import sys,string

infname = sys.argv[1]


infile = file(infname, 'rb')
data = bytearray(infile.read())

length = len(data)

print
print "/* Song data converted from file '%s': */"%infname
print "unsigned int %s_length = %d;"%(sys.argv[2],length)
print "unsigned char %s_data[%d] = {"%(sys.argv[2],length)
print string.join(map(lambda x: "0x%02x,"%x ,  data))
print "};"
