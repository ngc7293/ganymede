#!/usr/bin/env python3

from argparse import ArgumentParser
from requests import post


def get_device_code():
    payload = {
        'client_id': 'hcqs05h2GTzNdaXAF0Wnp43Z955VB4GP',
        'scope': 'offline_access',
        'audience': 'ganymede-api'
    }

    response = post('https://dev-ganymede.us.auth0.com/oauth/device/code', data=payload).json()

    print(response['verification_uri_complete'])
    return response['device_code']


def get_device_tokens(device_code):
    payload = {
        'client_id': 'hcqs05h2GTzNdaXAF0Wnp43Z955VB4GP',
        'grant_type': 'urn:ietf:params:oauth:grant-type:device_code',
        'device_code': device_code
    }

    response = post('https://dev-ganymede.us.auth0.com/oauth/token', data=payload).json()

    print(f'Expires in: {response["expires_in"]}')
    return response['access_token'], response['refresh_token']


def refresh_token(refresh_token):
    payload = {
        'client_id': 'hcqs05h2GTzNdaXAF0Wnp43Z955VB4GP',
        'grant_type': 'refresh_token',
        'refresh_token': refresh_token
    }

    response = post('https://dev-ganymede.us.auth0.com/oauth/token', data=payload).json()
    print(f'Expires in: {response["expires_in"]}')
    return response['access_token']


def main(args):
    if not args.refresh:
        code = get_device_code()
        input('Please authorize app...')
        acc, ref = get_device_tokens(code)

        print(f'{acc}', file=open('access.token', 'w'), end='')
        print(f'{ref}', file=open('refresh.token', 'w'), end='')
    else:
        acc = refresh_token(open('refresh.token').read())
        print(f'{acc}', file=open('access.token', 'w'), end='')


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--refresh', action='store_true')
    main(parser.parse_args())
