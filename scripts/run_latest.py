import sys
import os
import re
import subprocess

def Call(command):
    subprocess.call(command, shell=True)

def SafeCall(command):
    if 0 != subprocess.call(command, shell=True):
        print "Fail at '" + command + "'"

namePattern = re.compile(r"multislider-([0-9]+\.[0-9]+)\.zip")

latestName = None
latestVer  = 0
for fileName in os.listdir(r"."):
    match = re.match(namePattern, fileName)
    if(match):
        ver = float(match.group(1))
        if ver > latestVer:
            latestVer = ver
            latestName = fileName

if latestName != None:
    dirName = os.path.splitext(latestName)[0]
    Call(r"rm -rf " + dirName)
    SafeCall(r"unzip " + latestName)
    SafeCall(r"screen -S multislider -d -m ./" + dirName + r"/bin/multislider 95.213.199.37 8800 8700")