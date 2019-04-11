#!/usr/bin/env python3

import os
import re
import sys
import xml.etree.ElementTree as ElementTree

if len(sys.argv) != 2:
    print("Usage: ./update_version.py 0.4.2-alpha.1")
    exit(1)

new_version = sys.argv[1]
new_version_components = new_version.split('.')

if len(new_version_components) < 3 or len(new_version_components) > 4:
    print("""
The version number is in the wrong format
Some accepted versions:
    0.3.4
    0.3.4-alpha.2
    0.3.4-beta.2
""")
    exit(1)


def update_csharp_version(new_version):
    csproj = os.path.join("pitaya-sharp", "NPitaya", "NPitaya.csproj")
    tree = ElementTree.parse(csproj)
    version_el = tree.getroot().find('PropertyGroup').find('PackageVersion')
    current_version = version_el.text
    print("Replacing {} in NPitaya.csproj for {}".format(current_version, new_version))
    version_el.text = new_version
    tree.write(csproj, xml_declaration=False, encoding='utf-8')


def update_cpp_version(new_version):
    version_txt = os.path.join("cpp-lib", "version.txt")
    current_version = open(version_txt, "r").read()
    print("Replacing {} in version.txt with {}".format(current_version, new_version))
    os.unlink(version_txt)
    open(version_txt, "w").write(new_version)


update_cpp_version(new_version)
update_csharp_version(new_version)

