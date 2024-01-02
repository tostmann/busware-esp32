FILENAME_BUILDNO = '.buildcounter'
FILENAME_VERSION = 'VERSION'
FILENAME_VERSION_H = 'include/version.h'

import datetime

version = '1.0'

try:
    with open(FILENAME_VERSION) as f:
        version = f.readline()
except:
    print('Starting version number from 1.0')
    version = '1.0'
    with open(FILENAME_VERSION, 'w+') as f:
        f.write(version)

version = 'v' + version + '.'
build_no = 0

try:
    with open(FILENAME_BUILDNO) as f:
        build_no = int(f.readline()) + 1
except:
    print('Starting build number from 1..')
    build_no = 1
with open(FILENAME_BUILDNO, 'w+') as f:
    f.write(str(build_no))
    print('Build number: {}'.format(build_no))

hf = """
#ifndef BUILD_NUMBER
  #define BUILD_NUMBER "{}"
#endif
#ifndef VERSION
  #define VERSION "{} - {}"
#endif
#ifndef VERSION_SHORT
  #define VERSION_SHORT "{}"
#endif
""".format(build_no, version+str(build_no), datetime.datetime.now(), version+str(build_no))
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)
