#!/usr/bin/python3
# -*-coding:utf-8 -*-

import os
# from jinja2 import Environment, FileSystemLoader

from data_types import BasicTypes, Types
from interface import InterfaceTemplate, Api, Apis, Function
import utils


class DRPCGenerator:
    def __init__(self):
        self.basic_types = BasicTypes()
        return

    def load_basic_types(self, languages_):
        path = "bin/basic_type"
        file = "basic_type.yaml"

        for language in languages_:
            yaml_data = utils.ordered_yaml_load(path + "/" + language + "/" + file)
            self.basic_types.insert(language, yaml_data)

        return

    def parse_interface(self, interface_):
        # 1. parse data struct.
        yaml_datas = utils.ordered_yaml_load("interface/" + interface_ + "_" + "data.yaml")
        self.customize_types = Types(yaml_datas)

        # 2. parse api
        yaml_apis = utils.ordered_yaml_load("interface/" + interface_ + "_" + "api.yaml")
        apis = Apis(interface_, yaml_apis)

        return apis

    def instant_interface(self, apis_, language_):
        # 3. replace abstract type
        apis_.replace_abstract_types(self.customize_types, self.basic_types[language_])

        # 4. replace basic type
        apis = apis_.replace_basic_types(self.basic_types[language_])
        return apis

    @staticmethod
    def load_template(args):
        template_files = ["data", "client", "server"]
        template_path = "template"
        templates = []

        for file in template_files:
            template = InterfaceTemplate()
            template.load_template(template_path, args.language, file)
            templates.append(template)

        return templates

    def render(self, apis_, templates_):
        for template in templates_:
            template.render(apis_[0], self.customize_types)

        return templates_

    @staticmethod
    def generate(args, apis_, templates_):
        output_path = "./output"

        for template in templates_:
            template.generate(output_path, args.language, apis_[0].interface_name)
        return

    @staticmethod
    def install_am(apis_):
        # 1. generate new interface Makefile.am file to output path.
        output_path = "./output"
        content = "include_HEADERS = \\ \n"

        file_name = ""
        prefix = 0

        for api in apis_:
            for index, charactor in enumerate(api.name):
                if charactor.isupper():
                    if prefix != index:
                        file_name = file_name + api.name[prefix:index].lower() + "_"
                        prefix = index

                if index + 1 == len(api.name):
                    file_name = file_name + api.name[prefix:index + 1].lower() + "_"

            content += file_name + "data.hpp "
            content += file_name + "client.hpp "
            content += file_name + "server.hpp "

            file_name = ""
            prefix = 0

        file = open(output_path + "/" + apis_[0].interface_name + "/Makefile.am", "w+")
        file.write(content)
        file.close()

        # 2. append new interface path to Makefile.am
        interface_filepath = "output/" + apis_[0].interface_name

        file = open("Makefile.am", "r+")
        subdirs = file.read()
        file.close()

        if subdirs.rfind(interface_filepath) == -1:
            if subdirs[-1] == "\n":
                subdirs = subdirs[0:len(subdirs)-1]
            file = open("Makefile.am", "w+")
            file.write(subdirs + " " + interface_filepath)
            file.close()

        # 3. append new interface Makefile to configure.ac
        makefile = "Makefile"
        space = "\n                 "
        api_makefile = "output/" + apis_[0].interface_name + "/" + makefile

        file = open("configure.ac", "r+")
        config = file.read()
        file.close()

        api_index = config.rfind(api_makefile)
        if api_index == -1:
            index = config.rfind(makefile) + len(makefile)
            config = list(config)
            config.insert(index, space + api_makefile)

            file = open("configure.ac", "w+")
            file.write("".join(config))
            file.close()

        return

    def make(self):
        os.system("./autogen.sh")
        os.system("./configure")
        os.system("make clean")
        os.system("sudo make install")

        return
