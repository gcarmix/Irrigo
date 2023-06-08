#file2array.py
import sys
import os
import glob
try:
    os.remove("include/files.h")
except:
    print("include/files.h doesn't exists")
for filename in glob.glob("data/*"):
    print(filename)
    with open(filename,'rb') as file:
        data = file.read()
        with open("include/files.h",'at') as fileout:
            count = 0
            file_len = len(data)
            fileout.write("#define "+str(filename).split("\\")[1].replace(".","_")+"_len "+ str(file_len) + "\n")
            fileout.write("const uint8_t " + str(filename).split("\\")[1].replace(".","_") + "[] PROGMEM = {\n")
            
            idx = 0
            for x in data:
                numstr = "0x" + "{:02X}".format(x)
                fileout.write(numstr)
                count += 1
                idx = idx + 1
                if(idx < file_len):
                    fileout.write(", ")
                if count >= 16:
                    count = 0
                    fileout.write("\n")
            fileout.write("};\n\n")