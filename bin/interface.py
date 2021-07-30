#!/usr/bin/python3
# -*-coding:utf-8 -*-

import os
from collections import OrderedDict
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
                is_input_last = index == (self.inputs_size - 1)
                member = DataMember(index, member_name, member_type, is_input_last)
                self.inputs.append(member)
                index += 1

        if output_ != "None":
            self.inputs_size = len(output_.keys())
            index = 0
            for member_name, member_type in output_.items():
                is_output_last = index == (self.inputs_size - 1)
                member = DataMember(index, member_name, member_type, is_output_last)
                self.outputs.append(member)
                index += 1

        return

    def replace_abstract_type(self, basic_types_):
        for input in self.inputs:
            for abstract_type, language_type in basic_types_.items():
                if abstract_type in input.type:
                    input.type = input.type.replace(abstract_type, language_type)

        for output in self.outputs:
            for abstract_type, language_type in basic_types_.items():
                if abstract_type in output.type:
                    output.type = output.type.replace(abstract_type, language_type)

        return


class Api:
    def __init__(self):
        self.interface_name = None
        self.version = None
        self.name = None
        self.type = None
        self.scope = None
        self.functions = []


class Apis:
    def __init__(self, interface_name_, apis_):
        self.inner = []

        for api_name, api_attr in apis_.items():
            api_id_offset = 0

            version = ''
            scope = ''

            for item_key, item_value in api_attr.items():
                if item_key == 'scope':
                    scope = item_value
                    continue

                if item_key == 'version':
                    version = item_value
                    continue

                api = Api()
                api.interface_name = interface_name_
                api.version = version
                api.scope = scope
                api.name = api_name

                if item_key == 'reqrep':
                    api.type = item_key
                    api.functions = self.parse_api_attr(api_id_offset, item_value)

                if item_key == 'broadcast':
                    api.type = item_key
                    api.functions = self.parse_api_attr(api_id_offset, item_value)

                api_id_offset += 20

                self.inner.append(api)

        return

    def types(self):
        types = []
        for api in self.inner:
            types.append(api.type)

        return types

    def parse_api_attr(self, api_id_offset_, items_):
        functions = []

        api_id = api_id_offset_
        for ident, info in items_.items():
            comment = ''
            input = OrderedDict()
            output = OrderedDict()

            if 'comment' in info.keys():
                comment = info['comment']

            if 'input' in info.keys():
                input = info['input']

            if 'output' in info.keys():
                output = info['output']

            interface = Function(api_id,
                                 ident,
                                 comment,
                                 input,
                                 output)

            functions.append(interface)
            api_id = api_id + 1

        return functions

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

    def render_api(self, interface_, customize_types_):
        self.__content = self.__template.render(version=interface_.version,
                                                scope=interface_.scope,
                                                interface_name=interface_.interface_name,
                                                name=interface_.name,
                                                functions=interface_.functions,
                                                data_types=customize_types_)
        return self.__content

    def render_main(self, interface_, reqrep_functions_, broadcast_functions_, customize_types_):
        version_major, version_minor, version_patch = interface_.version.split('.')

        self.__content = self.__template.render(version=interface_.version,
                                                version_major=version_major,
                                                version_minor=version_minor,
                                                version_patch=version_patch,
                                                scope=interface_.scope,
                                                interface_name=interface_.interface_name,
                                                name=interface_.name,
                                                reqrep_functions=reqrep_functions_,
                                                broadcast_functions=broadcast_functions_,
                                                data_types=customize_types_)
        return self.__content

    def render_data(self, interface_, functions_, customize_types_):
        version_major, version_minor, version_patch = interface_.version.split('.')

        self.__content = self.__template.render(version=interface_.version,
                                                version_major=version_major,
                                                version_minor=version_minor,
                                                version_patch=version_patch,
                                                scope=interface_.scope,
                                                interface_name=interface_.interface_name,
                                                name=interface_.name,
                                                functions=functions_,
                                                data_types=customize_types_)
        return self.__content

    def generate(self, path_, language_, interface_name_):
        # if self.__language == 'cpp':
        #     suffix = 'hpp'
        #
        interface_path = path_ + '/' + interface_name_

        output_path = ''
        output_file = ''

        header = utils.declration + "// DRPC version is " + utils.version + ".\n"
        header = header + "// " + interface_name_ + "interface version is " + utils.version + ".\n\n"

        file_name, suffix = self.__template_file.split('-')

        if file_name == "CMakeLists" or file_name == "am":
            output_path = interface_path
            output_file = file_name
        elif "Version" in file_name or "SupportMacros" in file_name or "package" in file_name:
            output_path = interface_path
            output_file = file_name
        elif suffix == "hpp":
            output_path = interface_path + '/include'
            output_file = interface_name_ + '_' + file_name
        elif suffix == "cpp":
            output_path = interface_path + '/src'
            output_file = interface_name_ + '_' + file_name
        else:
            return

        if '/' in file_name:
            paths = file_name.split('/')
            output_file = paths.pop(-1)
            output_path = output_path + '/' + '/'.join(paths)

        if not os.path.exists(output_path):
            os.makedirs(output_path)

        file = open(output_path + '/' + output_file + '.' + suffix, "w+")

        if suffix == "hpp" or suffix == "cpp":
            file.write(header)

        file.write(self.__content)
        file.close()
        return
