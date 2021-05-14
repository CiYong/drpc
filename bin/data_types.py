#!/usr/bin/python3
# -*-coding:utf-8 -*-

import os
import argparse
import utils

version = '0.1.0'
language_support = {'cpp'}


class BasicTypes:
    def __init__(self):
        self.inner = dict()
        return

    def insert(self, language_, types_):
        self.inner[language_] = types_

    def get_types(self, language_):
        return self.inner[language_]

    def get_type(self, language_, type_):
        return self.inner[language_][type_]

    def contained(self, language_, type_):
        if type_ in self.inner[language_]:
            return True
        return False

    def items(self):
        return self.inner.items()

    def __getitem__(self, key):
        return self.inner[key]

    def __setitem__(self, key, value):
        self.inner[key] = value
        return self.inner[key]

    def __delitem__(self, key):
        del self.inner[key]


class DataMember:
    def __init__(self, index_, name_, type_, is_last_):
        self.index = index_
        self.type = type_
        self.name = name_
        self.is_last = is_last_

        return


class Enumerator:
    def __init__(self, ident_, members_):
        self.ident = ident_
        self.size = len(members_.keys())
        self.members = []

        index = 0

        for member_name, member_type in members_.items():
            is_last = index == (len(members_.keys()) - 1)
            member = DataMember(index, member_name, member_type, is_last)
            self.members.append(member)
            index += 1

        return


class Structure:
    def __init__(self, ident_, members_):
        self.ident = ident_
        self.size = len(members_.keys())
        self.members = []

        index = 0

        for member_name, member_type in members_.items():
            is_last = index == (len(members_.keys()) - 1)
            member = DataMember(index, member_name, member_type, is_last)
            self.members.append(member)
            index += 1

        return


class Container:
    def __init__(self, ident_, members_):
        return


class Types:
    def __init__(self, datas_):
        self.enums = []
        self.structs = []
        self.containers = []

        for ident, attr in datas_.items():
            data = None
            if attr["type"] == "enum":
                self.enums.append(Enumerator(ident, attr["members"]))
            elif attr["type"] == "struct":
                self.structs.append(Structure(ident, attr["members"]))
            elif attr["type"] == "template":
                self.containers.append(Container(ident, attr["members"]))
        return

    def enums(self):
        return self.enums

    def structs(self):
        return self.structs

    def containers(self):
        return self.containers
