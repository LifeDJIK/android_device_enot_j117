#!/usr/bin/env python
""" Create parameter image """
import sys


def main():
    """ Main """
    if len(sys.argv) != 3:
        print "Usage: %s input_file output_file" % (sys.argv[0])
        return
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    with open(input_file, "rb") as file_:
        data = file_.read()
    with open(output_file, "wb") as file_:
        for _ in xrange(8):
            file_.write(data)
            file_.write("\x00" * (0x80000 - len(data)))


if __name__ == "__main__":
    main()
