#!/usr/bin/python3
# -*-coding:utf-8 -*-

import os
from jinja2 import Environment, FileSystemLoader
import utils
from data_types import DataMember


class Function:
    def __init__(self, id_, name_, comment_, input_, output_):
        self.id = id_
        self.ident = name_
        self.comment = comment_
        self.inputs_size = 0
        self.outputs_size = 0
        self.inputs = []
        self.outputs = []

        if input_ != "None":
            self.inputs_size = len(input_.keys())
            index = 0
            for member_name, member_type in input_.items():
                is_last = index == (self.inputs_size - 1)
                member = DataMember(index, member_name, member_type, is_last)
                self.inputs.append(member)
                index += 1

        if output_ == "None":
            self.outputs_size = len(output_)
            self.outputs.append("void")
        else:
            self.outputs.append(output_)

        # index = 0
        #
        # for member_type in output_.items():
        #     is_last = index == (self.output_size - 1)
        #     member = DataMember(index, member_name, member_type, is_last)
        #     self.input.append(member)
        #     index += 1

        return

    def replace_abstract_type(self, basic_types_):
        for input in self.inputs:
            for abstract_type, language_type in basic_types_.items():
                if abstract_type in input.type:
                    input.type = input.type.replace(abstract_type, language_type)


        for i in range(len(self.outputs)):
            for abstract_type, language_type in basic_types_.items():
                if abstract_type in self.outputs[i]:
                    self.outputs[i] = self.outputs[i].replace(abstract_type, language_type)

        return


class Api:
    def __init__(self):
        self.interface_name = None
        self.name = None
        self.type = None
        self.scope = None
        self.functions = []


class Apis:
    def __init__(self, interface_name_, apis_):
        self.inner = []

        for api_name, api_attr in apis_.items():
            api = Api()
            api.interface_name = interface_name_
            api.name = api_name
            api.type = api_attr['type']
            api.scope = api_attr['scope']

            api_id = 0

            for ident, info in api_attr['interface'].items():
                interface = Function(api_id,
                                     ident,
                                     info['comment'],
                                     info['input'],
                                     info['output'])

                api.functions.append(interface)
                api_id = api_id + 1

            self.inner.append(api)
        return

    def replace_abstract_types(self, customize_types_, basic_types_):
        for struct in customize_types_.structs:
            for member in struct.members:
                for abstract_type, language_type in basic_types_.items():
                    if abstract_type in member.type:
                        member.type = member.type.replace(abstract_type, language_type)

        return

    def replace_basic_types(self, basic_types_):
        for api in self.inner:
            for function in api.functions:
                function.replace_abstract_type(basic_types_)

        return

    def __getitem__(self, key):
        return self.inner[key]

    def __setitem__(self, key, value):
        self.inner[key] = value
        return self.inner[key]

    def __delitem__(self, key):
        del self.inner[key]


class InterfaceTemplate:
    def __init__(self):
        self.__template_file = None
        self.__template = None
        self.__content = None

    def load_template(self, path_, language_, template_file_):
        env = Environment(loader=FileSystemLoader(path_ + "/" + language_))
        self.__template = env.get_template(template_file_ + ".j2")
        self.__template_file = template_file_
        return

    def render(self, interface_, customize_types_):
        self.__content = self.__template.render(scope=interface_.scope,
                                                interface_name=interface_.interface_name,
                                                name=interface_.name,
                                                functions=interface_.functions,
                                                data_types=customize_types_)
        return

    def generate(self, path_, language_, interface_name_):
        # if self.__language == 'cpp':
        #     suffix = 'hpp'
        #
        interface_path = path_ + '/' + interface_name_
        if not os.path.exists(path_):
            os.mkdir(path_)

        if not os.path.exists(interface_path):
            os.mkdir(interface_path)

        header = utils.declration + "// DRPC version is " + utils.version + ".\n\n"

        suffix = "hpp"
        file = open(interface_path + '/' + interface_name_ + '_' + self.__template_file + '.' + suffix, "w+")
        file.write(header + self.__content)
        file.close()
        return
