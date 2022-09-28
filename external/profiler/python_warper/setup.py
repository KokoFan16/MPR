import os
import sys
import glob
import sysconfig
from distutils.core import setup

extension_suffix = sysconfig.get_config_var('EXT_SUFFIX')

# remove existing shared objects, etc.
for removing_extension in ['so', 'exp', 'lib', 'obj', 'pyd', 'dll']:
    for removing_file in glob.glob(f'*.{removing_extension}'):
        try:
            os.remove(removing_file)
        except OSError:
            print("Error while deleting existing compiled files")

if sys.platform.startswith('darwin'):
    if os.system('which brew') > 0:
        print('installing homebrew')
        os.system(
            '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"'
        )
    print('installing python3, pybind11')
    os.system('brew install pkg-config python3 pybind11')
    ## This part can be used to build with CMake (but for anaconda env, this doesn't work well)
    # print('processing cmake')
    # os.system('rm -f CMakeCache.txt')
    # os.system('cmake .')
    # print('processing make')
    # os.system('make')
    print('building Viveka C++ library')
    os.system(
        f'mpic++ -O3 -Wall -mtune=native -march=native -shared -std=c++11 $(python3 -m pybind11 --includes) src/events.cpp src/profiler.cpp src/wrapper.cpp -o viveka{extension_suffix}'
    )
    
elif sys.platform.startswith('linux'):
    print('installing pybind11')
    os.system('pip3 install pybind11')
    print('building Viveka C++ library')
    os.system(
        f'mpic++ -O3 -Wall -mtune=native -march=native -shared -std=c++11 `python3 -m pybind11 --includes` src/events.cpp src/profiler.cpp src/wrapper.cpp -o viveka{extension_suffix}'
    )
    

viveka_so = f'viveka{extension_suffix}'

setup(name='viveka',
      version="0.1",
      packages=[''],
      package_dir={'': '.'},
      package_data={'': [viveka_so]},
      install_requires=['pybind11>=2.2'],
      py_modules=['profiler', 'events'])