#!/usr/bin/env python

import os, numpy, sys
import Image

# -----------------------------------------------------

def png2rgba(namespace, filenames):

    fdH = open("%s.hpp" % namespace, "w")
    fdH.write("/* (Auto-generated binary data file). */\n")
    fdH.write("\n")
    fdH.write("#ifndef BINARY_%s_HPP\n" % namespace.upper())
    fdH.write("#define BINARY_%s_HPP\n" % namespace.upper())
    fdH.write("\n")
    fdH.write("namespace %s\n" % namespace)
    fdH.write("{\n")

    fdC = open("%s.cpp" % namespace, "w")
    fdC.write("/* (Auto-generated binary data file). */\n")
    fdC.write("\n")
    fdC.write("#include \"%s.hpp\"\n" % namespace)
    fdC.write("\n")

    tempIndex = 1

    for filename in filenames:
        shortFilename = filename.rsplit(os.sep, 1)[-1].split(".", 1)[0]
        shortFilename = shortFilename.replace("-", "_")

        png      = Image.open(filename)
        pngNumpy = numpy.array(png)
        pngData  = pngNumpy.tolist()
        #pngData.reverse()

        height = len(pngData)
        for dataBlock in pngData:
            width = len(dataBlock)
            channels = len(dataBlock[0])
            break
        else:
            print("Invalid image found, cannot continue!")
            quit()

        if channels not in (3, 4):
            print("Invalid image channel count, cannot continue!")
            quit()

        print("Generating data for \"%s\" using '%s' type" % (filename, "GL_BGR" if channels == 3 else "GL_BGRA"))
        #print("  Width:    %i" % width)
        #print("  Height:   %i" % height)
        #print("  DataSize: %i" % (width * height * channels))

        fdH.write("    extern const char* %sData;\n" % shortFilename)
        fdH.write("    const unsigned int %sDataSize = %i;\n" % (shortFilename, width * height * channels))
        fdH.write("    const unsigned int %sWidth    = %i;\n" % (shortFilename, width))
        fdH.write("    const unsigned int %sHeight   = %i;\n" % (shortFilename, height))

        if tempIndex != len(filenames):
            fdH.write("\n")

        fdC.write("static const unsigned char temp%i[] = {\n" % tempIndex)

        curColumn = 1
        fdC.write(" ")

        for dataBlock in pngData:
            if curColumn == 0:
                fdC.write(" ")

            for data in dataBlock:
                if channels == 3:
                    r, g, b = data
                    fdC.write(" %3u, %3u, %3u," % (b, g, r))
                else:
                    r, g, b, a = data

                    if filename in ("artwork/claw1.png",
                                    "artwork/claw2.png",
                                    "artwork/run1.png",
                                    "artwork/run2.png",
                                    "artwork/run3.png",
                                    "artwork/run4.png",
                                    "artwork/scratch1.png",
                                    "artwork/scratch2.png",
                                    "artwork/sit.png",
                                    "artwork/tail.png"):
                        if r == 255:
                            a -= 38
                            if a < 0: a = 0
                            #a = 0
                        #else:
                            #r = g = b = 255

                    fdC.write(" %3u, %3u, %3u, %3u," % (b, g, r, a))

                if curColumn > 20:
                    fdC.write("\n ")
                    curColumn = 1
                else:
                    curColumn += 1

        fdC.write("};\n")
        fdC.write("const char* %s::%sData = (const char*)temp%i;\n" % (namespace, shortFilename, tempIndex))

        if tempIndex != len(filenames):
            fdC.write("\n")

        tempIndex += 1

    fdH.write("}\n")
    fdH.write("\n")
    fdH.write("#endif // BINARY_%s_HPP\n" % namespace.upper())
    fdH.write("\n")
    fdH.close()

    fdC.write("\n")
    fdC.close()

# -----------------------------------------------------

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: %s <namespace> <artwork-folder>" % sys.argv[0])
        quit()

    namespace = sys.argv[1]
    artFolder = sys.argv[2]

    if not os.path.exists(artFolder):
        print("Folder '%s' does not exist" % artFolder)
        quit()

    # find png files
    pngFiles = []

    for root, dirs, files in os.walk(artFolder):
        for name in [name for name in files if name.lower().endswith(".png")]:
            pngFiles.append(os.path.join(root, name))

    pngFiles.sort()

    # create code now
    png2rgba(namespace, pngFiles)
