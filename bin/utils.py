#!/usr/bin/python3
# -*-coding:utf-8 -*-

import os
import sys
import yaml
from collections import OrderedDict

declration = "// Automatically generated file by DRPC. Do not edit.\n"
version = '0.1.0'


def error_handle(error_str, error):
    print(error_str, error)
    sys.exit()


def ordered_yaml_load(yaml_path, loader=yaml.Loader,
                      object_pairs_hook=OrderedDict):
    class OrderedLoader(loader):
        pass

    def construct_mapping(loader, node):
        loader.flatten_mapping(node)
        return object_pairs_hook(loader.construct_pairs(node))

    OrderedLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
        construct_mapping)
    with open(yaml_path) as stream:
        return yaml.load(stream, OrderedLoader)


def ordered_yaml_dump(data, stream=None, dumper=yaml.SafeDumper, **kwds):
    class OrderedDumper(dumper):
        pass

    def _dict_representer(dumper_, data_):
        return dumper_.represent_mapping(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, data.items())

    OrderedDumper.add_representer(OrderedDict, _dict_representer)
    return yaml.dump(data, stream, OrderedDumper, **kwds)


def remove_all(path):

    if not os.path.exists(path):
        return

    ls = os.listdir(path)
    for i in ls:
        c_path = os.path.join(path, i)
        if os.path.isdir(c_path):
            remove_all(c_path)
            if os.path.exists(c_path):
                os.removedirs(c_path)
        else:
            os.remove(c_path)

    return


def copy_nsis(path):
    dest_path = path + '/builds/cmake'
    if not os.path.exists(dest_path):
        os.makedirs(dest_path)

    
    src_path = os.getcwd() + '/template/cpp/builds/cmake'
    nsis_file32 = 'NSIS.template32.in'
    nsis_file64 = 'NSIS.template64.in'
    
    # NSIS32
    file = open(src_path + '/' + nsis_file32, "w+")
    content = file.read()
    file.close()

    file = open(dest_path + '/' + nsis_file32, "w+")
    file.write(content)
    file.close()

    # NSIS64
    file = open(src_path + '/' + nsis_file64, "w+")
    content = file.read()
    file.close()

    file = open(dest_path + '/' + nsis_file64, "w+")
    file.write(content)
    file.close()

    return


class FastDict:
    def __init__(self):
        self.begin = None
        self.end_key = None
        self.dict = dict()

    def replace(self, new_dict):
        self.dict = new_dict
        self.begin = list(new_dict.keys())[0]
        self.end = list(new_dict.keys())[-1]

    def append(self, key, value):
        if self.begin == None:
            self.begin = key
        self.end = key
        self.dict[key] = value

    def is_end(self, key):
        return self.begin == key

    def get(self, key):
        return self.dict[key]

    def modify(self, key, value):
        self.dict[key] = value

    def begin(self):
        return self.dict[self.begin]

    def end(self):
        return self.dict[self.end]

    def items(self):
        return self.dict.items()

    def __getitem__(self, key):
        return self.dict[key]

    def __setitem__(self, key, value):
        self.dict[key] = value
        return self.dict[key]

    def __delitem__(self, key):
        del self.dict[key]
