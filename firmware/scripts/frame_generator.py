#!/usr/bin/env python3

'''
    This script generates frame data for easy integration with the widget
    project. Animations used for the project should live in the 'animation'
    folder in /firmware. To create a new animation, create a sub folder
    with the name of the animation. Then put the ppm files for the animation
    in the sub folder. Name the frames in a sorted order according to their
    order in the animation. A simple naming scheme could be img1.ppm, img2.pmm,
    img3.ppm, ...
    
    An empty directory will produce an empty array for that animation

    The ppm images provided must be RGB and can be larger than 7x7.
    The images will be converted to grayscale, scaled to 7x7 and then
    scaled to the correct brightness range.
'''

import cv2
import os
import sys
import numpy as np
from natsort import natsorted

# This is the same as the max brightness define in include/led_matrix.h
MAX_VALUE = 4


# Define the functions we need
def add_header():
    print("#ifndef __GENERATED_ANIMATION_FRAMES_H__")
    print("#define __GENERATED_ANIMATION_FRAMES_H__")

def add_includes():
    print("")
    print("#include \"led_matrix.h\"")
    print("")

def add_footer():
    print("#endif /* __GENERATED_ANIMATION_FRAMES_H__ */")

def add_frame_header(name):
    print("struct led_matrix ", name, "[] = {", sep="")

def add_frame_footer():
    print("};")
    print("")

def add_matrix_entry_header(name):
    print("\t{")
    print("\t\t/* ", name," */", sep="")
    print("\t\t.mat = {")

# Print all the data for the matrix with braces around each row
def add_matrix_data(data):
    for i in range(0,7):
        print("\t\t\t{", end = "")
        for j in range(0,7):
            value = int(round(data[i][j], 0)) # bring to closest int
            print(value, end = "")

            # Format output nicely
            if(j == 6):
                pass
            else:
                print(", ", end = "", sep = "")

        print("},")

def add_matrix_entry_footer():
    print("\t\t}")
    print("\t},")


def main():
    # Go to the animations directory
    cwd = os.getcwd()
    os.chdir(cwd + "/animations")

    # get the start path so its easy to revert to
    base_path = os.getcwd()

    animations = os.listdir()

    # Begin Header file code here
    add_header()
    add_includes()

    # Begin generating each animation
    for animation in animations:    
        # we have a new led_matrix array to fill
        add_frame_header(animation)

        # go into the folder
        os.chdir(base_path + "/" + animation)
        frames = sorted(os.listdir())
        
        # natural sort (natsort) the files so they make sense chronologically
        for frame in natsorted(frames):
            frame_path = base_path + "/" + animation + "/" + frame
            
            # Convert to grayscale, scale down and scale to the precision we want
            image = cv2.imread(frame_path)
            image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
            image = cv2.resize(image, (7,7))
            image = (image / 255.0) * MAX_VALUE
            
            # add the matrix entry to the array
            add_matrix_entry_header(animation + "/" + frame)
            add_matrix_data(image)
            add_matrix_entry_footer()

        add_frame_footer()

    # End header file
    add_footer()

if __name__ == "__main__":
    main()

