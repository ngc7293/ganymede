#!/usr/bin/env python3
'''Script to build third party dependencies'''

import argparse
import os
import shutil
import subprocess


BUILD_OPTIONS = {
    'googletest': {},
    'grpc': {'gRPC_INSTALL': 'ON', 'gRPC_BUILD_TESTS': 'OFF'},
    'json': {'JSONBuildTests': 'OFF'},
    'cpp-jwt': {},
    'mongo-c-driver': {},
    'mongo-cxx-driver': {}
}


def build(submodule, cmake_args=None, clean=False):
    '''Build submodule'''
    cwd = os.getcwd()
    os.chdir(submodule)

    if clean:
        shutil.rmtree('cmake/build')
        shutil.rmtree('cmake/install')

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


def comma_seperated_list(values):
    return values.split(',')


def main(args):
    os.environ['CMAKE_PREFIX_PATH'] = ''

    if not args.projects:
        args.projects = BUILD_OPTIONS.keys()

    for project in BUILD_OPTIONS:
        if project in args.projects:
            build(project, cmake_args=BUILD_OPTIONS[project], clean=args.clean)

    print(f"CMAKE_PREFIX_PATH={os.environ['CMAKE_PREFIX_PATH']}")
    print(f"PATH={os.environ['PATH']}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--projects', type=comma_seperated_list)
    parser.add_argument('--clean', action='store_true')
    main(parser.parse_args())
