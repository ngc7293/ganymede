#!/usr/bin/env python3
'''Config setter'''

import argparse

import grpc

from google.protobuf.json_format import MessageToDict

from device_config_pb2 import CreateConfigRequest, UpdateConfigRequest, GetConfigRequest
from device_config_pb2_grpc import DeviceConfigServiceStub


def hex_to_rgb(color):
    return tuple(int(color[i:i+2], 16) for i in (0, 2, 4))


def create_config(config, args):
    if args.uid:
        config.uid = args.uid

    if args.name:
        config.name = args.name

    if args.flow:
        config.pomp_config.flow = args.flow

    if args.ph:
        config.solution_config.ph = args.ph

    if args.ec:
        config.solution_config.ec = args.ec

    if args.temperature:
        config.solution_config.temperature = args.temperature

    if args.intensity:
        config.light_config.intensity = args.intensity

    if args.color:
       config.light_config.color.r = hex_to_rgb(args.color)[0]
       config.light_config.color.g = hex_to_rgb(args.color)[1]
       config.light_config.color.b = hex_to_rgb(args.color)[2]


def create_or_update(service, metadata, args):
    request, response = None, None
    if args.command == 'create':
        if args.uid:
            print('warning: --uid discarded when command is \'create\'')    
        request = CreateConfigRequest()

    else:
        if not args.uid:
            print('error: \'update\' requires --uid')
            return 1
        request = UpdateConfigRequest()

    create_config(request.config, args)

    print(f'>>> {MessageToDict(request)}')
    if args.command == 'create':
        response = service.CreateConfig(request, metadata=metadata)
    else:
        response = service.UpdateConfig(request, metadata=metadata)
    print(f'<<< {MessageToDict(response)}')


def get(service, metadata, args):
    if not args.uid:
        print('error: \'get\' requires --uid')
        return 1

    request = GetConfigRequest()
    request.config_uid = args.uid
    print(f'>>> {MessageToDict(request)}')
    response = service.GetConfig(request, metadata=metadata)
    print(f'<<< {MessageToDict(response)}')


def main(args):
    channel = None
    if not args.ssl:
        channel = grpc.insecure_channel(args.host)
    else:
        channel = grpc.secure_channel(args.host, grpc.ssl_channel_credentials())
    service = DeviceConfigServiceStub(channel)

    metadata = [('authorization', 'Bearer ' + open('access.token', 'r').read().replace('\n', ''))]

    if args.command in ['create', 'update']:
        return create_or_update(service, metadata, args)
    elif args.command == 'get':
        return get(service, metadata, args)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=['create', 'update', 'get'])

    grpc_args = parser.add_argument_group('gRPC')
    grpc_args.add_argument('--host', required=True, type=str)
    grpc_args.add_argument('--ssl', action='store_true')

    parser.add_argument('--uid', type=str)
    parser.add_argument('--name', type=str)

    pomp_args = parser.add_argument_group('Pomp')
    pomp_args.add_argument('--pomp-flow', type=float, dest='flow')

    sol_args = parser.add_argument_group('Solution')
    sol_args.add_argument('--solution-ph', type=float, dest='ph')
    sol_args.add_argument('--solution-ec', type=float, dest='ec')
    sol_args.add_argument('--solution-temperature', type=float, dest='temperature')

    light_args = parser.add_argument_group('Lights')
    light_args.add_argument('--light-intensity', type=float, dest='intensity')
    light_args.add_argument('--light-color', type=str, dest='color')

    exit(main(parser.parse_args()))
