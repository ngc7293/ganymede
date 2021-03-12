#!/usr/bin/env python3
'''Script to build third party dependencies'''

import os
import subprocess


def build(submodule, cmake_args=None):
    '''Build submodule'''
    cwd = os.getcwd()
    os.chdir(submodule)
    os.makedirs('cmake/build', exist_ok=True)
    os.makedirs('cmake/install', exist_ok=True)
    os.chdir('cmake/build')

    args = ['cmake', '-GNinja', '-DCMAKE_INSTALL_PREFIX=../install', '../..']
    if cmake_args:
        args += [f'-D{a}={cmake_args[a]}' for a in cmake_args]

    subprocess.run(args)
    subprocess.run(['ninja'])
    subprocess.run(['ninja', 'install'])

    os.environ['CMAKE_PREFIX_PATH'] += f':{cwd}/{submodule}/cmake/install'
    os.environ['PATH'] += f':{cwd}/cmake/install/bin'
    os.chdir(cwd)


def main(args):
    os.environ['CMAKE_PREFIX_PATH'] = ''
    build('googletest')
    build('grpc', {'gRPC_INSTALL': 'ON', 'gRPC_BUILD_TESTS': 'OFF'})
    build('json', {'JSONBuildTests': 'OFF'})
    build('cpp-jwt')

    print(f"CMAKE_PREFIX_PATH={os.environ['CMAKE_PREFIX_PATH']}")
    print(f"PATH={os.environ['PATH']}")


if __name__ == '__main__':
    main(None)
