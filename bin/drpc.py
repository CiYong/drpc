#!/usr/bin/python3
# -*-coding:utf-8 -*-

import os
import argparse
from generator import DRPCGenerator
import utils

language_support = {'cpp'}


def parse_args():
    try:
        parser = argparse.ArgumentParser(description="show example")
        parser.add_argument("-f", "--file", help="interface file")
        parser.add_argument("-i", "--interface", help="interface name")
        parser.add_argument("-l", "--language", help="generate interface for language")
        parser.add_argument("-v", "--version", help="version info", action="store_true")
        # exptypegroup = parser.add_mutually_exclusive_group()
        # exptypegroup.add_argument("-r", "--remote", help="remote mode", action="store_true")
        # exptypegroup.add_argument("-l", "--local", help="local mode", action="store_true")
        args = parser.parse_args()

        if args.file:
            split_file_name = args.file.split('.yaml')[0].split('_')
            split_file_name.pop()

            if len(split_file_name) == 0:
                raise ValueError("Unknown input file format:", args.file, "format: xxx_api.yaml or xxx_data.yaml")
            if not os.path.exists(args.file):
                raise ValueError("Input file not exist:", args.file)

        if args.language:
            if not str(args.language) in language_support:
                raise ValueError("Unsupport language:", args.language)
        else:
            args.language = "all"

        if args.version:
            print("drpc version " + utils.version)

        return args

    except ValueError as e:
        utils.error_handle("Invalid input arguments,", repr(e))

def main():
    generator = DRPCGenerator()

    # 1. parse input argments.
    args = parse_args()

    languages = [args.language]
    generator.load_basic_types(languages)

    # 2. parse interface.yaml.
    apis = generator.parse_interface(args.interface)

    # 3. instant interface types for each language.
    generator.instant_interface(apis, args.language)

    # 4. load template by args.
    templates = generator.load_template(args)

    # 5. render template by api.
    generator.render(apis, templates)

    # 6. generate interface file.
    generator.generate(args, apis, templates)

    generator.install_am(apis)

    generator.make()

    print("[DRPC] <" + args.interface + "> interface file was generated in output directory.")

    return


if __name__ == '__main__':
    main()
