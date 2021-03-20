#!/usr/bin/env python3
'''Script to build third party dependencies'''

import argparse
import os
import shutil
import subprocess


BUILD_OPTIONS = {
    'googletest': {},
    'grpc': {'gRPC_INSTALL': 'ON', 'gRPC_BUILD_TESTS': 'OFF', 'gRPC_BUILD_CSHARP_EXT': 'OFF', 'gRPC_BUILD_GRPC_CSHARP_PLUGIN': 'OFF', 'gRPC_BUILD_GRPC_NODE_PLUGIN': 'OFF', 'gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN': 'OFF', 'gRPC_BUILD_GRPC_RUBY_PLUGIN': 'OFF'},
    'json': {'JSONBuildTests': 'OFF'},
    'cpp-jwt': {'CPP_JWT_BUILD_EXAMPLES': 'OFF', 'CPP_JWT_BUILD_TESTS': 'OFF'},
    'mongo-c-driver': {'ENABLE_EXAMPLES': 'OFF', 'ENABLE_TESTS': 'OFF', 'ENABLE_UNINSTALL': 'OFF', 'ENABLE_AUTOMATIC_INIT_AND_CLEANUP': 'OFF'},
    'mongo-cxx-driver': {'CMAKE_CXX_STANDARD': '17', 'BSONCXX_POLY_USE_STD': 'ON', 'ENABLE_UNINSTALL': 'OFF'}
}


def build(submodule, cmake_args=None, clean=False, debug=True):
    '''Build submodule'''
    cwd = os.getcwd()
    os.chdir(submodule)

    if clean:
        shutil.rmtree('cmake/build', ignore_errors=True)
        shutil.rmtree('cmake/install', ignore_errors=True)

    os.makedirs('cmake/build', exist_ok=True)
    os.makedirs('cmake/install', exist_ok=True)
    os.chdir('cmake/build')

    build_type = 'DEBUG' if debug else 'RELEASE'
    args = ['cmake', '-GNinja', '-DCMAKE_INSTALL_PREFIX=../install', f'-DCMAKE_BUILD_TYPE={build_type}', '../..']

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
            build(project, cmake_args=BUILD_OPTIONS[project], clean=args.clean, debug=(not args.release))

    print(f"CMAKE_PREFIX_PATH={os.environ['CMAKE_PREFIX_PATH']}")
    print(f"PATH={os.environ['PATH']}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--projects', type=comma_seperated_list)
    parser.add_argument('--clean', action='store_true')
    parser.add_argument('--release', action='store_true')
    main(parser.parse_args())
