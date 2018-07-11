#!/usr/bin/env python

import fileinput
import sys
import re

if len(sys.argv) == 1:
    print('Please provide the file to fix up')
    sys.exit(1)

def fixup(pattern, replace):
    for line in fileinput.input(inplace=1):
        line = re.sub(pattern, replace, line.rstrip())
        print(line)

fixup(r'___gnu_cxx_colon__colon___normal_iterator', r'')
fixup(r'__cxx11::', r'')
fixup(r'\(std::_Setw\&\&\)', r'')
fixup(r'chrono::_V2::', r'chrono::')
fixup(r'std::basic_string<char, std::char_traits<char[ ]?>, std::allocator<char[ ]?> >', r'std::string')
fixup(r'std::basic_ostream<char, std::char_traits<char[ ]?>[ ]?>', r'std::ostream')
fixup(r'std::basic_ifstream<char, std::char_traits<char[ ]?>[ ]?>', r'std::ifstream')
fixup(r'std::vector<(.*), std::allocator<\1 > >', r'std::vector<\1>')
fixup(r'__gnu_cxx::__normal_iterator<const ([^\*]*)\*, std::vector<\1> >', r'std::vector<\1>::const_iterator')
fixup(r'__gnu_cxx::__normal_iterator<([^\*]*)\*, std::vector<\1> >', r'std::vector<\1>::iterator')
fixup(r'std::_Rb_tree_iterator<std::pair<const ([^>]*)> >', r'std::map<\1>::iterator')
fixup(r'std::_Rb_tree_const_iterator<std::pair<const ([^>]*)> >', r'std::map<\1>::const_iterator')
fixup(r'__gnu_cxx::__normal_iterator<const char\*, std::string >', r'std::string::const_iterator')
fixup(r'__gnu_cxx::__normal_iterator<char\*, std::string >', r'std::string::iterator')
fixup(r'(var_[0-9]+)\.operator\*\(\).operator string_type\(\)', r'*\1')
